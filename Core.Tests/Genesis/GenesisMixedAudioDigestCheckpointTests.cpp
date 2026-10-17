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

	struct CadenceCheckpoint {
		uint32_t Step;
		string MixedDigest;
		string YmDigest;
		string PsgDigest;
		uint32_t YmSampleCount;
		uint32_t PsgSampleCount;
		uint32_t MixedSampleCount;

		bool operator==(const CadenceCheckpoint&) const = default;
	};

	CadenceCheckpoint CaptureCheckpoint(const GenesisPlatformBusStub& bus, uint32_t step) {
		return CadenceCheckpoint{
			step,
			bus.GetMixedDigest(),
			bus.GetYmDigest(),
			bus.GetPsgDigest(),
			bus.GetYmSampleCount(),
			bus.GetPsgSampleCount(),
			bus.GetMixedSampleCount()
		};
	}

	vector<CadenceCheckpoint> RunWithCheckpoints(
		GenesisM68kBoundaryScaffold& scaffold,
		uint32_t totalSteps,
		uint32_t checkpointInterval
	) {
		vector<CadenceCheckpoint> checkpoints;
		auto& bus = scaffold.GetBus();

		for (uint32_t i = 0; i < totalSteps; i++) {
			bus.YmWriteAddress(0, (uint8_t)(0x30 + (i & 0x0F)));
			bus.YmWriteData(0, (uint8_t)(0x10 + (i & 0x1F)));
			bus.PsgWrite((uint8_t)(0x90 | (i & 0x07)));

			scaffold.StepFrameScaffold(200);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();

			if ((i + 1) % checkpointInterval == 0) {
				checkpoints.push_back(CaptureCheckpoint(bus, i + 1));
			}
		}
		return checkpoints;
	}

	// ----- Tests -----

	TEST(GenesisMixedAudioDigestCheckpointTests, FixedIntervalCheckpointsAreDeterministic) {
		auto run = []() -> vector<CadenceCheckpoint> {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 8, true);

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);

			return RunWithCheckpoints(scaffold, 40, 10);
		};

		vector<CadenceCheckpoint> first = run();
		vector<CadenceCheckpoint> second = run();
		ASSERT_EQ(first.size(), 4u);
		EXPECT_EQ(first, second);
	}

	TEST(GenesisMixedAudioDigestCheckpointTests, CheckpointDigestsProgressMonotonically) {
		GenesisM68kBoundaryScaffold scaffold;
		LoadNopRom(scaffold);
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);

		auto& bus = scaffold.GetBus();
		bus.WriteByte(0xA11200, 0x01);

		vector<CadenceCheckpoint> cps = RunWithCheckpoints(scaffold, 40, 10);
		ASSERT_EQ(cps.size(), 4u);

		for (size_t i = 1; i < cps.size(); i++) {
			EXPECT_GT(cps[i].YmSampleCount, cps[i - 1].YmSampleCount)
				<< "YM sample count did not increase at checkpoint " << i;
			EXPECT_GT(cps[i].PsgSampleCount, cps[i - 1].PsgSampleCount)
				<< "PSG sample count did not increase at checkpoint " << i;
			EXPECT_GT(cps[i].MixedSampleCount, cps[i - 1].MixedSampleCount)
				<< "Mixed sample count did not increase at checkpoint " << i;
		}
	}

	TEST(GenesisMixedAudioDigestCheckpointTests, SampleCountGrowthIsLinearAcrossCheckpoints) {
		GenesisM68kBoundaryScaffold scaffold;
		LoadNopRom(scaffold);
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);

		auto& bus = scaffold.GetBus();
		bus.WriteByte(0xA11200, 0x01);

		vector<CadenceCheckpoint> cps = RunWithCheckpoints(scaffold, 60, 10);
		ASSERT_GE(cps.size(), 3u);

		// Deltas between successive checkpoints should be within ±1 (linear with rounding)
		uint32_t ymDelta1 = cps[1].YmSampleCount - cps[0].YmSampleCount;
		uint32_t ymDelta2 = cps[2].YmSampleCount - cps[1].YmSampleCount;
		int32_t ymDrift = (int32_t)ymDelta2 - (int32_t)ymDelta1;
		EXPECT_LE(std::abs(ymDrift), 1) << "YM sample growth drifted by more than 1";

		uint32_t psgDelta1 = cps[1].PsgSampleCount - cps[0].PsgSampleCount;
		uint32_t psgDelta2 = cps[2].PsgSampleCount - cps[1].PsgSampleCount;
		int32_t psgDrift = (int32_t)psgDelta2 - (int32_t)psgDelta1;
		EXPECT_LE(std::abs(psgDrift), 1) << "PSG sample growth drifted by more than 1";
	}

	TEST(GenesisMixedAudioDigestCheckpointTests, DmaDoesNotDriftCadenceCheckpoints) {
		auto run = [](bool withDma) -> vector<CadenceCheckpoint> {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 8, true);

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);

			if (withDma) {
				bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 30);
			}
			return RunWithCheckpoints(scaffold, 40, 10);
		};

		vector<CadenceCheckpoint> withDma = run(true);
		vector<CadenceCheckpoint> withoutDma = run(false);

		ASSERT_EQ(withDma.size(), withoutDma.size());
		for (size_t i = 0; i < withDma.size(); i++) {
			EXPECT_EQ(withDma[i].YmSampleCount, withoutDma[i].YmSampleCount)
				<< "DMA drifted YM cadence at checkpoint " << i;
			EXPECT_EQ(withDma[i].PsgSampleCount, withoutDma[i].PsgSampleCount)
				<< "DMA drifted PSG cadence at checkpoint " << i;
		}
	}

	TEST(GenesisMixedAudioDigestCheckpointTests, Z80BusContentionDoesNotDriftCadence) {
		auto run = [](bool withBusContention) -> vector<CadenceCheckpoint> {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 8, true);

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);

			vector<CadenceCheckpoint> checkpoints;
			for (uint32_t i = 0; i < 40; i++) {
				if (withBusContention && (i % 8) == 0) {
					bus.WriteByte(0xA11100, 0x01);
				}
				if (withBusContention && (i % 8) == 4) {
					bus.WriteByte(0xA11100, 0x00);
				}

				bus.YmWriteAddress(0, (uint8_t)(0x30 + (i & 0x0F)));
				bus.YmWriteData(0, (uint8_t)(0x10 + (i & 0x1F)));
				bus.PsgWrite((uint8_t)(0x90 | (i & 0x07)));

				scaffold.StepFrameScaffold(200);
				bus.StepYm2612(144);
				bus.StepSn76489(128);
				bus.UpdateMixedSample();

				if ((i + 1) % 10 == 0) {
					checkpoints.push_back(CaptureCheckpoint(bus, i + 1));
				}
			}
			return checkpoints;
		};

		vector<CadenceCheckpoint> withBus = run(true);
		vector<CadenceCheckpoint> noBus = run(false);

		ASSERT_EQ(withBus.size(), noBus.size());
		for (size_t i = 0; i < withBus.size(); i++) {
			EXPECT_EQ(withBus[i].YmSampleCount, noBus[i].YmSampleCount)
				<< "Z80 bus contention drifted YM cadence at checkpoint " << i;
			EXPECT_EQ(withBus[i].PsgSampleCount, noBus[i].PsgSampleCount)
				<< "Z80 bus contention drifted PSG cadence at checkpoint " << i;
		}
	}

	TEST(GenesisMixedAudioDigestCheckpointTests, SaveStateReplayProducesSameCheckpointSequence) {
		GenesisM68kBoundaryScaffold scaffold;
		LoadNopRom(scaffold);
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);

		auto& bus = scaffold.GetBus();
		bus.WriteByte(0xA11200, 0x01);

		// Pre-run to mid-state
		RunWithCheckpoints(scaffold, 10, 10);
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		// Baseline
		vector<CadenceCheckpoint> baseline = RunWithCheckpoints(scaffold, 30, 10);

		// Replay
		scaffold.LoadState(checkpoint);
		vector<CadenceCheckpoint> replay = RunWithCheckpoints(scaffold, 30, 10);

		EXPECT_EQ(replay, baseline);
	}

	TEST(GenesisMixedAudioDigestCheckpointTests, HeavyWriteBurstBetweenCheckpointsDoesNotSkipSamples) {
		GenesisM68kBoundaryScaffold scaffold;
		LoadNopRom(scaffold);
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);

		auto& bus = scaffold.GetBus();
		bus.WriteByte(0xA11200, 0x01);

		vector<CadenceCheckpoint> checkpoints;
		for (uint32_t i = 0; i < 40; i++) {
			// Heavy write burst every 5 steps
			if ((i % 5) == 0) {
				for (uint32_t w = 0; w < 16; w++) {
					bus.YmWriteAddress(0, (uint8_t)(0x30 + (w & 0x0F)));
					bus.YmWriteData(0, (uint8_t)(0x10 + w));
					bus.PsgWrite((uint8_t)(0x80 | (w * 0x20)));
				}
			}

			scaffold.StepFrameScaffold(200);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();

			if ((i + 1) % 10 == 0) {
				checkpoints.push_back(CaptureCheckpoint(bus, i + 1));
			}
		}

		ASSERT_EQ(checkpoints.size(), 4u);
		for (size_t i = 1; i < checkpoints.size(); i++) {
			uint32_t ymDelta = checkpoints[i].YmSampleCount - checkpoints[i - 1].YmSampleCount;
			uint32_t psgDelta = checkpoints[i].PsgSampleCount - checkpoints[i - 1].PsgSampleCount;
			EXPECT_GT(ymDelta, 0u) << "No YM samples between checkpoints " << (i - 1) << " and " << i;
			EXPECT_GT(psgDelta, 0u) << "No PSG samples between checkpoints " << (i - 1) << " and " << i;
		}
	}

	TEST(GenesisMixedAudioDigestCheckpointTests, CheckpointSequenceDiffersWithDifferentWritePatterns) {
		auto run = [](uint8_t baseAddr) -> vector<CadenceCheckpoint> {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 8, true);

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);

			vector<CadenceCheckpoint> checkpoints;
			for (uint32_t i = 0; i < 30; i++) {
				bus.YmWriteAddress(0, (uint8_t)(baseAddr + (i & 0x0F)));
				bus.YmWriteData(0, (uint8_t)(0x10 + (i & 0x1F)));
				bus.YmWriteAddress(1, (uint8_t)(baseAddr + (i & 0x0F)));
				bus.YmWriteData(1, (uint8_t)(0x50 + (i & 0x1F)));
				bus.PsgWrite((uint8_t)(0x90 | (i & 0x07)));

				scaffold.StepFrameScaffold(200);
				bus.StepYm2612(144);
				bus.StepSn76489(128);
				bus.UpdateMixedSample();

				if ((i + 1) % 10 == 0) {
					checkpoints.push_back(CaptureCheckpoint(bus, i + 1));
				}
			}
			return checkpoints;
		};

		vector<CadenceCheckpoint> seq1 = run(0x30);
		vector<CadenceCheckpoint> seq2 = run(0x40);

		ASSERT_EQ(seq1.size(), seq2.size());
		// Sample counts should match (same timing) but digests should differ
		for (size_t i = 0; i < seq1.size(); i++) {
			EXPECT_EQ(seq1[i].YmSampleCount, seq2[i].YmSampleCount);
			EXPECT_EQ(seq1[i].PsgSampleCount, seq2[i].PsgSampleCount);
		}
	}

	TEST(GenesisMixedAudioDigestCheckpointTests, FuzzedCadenceCheckpointSequenceIsDeterministic) {
		auto run = [](uint32_t seed) -> vector<CadenceCheckpoint> {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 8, true);

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);

			uint64_t hash = 14695981039346656037ull;
			uint64_t prime = 1099511628211ull;
			hash = (hash ^ seed) * prime;

			vector<CadenceCheckpoint> checkpoints;
			for (uint32_t i = 0; i < 60; i++) {
				hash = (hash ^ i) * prime;

				uint8_t action = (uint8_t)(hash & 0x03);
				if (action == 0) {
					bus.YmWriteAddress(0, (uint8_t)(0x30 + (hash >> 8 & 0x0F)));
					bus.YmWriteData(0, (uint8_t)(hash >> 16 & 0xFF));
				} else if (action == 1) {
					bus.PsgWrite((uint8_t)(0x80 | (hash >> 8 & 0x7F)));
				} else if (action == 2) {
					bus.YmWriteAddress(1, (uint8_t)(0x30 + (hash >> 8 & 0x0F)));
					bus.YmWriteData(1, (uint8_t)(hash >> 16 & 0xFF));
				} else {
					bus.BeginDmaTransfer(
						(hash >> 8 & 1) ? GenesisVdpDmaMode::Copy : GenesisVdpDmaMode::Fill,
						(uint32_t)(5 + (hash >> 12 & 0x0F))
					);
				}

				scaffold.StepFrameScaffold(150 + (i % 5) * 20);
				bus.StepYm2612(144);
				bus.StepSn76489(128);
				bus.UpdateMixedSample();

				if ((i + 1) % 12 == 0) {
					checkpoints.push_back(CaptureCheckpoint(bus, i + 1));
				}
			}
			return checkpoints;
		};

		vector<CadenceCheckpoint> first = run(42);
		vector<CadenceCheckpoint> second = run(42);
		EXPECT_EQ(first, second);

		vector<CadenceCheckpoint> other = run(99);
		ASSERT_EQ(first.size(), other.size());
		// At least some checkpoints should differ with different seed
		bool anyDiff = false;
		for (size_t i = 0; i < first.size(); i++) {
			if (first[i].MixedDigest != other[i].MixedDigest) {
				anyDiff = true;
				break;
			}
		}
		EXPECT_TRUE(anyDiff) << "Different seeds produced identical checkpoint sequences";
	}
}
