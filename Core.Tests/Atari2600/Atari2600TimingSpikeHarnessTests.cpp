#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600SmokeHarness.h"
#include "Utilities/VirtualFile.h"

namespace {
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
}
