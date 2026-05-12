#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
		bool HasEventPrefix(const vector<string>& events, const string& prefix) {
			return std::any_of(events.begin(), events.end(), [&](const string& line) {
				return line.starts_with(prefix);
			});
		}

	TEST(GenesisInterruptSchedulingTests, HorizontalInterruptFiresAtConfiguredInterval) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, false);
		scaffold.ClearTimingEvents();

		scaffold.StepFrameScaffold(488u * 24u);

		EXPECT_EQ(scaffold.GetHorizontalInterruptCount(), 3u);
		EXPECT_EQ(scaffold.GetVerticalInterruptCount(), 0u);
		EXPECT_EQ(scaffold.GetTimingEvents().size(), 3u);
		EXPECT_TRUE(scaffold.GetTimingEvents().at(0).starts_with("HINT "));
	}

	TEST(GenesisInterruptSchedulingTests, VerticalInterruptFiresOnFrameRollover) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(false, 16, true);
		scaffold.ClearTimingEvents();

		scaffold.StepFrameScaffold(488u * 262u);

		EXPECT_EQ(scaffold.GetHorizontalInterruptCount(), 0u);
		EXPECT_EQ(scaffold.GetVerticalInterruptCount(), 1u);
		EXPECT_EQ(scaffold.GetTimingFrame(), 1u);
		EXPECT_EQ(scaffold.GetTimingScanline(), 0u);
		EXPECT_EQ(scaffold.GetCpu().GetInterruptLevel(), 6u);
	}

	TEST(GenesisInterruptSchedulingTests, EventOrderingIsDeterministicWithBothInterruptsEnabled) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 1, true);
		scaffold.ClearTimingEvents();

		scaffold.StepFrameScaffold(488u * 262u);
		const vector<string>& events = scaffold.GetTimingEvents();

		ASSERT_GE(events.size(), 4u);
		size_t hintIdx = events.size();
		size_t vblankEnterIdx = events.size();
		size_t vintIdx = events.size();
		size_t vblankExitIdx = events.size();
		for (size_t i = 0; i < events.size(); i++) {
			if (events[i].starts_with("HINT ")) {
				hintIdx = i;
			}
			if (events[i].starts_with("VBLANK_ENTER ")) {
				vblankEnterIdx = i;
			}
			if (events[i].starts_with("VINT ")) {
				vintIdx = i;
			}
			if (events[i].starts_with("VBLANK_EXIT ")) {
				vblankExitIdx = i;
			}
		}

		ASSERT_LT(hintIdx, events.size());
		ASSERT_LT(vblankEnterIdx, events.size());
		ASSERT_LT(vintIdx, events.size());
		ASSERT_LT(vblankExitIdx, events.size());
		EXPECT_LT(hintIdx, vblankEnterIdx);
		EXPECT_LT(vblankEnterIdx, vintIdx);
		EXPECT_LT(vintIdx, vblankExitIdx);
	}

	TEST(GenesisInterruptSchedulingTests, HAndVInterruptsAppearInSingleBoundaryStepOrder) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 1, true);
		scaffold.ClearTimingEvents();

		scaffold.StepFrameScaffold(488u * 262u);
		const vector<string>& events = scaffold.GetTimingEvents();

		ASSERT_GE(events.size(), 4u);
		EXPECT_TRUE(events.at(events.size() - 4).starts_with("HINT frame=0 scanline=223"));
		EXPECT_TRUE(events.at(events.size() - 3).starts_with("VBLANK_ENTER frame=0 scanline=224"));
		EXPECT_TRUE(events.at(events.size() - 2).starts_with("VINT frame=1"));
		EXPECT_TRUE(events.at(events.size() - 1).starts_with("VBLANK_EXIT frame=1"));
	}

	TEST(GenesisInterruptSchedulingTests, SaveStateReplayKeepsInterruptEventOrderStable) {
		GenesisM68kBoundaryScaffold runA;
		runA.Startup();
		runA.ConfigureInterruptSchedule(true, 4, true);
		runA.ClearTimingEvents();
		runA.StepFrameScaffold(488u * 40u);
		GenesisBoundaryScaffoldSaveState checkpoint = runA.SaveState();
		runA.StepFrameScaffold(488u * 230u);

		GenesisM68kBoundaryScaffold runB;
		runB.LoadState(checkpoint);
		runB.StepFrameScaffold(488u * 230u);

		EXPECT_EQ(runA.GetHorizontalInterruptCount(), runB.GetHorizontalInterruptCount());
		EXPECT_EQ(runA.GetVerticalInterruptCount(), runB.GetVerticalInterruptCount());
		EXPECT_EQ(runA.GetTimingEvents(), runB.GetTimingEvents());
		EXPECT_TRUE(HasEventPrefix(runA.GetTimingEvents(), "HINT "));
		EXPECT_TRUE(HasEventPrefix(runA.GetTimingEvents(), "VINT "));
	}
}
