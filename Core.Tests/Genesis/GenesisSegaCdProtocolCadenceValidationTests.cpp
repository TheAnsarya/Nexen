#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	string BuildProtocolDigest(GenesisM68kBoundaryScaffold& scaffold) {
		uint64_t hash = 1469598103934665603ull;
		auto mix = [&](uint64_t value) {
			hash ^= value;
			hash *= 1099511628211ull;
		};

		mix(scaffold.GetTimingFrame());
		mix(scaffold.GetTimingScanline());
		mix(scaffold.GetHorizontalInterruptCount());
		mix(scaffold.GetVerticalInterruptCount());
		mix(scaffold.GetBus().GetCommandResponseLaneCount());

		const vector<string>& laneEntries = scaffold.GetBus().GetCommandResponseLane();
		for (const string& lane : laneEntries) {
			for (char ch : lane) {
				mix((uint8_t)ch);
			}
		}

		return std::format("{:016x}", hash);
	}

	void RunProtocolCadenceScenario(GenesisM68kBoundaryScaffold& scaffold) {
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 7, true);
		scaffold.ClearTimingEvents();

		auto& bus = scaffold.GetBus();
		bus.ClearCommandResponseLane();
		bus.WriteByte(0xA11200, 0x01);
		bus.WriteByte(0xA11100, 0x01);
		bus.WriteByte(0xA11100, 0x00);

		for (uint32_t phase = 0; phase < 6; phase++) {
			scaffold.StepFrameScaffold(488 * (32 + (phase * 2)));
			uint8_t cmd = (uint8_t)(0x40 + phase);
			uint8_t rsp = (uint8_t)(0x80 + phase);
			bus.WriteByte((uint32_t)(0xA12000 + (phase & 0x0F)), cmd);
			bus.WriteByte((uint32_t)(0xA12010 + (phase & 0x0F)), rsp);
			bus.WriteByte((uint32_t)(0xA13000 + (phase & 0x0F)), (uint8_t)(cmd + 1));
			bus.WriteByte((uint32_t)(0xA13010 + (phase & 0x0F)), (uint8_t)(rsp + 1));
			bus.WriteByte((uint32_t)(0xA14000 + (phase & 0x0F)), (uint8_t)(cmd + 2));
			bus.WriteByte((uint32_t)(0xA14010 + (phase & 0x0F)), (uint8_t)(rsp + 2));
			(void)bus.ReadByte((uint32_t)(0xA12010 + (phase & 0x0F)));
			(void)bus.ReadByte((uint32_t)(0xA13010 + (phase & 0x0F)));
			(void)bus.ReadByte((uint32_t)(0xA14010 + (phase & 0x0F)));
		}
	}

	TEST(GenesisSegaCdProtocolCadenceValidationTests, DeterministicDigestAcrossRepeatedRuns) {
		auto run = []() {
			GenesisM68kBoundaryScaffold scaffold;
			RunProtocolCadenceScenario(scaffold);
			return std::tuple<string, uint32_t, string>(
				BuildProtocolDigest(scaffold),
				scaffold.GetBus().GetCommandResponseLaneCount(),
				scaffold.GetBus().GetCommandResponseLaneDigest());
		};

		auto runA = run();
		auto runB = run();
		EXPECT_EQ(runA, runB);
		EXPECT_FALSE(std::get<0>(runA).empty());
		EXPECT_FALSE(std::get<2>(runA).empty());
		EXPECT_GT(std::get<1>(runA), 0u);
	}

	TEST(GenesisSegaCdProtocolCadenceValidationTests, SaveLoadReplayMatchesProtocolDigestAndLaneState) {
		GenesisM68kBoundaryScaffold baseline;
		RunProtocolCadenceScenario(baseline);
		GenesisBoundaryScaffoldSaveState checkpoint = baseline.SaveState();

		baseline.StepFrameScaffold(488 * 48);
		baseline.GetBus().WriteByte(0xA13005, 0xD1);
		baseline.GetBus().WriteByte(0xA14015, 0xE2);
		(void)baseline.GetBus().ReadByte(0xA14015);
		string baselineDigest = BuildProtocolDigest(baseline);
		string baselineLaneDigest = baseline.GetBus().GetCommandResponseLaneDigest();
		uint32_t baselineLaneCount = baseline.GetBus().GetCommandResponseLaneCount();

		GenesisM68kBoundaryScaffold replay;
		replay.LoadState(checkpoint);
		replay.StepFrameScaffold(488 * 48);
		replay.GetBus().WriteByte(0xA13005, 0xD1);
		replay.GetBus().WriteByte(0xA14015, 0xE2);
		(void)replay.GetBus().ReadByte(0xA14015);
		string replayDigest = BuildProtocolDigest(replay);
		string replayLaneDigest = replay.GetBus().GetCommandResponseLaneDigest();
		uint32_t replayLaneCount = replay.GetBus().GetCommandResponseLaneCount();

		EXPECT_EQ(replayDigest, baselineDigest);
		EXPECT_EQ(replayLaneDigest, baselineLaneDigest);
		EXPECT_EQ(replayLaneCount, baselineLaneCount);
	}
}
