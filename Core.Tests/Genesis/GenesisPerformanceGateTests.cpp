#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"
#include "Genesis/GenesisSmokeHarness.h"

namespace {
	vector<GenesisCompatibilityRomCase> BuildPerfCorpus() {
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"sonic-perf.bin", vector<uint8_t>(0x8000, 0x4E)});
		corpus.push_back({"jurassic-perf.bin", vector<uint8_t>(0x8000, 0x4E)});
		corpus.push_back({"generic-perf.bin", vector<uint8_t>(0x8000, 0x4E)});
		return corpus;
	}

	TEST(GenesisPerformanceGateTests, PerformanceGateDigestIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildPerfCorpus();

		GenesisPerformanceGateResult runA = GenesisSmokeHarness::RunPerformanceGate(scaffold, corpus, 5000000);
		GenesisPerformanceGateResult runB = GenesisSmokeHarness::RunPerformanceGate(scaffold, corpus, 5000000);

		EXPECT_EQ(runA.PassCount, (int)corpus.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);

		bool hasSummary = std::any_of(runA.OutputLines.begin(), runA.OutputLines.end(), [](const string& line) {
			return line.starts_with("GEN_PERF_GATE_SUMMARY ");
		});
		EXPECT_TRUE(hasSummary);
	}

	TEST(GenesisPerformanceGateTests, StrictBudgetTriggersFailurePath) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildPerfCorpus();

		GenesisPerformanceGateResult result = GenesisSmokeHarness::RunPerformanceGate(scaffold, corpus, 0);
		EXPECT_GT(result.FailCount, 0);
		EXPECT_FALSE(result.Digest.empty());

		bool hasFailContext = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("GEN_PERF_FAIL_CONTEXT ");
		});
		EXPECT_TRUE(hasFailContext);
	}
}
