#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisZ80HandoffTests, ResetReleaseBootstrapsAndStartsZ80) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xA11200, 0x01);

		EXPECT_TRUE(scaffold.GetBus().IsZ80Bootstrapped());
		EXPECT_TRUE(scaffold.GetBus().IsZ80Running());
		EXPECT_EQ(scaffold.GetBus().GetZ80BootstrapCount(), 1u);
	}

	TEST(GenesisZ80HandoffTests, BusRequestHaltsAndReleaseResumesZ80Execution) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.GetBus().WriteByte(0xA11200, 0x01);

		scaffold.StepFrameScaffold(40);
		uint64_t runningCycles = scaffold.GetBus().GetZ80ExecutedCycles();
		EXPECT_GT(runningCycles, 0u);

		scaffold.GetBus().WriteByte(0xA11100, 0x01);
		scaffold.StepFrameScaffold(40);
		uint64_t haltedCycles = scaffold.GetBus().GetZ80ExecutedCycles();
		EXPECT_EQ(runningCycles, haltedCycles);
		EXPECT_FALSE(scaffold.GetBus().IsZ80Running());

		scaffold.GetBus().WriteByte(0xA11100, 0x00);
		scaffold.StepFrameScaffold(40);
		EXPECT_GT(scaffold.GetBus().GetZ80ExecutedCycles(), haltedCycles);
		EXPECT_TRUE(scaffold.GetBus().IsZ80Running());
		EXPECT_GE(scaffold.GetBus().GetZ80HandoffCount(), 2u);
	}

	TEST(GenesisZ80HandoffTests, Z80WindowAccessReflectsBusHandoffState) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.GetBus().WriteByte(0xA11200, 0x01);

		uint8_t blockedValue = scaffold.GetBus().ReadByte(0xA00000);
		EXPECT_EQ(blockedValue, 0xFFu);

		scaffold.GetBus().WriteByte(0xA11100, 0x01);
		uint8_t handoffValue = scaffold.GetBus().ReadByte(0xA00000);
		EXPECT_EQ(handoffValue, 0x00u);
	}

	TEST(GenesisZ80HandoffTests, DmaAndResetHandoffSequenceKeepsCountersDeterministic) {
		auto runScenario = []() {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.Startup();
			auto& bus = scaffold.GetBus();

			bus.WriteByte(0xA11200, 0x01);
			bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 24);

			for (uint32_t i = 0; i < 8; i++) {
				if (i == 2) {
					bus.WriteByte(0xA11100, 0x01);
				}
				if (i == 4) {
					bus.WriteByte(0xA11200, 0x00);
				}
				if (i == 5) {
					bus.WriteByte(0xA11200, 0x01);
				}
				if (i == 6) {
					bus.WriteByte(0xA11100, 0x00);
				}

				scaffold.StepFrameScaffold(96);
			}

			return std::tuple<uint64_t, uint32_t, uint32_t, uint32_t, bool>(
				bus.GetZ80ExecutedCycles(),
				bus.GetZ80HandoffCount(),
				bus.GetDmaContentionCycles(),
				bus.GetDmaContentionEvents(),
				bus.IsZ80Running());
		};

		auto runA = runScenario();
		auto runB = runScenario();

		EXPECT_GT(std::get<0>(runA), 0u);
		EXPECT_GE(std::get<1>(runA), 2u);
		EXPECT_GT(std::get<2>(runA), 0u);
		EXPECT_GT(std::get<3>(runA), 0u);
		EXPECT_TRUE(std::get<4>(runA));
		EXPECT_EQ(runA, runB);
	}

	TEST(GenesisZ80HandoffTests, SaveStateReplayKeepsZ80AndDmaStateStable) {
		GenesisM68kBoundaryScaffold firstRun;
		firstRun.Startup();
		auto& firstBus = firstRun.GetBus();
		firstBus.WriteByte(0xA11200, 0x01);
		firstBus.BeginDmaTransfer(GenesisVdpDmaMode::Fill, 16);

		firstRun.StepFrameScaffold(160);
		GenesisBoundaryScaffoldSaveState checkpoint = firstRun.SaveState();
		firstBus.WriteByte(0xA11100, 0x01);
		firstRun.StepFrameScaffold(96);
		firstBus.WriteByte(0xA11100, 0x00);
		firstRun.StepFrameScaffold(128);

		GenesisM68kBoundaryScaffold replayRun;
		replayRun.LoadState(checkpoint);
		auto& replayBus = replayRun.GetBus();
		replayBus.WriteByte(0xA11100, 0x01);
		replayRun.StepFrameScaffold(96);
		replayBus.WriteByte(0xA11100, 0x00);
		replayRun.StepFrameScaffold(128);

		EXPECT_EQ(firstBus.GetZ80ExecutedCycles(), replayBus.GetZ80ExecutedCycles());
		EXPECT_EQ(firstBus.GetZ80HandoffCount(), replayBus.GetZ80HandoffCount());
		EXPECT_EQ(firstBus.GetDmaContentionCycles(), replayBus.GetDmaContentionCycles());
		EXPECT_EQ(firstBus.GetDmaContentionEvents(), replayBus.GetDmaContentionEvents());
		EXPECT_EQ(firstBus.IsZ80Running(), replayBus.IsZ80Running());
	}
}
