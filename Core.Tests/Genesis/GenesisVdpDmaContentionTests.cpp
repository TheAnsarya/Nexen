#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisVdpDmaContentionTests, DmaModeLatchesFromRegisterAndStartsTransfer) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xC00004, 0x97);
		scaffold.GetBus().WriteByte(0xC00005, 0x80);
		scaffold.GetBus().WriteByte(0xC00004, 0x81);
		scaffold.GetBus().WriteByte(0xC00005, 0x20);

		EXPECT_TRUE(scaffold.GetBus().WasDmaRequested());
		EXPECT_EQ(scaffold.GetBus().GetDmaMode(), GenesisVdpDmaMode::Fill);
		EXPECT_EQ(scaffold.GetBus().GetDmaTransferWords(), 16u);
		EXPECT_GT(scaffold.GetBus().GetDmaActiveCyclesRemaining(), 0u);
	}

	TEST(GenesisVdpDmaContentionTests, StepFrameAppliesDeterministicContentionPenalty) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<uint8_t> rom(0x400, 0x4E);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4E;
			rom[i + 1] = 0x71;
		}

		scaffold.LoadRom(rom);
		scaffold.Startup();
		scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 32);

		uint64_t beforeCycles = scaffold.GetCpu().GetCycleCount();
		scaffold.StepFrameScaffold(64);
		uint64_t afterCycles = scaffold.GetCpu().GetCycleCount();

		EXPECT_LT(afterCycles - beforeCycles, 64u);
		EXPECT_GT(scaffold.GetBus().GetDmaContentionCycles(), 0u);
		EXPECT_GT(scaffold.GetBus().GetDmaContentionEvents(), 0u);
	}

	TEST(GenesisVdpDmaContentionTests, ContentionPenaltyIsDeterministicAcrossIdenticalRuns) {
		vector<uint8_t> rom(0x400, 0x4E);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4E;
			rom[i + 1] = 0x71;
		}

		GenesisM68kBoundaryScaffold scaffoldA;
		scaffoldA.LoadRom(rom);
		scaffoldA.Startup();
		scaffoldA.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 24);
		scaffoldA.StepFrameScaffold(96);

		GenesisM68kBoundaryScaffold scaffoldB;
		scaffoldB.LoadRom(rom);
		scaffoldB.Startup();
		scaffoldB.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 24);
		scaffoldB.StepFrameScaffold(96);

		EXPECT_EQ(scaffoldA.GetCpu().GetCycleCount(), scaffoldB.GetCpu().GetCycleCount());
		EXPECT_EQ(scaffoldA.GetBus().GetDmaContentionCycles(), scaffoldB.GetBus().GetDmaContentionCycles());
		EXPECT_EQ(scaffoldA.GetBus().GetDmaActiveCyclesRemaining(), scaffoldB.GetBus().GetDmaActiveCyclesRemaining());
	}

	TEST(GenesisVdpDmaContentionTests, ContentionPenaltyTracksSmallTimingWindows) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<uint8_t> rom(0x400, 0x4e);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4e;
			rom[i + 1] = 0x71;
		}

		scaffold.LoadRom(rom);
		scaffold.Startup();
		scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 8);

		scaffold.StepFrameScaffold(3);
		EXPECT_EQ(scaffold.GetCpu().GetCycleCount(), 2u);
		EXPECT_EQ(scaffold.GetBus().GetDmaContentionCycles(), 1u);

		scaffold.StepFrameScaffold(4);
		EXPECT_EQ(scaffold.GetCpu().GetCycleCount(), 5u);
		EXPECT_EQ(scaffold.GetBus().GetDmaContentionCycles(), 2u);

		scaffold.StepFrameScaffold(8);
		EXPECT_EQ(scaffold.GetCpu().GetCycleCount(), 11u);
		EXPECT_EQ(scaffold.GetBus().GetDmaContentionCycles(), 4u);
		EXPECT_EQ(scaffold.GetBus().GetDmaContentionEvents(), 3u);
	}

	TEST(GenesisVdpDmaContentionTests, DmaContentionEndsAfterTransferCyclesAreConsumed) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<uint8_t> rom(0x400, 0x4e);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4e;
			rom[i + 1] = 0x71;
		}

		scaffold.LoadRom(rom);
		scaffold.Startup();
		scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Fill, 2);

		scaffold.StepFrameScaffold(64);
		EXPECT_EQ(scaffold.GetBus().GetDmaActiveCyclesRemaining(), 0u);
		EXPECT_EQ(scaffold.GetBus().GetDmaMode(), GenesisVdpDmaMode::None);
		EXPECT_EQ(scaffold.GetBus().GetDmaContentionCycles(), 8u);
		EXPECT_EQ(scaffold.GetCpu().GetCycleCount(), 56u);

		uint64_t cpuBefore = scaffold.GetCpu().GetCycleCount();
		uint32_t contentionBefore = scaffold.GetBus().GetDmaContentionCycles();
		scaffold.StepFrameScaffold(16);
		EXPECT_EQ(scaffold.GetCpu().GetCycleCount() - cpuBefore, 16u);
		EXPECT_EQ(scaffold.GetBus().GetDmaContentionCycles(), contentionBefore);
	}
}
