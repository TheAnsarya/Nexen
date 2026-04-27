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

	TEST(GenesisCompatibilityHarnessTests, BenchmarkCorpusAssumptionsHoldForAllPassScenario) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		GenesisCompatibilityMatrixResult result = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_FALSE(corpus.empty());
		EXPECT_EQ((int)result.Entries.size(), (int)corpus.size());
		EXPECT_EQ(result.PassCount + result.FailCount, (int)corpus.size());
		EXPECT_EQ(result.FailCount, 0);
		EXPECT_FALSE(result.Digest.empty());
	}

	TEST(GenesisCompatibilityHarnessTests, MapperEdgeAndOwnershipCheckpointsArePresentAndPassing) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		GenesisCompatibilityMatrixResult result = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)result.Entries.size(), (int)corpus.size());
		for (const GenesisCompatibilityEntry& entry : result.Entries) {
			auto hasPassingCheckpoint = [&](const string& checkpointId) {
				return std::any_of(entry.Checkpoints.begin(), entry.Checkpoints.end(), [&](const GenesisCompatibilityCheckpoint& checkpoint) {
					return checkpoint.Id == checkpointId && checkpoint.Pass;
				});
			};

			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-BUS-OWNERSHIP"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-MAPPER-EDGE"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-HOST-MODE"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-TOOLING-MATRIX"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-32X-DUAL-SH2"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-32X-COMPOSE-SYNC"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-32X-TOOLING"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-TAS-CHEAT"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-PBC-BOOT"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-PBC-RUNTIME"));
		}
	}

	TEST(GenesisCompatibilityHarnessTests, PbcHostModeScenarioDigestIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"pbc-host-sms-boot.bin", vector<uint8_t>(0x8000, 0x4E)});
		corpus.push_back({"pbc-host-sms-runtime.bin", vector<uint8_t>(0x10000, 0x4E)});

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		EXPECT_EQ(runA.PassCount, (int)corpus.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_EQ(runA.Digest, runB.Digest);
		EXPECT_EQ(runA.OutputLines, runB.OutputLines);
	}

	TEST(GenesisCompatibilityHarnessTests, PbcHostModeScenarioIncludesDeterministicCheckpointHooks) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"pbc-host-sms-scenario.bin", vector<uint8_t>(0xC000, 0x4E)});

		GenesisCompatibilityMatrixResult result = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ(result.PassCount, 1);
		ASSERT_EQ(result.FailCount, 0);
		ASSERT_EQ(result.Entries.size(), 1u);

		const GenesisCompatibilityEntry& entry = result.Entries[0];
		auto hasPassingCheckpoint = [&](const string& checkpointId) {
			return std::any_of(entry.Checkpoints.begin(), entry.Checkpoints.end(), [&](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == checkpointId && checkpoint.Pass;
			});
		};

		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-HOST-MODE"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-MAPPER-EDGE"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-DETERMINISM"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-TAS-CHEAT"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-PBC-BOOT"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-PBC-RUNTIME"));
	}

	TEST(GenesisCompatibilityHarnessTests, TasCheatCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"tas-cheat-parity.bin", vector<uint8_t>(0x10000, 0x4E)});

		auto readTasCheatContext = [&](const GenesisCompatibilityMatrixResult& result) {
			EXPECT_EQ(result.PassCount, 1);
			EXPECT_EQ(result.FailCount, 0);
			const GenesisCompatibilityEntry& entry = result.Entries[0];
			auto it = std::find_if(entry.Checkpoints.begin(), entry.Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-TAS-CHEAT";
			});
			if (it == entry.Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-TAS-CHEAT checkpoint";
				return string();
			}
			EXPECT_TRUE(it->Pass);
			return it->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		string contextA = readTasCheatContext(runA);
		string contextB = readTasCheatContext(runB);
		EXPECT_EQ(contextA, contextB);
	}

	TEST(GenesisCompatibilityHarnessTests, PbcRuntimeParityCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"pbc-runtime-parity.bin", vector<uint8_t>(0x10000, 0x4E)});

		auto readRuntimeContext = [&](const GenesisCompatibilityMatrixResult& result) {
			EXPECT_EQ(result.PassCount, 1);
			EXPECT_EQ(result.FailCount, 0);
			const GenesisCompatibilityEntry& entry = result.Entries[0];
			auto it = std::find_if(entry.Checkpoints.begin(), entry.Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-PBC-RUNTIME";
			});
			if (it == entry.Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-PBC-RUNTIME checkpoint";
				return string();
			}
			EXPECT_TRUE(it->Pass);
			return it->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		string contextA = readRuntimeContext(runA);
		string contextB = readRuntimeContext(runB);
		EXPECT_EQ(contextA, contextB);
	}
}
