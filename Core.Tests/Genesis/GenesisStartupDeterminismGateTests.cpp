#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"
#include "Genesis/GenesisSmokeHarness.h"

namespace {
	struct ScopedEnvVar {
		std::string Name;
		bool HadValue = false;
		std::string OldValue;

		ScopedEnvVar(const char* name, const char* value)
			: Name(name) {
			char* existing = nullptr;
			size_t existingLen = 0;
			if (_dupenv_s(&existing, &existingLen, Name.c_str()) == 0 && existing != nullptr) {
				HadValue = true;
				OldValue = existing;
			}
			std::free(existing);

			if (value) {
				_putenv_s(Name.c_str(), value);
			} else {
				_putenv_s(Name.c_str(), "");
			}
		}

		~ScopedEnvVar() {
			if (HadValue) {
				_putenv_s(Name.c_str(), OldValue.c_str());
			} else {
				_putenv_s(Name.c_str(), "");
			}
		}
	};

	vector<GenesisCompatibilityRomCase> BuildStartupDeterminismCorpus() {
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"sonic-startup-gate.bin", vector<uint8_t>(0x8000, 0x4E)});
		corpus.push_back({"jurassic-startup-gate.bin", vector<uint8_t>(0x8000, 0x4E)});
		corpus.push_back({"golden-axe-startup-gate.bin", vector<uint8_t>(0x8000, 0x4E)});
		corpus.push_back({"streets-of-rage-startup-gate.bin", vector<uint8_t>(0x8000, 0x4E)});
		return corpus;
	}

	TEST(GenesisStartupDeterminismGateTests, First10SecondsGateDigestIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildStartupDeterminismCorpus();

		GenesisStartupDeterminismGateResult runA = GenesisSmokeHarness::RunStartupDeterminismGate(scaffold, corpus, 600);
		GenesisStartupDeterminismGateResult runB = GenesisSmokeHarness::RunStartupDeterminismGate(scaffold, corpus, 600);

		EXPECT_EQ(runA.PassCount, (int)corpus.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);

		bool hasSummaryLine = std::any_of(runA.OutputLines.begin(), runA.OutputLines.end(), [](const string& line) {
			return line.starts_with("GEN_STARTUP_GATE_SUMMARY ")
				&& line.find("FRAME_WINDOW=600") != string::npos
				&& line.find("REQUIRED_CHECKPOINTS=14") != string::npos;
		});
		EXPECT_TRUE(hasSummaryLine);
	}

	TEST(GenesisStartupDeterminismGateTests, First10SecondsGateRequiresAllStartupCheckpointsToPass) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildStartupDeterminismCorpus();

		GenesisStartupDeterminismGateResult result = GenesisSmokeHarness::RunStartupDeterminismGate(scaffold, corpus, 600);

		ASSERT_EQ((int)result.Entries.size(), (int)corpus.size());
		for (const GenesisStartupDeterminismGateEntry& entry : result.Entries) {
			EXPECT_TRUE(entry.Pass);
			EXPECT_EQ(entry.RequiredCheckpointCount, 14);
			EXPECT_EQ(entry.PassingRequiredCheckpointCount, 14);
			EXPECT_FALSE(entry.SignatureA.empty());
			EXPECT_EQ(entry.SignatureA, entry.SignatureB);
		}
	}

	TEST(GenesisStartupDeterminismGateTests, ExtendedWindowAddsFirst20SecondsGateRequirement) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildStartupDeterminismCorpus();

		GenesisStartupDeterminismGateResult startup10s = GenesisSmokeHarness::RunStartupDeterminismGate(scaffold, corpus, 600);
		GenesisStartupDeterminismGateResult startup20s = GenesisSmokeHarness::RunStartupDeterminismGate(scaffold, corpus, 1200);

		EXPECT_EQ(startup10s.PassCount, (int)corpus.size());
		EXPECT_EQ(startup20s.PassCount, (int)corpus.size());
		ASSERT_EQ((int)startup10s.Entries.size(), (int)corpus.size());
		ASSERT_EQ((int)startup20s.Entries.size(), (int)corpus.size());

		for (size_t i = 0; i < startup10s.Entries.size(); i++) {
			EXPECT_EQ(startup10s.Entries[i].RequiredCheckpointCount, 14);
			EXPECT_EQ(startup20s.Entries[i].RequiredCheckpointCount, 15);
			EXPECT_EQ(startup20s.Entries[i].PassingRequiredCheckpointCount, 15);
		}

		EXPECT_NE(startup10s.Digest, startup20s.Digest);
	}

	TEST(GenesisStartupDeterminismGateTests, EmptyRomEntryTriggersStartupGateFailureDiagnostics) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"startup-empty.bin", {}});

		GenesisStartupDeterminismGateResult result = GenesisSmokeHarness::RunStartupDeterminismGate(scaffold, corpus, 600);
		EXPECT_EQ(result.PassCount, 0);
		EXPECT_EQ(result.FailCount, 1);
		ASSERT_EQ((int)result.Entries.size(), 1);
		EXPECT_FALSE(result.Entries.front().Pass);

		bool hasFailContext = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("GEN_STARTUP_GATE_FAIL_CONTEXT ");
		});
		EXPECT_TRUE(hasFailContext);
	}

	TEST(GenesisStartupDeterminismGateTests, StartupProfileChangesGateDigestForSameCorpus) {
		vector<GenesisCompatibilityRomCase> corpus = BuildStartupDeterminismCorpus();
		GenesisStartupDeterminismGateResult strictResult = {};
		GenesisStartupDeterminismGateResult mesenResult = {};

		{
			ScopedEnvVar startupProfile("NEXEN_GENESIS_STARTUP_PROFILE", "strict");
			GenesisM68kBoundaryScaffold strictScaffold;
			strictResult = GenesisSmokeHarness::RunStartupDeterminismGate(strictScaffold, corpus, 600);
		}

		{
			ScopedEnvVar startupProfile("NEXEN_GENESIS_STARTUP_PROFILE", "mesen");
			GenesisM68kBoundaryScaffold mesenScaffold;
			mesenResult = GenesisSmokeHarness::RunStartupDeterminismGate(mesenScaffold, corpus, 600);
		}

		EXPECT_EQ(strictResult.PassCount, (int)corpus.size());
		EXPECT_EQ(mesenResult.PassCount, (int)corpus.size());
		EXPECT_FALSE(strictResult.Digest.empty());
		EXPECT_FALSE(mesenResult.Digest.empty());
		EXPECT_NE(strictResult.Digest, mesenResult.Digest);
	}
}
