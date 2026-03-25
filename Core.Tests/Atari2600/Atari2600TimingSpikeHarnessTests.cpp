#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600SmokeHarness.h"
#include "Utilities/VirtualFile.h"

namespace {
	vector<Atari2600BaselineRomCase> CreateBaselineRomSet() {
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

		vector<Atari2600BaselineRomCase> romSet;

		Atari2600BaselineRomCase nopRom = {};
		nopRom.Name = "baseline-nop-fill.a26";
		nopRom.RomData = makeNopRom(4096);
		romSet.push_back(std::move(nopRom));

		Atari2600BaselineRomCase rom8k = {};
		rom8k.Name = "baseline-nop-8k.a26";
		rom8k.RomData = makeNopRom(8192);
		romSet.push_back(std::move(rom8k));

		Atari2600BaselineRomCase rom16k = {};
		rom16k.Name = "baseline-nop-16k.a26";
		rom16k.RomData = makeNopRom(16384);
		romSet.push_back(std::move(rom16k));

		return romSet;
	}

	TEST(Atari2600TimingSpikeHarnessTests, RiotSkeletonTracksCpuCycleProgress) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.StepCpuCycles(4);
		Atari2600RiotState riotState = console.GetRiotState();

		EXPECT_EQ(riotState.CpuCycles, 4u);
		EXPECT_TRUE(riotState.TimerUnderflow);
	}

	TEST(Atari2600TimingSpikeHarnessTests, SmokeRomFrameStepCompletesWithoutCrash) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		rom[0x0FFD] = 0x4C;
		rom[0x0FFE] = 0x00;
		rom[0x0FFF] = 0x10;
		VirtualFile smokeRom(rom.data(), rom.size(), "smoke.a26");

		EXPECT_EQ(console.LoadRom(smokeRom), LoadRomResult::Success);
		console.RunFrame();

		Atari2600FrameStepSummary summary = console.GetLastFrameSummary();
		EXPECT_EQ(summary.FrameCount, 1u);
		EXPECT_EQ(summary.CpuCyclesThisFrame, Atari2600Console::CpuCyclesPerFrame);
	}

	TEST(Atari2600TimingSpikeHarnessTests, BaselineHarnessReturnsPassingCheckpointSet) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		rom[0x0FFD] = 0x4C;
		rom[0x0FFE] = 0x00;
		rom[0x0FFF] = 0x10;
		VirtualFile nopRom(rom.data(), rom.size(), "baseline.a26");
		console.LoadRom(nopRom);

		Atari2600HarnessResult result = Atari2600SmokeHarness::RunBaseline(console);

		EXPECT_GT(result.PassCount, 0);
		EXPECT_EQ(result.FailCount, 0);
		EXPECT_FALSE(result.Digest.empty());
	}

	TEST(Atari2600TimingSpikeHarnessTests, TimingSpikeHarnessHasStableScanlineDeltas) {
		Emulator emu;
		Atari2600Console console(&emu);

		Atari2600TimingSpikeResult result = Atari2600SmokeHarness::RunTimingSpike(console, 12);

		EXPECT_TRUE(result.Stable);
		EXPECT_EQ(result.ScanlineDeltas.size(), 12u);
		for (uint32_t delta : result.ScanlineDeltas) {
			EXPECT_EQ(delta, 1u);
		}
	}

	TEST(Atari2600TimingSpikeHarnessTests, TimingSpikeHarnessDigestIsDeterministic) {
		Emulator emu;
		Atari2600Console console(&emu);

		Atari2600TimingSpikeResult runA = Atari2600SmokeHarness::RunTimingSpike(console, 10);
		Atari2600TimingSpikeResult runB = Atari2600SmokeHarness::RunTimingSpike(console, 10);

		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);
	}

	TEST(Atari2600TimingSpikeHarnessTests, BaselineRomSetProducesPerRomPassFailResults) {
		Emulator emu;
		Atari2600Console console(&emu);
		vector<Atari2600BaselineRomCase> romSet = CreateBaselineRomSet();

		Atari2600BaselineRomSetResult result = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);

		EXPECT_EQ(result.Entries.size(), romSet.size());
		EXPECT_EQ(result.PassCount, 3);
		EXPECT_EQ(result.FailCount, 0);
		EXPECT_FALSE(result.Digest.empty());
		for (const Atari2600BaselineRomEntry& entry : result.Entries) {
			EXPECT_TRUE(entry.Pass);
			EXPECT_EQ(entry.Result.FailCount, 0);
		}

		bool hasRomResultLine = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("ROM_RESULT ");
		});
		bool hasRomSetSummaryLine = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("ROM_SET_SUMMARY ");
		});

		EXPECT_TRUE(hasRomResultLine);
		EXPECT_TRUE(hasRomSetSummaryLine);
	}

	TEST(Atari2600TimingSpikeHarnessTests, BaselineRomSetDigestIsDeterministic) {
		Emulator emu;
		Atari2600Console console(&emu);
		vector<Atari2600BaselineRomCase> romSet = CreateBaselineRomSet();

		Atari2600BaselineRomSetResult runA = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);
		Atari2600BaselineRomSetResult runB = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);

		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);
	}

	TEST(Atari2600TimingSpikeHarnessTests, BaselineRomSetFailureIncludesTriageContext) {
		Emulator emu;
		Atari2600Console console(&emu);
		vector<Atari2600BaselineRomCase> romSet;
		romSet.push_back({"broken-empty.a26", {}});

		Atari2600BaselineRomSetResult result = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);
		EXPECT_EQ(result.PassCount, 0);
		EXPECT_EQ(result.FailCount, 1);

		bool hasFailContext = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("ROM_FAIL_CONTEXT ");
		});
		EXPECT_TRUE(hasFailContext);
	}
}
