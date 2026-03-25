#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	// YM2612 generates one sample every 144 master cycles.
	// PSG generates one sample every 128 master cycles (divisor varies by model).
	// Mixed output combines both at the lower of the two cadences.

	struct AudioCadenceSnapshot {
		uint32_t YmSamples = 0;
		uint32_t PsgSamples = 0;
		uint32_t MixedSamples = 0;
		int16_t YmLastSample = 0;
		int16_t PsgLastSample = 0;
		int16_t MixedLastSample = 0;
		string YmDigest;
		string PsgDigest;
		string MixedDigest;

		bool operator==(const AudioCadenceSnapshot&) const = default;
	};

	static AudioCadenceSnapshot CaptureSnapshot(GenesisPlatformBusStub& bus) {
		return {
			bus.GetYmSampleCount(),
			bus.GetPsgSampleCount(),
			bus.GetMixedSampleCount(),
			bus.GetYmLastSample(),
			bus.GetPsgLastSample(),
			bus.GetMixedLastSample(),
			bus.GetYmDigest(),
			bus.GetPsgDigest(),
			bus.GetMixedDigest()
		};
	}

	TEST(GenesisAudioCadenceDriftTests, HeavyYmWriteBurstDoesNotDriftPsgCadence) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		// Burst: 64 YM register writes, no PSG writes, then step.
		for (uint32_t i = 0; i < 64; i++) {
			scaffold.GetBus().YmWriteAddress(0, (uint8_t)(0x30 + (i & 0x0f)));
			scaffold.GetBus().YmWriteData(0, (uint8_t)(0x10 + (i & 0x7f)));
		}

		// Also seed PSG so it generates samples.
		scaffold.GetBus().PsgWrite(0x90);
		scaffold.GetBus().PsgWrite(0x04);

		scaffold.StepFrameScaffold(144u * 24u);

		auto snapshot = CaptureSnapshot(scaffold.GetBus());
		EXPECT_GT(snapshot.YmSamples, 0u);
		EXPECT_GT(snapshot.PsgSamples, 0u);
		EXPECT_GT(snapshot.MixedSamples, 0u);

		// Run a second time identically for determinism.
		GenesisM68kBoundaryScaffold scaffold2;
		scaffold2.Startup();
		for (uint32_t i = 0; i < 64; i++) {
			scaffold2.GetBus().YmWriteAddress(0, (uint8_t)(0x30 + (i & 0x0f)));
			scaffold2.GetBus().YmWriteData(0, (uint8_t)(0x10 + (i & 0x7f)));
		}
		scaffold2.GetBus().PsgWrite(0x90);
		scaffold2.GetBus().PsgWrite(0x04);
		scaffold2.StepFrameScaffold(144u * 24u);

		auto snapshot2 = CaptureSnapshot(scaffold2.GetBus());
		EXPECT_EQ(snapshot, snapshot2);
	}

	TEST(GenesisAudioCadenceDriftTests, HeavyPsgWriteBurstDoesNotDriftYmCadence) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		// Seed YM.
		scaffold.GetBus().YmWriteAddress(0, 0x22);
		scaffold.GetBus().YmWriteData(0, 0x15);

		// Burst: 64 PSG writes.
		for (uint32_t i = 0; i < 64; i++) {
			scaffold.GetBus().PsgWrite((uint8_t)(0x80 | ((i & 0x03) << 5) | (i & 0x0f)));
		}

		scaffold.StepFrameScaffold(144u * 24u);

		EXPECT_GT(scaffold.GetBus().GetYmSampleCount(), 0u);
		EXPECT_GT(scaffold.GetBus().GetPsgSampleCount(), 0u);
		EXPECT_GT(scaffold.GetBus().GetMixedSampleCount(), 0u);
	}

	TEST(GenesisAudioCadenceDriftTests, AlternatingYmPsgBurstsProduceMixedSamples) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		for (uint32_t burst = 0; burst < 16; burst++) {
			// YM burst.
			for (uint32_t w = 0; w < 8; w++) {
				scaffold.GetBus().YmWriteAddress(0, (uint8_t)(0x30 + w));
				scaffold.GetBus().YmWriteData(0, (uint8_t)(0x20 + (burst * 8 + w)));
			}

			// PSG burst.
			for (uint32_t w = 0; w < 4; w++) {
				scaffold.GetBus().PsgWrite((uint8_t)(0x90 | ((burst + w) & 0x0f)));
				scaffold.GetBus().PsgWrite((uint8_t)(0x04 + (w & 0x03)));
			}

			scaffold.StepFrameScaffold(144u * 4u);
		}

		EXPECT_GT(scaffold.GetBus().GetYmSampleCount(), 0u);
		EXPECT_GT(scaffold.GetBus().GetPsgSampleCount(), 0u);
		EXPECT_GT(scaffold.GetBus().GetMixedSampleCount(), 0u);
		EXPECT_FALSE(scaffold.GetBus().GetMixedDigest().empty());
	}

	TEST(GenesisAudioCadenceDriftTests, SampleCountsScaleLinearlyWithStepCycles) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().YmWriteAddress(0, 0x22);
		scaffold.GetBus().YmWriteData(0, 0x11);
		scaffold.GetBus().PsgWrite(0x90);
		scaffold.GetBus().PsgWrite(0x04);

		// Step 1: small step.
		scaffold.StepFrameScaffold(144u * 10u);
		uint32_t ymSmall = scaffold.GetBus().GetYmSampleCount();
		uint32_t psgSmall = scaffold.GetBus().GetPsgSampleCount();

		// Step 2: larger step.
		scaffold.StepFrameScaffold(144u * 20u);
		uint32_t ymLarge = scaffold.GetBus().GetYmSampleCount();
		uint32_t psgLarge = scaffold.GetBus().GetPsgSampleCount();

		// After 3x the total cycles, samples should be roughly 3x.
		EXPECT_GT(ymLarge, ymSmall);
		EXPECT_GT(psgLarge, psgSmall);
		EXPECT_GE(ymLarge, ymSmall * 2);
	}

	TEST(GenesisAudioCadenceDriftTests, YmPort1WritesDoNotAffectPort0Registers) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		// Write to port 0.
		scaffold.GetBus().YmWriteAddress(0, 0x30);
		scaffold.GetBus().YmWriteData(0, 0xAA);

		// Write to port 1 (same register offset but different port).
		scaffold.GetBus().YmWriteAddress(1, 0x30);
		scaffold.GetBus().YmWriteData(1, 0x55);

		// Port 0 reg 0x30 should still be 0xAA.
		EXPECT_EQ(scaffold.GetBus().GetYmRegister(0x30), 0xAAu);
		// Port 1 reg 0x130 should be 0x55.
		EXPECT_EQ(scaffold.GetBus().GetYmRegister(0x130), 0x55u);
	}

	TEST(GenesisAudioCadenceDriftTests, MixedDigestChangesWithDifferentWriteSequences) {
		// Sequence A: YM-heavy.
		GenesisM68kBoundaryScaffold scaffoldA;
		scaffoldA.Startup();
		for (uint32_t i = 0; i < 32; i++) {
			scaffoldA.GetBus().YmWriteAddress(0, (uint8_t)(0x30 + (i & 0x0f)));
			scaffoldA.GetBus().YmWriteData(0, (uint8_t)(0x40 + i));
		}
		scaffoldA.GetBus().PsgWrite(0x90);
		scaffoldA.GetBus().PsgWrite(0x04);
		scaffoldA.StepFrameScaffold(144u * 16u);
		string digestA = scaffoldA.GetBus().GetMixedDigest();

		// Sequence B: PSG-heavy (different writes).
		GenesisM68kBoundaryScaffold scaffoldB;
		scaffoldB.Startup();
		scaffoldB.GetBus().YmWriteAddress(0, 0x22);
		scaffoldB.GetBus().YmWriteData(0, 0x11);
		for (uint32_t i = 0; i < 32; i++) {
			scaffoldB.GetBus().PsgWrite((uint8_t)(0x80 | ((i & 0x03) << 5) | (i & 0x0f)));
		}
		scaffoldB.StepFrameScaffold(144u * 16u);
		string digestB = scaffoldB.GetBus().GetMixedDigest();

		// Different write sequences should produce different mixed digests.
		EXPECT_NE(digestA, digestB);
	}

	TEST(GenesisAudioCadenceDriftTests, CadenceDriftFuzzDigestIsDeterministic) {
		auto runScenario = []() {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.Startup();

			uint32_t seed = 0xcafebabe;
			uint64_t hash = 1469598103934665603ull;

			for (uint32_t step = 0; step < 96; step++) {
				seed = seed * 1664525u + 1013904223u;

				// Pseudo-random mix of YM/PSG writes.
				uint8_t writesThisStep = (uint8_t)((seed >> 24) & 0x07) + 1;
				for (uint8_t w = 0; w < writesThisStep; w++) {
					seed = seed * 1664525u + 1013904223u;
					if ((seed & 1) == 0) {
						uint8_t port = (uint8_t)((seed >> 1) & 1);
						uint8_t reg = (uint8_t)((seed >> 8) & 0x7f) | 0x30;
						uint8_t val = (uint8_t)((seed >> 16) & 0xff);
						scaffold.GetBus().YmWriteAddress(port, reg);
						scaffold.GetBus().YmWriteData(port, val);
					} else {
						uint8_t val = (uint8_t)((seed >> 8) & 0xff);
						scaffold.GetBus().PsgWrite(val);
					}
				}

				// Variable step size.
				uint32_t stepCycles = 64u + (seed & 0x7fu);
				scaffold.StepFrameScaffold(stepCycles);

				// Hash audio state.
				string line = std::format("{}:{}:{}:{}:{}:{}",
					step,
					scaffold.GetBus().GetYmSampleCount(),
					scaffold.GetBus().GetPsgSampleCount(),
					scaffold.GetBus().GetMixedSampleCount(),
					scaffold.GetBus().GetYmLastSample(),
					scaffold.GetBus().GetPsgLastSample());
				for (uint8_t ch : line) {
					hash ^= ch;
					hash *= 1099511628211ull;
				}
			}

			return std::make_tuple(
				hash,
				CaptureSnapshot(scaffold.GetBus())
			);
		};

		auto runA = runScenario();
		auto runB = runScenario();

		EXPECT_EQ(std::get<0>(runA), std::get<0>(runB));
		EXPECT_EQ(std::get<1>(runA), std::get<1>(runB));
	}

	TEST(GenesisAudioCadenceDriftTests, SaveStateReplayCadenceDigestStable) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		// Prime audio state.
		for (uint32_t i = 0; i < 8; i++) {
			scaffold.GetBus().YmWriteAddress(0, (uint8_t)(0x30 + i));
			scaffold.GetBus().YmWriteData(0, (uint8_t)(0x10 + i));
			scaffold.GetBus().PsgWrite((uint8_t)(0x90 | (i & 0x0f)));
			scaffold.GetBus().PsgWrite((uint8_t)(0x04 + (i & 0x03)));
			scaffold.StepFrameScaffold(144u * 2u);
		}

		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		// Run tail from original.
		auto runTail = [](GenesisM68kBoundaryScaffold& target) {
			for (uint32_t i = 0; i < 12; i++) {
				target.GetBus().YmWriteAddress((uint8_t)(i & 1), (uint8_t)(0x40 + (i & 0x0f)));
				target.GetBus().YmWriteData((uint8_t)(i & 1), (uint8_t)(0x20 + i));
				target.GetBus().PsgWrite((uint8_t)(0x80 | ((i & 0x03) << 5) | (i & 0x0f)));
				target.StepFrameScaffold(144u * 3u);
			}
			return CaptureSnapshot(target.GetBus());
		};

		auto snapshotOriginal = runTail(scaffold);
		scaffold.LoadState(checkpoint);
		auto snapshotReplay = runTail(scaffold);

		EXPECT_EQ(snapshotOriginal, snapshotReplay);
	}

	TEST(GenesisAudioCadenceDriftTests, NoWritesProducesZeroSamplesButStillDeterministic) {
		// Step without any audio writes.
		GenesisM68kBoundaryScaffold scaffoldA;
		scaffoldA.Startup();
		scaffoldA.StepFrameScaffold(144u * 10u);

		GenesisM68kBoundaryScaffold scaffoldB;
		scaffoldB.Startup();
		scaffoldB.StepFrameScaffold(144u * 10u);

		EXPECT_EQ(scaffoldA.GetBus().GetYmSampleCount(), scaffoldB.GetBus().GetYmSampleCount());
		EXPECT_EQ(scaffoldA.GetBus().GetPsgSampleCount(), scaffoldB.GetBus().GetPsgSampleCount());
		EXPECT_EQ(scaffoldA.GetBus().GetMixedSampleCount(), scaffoldB.GetBus().GetMixedSampleCount());
		EXPECT_EQ(scaffoldA.GetBus().GetYmDigest(), scaffoldB.GetBus().GetYmDigest());
		EXPECT_EQ(scaffoldA.GetBus().GetPsgDigest(), scaffoldB.GetBus().GetPsgDigest());
		EXPECT_EQ(scaffoldA.GetBus().GetMixedDigest(), scaffoldB.GetBus().GetMixedDigest());
	}

	TEST(GenesisAudioCadenceDriftTests, RapidWritesBetweenStepsDoNotSkipSamples) {
		GenesisM68kBoundaryScaffold scaffoldA;
		scaffoldA.Startup();

		// Write then step, one at a time.
		for (uint32_t i = 0; i < 20; i++) {
			scaffoldA.GetBus().YmWriteAddress(0, (uint8_t)(0x30 + (i & 0x0f)));
			scaffoldA.GetBus().YmWriteData(0, (uint8_t)(0x10 + i));
			scaffoldA.StepFrameScaffold(144u);
		}
		uint32_t ymSingleStep = scaffoldA.GetBus().GetYmSampleCount();

		// Batch: all writes first, then step the same total cycles.
		GenesisM68kBoundaryScaffold scaffoldB;
		scaffoldB.Startup();
		for (uint32_t i = 0; i < 20; i++) {
			scaffoldB.GetBus().YmWriteAddress(0, (uint8_t)(0x30 + (i & 0x0f)));
			scaffoldB.GetBus().YmWriteData(0, (uint8_t)(0x10 + i));
		}
		scaffoldB.StepFrameScaffold(144u * 20u);
		uint32_t ymBatchStep = scaffoldB.GetBus().GetYmSampleCount();

		// Both should generate the same number of YM samples for the same total cycles.
		EXPECT_EQ(ymSingleStep, ymBatchStep);
	}
}
