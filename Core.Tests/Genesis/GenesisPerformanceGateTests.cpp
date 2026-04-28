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

	TEST(GenesisPerformanceGateTests, BenchmarkCorpusAssumptionsHoldForConfiguredBudget) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildPerfCorpus();

		GenesisPerformanceGateResult result = GenesisSmokeHarness::RunPerformanceGate(scaffold, corpus, 5000000);

		ASSERT_FALSE(corpus.empty());
		EXPECT_EQ((int)result.Entries.size(), (int)corpus.size());
		EXPECT_EQ(result.PassCount + result.FailCount, (int)corpus.size());
		EXPECT_EQ(result.FailCount, 0);
		EXPECT_FALSE(result.Digest.empty());
	}

	TEST(GenesisPerformanceGateTests, PerClassBudgetMarkersAreReportedForMeasurableGateCriteria) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildPerfCorpus();

		GenesisPerformanceGateResult result = GenesisSmokeHarness::RunPerformanceGate(scaffold, corpus, 5000000);

		bool hasClassBudgetInResultLine = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("GEN_PERF_RESULT ") && line.find("CLASS_BUDGET_US=") != string::npos;
		});
		EXPECT_TRUE(hasClassBudgetInResultLine);

		bool hasSonicClass = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("GEN_PERF_RESULT ") && line.find("CLASS=sonic") != string::npos;
		});
		bool hasJurassicClass = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("GEN_PERF_RESULT ") && line.find("CLASS=jurassic") != string::npos;
		});
		bool hasGenericClass = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("GEN_PERF_RESULT ") && line.find("CLASS=generic") != string::npos;
		});

		EXPECT_TRUE(hasSonicClass);
		EXPECT_TRUE(hasJurassicClass);
		EXPECT_TRUE(hasGenericClass);
	}

	TEST(GenesisPerformanceGateTests, TelemetryEvidenceMarkersAreReportedInPerfResultLines) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildPerfCorpus();

		GenesisPerformanceGateResult result = GenesisSmokeHarness::RunPerformanceGate(scaffold, corpus, 5000000);

		bool allPerfResultsHaveEvidenceMarkers = true;
		for (const string& line : result.OutputLines) {
			if (!line.starts_with("GEN_PERF_RESULT ")) {
				continue;
			}

			if (line.find("SCD_LANE_CT=") == string::npos
				|| line.find("SCD_EVT_CT=") == string::npos
				|| line.find("M32X_EVT_CT=") == string::npos
				|| line.find("REPLAY_OK=") == string::npos) {
				allPerfResultsHaveEvidenceMarkers = false;
				break;
			}
		}

		EXPECT_TRUE(allPerfResultsHaveEvidenceMarkers);
	}

	TEST(GenesisPerformanceGateTests, SummaryIncludesAggregateTelemetryMarkers) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildPerfCorpus();

		GenesisPerformanceGateResult result = GenesisSmokeHarness::RunPerformanceGate(scaffold, corpus, 5000000);

		bool hasAggregateTelemetrySummaryMarkers = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("GEN_PERF_GATE_SUMMARY ")
				&& line.find("CASE_TOTAL=") != string::npos
				&& line.find("CLASS_BUDGET_TOTAL_US=") != string::npos
				&& line.find("ELAPSED_TOTAL_US=") != string::npos
				&& line.find("REPLAY_FAIL_TOTAL=") != string::npos
				&& line.find("SCD_LANE_TOTAL=") != string::npos
				&& line.find("SCD_EVT_TOTAL=") != string::npos
				&& line.find("M32X_EVT_TOTAL=") != string::npos;
		});

		EXPECT_TRUE(hasAggregateTelemetrySummaryMarkers);
	}

	TEST(GenesisPerformanceGateTests, SummaryIncludesReplayAggregateMarker) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildPerfCorpus();

		GenesisPerformanceGateResult result = GenesisSmokeHarness::RunPerformanceGate(scaffold, corpus, 5000000);

		bool hasReplayAggregateSummaryMarker = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("GEN_PERF_GATE_SUMMARY ")
				&& line.find("REPLAY_OK_TOTAL=") != string::npos;
		});

		EXPECT_TRUE(hasReplayAggregateSummaryMarker);
	}
}
