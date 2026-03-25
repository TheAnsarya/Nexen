#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	vector<uint8_t> BuildNopRom(size_t size = 0x200) {
		vector<uint8_t> rom(size, 0);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4E;
			rom[i + 1] = 0x71;
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
}
