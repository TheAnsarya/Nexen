#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	static constexpr uint32_t CyclesPerScanline = 488;
	static constexpr uint32_t ScanlinesPerFrame = 262;

	static bool HasEventPrefix(const vector<string>& events, const string& prefix) {
		return std::any_of(events.begin(), events.end(), [&](const string& line) {
			return line.starts_with(prefix);
		});
	}

	TEST(GenesisInterruptCoincidenceTests, HIntAtVBlankBoundaryProducesBothEvents) {
		// Configure H-INT interval so it fires exactly on the VBlank boundary scanline.
		// VBlank occurs at scanline 262 (frame rollover). With interval=262,
		// H-INT fires at scanline 262 = same as VBlank.
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, ScanlinesPerFrame, true);
		scaffold.ClearTimingEvents();

		scaffold.StepFrameScaffold(CyclesPerScanline * ScanlinesPerFrame);

		// Both should fire: H-INT at exactly the VBlank boundary scanline.
		EXPECT_EQ(scaffold.GetHorizontalInterruptCount(), 1u);
		EXPECT_EQ(scaffold.GetVerticalInterruptCount(), 1u);
		EXPECT_TRUE(HasEventPrefix(scaffold.GetTimingEvents(), "HINT "));
		EXPECT_TRUE(HasEventPrefix(scaffold.GetTimingEvents(), "VINT "));
	}

	TEST(GenesisInterruptCoincidenceTests, HIntOneLineBelowVBlankDoesNotCoincide) {
		// H-INT fires at scanline 261 (one before VBlank). After full frame,
		// both should fire but at different scanlines.
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 261, true);
		scaffold.ClearTimingEvents();

		scaffold.StepFrameScaffold(CyclesPerScanline * ScanlinesPerFrame);

		EXPECT_EQ(scaffold.GetHorizontalInterruptCount(), 1u);
		EXPECT_EQ(scaffold.GetVerticalInterruptCount(), 1u);

		const auto& events = scaffold.GetTimingEvents();
		ASSERT_GE(events.size(), 2u);

		// H-INT should come before V-INT since scanline 261 < 262.
		size_t hintIdx = 0, vintIdx = 0;
		for (size_t i = 0; i < events.size(); i++) {
			if (events[i].starts_with("HINT ")) hintIdx = i;
			if (events[i].starts_with("VINT ")) vintIdx = i;
		}
		EXPECT_LT(hintIdx, vintIdx);
	}

	TEST(GenesisInterruptCoincidenceTests, HighFrequencyHIntOverlapsVBlankFrame) {
		// H-INT every 4 scanlines over a full frame: should get many H-INTs plus 1 V-INT.
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 4, true);
		scaffold.ClearTimingEvents();

		scaffold.StepFrameScaffold(CyclesPerScanline * ScanlinesPerFrame);

		uint32_t expectedHints = ScanlinesPerFrame / 4;
		EXPECT_EQ(scaffold.GetHorizontalInterruptCount(), expectedHints);
		EXPECT_EQ(scaffold.GetVerticalInterruptCount(), 1u);

		// V-INT should always be the last event in the frame.
		const auto& events = scaffold.GetTimingEvents();
		ASSERT_FALSE(events.empty());
		EXPECT_TRUE(events.back().starts_with("VINT "));
	}

	TEST(GenesisInterruptCoincidenceTests, CoincidenceWindowOrderingIsHIntThenVInt) {
		// When H-INT and V-INT fire on the same scanline, H-INT should precede V-INT.
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		// Interval 131 → H-INT at scanlines 131, 262. VBlank at 262.
		scaffold.ConfigureInterruptSchedule(true, 131, true);
		scaffold.ClearTimingEvents();

		scaffold.StepFrameScaffold(CyclesPerScanline * ScanlinesPerFrame);

		const auto& events = scaffold.GetTimingEvents();
		ASSERT_GE(events.size(), 3u); // 2 HINTs + 1 VINT

		// Find the last HINT and the VINT — they should coincide at scanline 262.
		size_t lastHint = 0, vintPos = 0;
		for (size_t i = 0; i < events.size(); i++) {
			if (events[i].starts_with("HINT ")) lastHint = i;
			if (events[i].starts_with("VINT ")) vintPos = i;
		}
		// HINT before VINT even at coincidence.
		EXPECT_LT(lastHint, vintPos);
	}

	TEST(GenesisInterruptCoincidenceTests, InterruptLevelReflectsHighestPendingAtCoincidence) {
		// At coincidence (H-INT level 4, V-INT level 6), CPU interrupt level should be 6.
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, ScanlinesPerFrame, true);
		scaffold.ClearTimingEvents();

		scaffold.StepFrameScaffold(CyclesPerScanline * ScanlinesPerFrame);

		// V-INT is level 6 — the higher priority interrupt.
		EXPECT_EQ(scaffold.GetCpu().GetInterruptLevel(), 6u);
	}

	TEST(GenesisInterruptCoincidenceTests, DisablingVIntStillFiresHIntAtBoundary) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, ScanlinesPerFrame, false);
		scaffold.ClearTimingEvents();

		scaffold.StepFrameScaffold(CyclesPerScanline * ScanlinesPerFrame);

		EXPECT_EQ(scaffold.GetHorizontalInterruptCount(), 1u);
		EXPECT_EQ(scaffold.GetVerticalInterruptCount(), 0u);
		EXPECT_TRUE(HasEventPrefix(scaffold.GetTimingEvents(), "HINT "));
		EXPECT_FALSE(HasEventPrefix(scaffold.GetTimingEvents(), "VINT "));
	}

	TEST(GenesisInterruptCoincidenceTests, DisablingHIntStillFiresVIntAtBoundary) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(false, 16, true);
		scaffold.ClearTimingEvents();

		scaffold.StepFrameScaffold(CyclesPerScanline * ScanlinesPerFrame);

		EXPECT_EQ(scaffold.GetHorizontalInterruptCount(), 0u);
		EXPECT_EQ(scaffold.GetVerticalInterruptCount(), 1u);
		EXPECT_FALSE(HasEventPrefix(scaffold.GetTimingEvents(), "HINT "));
		EXPECT_TRUE(HasEventPrefix(scaffold.GetTimingEvents(), "VINT "));
	}

	TEST(GenesisInterruptCoincidenceTests, MultiFrameCoincidenceCountsAccumulate) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);
		scaffold.ClearTimingEvents();

		// Run 3 full frames.
		scaffold.StepFrameScaffold(CyclesPerScanline * ScanlinesPerFrame * 3);

		uint32_t expectedHints = (ScanlinesPerFrame / 8) * 3;
		EXPECT_EQ(scaffold.GetHorizontalInterruptCount(), expectedHints);
		EXPECT_EQ(scaffold.GetVerticalInterruptCount(), 3u);
		EXPECT_EQ(scaffold.GetTimingFrame(), 3u);
	}

	TEST(GenesisInterruptCoincidenceTests, CoincidenceEventDigestIsDeterministicAcrossRuns) {
		auto runScenario = []() {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 7, true);
			scaffold.ClearTimingEvents();

			// Run 2 full frames with both interrupt types.
			scaffold.StepFrameScaffold(CyclesPerScanline * ScanlinesPerFrame * 2);

			// FNV-1a hash of all timing events.
			uint64_t hash = 1469598103934665603ull;
			for (const auto& event : scaffold.GetTimingEvents()) {
				for (uint8_t ch : event) {
					hash ^= ch;
					hash *= 1099511628211ull;
				}
			}

			return std::make_tuple(
				hash,
				scaffold.GetHorizontalInterruptCount(),
				scaffold.GetVerticalInterruptCount(),
				scaffold.GetTimingEvents().size()
			);
		};

		auto runA = runScenario();
		auto runB = runScenario();

		EXPECT_EQ(std::get<0>(runA), std::get<0>(runB));
		EXPECT_EQ(std::get<1>(runA), std::get<1>(runB));
		EXPECT_EQ(std::get<2>(runA), std::get<2>(runB));
		EXPECT_EQ(std::get<3>(runA), std::get<3>(runB));
	}

	TEST(GenesisInterruptCoincidenceTests, SaveStateReplayPreservesCoincidenceOrdering) {
		GenesisM68kBoundaryScaffold runA;
		runA.Startup();
		runA.ConfigureInterruptSchedule(true, 16, true);
		runA.ClearTimingEvents();

		// Run to mid-frame, save state, then complete 2 more frames.
		runA.StepFrameScaffold(CyclesPerScanline * 130);
		GenesisBoundaryScaffoldSaveState checkpoint = runA.SaveState();
		runA.StepFrameScaffold(CyclesPerScanline * (ScanlinesPerFrame * 2 - 130));

		// Replay from checkpoint.
		GenesisM68kBoundaryScaffold runB;
		runB.LoadState(checkpoint);
		runB.StepFrameScaffold(CyclesPerScanline * (ScanlinesPerFrame * 2 - 130));

		EXPECT_EQ(runA.GetHorizontalInterruptCount(), runB.GetHorizontalInterruptCount());
		EXPECT_EQ(runA.GetVerticalInterruptCount(), runB.GetVerticalInterruptCount());
		EXPECT_EQ(runA.GetTimingEvents(), runB.GetTimingEvents());
	}

	TEST(GenesisInterruptCoincidenceTests, InterruptIntervalChangesMidFrameAffectsSubsequentFires) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);
		scaffold.ClearTimingEvents();

		// Run 100 scanlines with interval 8.
		scaffold.StepFrameScaffold(CyclesPerScanline * 100);
		uint32_t hintsAfterFirst = scaffold.GetHorizontalInterruptCount();
		EXPECT_EQ(hintsAfterFirst, 100u / 8u);

		// Change interval to 16, run remaining frame.
		scaffold.ConfigureInterruptSchedule(true, 16, true);
		scaffold.StepFrameScaffold(CyclesPerScanline * (ScanlinesPerFrame - 100));

		// Total HINTs should be less than if interval stayed at 8.
		uint32_t totalHints = scaffold.GetHorizontalInterruptCount();
		uint32_t hintsAtInterval8Only = ScanlinesPerFrame / 8;
		EXPECT_LT(totalHints, hintsAtInterval8Only);
		EXPECT_GT(totalHints, hintsAfterFirst);
	}
}
