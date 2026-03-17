#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisM68kBoundaryScaffoldTests, StartupMarksScaffoldStartedAndResetsCpuState) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		EXPECT_TRUE(scaffold.IsStarted());
		EXPECT_EQ(scaffold.GetCpu().GetProgramCounter(), 0u);
		EXPECT_EQ(scaffold.GetCpu().GetCycleCount(), 0u);
	}

	TEST(GenesisM68kBoundaryScaffoldTests, StepFrameAdvancesCpuCycles) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<uint8_t> rom(0x200, 0);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4E;
			rom[i + 1] = 0x71;
		}
		scaffold.LoadRom(rom);
		scaffold.Startup();
		scaffold.StepFrameScaffold(32);

		EXPECT_EQ(scaffold.GetCpu().GetCycleCount(), 32u);
		EXPECT_EQ(scaffold.GetCpu().GetProgramCounter(), 16u);
	}

	TEST(GenesisM68kBoundaryScaffoldTests, InterruptSequenceLoadsVectorAndLatchesState) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<uint8_t> rom(0x200, 0);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4E;
			rom[i + 1] = 0x71;
		}

		rom[112] = 0x00;
		rom[113] = 0x00;
		rom[114] = 0x01;
		rom[115] = 0x20;

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

	TEST(GenesisM68kBoundaryScaffoldTests, Z80WindowAccessIsTrackedByBusStub) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xA00010, 0x12);
		EXPECT_TRUE(scaffold.GetBus().WasZ80WindowAccessed());
	}
}
