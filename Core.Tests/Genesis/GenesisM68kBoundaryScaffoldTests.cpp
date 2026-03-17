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
		vector<uint8_t> rom(0x200, 0x4E);
		scaffold.LoadRom(rom);
		scaffold.Startup();
		scaffold.StepFrameScaffold(32);

		EXPECT_EQ(scaffold.GetCpu().GetCycleCount(), 32u);
		EXPECT_EQ(scaffold.GetCpu().GetProgramCounter(), 64u);
	}

	TEST(GenesisM68kBoundaryScaffoldTests, Z80WindowAccessIsTrackedByBusStub) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xA00010, 0x12);
		EXPECT_TRUE(scaffold.GetBus().WasZ80WindowAccessed());
	}
}
