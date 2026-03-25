#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	struct GenesisMixedAudioSnapshot {
		uint32_t YmSamples = 0;
		uint32_t PsgSamples = 0;
		uint32_t MixedSamples = 0;
		string YmDigest;
		string PsgDigest;
		string MixedDigest;

		bool operator==(const GenesisMixedAudioSnapshot&) const = default;
	};

	GenesisMixedAudioSnapshot RunAlternatingAudioCadenceScenario() {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		for (uint32_t i = 0; i < 24; i++) {
			scaffold.GetBus().WriteByte(0xA04000, 0x22);
			scaffold.GetBus().WriteByte(0xA04001, (uint8_t)(0x10 + i));
			scaffold.GetBus().WriteByte(0xA04002, 0x30);
			scaffold.GetBus().WriteByte(0xA04003, (uint8_t)(0x08 + (i & 0x0F)));
			scaffold.GetBus().WriteByte(0xC00011, (uint8_t)(0x90 | (i & 0x0F)));
			scaffold.GetBus().WriteByte(0xC00011, (uint8_t)(0x04 + (i & 0x07)));
			scaffold.StepFrameScaffold(128u + (i % 5u));
		}

		GenesisMixedAudioSnapshot snapshot = {};
		snapshot.YmSamples = scaffold.GetBus().GetYmSampleCount();
		snapshot.PsgSamples = scaffold.GetBus().GetPsgSampleCount();
		snapshot.MixedSamples = scaffold.GetBus().GetMixedSampleCount();
		snapshot.YmDigest = scaffold.GetBus().GetYmDigest();
		snapshot.PsgDigest = scaffold.GetBus().GetPsgDigest();
		snapshot.MixedDigest = scaffold.GetBus().GetMixedDigest();
		return snapshot;
	}

	TEST(GenesisPsgMixedAudioTests, PsgWritesThroughVdpPortUpdateRegisterState) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xC00011, 0x90);
		scaffold.GetBus().WriteByte(0xC00011, 0x05);

		EXPECT_EQ(scaffold.GetBus().GetPsgWriteCount(), 2u);
		EXPECT_NE(scaffold.GetBus().GetPsgRegister(1), 0u);
	}

	TEST(GenesisPsgMixedAudioTests, PsgTimingProgressesWithStepCycles) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.GetBus().WriteByte(0xC00011, 0x90);
		scaffold.GetBus().WriteByte(0xC00011, 0x08);

		scaffold.StepFrameScaffold(127);
		EXPECT_EQ(scaffold.GetBus().GetPsgSampleCount(), 0u);
		scaffold.StepFrameScaffold(1);
		EXPECT_EQ(scaffold.GetBus().GetPsgSampleCount(), 1u);
	}

	TEST(GenesisPsgMixedAudioTests, MixedOutputDigestIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffoldA;
		scaffoldA.Startup();
		scaffoldA.GetBus().WriteByte(0xA04000, 0x22);
		scaffoldA.GetBus().WriteByte(0xA04001, 0x12);
		scaffoldA.GetBus().WriteByte(0xC00011, 0x90);
		scaffoldA.GetBus().WriteByte(0xC00011, 0x04);
		scaffoldA.StepFrameScaffold(144u * 8u);

		GenesisM68kBoundaryScaffold scaffoldB;
		scaffoldB.Startup();
		scaffoldB.GetBus().WriteByte(0xA04000, 0x22);
		scaffoldB.GetBus().WriteByte(0xA04001, 0x12);
		scaffoldB.GetBus().WriteByte(0xC00011, 0x90);
		scaffoldB.GetBus().WriteByte(0xC00011, 0x04);
		scaffoldB.StepFrameScaffold(144u * 8u);

		EXPECT_EQ(scaffoldA.GetBus().GetMixedSampleCount(), scaffoldB.GetBus().GetMixedSampleCount());
		EXPECT_EQ(scaffoldA.GetBus().GetMixedLastSample(), scaffoldB.GetBus().GetMixedLastSample());
		EXPECT_EQ(scaffoldA.GetBus().GetMixedDigest(), scaffoldB.GetBus().GetMixedDigest());
	}

	TEST(GenesisPsgMixedAudioTests, AlternatingYmAndPsgCadenceStaysDeterministicAcrossRuns) {
		GenesisMixedAudioSnapshot runA = RunAlternatingAudioCadenceScenario();
		GenesisMixedAudioSnapshot runB = RunAlternatingAudioCadenceScenario();

		EXPECT_GT(runA.YmSamples, 0u);
		EXPECT_GT(runA.PsgSamples, 0u);
		EXPECT_GT(runA.MixedSamples, 0u);
		EXPECT_FALSE(runA.YmDigest.empty());
		EXPECT_FALSE(runA.PsgDigest.empty());
		EXPECT_FALSE(runA.MixedDigest.empty());

		EXPECT_EQ(runA, runB);
	}

	TEST(GenesisPsgMixedAudioTests, SaveStateReplayKeepsMixedAudioDigestsStable) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		for (uint32_t i = 0; i < 10; i++) {
			scaffold.GetBus().WriteByte(0xA04000, 0x22);
			scaffold.GetBus().WriteByte(0xA04001, (uint8_t)(0x20 + i));
			scaffold.GetBus().WriteByte(0xC00011, (uint8_t)(0x90 | (i & 0x0F)));
			scaffold.GetBus().WriteByte(0xC00011, (uint8_t)(0x06 + (i & 0x03)));
			scaffold.StepFrameScaffold(144u);
		}

		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		auto runTail = [](GenesisM68kBoundaryScaffold& target) {
			for (uint32_t i = 0; i < 12; i++) {
				target.GetBus().WriteByte(0xA04002, 0x30);
				target.GetBus().WriteByte(0xA04003, (uint8_t)(0x10 + i));
				target.GetBus().WriteByte(0xC00011, (uint8_t)(0x98 | (i & 0x07)));
				target.StepFrameScaffold(132u + (i % 4u));
			}
			return target.SaveState();
		};

		GenesisBoundaryScaffoldSaveState first = runTail(scaffold);
		scaffold.LoadState(checkpoint);
		GenesisBoundaryScaffoldSaveState replay = runTail(scaffold);

		EXPECT_EQ(first.Bus.Ym2612SampleCount, replay.Bus.Ym2612SampleCount);
		EXPECT_EQ(first.Bus.Sn76489SampleCount, replay.Bus.Sn76489SampleCount);
		EXPECT_EQ(first.Bus.MixedSampleCount, replay.Bus.MixedSampleCount);
		EXPECT_EQ(first.Bus.Ym2612Digest, replay.Bus.Ym2612Digest);
		EXPECT_EQ(first.Bus.Sn76489Digest, replay.Bus.Sn76489Digest);
		EXPECT_EQ(first.Bus.MixedDigest, replay.Bus.MixedDigest);
	}
}
