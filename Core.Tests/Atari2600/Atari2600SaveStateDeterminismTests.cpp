#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Utilities/Serializer.h"
#include "Utilities/VirtualFile.h"

namespace {
	void LoadMapper3fRom(Atari2600Console& console) {
		vector<uint8_t> rom8k(8192, 0xEA);
		for (uint8_t bank = 0; bank < 4; bank++) {
			std::fill(
				rom8k.begin() + (size_t)bank * 0x0800,
				rom8k.begin() + ((size_t)bank + 1) * 0x0800,
				(uint8_t)(0x40 + bank));
		}

		VirtualFile romFile(rom8k.data(), rom8k.size(), "savestate-mapper-3f.a26");
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
	}

	string SaveStatePayload(Atari2600Console& console) {
		Serializer saver(1, true, SerializeFormat::Binary);
		console.Serialize(saver);
		stringstream ss;
		saver.SaveTo(ss, 0);
		return ss.str();
	}

	void LoadStatePayload(Atari2600Console& console, const string& payload) {
		stringstream ss(payload);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(ss));
		console.Serialize(loader);
	}

	void RunDeterministicExecutionBlock(Atari2600Console& console) {
		console.DebugWriteCartridge(0x0015, 0x01);
		console.DebugWriteCartridge(0x0016, 0x02);
		console.DebugWriteCartridge(0x0017, 0x05);
		console.DebugWriteCartridge(0x0018, 0x03);
		console.DebugWriteCartridge(0x0019, 0x0C);
		console.DebugWriteCartridge(0x001A, 0x07);
		console.DebugWriteCartridge(0x003F, 0x02);
		console.RequestWsync();
		console.RequestHmove();
		console.StepCpuCycles(137);
		console.DebugWriteCartridge(0x0084, 0x09);
		console.StepCpuCycles(41);
	}

	uint64_t HashMix(uint64_t hash, uint64_t value) {
		hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
		return hash;
	}

	vector<uint64_t> RunLongFrameDigestCheckpoints(uint32_t frameCount, uint32_t checkpointInterval) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadMapper3fRom(console);

		vector<uint64_t> checkpoints;
		checkpoints.reserve(frameCount / checkpointInterval);
		uint64_t digest = 0xcbf29ce484222325ULL;

		for (uint32_t frame = 1; frame <= frameCount; frame++) {
			console.RunFrame();

			if ((frame % checkpointInterval) == 0) {
				Atari2600FrameStepSummary summary = console.GetLastFrameSummary();
				Atari2600RiotState riotState = console.GetRiotState();
				Atari2600TiaState tiaState = console.GetTiaState();

				digest = HashMix(digest, summary.FrameCount);
				digest = HashMix(digest, summary.CpuCyclesThisFrame);
				digest = HashMix(digest, summary.ScanlineAtFrameEnd);
				digest = HashMix(digest, summary.ColorClockAtFrameEnd);
				digest = HashMix(digest, riotState.InterruptEdgeCount);
				digest = HashMix(digest, riotState.Timer);
				digest = HashMix(digest, tiaState.TotalColorClocks);
				digest = HashMix(digest, tiaState.HmoveApplyCount);
				digest = HashMix(digest, tiaState.AudioMixAccumulator);
				digest = HashMix(digest, tiaState.CollisionCxp0fb);
				digest = HashMix(digest, tiaState.CollisionCxppmm);

				checkpoints.push_back(digest);
			}
		}

		return checkpoints;
	}

	TEST(Atari2600SaveStateDeterminismTests, SerializeRoundTripReplaysDeterministically) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadMapper3fRom(console);

		console.StepCpuCycles(64);
		string snapshot = SaveStatePayload(console);

		RunDeterministicExecutionBlock(console);
		Atari2600RiotState expectedRiot = console.GetRiotState();
		Atari2600TiaState expectedTia = console.GetTiaState();
		uint64_t expectedClock = console.GetMasterClock();
		uint8_t expectedMapperBank = console.DebugGetMapperBankIndex();

		LoadStatePayload(console, snapshot);
		RunDeterministicExecutionBlock(console);

		Atari2600RiotState replayRiot = console.GetRiotState();
		Atari2600TiaState replayTia = console.GetTiaState();
		EXPECT_EQ(console.GetMasterClock(), expectedClock);
		EXPECT_EQ(console.DebugGetMapperBankIndex(), expectedMapperBank);
		EXPECT_EQ(replayRiot.CpuCycles, expectedRiot.CpuCycles);
		EXPECT_EQ(replayRiot.Timer, expectedRiot.Timer);
		EXPECT_EQ(replayRiot.InterruptEdgeCount, expectedRiot.InterruptEdgeCount);
		EXPECT_EQ(replayTia.Scanline, expectedTia.Scanline);
		EXPECT_EQ(replayTia.ColorClock, expectedTia.ColorClock);
		EXPECT_EQ(replayTia.TotalColorClocks, expectedTia.TotalColorClocks);
		EXPECT_EQ(replayTia.WsyncCount, expectedTia.WsyncCount);
		EXPECT_EQ(replayTia.HmoveApplyCount, expectedTia.HmoveApplyCount);
		EXPECT_EQ(replayTia.AudioSampleCount, expectedTia.AudioSampleCount);
		EXPECT_EQ(replayTia.AudioMixAccumulator, expectedTia.AudioMixAccumulator);
		EXPECT_EQ(replayTia.LastMixedSample, expectedTia.LastMixedSample);
	}

	TEST(Atari2600SaveStateDeterminismTests, StatePayloadIsDeterministicForUnchangedState) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadMapper3fRom(console);

		console.StepCpuCycles(120);
		console.DebugWriteCartridge(0x0019, 0x0A);
		console.DebugWriteCartridge(0x001A, 0x08);
		console.StepCpuCycles(20);

		string payloadA = SaveStatePayload(console);
		string payloadB = SaveStatePayload(console);

		EXPECT_FALSE(payloadA.empty());
		EXPECT_EQ(payloadA, payloadB);
	}

	TEST(Atari2600SaveStateDeterminismTests, LongRunFrameDigestCheckpointsRemainStableAt10kFrames) {
		vector<uint64_t> runA = RunLongFrameDigestCheckpoints(10000, 1000);
		vector<uint64_t> runB = RunLongFrameDigestCheckpoints(10000, 1000);

		ASSERT_EQ(runA.size(), 10u);
		ASSERT_EQ(runB.size(), 10u);
		EXPECT_EQ(runA, runB);
		EXPECT_NE(runA.back(), 0u);
	}
}
