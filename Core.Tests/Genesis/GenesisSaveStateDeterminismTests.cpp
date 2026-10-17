#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	void LoadNopRom(GenesisM68kBoundaryScaffold& scaffold) {
		vector<uint8_t> rom(0x4000, 0);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4E;
			rom[i + 1] = 0x71;
		}
		scaffold.LoadRom(rom);
	}

	void RunDeterministicExecutionBlock(GenesisM68kBoundaryScaffold& scaffold) {
		auto& bus = scaffold.GetBus();
		bus.WriteByte(0xA11200, 0x01);
		bus.WriteByte(0xA11100, 0x01);
		bus.WriteByte(0xA11100, 0x00);

		bus.WriteByte(0xA04000, 0x22);
		bus.WriteByte(0xA04001, 0x14);
		bus.WriteByte(0xA04002, 0x30);
		bus.WriteByte(0xA04003, 0x08);

		bus.WriteByte(0xC00004, 0x97);
		bus.WriteByte(0xC00005, 0x80);
		bus.WriteByte(0xC00004, 0x81);
		bus.WriteByte(0xC00005, 0x60);
		bus.WriteByte(0xC00011, 0x90);
		bus.WriteByte(0xC00011, 0x25);

		bus.SetRenderCompositionInputs(0x18, true, 0x05, false, 0x0F, true, true, 0x24, true);
		bus.SetScroll(7, 3);
		bus.RenderScaffoldLine(64);

		scaffold.StepFrameScaffold(488 + 73);
	}

	TEST(GenesisSaveStateDeterminismTests, SaveStateRoundTripRestoresStateExactly) {
		GenesisM68kBoundaryScaffold scaffold;
		LoadNopRom(scaffold);
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);
		RunDeterministicExecutionBlock(scaffold);

		GenesisBoundaryScaffoldSaveState saved = scaffold.SaveState();

		scaffold.StepFrameScaffold(1337);
		scaffold.GetBus().WriteByte(0xA04000, 0x24);
		scaffold.GetBus().WriteByte(0xA04001, 0x7F);
		scaffold.GetBus().WriteByte(0xC00011, 0x9F);
		scaffold.GetBus().RenderScaffoldLine(32);

		scaffold.LoadState(saved);
		GenesisBoundaryScaffoldSaveState restored = scaffold.SaveState();
		EXPECT_EQ(restored, saved);
	}

	TEST(GenesisSaveStateDeterminismTests, RestoreReplayProducesDeterministicStateAndDigests) {
		GenesisM68kBoundaryScaffold scaffold;
		LoadNopRom(scaffold);
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 16, true);
		scaffold.StepFrameScaffold(320);

		GenesisBoundaryScaffoldSaveState baseline = scaffold.SaveState();
		RunDeterministicExecutionBlock(scaffold);
		GenesisBoundaryScaffoldSaveState firstRun = scaffold.SaveState();

		scaffold.LoadState(baseline);
		RunDeterministicExecutionBlock(scaffold);
		GenesisBoundaryScaffoldSaveState replayRun = scaffold.SaveState();

		EXPECT_EQ(replayRun, firstRun);
		EXPECT_EQ(replayRun.Bus.RenderLineDigest, firstRun.Bus.RenderLineDigest);
		EXPECT_EQ(replayRun.Bus.Ym2612Digest, firstRun.Bus.Ym2612Digest);
		EXPECT_EQ(replayRun.Bus.Sn76489Digest, firstRun.Bus.Sn76489Digest);
		EXPECT_EQ(replayRun.Bus.MixedDigest, firstRun.Bus.MixedDigest);
	}

	TEST(GenesisSaveStateDeterminismTests, DeepReplayWithDmaInterruptAndAudioStateMatchesExactly) {
		auto setupBase = [](GenesisM68kBoundaryScaffold& scaffold) {
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 4, true);
			scaffold.ClearTimingEvents();

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);
			bus.WriteByte(0xA04000, 0x22);
			bus.WriteByte(0xA04001, 0x1A);
			bus.WriteByte(0xC00011, 0x90);
			bus.WriteByte(0xC00011, 0x08);
			bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 28);

			for (uint32_t i = 0; i < 6; i++) {
				scaffold.StepFrameScaffold(244u + i);
			}
		};

		auto runTail = [](GenesisM68kBoundaryScaffold& scaffold) {
			auto& bus = scaffold.GetBus();
			for (uint32_t i = 0; i < 16; i++) {
				if (i == 2 || i == 9) {
					bus.WriteByte(0xA11100, 0x01);
				}
				if (i == 4 || i == 12) {
					bus.WriteByte(0xA11100, 0x00);
				}
				if (i == 5) {
					bus.BeginDmaTransfer(GenesisVdpDmaMode::Fill, 18);
				}
				if ((i % 3) == 0) {
					bus.WriteByte(0xA04002, 0x30);
					bus.WriteByte(0xA04003, (uint8_t)(0x0E + i));
					bus.WriteByte(0xC00011, (uint8_t)(0x98 | (i & 0x07)));
				}
				bus.SetRenderCompositionInputs((uint8_t)(0x10 + i), true, 0x04, false, 0x0A, true, true, (uint8_t)(0x20 + i), true);
				bus.SetScroll((uint16_t)(i + 3), (uint16_t)(i + 1));
				scaffold.StepFrameScaffold(170u + (i % 6u));
				bus.RenderScaffoldLine(64);
			}
			return scaffold.SaveState();
		};

		GenesisM68kBoundaryScaffold scaffold;
		setupBase(scaffold);
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();
		GenesisBoundaryScaffoldSaveState first = runTail(scaffold);

		scaffold.LoadState(checkpoint);
		GenesisBoundaryScaffoldSaveState replay = runTail(scaffold);

		EXPECT_EQ(first, replay);
		EXPECT_FALSE(first.Bus.RenderLineDigest.empty());
		EXPECT_FALSE(first.Bus.Ym2612Digest.empty());
		EXPECT_FALSE(first.Bus.Sn76489Digest.empty());
		EXPECT_FALSE(first.Bus.MixedDigest.empty());
		EXPECT_GT(first.HInterruptCount, 0u);
		EXPECT_EQ(first.VInterruptCount, 0u);
		EXPECT_GT(first.Bus.DmaContentionEvents, 0u);
		EXPECT_GT(first.Bus.Z80HandoffCount, 0u);
	}

	TEST(GenesisSaveStateDeterminismTests, LongRunReplayAcrossFrameRolloverKeepsInterruptCountsStable) {
		auto runTail = [](GenesisM68kBoundaryScaffold& scaffold) {
			auto& bus = scaffold.GetBus();
			for (uint32_t i = 0; i < 100; i++) {
				if ((i % 25) == 0) {
					bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 12);
				}
				if ((i % 10) == 0) {
					bus.WriteByte(0xA04000, 0x22);
					bus.WriteByte(0xA04001, (uint8_t)(0x10 + (i & 0x1F)));
					bus.WriteByte(0xC00011, (uint8_t)(0x90 | (i & 0x0F)));
				}
				scaffold.StepFrameScaffold(488u);
			}
			return scaffold.SaveState();
		};

		GenesisM68kBoundaryScaffold scaffold;
		LoadNopRom(scaffold);
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);
		scaffold.StepFrameScaffold(488u * 200u);

		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();
		GenesisBoundaryScaffoldSaveState first = runTail(scaffold);

		scaffold.LoadState(checkpoint);
		GenesisBoundaryScaffoldSaveState replay = runTail(scaffold);

		EXPECT_EQ(first, replay);
		EXPECT_GT(first.HInterruptCount, checkpoint.HInterruptCount);
		EXPECT_GT(first.VInterruptCount, checkpoint.VInterruptCount);
		EXPECT_GE(first.TimingFrame, checkpoint.TimingFrame + 1u);
		EXPECT_FALSE(first.Bus.MixedDigest.empty());
	}
}
