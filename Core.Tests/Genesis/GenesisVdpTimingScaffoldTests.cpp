#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisVdpTimingScaffoldTests, StartupResetsTimingCounters) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		EXPECT_EQ(scaffold.GetTimingScanline(), 0u);
		EXPECT_EQ(scaffold.GetTimingFrame(), 0u);
	}

	TEST(GenesisVdpTimingScaffoldTests, ScanlineAdvancesAtExpectedCycleBoundary) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.StepFrameScaffold(487);
		EXPECT_EQ(scaffold.GetTimingScanline(), 0u);
		EXPECT_EQ(scaffold.GetTimingFrame(), 0u);

		scaffold.StepFrameScaffold(1);
		EXPECT_EQ(scaffold.GetTimingScanline(), 1u);
		EXPECT_EQ(scaffold.GetTimingFrame(), 0u);
	}

	TEST(GenesisVdpTimingScaffoldTests, FrameRollsAfterFullScanlineSet) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.StepFrameScaffold(488u * 262u);
		EXPECT_EQ(scaffold.GetTimingScanline(), 0u);
		EXPECT_EQ(scaffold.GetTimingFrame(), 1u);
	}

	TEST(GenesisVdpTimingScaffoldTests, VBlankEntersAtScanline224AndExitsAtFrameWrap) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.StepFrameScaffold(488u * 223u);
		EXPECT_FALSE(scaffold.IsInVBlank());
		EXPECT_EQ(scaffold.GetVBlankEnterCount(), 0u);

		scaffold.StepFrameScaffold(488u);
		EXPECT_TRUE(scaffold.IsInVBlank());
		EXPECT_EQ(scaffold.GetTimingScanline(), 224u);
		EXPECT_EQ(scaffold.GetVBlankEnterCount(), 1u);

		scaffold.StepFrameScaffold(488u * (262u - 224u));
		EXPECT_FALSE(scaffold.IsInVBlank());
		EXPECT_EQ(scaffold.GetTimingFrame(), 1u);
		EXPECT_EQ(scaffold.GetVBlankExitCount(), 1u);
	}

	TEST(GenesisVdpTimingScaffoldTests, HIntFiresOnlyDuringActiveDisplayLines) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8u, false);

		scaffold.StepFrameScaffold(488u * 262u);
		EXPECT_EQ(scaffold.GetHorizontalInterruptCount(), 27u);
		EXPECT_EQ(scaffold.GetVerticalInterruptCount(), 0u);
	}

	TEST(GenesisVdpTimingScaffoldTests, HIntDoesNotFireWhenDisabled) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(false, 8u, true);

		scaffold.StepFrameScaffold(488u * 262u);
		EXPECT_EQ(scaffold.GetHorizontalInterruptCount(), 0u);
		EXPECT_EQ(scaffold.GetVerticalInterruptCount(), 1u);
	}

	TEST(GenesisVdpTimingScaffoldTests, VIntFiresOncePerFrameWhenEnabled) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(false, 16u, true);

		scaffold.StepFrameScaffold(488u * 262u * 2u);
		EXPECT_EQ(scaffold.GetVerticalInterruptCount(), 2u);
		EXPECT_EQ(scaffold.GetVBlankEnterCount(), 2u);
		EXPECT_EQ(scaffold.GetVBlankExitCount(), 2u);
	}

	TEST(GenesisVdpTimingScaffoldTests, VBlankAndVIntEventsAppearInDeterministicOrder) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 16u, true);

		scaffold.StepFrameScaffold(488u * 262u);
		const auto& events = scaffold.GetTimingEvents();

		size_t enterIdx = events.size();
		size_t vintIdx = events.size();
		size_t exitIdx = events.size();
		for (size_t i = 0; i < events.size(); i++) {
			if (events[i].starts_with("VBLANK_ENTER ")) {
				enterIdx = i;
			}
			if (events[i].starts_with("VINT ")) {
				vintIdx = i;
			}
			if (events[i].starts_with("VBLANK_EXIT ")) {
				exitIdx = i;
			}
		}

		ASSERT_LT(enterIdx, events.size());
		ASSERT_LT(vintIdx, events.size());
		ASSERT_LT(exitIdx, events.size());
		EXPECT_LT(enterIdx, vintIdx);
		EXPECT_LT(vintIdx, exitIdx);
	}

	TEST(GenesisVdpTimingScaffoldTests, TimingSaveStateReplayPreservesVBlankCountersAndEvents) {
		GenesisM68kBoundaryScaffold runA;
		runA.Startup();
		runA.ConfigureInterruptSchedule(true, 7u, true);
		runA.StepFrameScaffold(488u * 140u);
		GenesisBoundaryScaffoldSaveState checkpoint = runA.SaveState();
		runA.StepFrameScaffold(488u * 400u);

		GenesisM68kBoundaryScaffold runB;
		runB.LoadState(checkpoint);
		runB.StepFrameScaffold(488u * 400u);

		EXPECT_EQ(runA.GetTimingFrame(), runB.GetTimingFrame());
		EXPECT_EQ(runA.GetTimingScanline(), runB.GetTimingScanline());
		EXPECT_EQ(runA.GetVBlankEnterCount(), runB.GetVBlankEnterCount());
		EXPECT_EQ(runA.GetVBlankExitCount(), runB.GetVBlankExitCount());
		EXPECT_EQ(runA.GetHorizontalInterruptCount(), runB.GetHorizontalInterruptCount());
		EXPECT_EQ(runA.GetVerticalInterruptCount(), runB.GetVerticalInterruptCount());
		EXPECT_EQ(runA.GetTimingEvents(), runB.GetTimingEvents());
	}
}
