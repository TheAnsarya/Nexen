#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"
#include "Genesis/GenesisSmokeHarness.h"

namespace {
	vector<GenesisCompatibilityRomCase> BuildGenesisCompatibilityCorpus() {
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"sonic-1.bin", vector<uint8_t>(0x8000, 0x4E)});
		corpus.push_back({"jurassic-park.bin", vector<uint8_t>(0x8000, 0x4E)});
		return corpus;
	}

	TEST(GenesisCompatibilityHarnessTests, CompatibilityDigestIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		EXPECT_EQ(runA.PassCount, (int)corpus.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);
	}

	TEST(GenesisCompatibilityHarnessTests, EmptyRomEntryTriggersFailureDiagnostics) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"empty.bin", {}});

		GenesisCompatibilityMatrixResult result = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		EXPECT_EQ(result.PassCount, 0);
		EXPECT_EQ(result.FailCount, 1);
		EXPECT_FALSE(result.Digest.empty());

		bool hasFailContext = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("GEN_COMPAT_FAIL_CONTEXT ");
		});
		EXPECT_TRUE(hasFailContext);
	}
}
