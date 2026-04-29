#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	constexpr uint32_t MapperWindowSize = 0x80000;

	vector<uint8_t> BuildNopRom(size_t size = 0x200) {
		vector<uint8_t> rom(size, 0);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4E;
			rom[i + 1] = 0x71;
		}
		return rom;
	}

	vector<uint8_t> BuildMapperPatternRom(uint32_t bankCount, uint8_t stride = 0x11) {
		vector<uint8_t> rom((size_t)bankCount * MapperWindowSize, 0);
		for (uint32_t bank = 0; bank < bankCount; bank++) {
			uint8_t pattern = (uint8_t)(bank * stride);
			uint32_t base = bank * MapperWindowSize;
			rom[base + 0] = pattern;
			rom[base + 1] = (uint8_t)(pattern ^ 0x5a);
			rom[base + MapperWindowSize - 1] = (uint8_t)(pattern ^ 0xa5);
		}
		return rom;
	}

	void WriteVector(vector<uint8_t>& rom, uint32_t vectorAddress, uint32_t targetPc) {
		rom[vectorAddress + 0] = (uint8_t)((targetPc >> 24) & 0xFF);
		rom[vectorAddress + 1] = (uint8_t)((targetPc >> 16) & 0xFF);
		rom[vectorAddress + 2] = (uint8_t)((targetPc >> 8) & 0xFF);
		rom[vectorAddress + 3] = (uint8_t)(targetPc & 0xFF);
	}

	TEST(GenesisM68kBoundaryScaffoldTests, StartupMarksScaffoldStartedAndResetsCpuState) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		EXPECT_TRUE(scaffold.IsStarted());
		EXPECT_EQ(scaffold.GetCpu().GetProgramCounter(), 0u);
		EXPECT_EQ(scaffold.GetCpu().GetCycleCount(), 0u);
	}

	TEST(GenesisM68kBoundaryScaffoldTests, StepFrameAdvancesCpuCycles) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<uint8_t> rom = BuildNopRom();
		scaffold.LoadRom(rom);
		scaffold.Startup();
		scaffold.StepFrameScaffold(32);

		EXPECT_EQ(scaffold.GetCpu().GetCycleCount(), 32u);
		EXPECT_EQ(scaffold.GetCpu().GetProgramCounter(), 16u);
	}

	TEST(GenesisM68kBoundaryScaffoldTests, InterruptSequenceLoadsVectorAndLatchesState) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<uint8_t> rom = BuildNopRom();
		WriteVector(rom, 112, 0x120);

		scaffold.LoadRom(rom);
		scaffold.Startup();
		scaffold.GetCpu().SetInterrupt(4);
		scaffold.StepFrameScaffold(1);

		EXPECT_EQ(scaffold.GetCpu().GetInterruptSequenceCount(), 1u);
		EXPECT_EQ(scaffold.GetCpu().GetLastExceptionVectorAddress(), 112u);
		EXPECT_EQ(scaffold.GetCpu().GetProgramCounter(), 0x120u);
		EXPECT_EQ(scaffold.GetCpu().GetInterruptLevel(), 0u);
		EXPECT_EQ((uint16_t)((scaffold.GetCpu().GetStatusRegister() >> 8) & 0x7), (uint16_t)4);
		EXPECT_EQ(scaffold.GetCpu().GetSupervisorStackPointer(), 0xFFFFF8u);
	}

	TEST(GenesisM68kBoundaryScaffoldTests, InterruptLevelIsClampedToSevenWhenRequestedAboveRange) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<uint8_t> rom = BuildNopRom();
		WriteVector(rom, 124, 0x180);

		scaffold.LoadRom(rom);
		scaffold.Startup();
		scaffold.GetCpu().SetInterrupt(12);
		scaffold.StepFrameScaffold(1);

		EXPECT_EQ(scaffold.GetCpu().GetInterruptSequenceCount(), 1u);
		EXPECT_EQ(scaffold.GetCpu().GetLastExceptionVectorAddress(), 124u);
		EXPECT_EQ(scaffold.GetCpu().GetProgramCounter(), 0x180u);
		EXPECT_EQ((uint16_t)((scaffold.GetCpu().GetStatusRegister() >> 8) & 0x7), (uint16_t)7);
	}

	TEST(GenesisM68kBoundaryScaffoldTests, InterruptMaskBlocksSamePriorityUntilMaskIsLowered) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<uint8_t> rom = BuildNopRom();
		WriteVector(rom, 112, 0x1A0);

		scaffold.LoadRom(rom);
		scaffold.Startup();

		scaffold.GetCpu().SetInterrupt(4);
		GenesisBoundaryScaffoldSaveState blockedState = scaffold.SaveState();
		blockedState.Cpu.StatusRegister = 0x2400;
		scaffold.LoadState(blockedState);
		scaffold.StepFrameScaffold(4);

		EXPECT_EQ(scaffold.GetCpu().GetInterruptSequenceCount(), 0u);
		EXPECT_EQ(scaffold.GetCpu().GetProgramCounter(), 2u);

		GenesisBoundaryScaffoldSaveState unmaskedState = scaffold.SaveState();
		unmaskedState.Cpu.StatusRegister = 0x2000;
		scaffold.LoadState(unmaskedState);
		scaffold.GetCpu().SetInterrupt(4);
		scaffold.StepFrameScaffold(1);

		EXPECT_EQ(scaffold.GetCpu().GetInterruptSequenceCount(), 1u);
		EXPECT_EQ(scaffold.GetCpu().GetLastExceptionVectorAddress(), 112u);
		EXPECT_EQ(scaffold.GetCpu().GetProgramCounter(), 0x1A0u);
	}

	TEST(GenesisM68kBoundaryScaffoldTests, UnknownOpcodePathStillAdvancesProgramCounterDeterministically) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<uint8_t> rom(0x200, 0xFF);

		scaffold.LoadRom(rom);
		scaffold.Startup();
		scaffold.StepFrameScaffold(8);

		EXPECT_EQ(scaffold.GetCpu().GetCycleCount(), 8u);
		EXPECT_EQ(scaffold.GetCpu().GetProgramCounter(), 4u);
	}

	TEST(GenesisM68kBoundaryScaffoldTests, Z80WindowAccessIsTrackedByBusStub) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xA00010, 0x12);
		EXPECT_TRUE(scaffold.GetBus().WasZ80WindowAccessed());
	}

	TEST(GenesisM68kBoundaryScaffoldTests, MapperRegisterWindowCoverageAcrossAllSlotsUpdatesMappedWindows) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.LoadRom(BuildMapperPatternRom(8));
		scaffold.Startup();

		auto& bus = scaffold.GetBus();
		for (uint32_t slot = 0; slot < 7; slot++) {
			uint32_t regAddress = 0xA130F3 + (slot * 2);
			uint8_t targetBank = (uint8_t)((7 - slot) & 0x07);
			uint8_t expectedPattern = (uint8_t)(targetBank * 0x11);
			uint32_t mappedAddress = 0x080000 + (slot * MapperWindowSize);

			bus.WriteByte(regAddress, targetBank);
			EXPECT_EQ(bus.ReadByte(regAddress), targetBank);
			EXPECT_EQ(bus.ReadByte(mappedAddress), expectedPattern);
			EXPECT_EQ(bus.ReadByte(mappedAddress + 1), (uint8_t)(expectedPattern ^ 0x5a));
		}

		// Even addresses in the mapper range should not retarget mapper slots.
		bus.WriteByte(0xA130F2, 0x06);
		EXPECT_EQ(bus.ReadByte(0x080000), (uint8_t)(7 * 0x11));
	}

	TEST(GenesisM68kBoundaryScaffoldTests, MapperStateRoundtripThroughSaveLoadPreservesRegisterAndWindowMapping) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.LoadRom(BuildMapperPatternRom(8));
		scaffold.Startup();

		auto& bus = scaffold.GetBus();
		bus.WriteByte(0xA130F3, 0x06);
		bus.WriteByte(0xA130F5, 0x02);
		uint8_t preSlot0 = bus.ReadByte(0x080000);
		uint8_t preSlot1 = bus.ReadByte(0x100000);

		GenesisBoundaryScaffoldSaveState state = scaffold.SaveState();

		bus.WriteByte(0xA130F3, 0x01);
		bus.WriteByte(0xA130F5, 0x07);
		EXPECT_NE(bus.ReadByte(0x080000), preSlot0);
		EXPECT_NE(bus.ReadByte(0x100000), preSlot1);

		scaffold.LoadState(state);
		EXPECT_EQ(bus.ReadByte(0xA130F3), 0x06);
		EXPECT_EQ(bus.ReadByte(0xA130F5), 0x02);
		EXPECT_EQ(bus.ReadByte(0x080000), preSlot0);
		EXPECT_EQ(bus.ReadByte(0x100000), preSlot1);
	}

	TEST(GenesisM68kBoundaryScaffoldTests, SmallRomStartupMappingRemainsModuloMirroredEvenAfterMapperWrites) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<uint8_t> rom(0x20000, 0);
		rom[0] = 0x3c;
		rom[1] = 0xa5;
		rom.back() = 0x5a;
		scaffold.LoadRom(rom);
		scaffold.Startup();

		auto& bus = scaffold.GetBus();
		uint8_t baseline = bus.ReadByte(0x080000);
		uint8_t baselineTail = bus.ReadByte(0x09FFFF);
		EXPECT_EQ(baseline, rom[0]);
		EXPECT_EQ(baselineTail, rom.back());

		bus.WriteByte(0xA130F3, 0x07);
		bus.WriteByte(0xA130F5, 0x03);

		EXPECT_EQ(bus.ReadByte(0x080000), baseline);
		EXPECT_EQ(bus.ReadByte(0x09FFFF), baselineTail);
		EXPECT_EQ(bus.ReadByte(0xA130F3), 0x07);
		EXPECT_EQ(bus.ReadByte(0xA130F5), 0x03);
	}
}
