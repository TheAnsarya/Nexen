#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	string BuildTranscriptDigest(const GenesisM68kBoundaryScaffold& scaffold) {
		uint64_t hash = 1469598103934665603ull;
		auto mix = [&](uint64_t v) {
			hash ^= v;
			hash *= 1099511628211ull;
		};

		mix(scaffold.GetTimingFrame());
		mix(scaffold.GetTimingScanline());
		mix(scaffold.GetHorizontalInterruptCount());
		mix(scaffold.GetVerticalInterruptCount());

		for (const string& evt : scaffold.GetTimingEvents()) {
			for (char ch : evt) {
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
			return std::tuple<string, uint32_t, uint32_t>(
				BuildTranscriptDigest(scaffold),
				(uint32_t)scaffold.GetTimingEvents().size(),
				scaffold.GetVerticalInterruptCount());
		};

		auto runA = run();
		auto runB = run();
		EXPECT_EQ(runA, runB);
		EXPECT_FALSE(std::get<0>(runA).empty());
		EXPECT_GT(std::get<1>(runA), 0u);
		EXPECT_GT(std::get<2>(runA), 0u);
	}

	TEST(GenesisSegaCdCadenceTranscriptScaffoldTests, CadenceTranscriptDigestMatchesAfterSaveLoadReplay) {
		GenesisM68kBoundaryScaffold scaffold;
		RunCadenceScenario(scaffold);
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		scaffold.StepFrameScaffold(488 * 40);
		string baselineDigest = BuildTranscriptDigest(scaffold);
		uint32_t baselineEventCount = (uint32_t)scaffold.GetTimingEvents().size();

		scaffold.LoadState(checkpoint);
		scaffold.StepFrameScaffold(488 * 40);
		string replayDigest = BuildTranscriptDigest(scaffold);
		uint32_t replayEventCount = (uint32_t)scaffold.GetTimingEvents().size();

		EXPECT_EQ(replayDigest, baselineDigest);
		EXPECT_EQ(replayEventCount, baselineEventCount);
	}
}
