#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600SmokeHarness.h"

namespace {
	vector<Atari2600BaselineRomCase> BuildPerfCorpus() {
		vector<Atari2600BaselineRomCase> corpus;
		corpus.push_back({"perf-f8.a26", vector<uint8_t>(8192, 0xEA)});
		corpus.push_back({"perf-f6.a26", vector<uint8_t>(16384, 0xEA)});
		corpus.push_back({"perf-3f.a26", vector<uint8_t>(8192, 0xEA)});
		return corpus;
	}

	TEST(Atari2600PerformanceGateTests, PerformanceGateDigestIsDeterministicAcrossRuns) {
		Emulator emu;
		Atari2600Console console(&emu);
		vector<Atari2600BaselineRomCase> corpus = BuildPerfCorpus();

		Atari2600PerformanceGateResult runA = Atari2600SmokeHarness::RunPerformanceGate(console, corpus, 5000000);
		Atari2600PerformanceGateResult runB = Atari2600SmokeHarness::RunPerformanceGate(console, corpus, 5000000);

		EXPECT_EQ(runA.PassCount, (int)corpus.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);

		bool hasSummary = std::any_of(runA.OutputLines.begin(), runA.OutputLines.end(), [](const string& line) {
			return line.starts_with("PERF_GATE_SUMMARY ");
		});
		EXPECT_TRUE(hasSummary);
	}

	TEST(Atari2600PerformanceGateTests, StrictBudgetTriggersFailurePath) {
		Emulator emu;
		Atari2600Console console(&emu);
		vector<Atari2600BaselineRomCase> corpus = BuildPerfCorpus();

		Atari2600PerformanceGateResult result = Atari2600SmokeHarness::RunPerformanceGate(console, corpus, 0);
		EXPECT_GT(result.FailCount, 0);
		EXPECT_FALSE(result.Digest.empty());
	}
}
