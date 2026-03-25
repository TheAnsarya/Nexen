#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600SmokeHarness.h"

namespace {
	vector<Atari2600BaselineRomCase> BuildPerfCorpus() {
		auto makeNopRom = [](size_t size) -> vector<uint8_t> {
			vector<uint8_t> rom(size, 0xEA);
			for (size_t offset = 0x1000; offset <= size; offset += 0x1000) {
				rom[offset - 3] = 0x4C;
				rom[offset - 2] = 0x00;
				rom[offset - 1] = 0x10;
			}
			if (size < 0x1000) {
				rom[size - 3] = 0x4C;
				rom[size - 2] = 0x00;
				rom[size - 1] = 0x10;
			}
			return rom;
		};

		vector<Atari2600BaselineRomCase> corpus;
		corpus.push_back({"perf-f8.a26", makeNopRom(8192)});
		corpus.push_back({"perf-f6.a26", makeNopRom(16384)});
		corpus.push_back({"perf-3f.a26", makeNopRom(8192)});
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

		bool hasFailContext = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("PERF_FAIL_CONTEXT ");
		});
		EXPECT_TRUE(hasFailContext);
	}
}
