#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600SmokeHarness.h"

namespace {
	vector<Atari2600BaselineRomCase> BuildCompatibilityCorpus() {
		vector<Atari2600BaselineRomCase> corpus;
		corpus.push_back({"compat-target-f8.a26", vector<uint8_t>(8192, 0xEA)});
		corpus.push_back({"compat-target-f6.a26", vector<uint8_t>(16384, 0xEA)});
		corpus.push_back({"compat-target-3f.a26", vector<uint8_t>(8192, 0xEA)});
		return corpus;
	}

	string FindFirstLineWithPrefix(const vector<string>& lines, const string& prefix) {
		auto it = std::find_if(lines.begin(), lines.end(), [&](const string& line) {
			return line.starts_with(prefix);
		});
		if (it == lines.end()) {
			return "";
		}
		return *it;
	}

	TEST(Atari2600CompatibilityMatrixTests, CompatibilityCorpusProducesDeterministicMatrixDigest) {
		Emulator emu;
		Atari2600Console console(&emu);
		vector<Atari2600BaselineRomCase> corpus = BuildCompatibilityCorpus();

		Atari2600CompatibilityMatrixResult runA = Atari2600SmokeHarness::RunCompatibilityMatrix(console, corpus);
		Atari2600CompatibilityMatrixResult runB = Atari2600SmokeHarness::RunCompatibilityMatrix(console, corpus);

		ASSERT_EQ(runA.Entries.size(), corpus.size());
		EXPECT_EQ(runA.PassCount, (int)corpus.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);

		bool hasPerTitleResult = std::any_of(runA.OutputLines.begin(), runA.OutputLines.end(), [](const string& line) {
			return line.starts_with("COMPAT_RESULT ");
		});
		bool hasSummary = std::any_of(runA.OutputLines.begin(), runA.OutputLines.end(), [](const string& line) {
			return line.starts_with("COMPAT_MATRIX_SUMMARY ");
		});

		EXPECT_TRUE(hasPerTitleResult);
		EXPECT_TRUE(hasSummary);
	}

	TEST(Atari2600CompatibilityMatrixTests, EmptyRomCaseFailsWithDeterministicCheckpoint) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<Atari2600BaselineRomCase> corpus;
		corpus.push_back({"compat-empty.a26", {}});

		Atari2600CompatibilityMatrixResult result = Atari2600SmokeHarness::RunCompatibilityMatrix(console, corpus);
		ASSERT_EQ(result.Entries.size(), 1u);
		EXPECT_EQ(result.PassCount, 0);
		EXPECT_EQ(result.FailCount, 1);
		EXPECT_FALSE(result.Digest.empty());
		EXPECT_FALSE(result.Entries[0].Checkpoints.empty());
		EXPECT_FALSE(result.Entries[0].Pass);

		string failContext = FindFirstLineWithPrefix(result.OutputLines, "COMPAT_FAIL_CONTEXT ");
		EXPECT_FALSE(failContext.empty());
		EXPECT_NE(failContext.find("mapper=unknown"), string::npos);
		EXPECT_NE(failContext.find("expected_mapper=unknown"), string::npos);
		EXPECT_NE(failContext.find("mapper_match=1"), string::npos);
		EXPECT_NE(failContext.find("rom_size=0"), string::npos);
		EXPECT_NE(failContext.find("load_result=-1"), string::npos);
		EXPECT_NE(failContext.find("baseline_digest=n/a"), string::npos);
		EXPECT_NE(failContext.find("timing_digest=n/a"), string::npos);
	}

	TEST(Atari2600CompatibilityMatrixTests, MapperMismatchEmitsDeterministicMapperDiagnostics) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<Atari2600BaselineRomCase> corpus;
		corpus.push_back({"compat-target-f6-wrong-size.a26", vector<uint8_t>(8192, 0xEA)});

		Atari2600CompatibilityMatrixResult runA = Atari2600SmokeHarness::RunCompatibilityMatrix(console, corpus);
		Atari2600CompatibilityMatrixResult runB = Atari2600SmokeHarness::RunCompatibilityMatrix(console, corpus);

		ASSERT_EQ(runA.Entries.size(), 1u);
		EXPECT_EQ(runA.PassCount, 0);
		EXPECT_EQ(runA.FailCount, 1);
		EXPECT_EQ(runA.Digest, runB.Digest);

		string failContextA = FindFirstLineWithPrefix(runA.OutputLines, "COMPAT_FAIL_CONTEXT ");
		string failContextB = FindFirstLineWithPrefix(runB.OutputLines, "COMPAT_FAIL_CONTEXT ");
		EXPECT_FALSE(failContextA.empty());
		EXPECT_EQ(failContextA, failContextB);
		EXPECT_NE(failContextA.find("mapper=f8"), string::npos);
		EXPECT_NE(failContextA.find("expected_mapper=f6"), string::npos);
		EXPECT_NE(failContextA.find("mapper_match=0"), string::npos);
		EXPECT_NE(failContextA.find("rom_size=8192"), string::npos);
		EXPECT_NE(failContextA.find("load_result=0"), string::npos);
		EXPECT_NE(failContextA.find("baseline_digest="), string::npos);
		EXPECT_NE(failContextA.find("timing_digest="), string::npos);
	}
}
