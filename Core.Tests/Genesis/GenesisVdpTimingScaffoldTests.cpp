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
}
