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

	struct ContentionDigest {
		string RenderLineDigest;
		string Ym2612Digest;
		string Sn76489Digest;
		string MixedDigest;
		uint32_t DmaContentionEvents;
		uint32_t DmaContentionCycles;
		uint32_t YmSampleCount;
		uint32_t PsgSampleCount;
		uint32_t MixedSampleCount;

		bool operator==(const ContentionDigest&) const = default;
	};

	ContentionDigest CaptureDigest(const GenesisPlatformBusStub& bus) {
		return ContentionDigest{
			bus.GetRenderLineDigest(),
			bus.GetYmDigest(),
			bus.GetPsgDigest(),
			bus.GetMixedDigest(),
			bus.GetDmaContentionEvents(),
			bus.GetDmaContentionCycles(),
			bus.GetYmSampleCount(),
			bus.GetPsgSampleCount(),
			bus.GetMixedSampleCount()
		};
	}

	void SetupBaseState(GenesisM68kBoundaryScaffold& scaffold) {
		LoadNopRom(scaffold);
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);
		auto& bus = scaffold.GetBus();
		bus.WriteByte(0xA11200, 0x01);
		bus.YmWriteAddress(0, 0x22);
		bus.YmWriteData(0, 0x14);
		bus.PsgWrite(0x90);
		bus.PsgWrite(0x08);
		scaffold.StepFrameScaffold(488 * 10);
	}

	void RunContentionTail(GenesisM68kBoundaryScaffold& scaffold, uint32_t steps) {
		auto& bus = scaffold.GetBus();
		for (uint32_t i = 0; i < steps; i++) {
			if (i == 0) {
				bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 20);
			}
			if ((i % 3) == 0) {
				bus.YmWriteAddress(0, 0x30 + (uint8_t)(i & 0x0F));
				bus.YmWriteData(0, (uint8_t)(0x10 + i));
			}
			if ((i % 4) == 0) {
				bus.PsgWrite((uint8_t)(0x90 | (i & 0x07)));
			}
			scaffold.StepFrameScaffold(200);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
			if ((i % 5) == 0) {
				bus.SetRenderCompositionInputs((uint8_t)(0x10 + i), true, 0x04, false, 0x0A, true, true, (uint8_t)(0x20 + i), true);
				bus.RenderScaffoldLine(64);
			}
		}
	}

	// ----- Tests -----

	TEST(GenesisSaveStateBoundaryReplayTests, SaveBeforeDmaReplayMatchesBaseline) {
		GenesisM68kBoundaryScaffold scaffold;
		SetupBaseState(scaffold);

		// Checkpoint before DMA begins
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		// Baseline run
		RunContentionTail(scaffold, 20);
		ContentionDigest baseline = CaptureDigest(scaffold.GetBus());
		GenesisBoundaryScaffoldSaveState baselineState = scaffold.SaveState();

		// Replay from checkpoint
		scaffold.LoadState(checkpoint);
		RunContentionTail(scaffold, 20);
		ContentionDigest replay = CaptureDigest(scaffold.GetBus());
		GenesisBoundaryScaffoldSaveState replayState = scaffold.SaveState();

		EXPECT_EQ(replay, baseline);
		EXPECT_EQ(replayState, baselineState);
	}

	TEST(GenesisSaveStateBoundaryReplayTests, SaveDuringActiveDmaReplayMatchesBaseline) {
		GenesisM68kBoundaryScaffold scaffold;
		SetupBaseState(scaffold);

		auto& bus = scaffold.GetBus();
		bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 40);
		scaffold.StepFrameScaffold(200);
		bus.StepYm2612(144);

		// Mid-DMA checkpoint
		EXPECT_GT(bus.GetDmaActiveCyclesRemaining(), 0u);
		GenesisBoundaryScaffoldSaveState midDmaCheckpoint = scaffold.SaveState();

		// Baseline: continue from mid-DMA
		for (uint32_t i = 0; i < 15; i++) {
			scaffold.StepFrameScaffold(200);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest baseline = CaptureDigest(bus);

		// Replay from mid-DMA checkpoint
		scaffold.LoadState(midDmaCheckpoint);
		for (uint32_t i = 0; i < 15; i++) {
			scaffold.StepFrameScaffold(200);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest replay = CaptureDigest(bus);

		EXPECT_EQ(replay, baseline);
	}

	TEST(GenesisSaveStateBoundaryReplayTests, SaveAfterDmaCompletesReplayMatchesBaseline) {
		GenesisM68kBoundaryScaffold scaffold;
		SetupBaseState(scaffold);

		auto& bus = scaffold.GetBus();
		bus.BeginDmaTransfer(GenesisVdpDmaMode::Fill, 10);
		// Step enough to fully consume DMA
		scaffold.StepFrameScaffold(488 * 5);

		EXPECT_EQ(bus.GetDmaActiveCyclesRemaining(), 0u);
		GenesisBoundaryScaffoldSaveState postDma = scaffold.SaveState();

		// Baseline tail with audio
		for (uint32_t i = 0; i < 12; i++) {
			bus.YmWriteAddress(0, 0x30 + (uint8_t)(i & 0x0F));
			bus.YmWriteData(0, (uint8_t)(0x20 + i));
			bus.PsgWrite((uint8_t)(0x90 | (i & 0x07)));
			scaffold.StepFrameScaffold(300);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest baseline = CaptureDigest(bus);

		// Replay from post-DMA
		scaffold.LoadState(postDma);
		for (uint32_t i = 0; i < 12; i++) {
			bus.YmWriteAddress(0, 0x30 + (uint8_t)(i & 0x0F));
			bus.YmWriteData(0, (uint8_t)(0x20 + i));
			bus.PsgWrite((uint8_t)(0x90 | (i & 0x07)));
			scaffold.StepFrameScaffold(300);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest replay = CaptureDigest(bus);

		EXPECT_EQ(replay, baseline);
	}

	TEST(GenesisSaveStateBoundaryReplayTests, SaveDuringAudioSteppingReplayMatchesBaseline) {
		GenesisM68kBoundaryScaffold scaffold;
		SetupBaseState(scaffold);

		auto& bus = scaffold.GetBus();
		// Feed audio registers and step partially
		bus.YmWriteAddress(0, 0x40);
		bus.YmWriteData(0, 0x7F);
		bus.StepYm2612(144 * 3);
		bus.PsgWrite(0xA0);
		bus.PsgWrite(0x15);
		bus.StepSn76489(128 * 3);
		bus.UpdateMixedSample();

		GenesisBoundaryScaffoldSaveState midAudio = scaffold.SaveState();

		// Baseline: continue with more audio
		for (uint32_t i = 0; i < 10; i++) {
			bus.YmWriteAddress(1, 0x30 + (uint8_t)(i & 0x0F));
			bus.YmWriteData(1, (uint8_t)(0x08 + i));
			bus.StepYm2612(144);
			bus.PsgWrite((uint8_t)(0xA0 | (i & 0x07)));
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest baseline = CaptureDigest(bus);

		scaffold.LoadState(midAudio);
		for (uint32_t i = 0; i < 10; i++) {
			bus.YmWriteAddress(1, 0x30 + (uint8_t)(i & 0x0F));
			bus.YmWriteData(1, (uint8_t)(0x08 + i));
			bus.StepYm2612(144);
			bus.PsgWrite((uint8_t)(0xA0 | (i & 0x07)));
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest replay = CaptureDigest(bus);

		EXPECT_EQ(replay, baseline);
	}

	TEST(GenesisSaveStateBoundaryReplayTests, DmaAndYmSimultaneousContentionReplayMatchesBaseline) {
		GenesisM68kBoundaryScaffold scaffold;
		SetupBaseState(scaffold);

		auto& bus = scaffold.GetBus();
		// Kick off DMA and heavy YM writes in same window
		bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 30);
		for (uint32_t i = 0; i < 5; i++) {
			bus.YmWriteAddress(0, (uint8_t)(0x30 + i));
			bus.YmWriteData(0, (uint8_t)(0x10 + i));
			scaffold.StepFrameScaffold(100);
			bus.StepYm2612(144);
		}

		GenesisBoundaryScaffoldSaveState midContention = scaffold.SaveState();

		// Baseline
		for (uint32_t i = 0; i < 20; i++) {
			if ((i % 4) == 0) {
				bus.YmWriteAddress(0, (uint8_t)(0x40 + i));
				bus.YmWriteData(0, (uint8_t)(0x20 + i));
			}
			scaffold.StepFrameScaffold(200);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest baseline = CaptureDigest(bus);

		scaffold.LoadState(midContention);
		for (uint32_t i = 0; i < 20; i++) {
			if ((i % 4) == 0) {
				bus.YmWriteAddress(0, (uint8_t)(0x40 + i));
				bus.YmWriteData(0, (uint8_t)(0x20 + i));
			}
			scaffold.StepFrameScaffold(200);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest replay = CaptureDigest(bus);

		EXPECT_EQ(replay, baseline);
	}

	TEST(GenesisSaveStateBoundaryReplayTests, DmaAndPsgSimultaneousContentionReplayMatchesBaseline) {
		GenesisM68kBoundaryScaffold scaffold;
		SetupBaseState(scaffold);

		auto& bus = scaffold.GetBus();
		bus.BeginDmaTransfer(GenesisVdpDmaMode::Fill, 25);
		for (uint32_t i = 0; i < 5; i++) {
			bus.PsgWrite((uint8_t)(0x80 | (i * 0x20)));
			bus.PsgWrite((uint8_t)(0x08 + i));
			scaffold.StepFrameScaffold(100);
			bus.StepSn76489(128);
		}

		GenesisBoundaryScaffoldSaveState midContention = scaffold.SaveState();

		for (uint32_t i = 0; i < 16; i++) {
			if ((i % 3) == 0) {
				bus.PsgWrite((uint8_t)(0x90 | (i & 0x0F)));
			}
			scaffold.StepFrameScaffold(250);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest baseline = CaptureDigest(bus);

		scaffold.LoadState(midContention);
		for (uint32_t i = 0; i < 16; i++) {
			if ((i % 3) == 0) {
				bus.PsgWrite((uint8_t)(0x90 | (i & 0x0F)));
			}
			scaffold.StepFrameScaffold(250);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest replay = CaptureDigest(bus);

		EXPECT_EQ(replay, baseline);
	}

	TEST(GenesisSaveStateBoundaryReplayTests, MultiCheckpointAcrossDmaPhasesAllReplayCorrectly) {
		GenesisM68kBoundaryScaffold scaffold;
		SetupBaseState(scaffold);

		auto& bus = scaffold.GetBus();

		// Phase 0 checkpoint (pre-DMA)
		GenesisBoundaryScaffoldSaveState phase0 = scaffold.SaveState();

		// Phase 1 — DMA begins
		bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 15);
		scaffold.StepFrameScaffold(300);
		bus.StepYm2612(144);
		bus.StepSn76489(128);
		bus.UpdateMixedSample();
		GenesisBoundaryScaffoldSaveState phase1 = scaffold.SaveState();

		// Phase 2 — more stepping, DMA should complete
		scaffold.StepFrameScaffold(488 * 4);
		bus.StepYm2612(144 * 4);
		bus.StepSn76489(128 * 4);
		bus.UpdateMixedSample();
		GenesisBoundaryScaffoldSaveState phase2 = scaffold.SaveState();

		// Phase 3 — post-DMA tail with heavy writes
		for (uint32_t i = 0; i < 8; i++) {
			bus.YmWriteAddress(0, (uint8_t)(0x30 + i));
			bus.YmWriteData(0, (uint8_t)(0x0A + i));
			bus.PsgWrite((uint8_t)(0x90 | (i & 0x07)));
			scaffold.StepFrameScaffold(200);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest endBaseline = CaptureDigest(bus);

		// Replay from phase2 — should match end baseline
		scaffold.LoadState(phase2);
		for (uint32_t i = 0; i < 8; i++) {
			bus.YmWriteAddress(0, (uint8_t)(0x30 + i));
			bus.YmWriteData(0, (uint8_t)(0x0A + i));
			bus.PsgWrite((uint8_t)(0x90 | (i & 0x07)));
			scaffold.StepFrameScaffold(200);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest fromPhase2 = CaptureDigest(bus);
		EXPECT_EQ(fromPhase2, endBaseline);

		// Replay from phase1 — run phase2 tail + phase3 tail
		scaffold.LoadState(phase1);
		scaffold.StepFrameScaffold(488 * 4);
		bus.StepYm2612(144 * 4);
		bus.StepSn76489(128 * 4);
		bus.UpdateMixedSample();
		for (uint32_t i = 0; i < 8; i++) {
			bus.YmWriteAddress(0, (uint8_t)(0x30 + i));
			bus.YmWriteData(0, (uint8_t)(0x0A + i));
			bus.PsgWrite((uint8_t)(0x90 | (i & 0x07)));
			scaffold.StepFrameScaffold(200);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest fromPhase1 = CaptureDigest(bus);
		EXPECT_EQ(fromPhase1, endBaseline);
	}

	TEST(GenesisSaveStateBoundaryReplayTests, ContentionCycleBoundarySaveLoadPreservesDmaProgress) {
		GenesisM68kBoundaryScaffold scaffold;
		SetupBaseState(scaffold);

		auto& bus = scaffold.GetBus();
		bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 50);

		// Step to create partial DMA progress
		scaffold.StepFrameScaffold(200);
		uint32_t remaining = bus.GetDmaActiveCyclesRemaining();
		uint32_t contentionBefore = bus.GetDmaContentionCycles();

		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();
		EXPECT_EQ(checkpoint.Bus.DmaActiveCyclesRemaining, remaining);
		EXPECT_EQ(checkpoint.Bus.DmaContentionCycles, contentionBefore);

		// Mutate state
		scaffold.StepFrameScaffold(5000);
		bus.YmWriteAddress(0, 0x28);
		bus.YmWriteData(0, 0xFF);
		bus.StepYm2612(144 * 10);

		// Load and verify DMA progress restored
		scaffold.LoadState(checkpoint);
		EXPECT_EQ(bus.GetDmaActiveCyclesRemaining(), remaining);
		EXPECT_EQ(bus.GetDmaContentionCycles(), contentionBefore);
	}

	TEST(GenesisSaveStateBoundaryReplayTests, FullSystemContentionFuzzDigestIsDeterministic) {
		auto runFuzz = [](uint32_t seed) -> ContentionDigest {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 4, true);

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);

			// Seeded deterministic operations
			uint64_t hash = 14695981039346656037ull;
			uint64_t prime = 1099511628211ull;
			hash = (hash ^ seed) * prime;

			for (uint32_t i = 0; i < 64; i++) {
				hash = (hash ^ i) * prime;
				uint8_t action = (uint8_t)(hash & 0x07);

				if (action < 2) {
					bus.YmWriteAddress(0, (uint8_t)(0x30 + (hash >> 8 & 0x0F)));
					bus.YmWriteData(0, (uint8_t)(hash >> 16 & 0xFF));
				} else if (action < 4) {
					bus.PsgWrite((uint8_t)(0x80 | (hash >> 8 & 0x7F)));
				} else if (action == 4) {
					bus.BeginDmaTransfer(
						(hash >> 8 & 1) ? GenesisVdpDmaMode::Copy : GenesisVdpDmaMode::Fill,
						(uint32_t)(5 + (hash >> 12 & 0x1F))
					);
				} else if (action == 5) {
					bus.WriteByte(0xA11100, (uint8_t)(hash >> 8 & 1));
				} else {
					bus.SetRenderCompositionInputs(
						(uint8_t)(hash >> 8 & 0xFF), true, (uint8_t)(hash >> 16 & 0xFF),
						false, (uint8_t)(hash >> 24 & 0xFF), true, true,
						(uint8_t)(hash >> 32 & 0xFF), true
					);
					bus.RenderScaffoldLine(64);
				}

				scaffold.StepFrameScaffold(100 + (i % 7) * 30);
				bus.StepYm2612(144);
				bus.StepSn76489(128);
				bus.UpdateMixedSample();

				// Save/load at midpoint to test boundary replay
				if (i == 32) {
					GenesisBoundaryScaffoldSaveState mid = scaffold.SaveState();
					scaffold.StepFrameScaffold(50);
					scaffold.LoadState(mid);
				}
			}
			return CaptureDigest(bus);
		};

		ContentionDigest first = runFuzz(42);
		ContentionDigest second = runFuzz(42);
		EXPECT_EQ(first, second);

		// Different seed should produce different digest
		ContentionDigest other = runFuzz(99);
		EXPECT_NE(first.MixedDigest, other.MixedDigest);
	}

	TEST(GenesisSaveStateBoundaryReplayTests, ReplayAfterZ80BusContentionDuringDmaMatchesBaseline) {
		GenesisM68kBoundaryScaffold scaffold;
		SetupBaseState(scaffold);

		auto& bus = scaffold.GetBus();
		bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 30);
		bus.WriteByte(0xA11100, 0x01); // Request Z80 bus

		scaffold.StepFrameScaffold(300);
		bus.StepYm2612(144 * 2);
		bus.StepSn76489(128 * 2);
		bus.UpdateMixedSample();

		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		// Baseline
		bus.WriteByte(0xA11100, 0x00); // Release Z80 bus
		for (uint32_t i = 0; i < 15; i++) {
			bus.YmWriteAddress(0, (uint8_t)(0x30 + (i & 0x0F)));
			bus.YmWriteData(0, (uint8_t)(0x10 + i));
			scaffold.StepFrameScaffold(250);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest baseline = CaptureDigest(bus);

		scaffold.LoadState(checkpoint);
		bus.WriteByte(0xA11100, 0x00);
		for (uint32_t i = 0; i < 15; i++) {
			bus.YmWriteAddress(0, (uint8_t)(0x30 + (i & 0x0F)));
			bus.YmWriteData(0, (uint8_t)(0x10 + i));
			scaffold.StepFrameScaffold(250);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest replay = CaptureDigest(bus);

		EXPECT_EQ(replay, baseline);
	}

	TEST(GenesisSaveStateBoundaryReplayTests, MismatchTriageLinesShowExplicitFieldsOnDigestDifference) {
		GenesisM68kBoundaryScaffold scaffold;
		SetupBaseState(scaffold);

		auto& bus = scaffold.GetBus();
		bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 20);
		scaffold.StepFrameScaffold(488);
		bus.StepYm2612(144 * 3);
		bus.StepSn76489(128 * 3);
		bus.UpdateMixedSample();

		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		// Run A — heavy YM writes on both ports, many steps
		for (uint32_t i = 0; i < 10; i++) {
			bus.YmWriteAddress(0, (uint8_t)(0x30 + i));
			bus.YmWriteData(0, (uint8_t)(0x10 + i));
			bus.YmWriteAddress(1, (uint8_t)(0x30 + i));
			bus.YmWriteData(1, (uint8_t)(0x50 + i));
			scaffold.StepFrameScaffold(200);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest runA = CaptureDigest(bus);

		// Run B — from same checkpoint, no YM writes at all (only PSG)
		scaffold.LoadState(checkpoint);
		for (uint32_t i = 0; i < 10; i++) {
			bus.PsgWrite((uint8_t)(0x80 | (i * 0x20)));
			bus.PsgWrite((uint8_t)(0x04 + i));
			scaffold.StepFrameScaffold(200);
			bus.StepYm2612(144);
			bus.StepSn76489(128);
			bus.UpdateMixedSample();
		}
		ContentionDigest runB = CaptureDigest(bus);

		// Triage: write counts differ, so mixed digest should differ
		EXPECT_NE(runA.MixedDigest, runB.MixedDigest)
			<< "Mixed digest mismatch triage: identical mixed digests despite entirely different write patterns";

		// Sample counts should match since timing step cycles are identical
		EXPECT_EQ(runA.YmSampleCount, runB.YmSampleCount)
			<< "YM sample count mismatch triage: counts diverged despite equal step cycles";
		EXPECT_EQ(runA.PsgSampleCount, runB.PsgSampleCount)
			<< "PSG sample count mismatch triage: counts diverged despite equal step cycles";
		EXPECT_EQ(runA.DmaContentionEvents, runB.DmaContentionEvents)
			<< "DMA contention event mismatch triage: contention counts diverged from same checkpoint";
	}
}
