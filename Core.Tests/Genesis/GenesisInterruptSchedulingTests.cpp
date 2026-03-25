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

		ASSERT_GE(events.size(), 2u);
		EXPECT_TRUE(events.at(events.size() - 2).starts_with("HINT "));
		EXPECT_TRUE(events.at(events.size() - 1).starts_with("VINT "));
	}

	TEST(GenesisInterruptSchedulingTests, HAndVInterruptsAppearInSingleBoundaryStepOrder) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 1, true);
		scaffold.ClearTimingEvents();

		scaffold.StepFrameScaffold(488u * 262u);
		const vector<string>& events = scaffold.GetTimingEvents();

		ASSERT_GE(events.size(), 2u);
		EXPECT_TRUE(events.at(events.size() - 2).starts_with("HINT frame=0 scanline=262"));
		EXPECT_TRUE(events.at(events.size() - 1).starts_with("VINT frame=1"));
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
