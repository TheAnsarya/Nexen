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

	TEST(GenesisVdpDmaContentionTests, BenchmarkAlignedBurstTransfersLatchModeAndProduceContention) {
		vector<uint8_t> rom(0x8000, 0x4e);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4e;
			rom[i + 1] = 0x71;
		}

		auto runBurst = [&](GenesisVdpDmaMode mode) {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.LoadRom(rom);
			scaffold.Startup();

			scaffold.GetBus().BeginDmaTransfer(mode, 64);
			EXPECT_EQ(scaffold.GetBus().GetDmaMode(), mode);

			scaffold.StepFrameScaffold(512);
			EXPECT_GT(scaffold.GetBus().GetDmaContentionCycles(), 0u);
			EXPECT_GT(scaffold.GetBus().GetDmaContentionEvents(), 0u);
			EXPECT_LT(scaffold.GetCpu().GetCycleCount(), 512u);
		};

		runBurst(GenesisVdpDmaMode::Copy);
		runBurst(GenesisVdpDmaMode::Fill);
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

	TEST(GenesisVdpDmaContentionTests, ZeroLengthDmaTransferDoesNotLatchActiveMode) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Fill, 0);

		EXPECT_EQ(scaffold.GetBus().GetDmaMode(), GenesisVdpDmaMode::None);
		EXPECT_EQ(scaffold.GetBus().GetDmaTransferWords(), 0u);
		EXPECT_EQ(scaffold.GetBus().GetDmaActiveCyclesRemaining(), 0u);
		EXPECT_FALSE(scaffold.GetBus().WasDmaRequested());

		uint64_t cpuBefore = scaffold.GetCpu().GetCycleCount();
		scaffold.StepFrameScaffold(32);
		EXPECT_EQ(scaffold.GetCpu().GetCycleCount() - cpuBefore, 32u);
		EXPECT_EQ(scaffold.GetBus().GetDmaContentionCycles(), 0u);
		EXPECT_EQ(scaffold.GetBus().GetDmaContentionEvents(), 0u);
	}

	TEST(GenesisVdpDmaContentionTests, LargeDmaTransferSaturatesActiveCyclesWithoutOverflow) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 0x40000000u);

		EXPECT_EQ(scaffold.GetBus().GetDmaMode(), GenesisVdpDmaMode::Copy);
		EXPECT_EQ(scaffold.GetBus().GetDmaTransferWords(), 0x40000000u);
		EXPECT_EQ(scaffold.GetBus().GetDmaActiveCyclesRemaining(), std::numeric_limits<uint32_t>::max());
		EXPECT_TRUE(scaffold.GetBus().WasDmaRequested());

		scaffold.StepFrameScaffold(12);
		EXPECT_EQ(scaffold.GetCpu().GetCycleCount(), 9u);
		EXPECT_EQ(scaffold.GetBus().GetDmaContentionCycles(), 3u);
		EXPECT_EQ(scaffold.GetBus().GetDmaActiveCyclesRemaining(), std::numeric_limits<uint32_t>::max() - 3u);
	}

	TEST(GenesisVdpDmaContentionTests, ContentionSlopeScalesAcrossTransferSizeBuckets) {
		constexpr std::array<uint32_t, 8> buckets = {1u, 2u, 4u, 8u, 16u, 32u, 64u, 128u};
		vector<uint8_t> rom(0x400, 0x4e);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4e;
			rom[i + 1] = 0x71;
		}

		auto assertSlopeForMode = [&](GenesisVdpDmaMode mode) {
			uint32_t previousContention = 0;
			for (size_t i = 0; i < buckets.size(); i++) {
				GenesisM68kBoundaryScaffold scaffold;
				scaffold.LoadRom(rom);
				scaffold.Startup();

				uint32_t transferWords = buckets[i];
				scaffold.GetBus().BeginDmaTransfer(mode, transferWords);
				scaffold.StepFrameScaffold(512);

				uint32_t actualContention = scaffold.GetBus().GetDmaContentionCycles();
				uint32_t expectedContention = std::min<uint32_t>(transferWords * 4u, 128u);
				uint64_t expectedCpuCycles = 512u - expectedContention;

				SCOPED_TRACE(std::format("mode={} bucket={} transferWords={}",
					mode == GenesisVdpDmaMode::Copy ? "copy" : "fill",
					i,
					transferWords));

				EXPECT_EQ(actualContention, expectedContention);
				EXPECT_EQ(scaffold.GetCpu().GetCycleCount(), expectedCpuCycles);
				EXPECT_GE(actualContention, previousContention);
				EXPECT_GT(scaffold.GetBus().GetDmaContentionEvents(), 0u);

				previousContention = actualContention;
			}
		};

		assertSlopeForMode(GenesisVdpDmaMode::Copy);
		assertSlopeForMode(GenesisVdpDmaMode::Fill);
	}

	TEST(GenesisVdpDmaContentionTests, ContentionSlopeDigestIsDeterministicAcrossRepeatedRuns) {
		constexpr std::array<uint32_t, 8> buckets = {1u, 2u, 4u, 8u, 16u, 32u, 64u, 128u};
		vector<uint8_t> rom(0x400, 0x4e);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4e;
			rom[i + 1] = 0x71;
		}

		auto runScenario = [&]() {
			uint64_t hash = 1469598103934665603ull;
			for (GenesisVdpDmaMode mode : {GenesisVdpDmaMode::Copy, GenesisVdpDmaMode::Fill}) {
				for (uint32_t transferWords : buckets) {
					GenesisM68kBoundaryScaffold scaffold;
					scaffold.LoadRom(rom);
					scaffold.Startup();
					scaffold.GetBus().BeginDmaTransfer(mode, transferWords);
					scaffold.StepFrameScaffold(512);

					string line = std::format("{}:{}:{}:{}:{}",
						mode == GenesisVdpDmaMode::Copy ? "copy" : "fill",
						transferWords,
						scaffold.GetBus().GetDmaContentionCycles(),
						scaffold.GetBus().GetDmaContentionEvents(),
						scaffold.GetCpu().GetCycleCount());
					for (uint8_t ch : line) {
						hash ^= ch;
						hash *= 1099511628211ull;
					}
				}
			}
			return hash;
		};

		uint64_t digestA = runScenario();
		uint64_t digestB = runScenario();
		EXPECT_EQ(digestA, digestB);
	}
}
