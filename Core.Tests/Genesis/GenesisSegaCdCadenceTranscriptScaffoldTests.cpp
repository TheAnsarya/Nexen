#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	string BuildTranscriptDigest(GenesisM68kBoundaryScaffold& scaffold) {
		uint64_t hash = 1469598103934665603ull;
		auto mix = [&](uint64_t v) {
			hash ^= v;
			hash *= 1099511628211ull;
		};

		mix(scaffold.GetTimingFrame());
		mix(scaffold.GetTimingScanline());
		mix(scaffold.GetHorizontalInterruptCount());
		mix(scaffold.GetVerticalInterruptCount());
		mix(scaffold.GetBus().GetCommandResponseLaneCount());

		for (const string& evt : scaffold.GetTimingEvents()) {
			for (char ch : evt) {
				mix((uint8_t)ch);
			}
		}

		for (const string& lane : scaffold.GetBus().GetCommandResponseLane()) {
			for (char ch : lane) {
				mix((uint8_t)ch);
			}
		}

		return std::format("{:016x}", hash);
	}

	void RunCadenceScenario(GenesisM68kBoundaryScaffold& scaffold) {
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);
		scaffold.ClearTimingEvents();

		auto& bus = scaffold.GetBus();
		bus.ClearCommandResponseLane();
		bus.WriteByte(0xA11200, 0x01);
		bus.WriteByte(0xA11100, 0x01);
		bus.WriteByte(0xA11100, 0x00);

		for (uint32_t i = 0; i < 6; i++) {
			scaffold.StepFrameScaffold(488 * 64);
			bus.WriteByte(0xA10000, (uint8_t)(0x20 + i));
			bus.WriteByte(0xA10001, (uint8_t)(0x40 + i));
			(void)bus.ReadByte(0xA10000);
		}
	}

	TEST(GenesisSegaCdCadenceTranscriptScaffoldTests, CadenceTranscriptDigestIsDeterministicAcrossRuns) {
		auto run = []() {
			GenesisM68kBoundaryScaffold scaffold;
			RunCadenceScenario(scaffold);
			return std::tuple<string, uint32_t, uint32_t, uint32_t, string>(
				BuildTranscriptDigest(scaffold),
				(uint32_t)scaffold.GetTimingEvents().size(),
				scaffold.GetVerticalInterruptCount(),
				scaffold.GetBus().GetCommandResponseLaneCount(),
				scaffold.GetBus().GetCommandResponseLaneDigest());
		};

		auto runA = run();
		auto runB = run();
		EXPECT_EQ(runA, runB);
		EXPECT_FALSE(std::get<0>(runA).empty());
		EXPECT_GT(std::get<1>(runA), 0u);
		EXPECT_GT(std::get<2>(runA), 0u);
		EXPECT_GT(std::get<3>(runA), 0u);
		EXPECT_FALSE(std::get<4>(runA).empty());
	}

	TEST(GenesisSegaCdCadenceTranscriptScaffoldTests, CadenceTranscriptDigestMatchesAfterSaveLoadReplay) {
		GenesisM68kBoundaryScaffold scaffold;
		RunCadenceScenario(scaffold);
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		scaffold.StepFrameScaffold(488 * 40);
		string baselineDigest = BuildTranscriptDigest(scaffold);
		uint32_t baselineEventCount = (uint32_t)scaffold.GetTimingEvents().size();
		uint32_t baselineLaneCount = scaffold.GetBus().GetCommandResponseLaneCount();
		string baselineLaneDigest = scaffold.GetBus().GetCommandResponseLaneDigest();

		scaffold.LoadState(checkpoint);
		scaffold.StepFrameScaffold(488 * 40);
		string replayDigest = BuildTranscriptDigest(scaffold);
		uint32_t replayEventCount = (uint32_t)scaffold.GetTimingEvents().size();
		uint32_t replayLaneCount = scaffold.GetBus().GetCommandResponseLaneCount();
		string replayLaneDigest = scaffold.GetBus().GetCommandResponseLaneDigest();

		EXPECT_EQ(replayDigest, baselineDigest);
		EXPECT_EQ(replayEventCount, baselineEventCount);
		EXPECT_EQ(replayLaneCount, baselineLaneCount);
		EXPECT_EQ(replayLaneDigest, baselineLaneDigest);
	}

	TEST(GenesisSegaCdCadenceTranscriptScaffoldTests, CommandResponseLaneEntriesPreserveDeterministicOrderingMetadata) {
		GenesisM68kBoundaryScaffold scaffold;
		RunCadenceScenario(scaffold);

		const vector<string>& lane = scaffold.GetBus().GetCommandResponseLane();
		ASSERT_GE(lane.size(), 6u);
		EXPECT_TRUE(lane.front().starts_with("SCD-LANE seq=1 "));
		EXPECT_TRUE(lane[1].starts_with("SCD-LANE seq=2 "));
		EXPECT_TRUE(lane[2].starts_with("SCD-LANE seq=3 "));
		EXPECT_TRUE(lane.front().find("op=W") != string::npos);
		EXPECT_TRUE(lane.back().find("addr=a10000") != string::npos || lane.back().find("addr=a10001") != string::npos);
		EXPECT_EQ(scaffold.GetBus().GetCommandResponseLaneCount(), (uint32_t)lane.size());
		EXPECT_FALSE(scaffold.GetBus().GetCommandResponseLaneDigest().empty());
	}
}
