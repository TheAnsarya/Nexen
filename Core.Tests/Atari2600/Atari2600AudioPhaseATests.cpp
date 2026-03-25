#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"

namespace {
	void ConfigureAudioRegisters(Atari2600Console& console) {
		console.DebugWriteCartridge(0x0015, 0x01); // audc0
		console.DebugWriteCartridge(0x0016, 0x02); // audc1
		console.DebugWriteCartridge(0x0017, 0x04); // audf0
		console.DebugWriteCartridge(0x0018, 0x06); // audf1
		console.DebugWriteCartridge(0x0019, 0x0A); // audv0
		console.DebugWriteCartridge(0x001A, 0x06); // audv1
	}

	TEST(Atari2600AudioPhaseATests, AudioRegisterWritesUpdateTiaState) {
		Emulator emu;
		Atari2600Console console(&emu);
		console.Reset();

		ConfigureAudioRegisters(console);
		Atari2600TiaState tiaState = console.GetTiaState();

		EXPECT_EQ(tiaState.AudioControl0, 0x01u);
		EXPECT_EQ(tiaState.AudioControl1, 0x02u);
		EXPECT_EQ(tiaState.AudioFrequency0, 0x04u);
		EXPECT_EQ(tiaState.AudioFrequency1, 0x06u);
		EXPECT_EQ(tiaState.AudioVolume0, 0x0Au);
		EXPECT_EQ(tiaState.AudioVolume1, 0x06u);
		EXPECT_GT(tiaState.AudioRevision, 0u);
	}

	TEST(Atari2600AudioPhaseATests, MixerOutputIsDeterministicForSameInputs) {
		Emulator emu;
		Atari2600Console consoleA(&emu);
		Atari2600Console consoleB(&emu);
		consoleA.Reset();
		consoleB.Reset();

		ConfigureAudioRegisters(consoleA);
		ConfigureAudioRegisters(consoleB);
		consoleA.StepCpuCycles(128);
		consoleB.StepCpuCycles(128);

		Atari2600TiaState stateA = consoleA.GetTiaState();
		Atari2600TiaState stateB = consoleB.GetTiaState();

		EXPECT_EQ(stateA.AudioSampleCount, 128u);
		EXPECT_EQ(stateA.AudioSampleCount, stateB.AudioSampleCount);
		EXPECT_EQ(stateA.AudioMixAccumulator, stateB.AudioMixAccumulator);
		EXPECT_EQ(stateA.LastMixedSample, stateB.LastMixedSample);
	}

	TEST(Atari2600AudioPhaseATests, AudioPlayerActionResetsMixerHistory) {
		Emulator emu;
		Atari2600Console console(&emu);
		console.Reset();

		ConfigureAudioRegisters(console);
		console.StepCpuCycles(64);
		EXPECT_GT(console.GetTiaState().AudioSampleCount, 0u);
		EXPECT_GT(console.GetTiaState().AudioMixAccumulator, 0u);

		AudioPlayerActionParams action = {};
		action.Action = AudioPlayerAction::NextTrack;
		action.TrackNumber = 0;
		console.ProcessAudioPlayerAction(action);

		Atari2600TiaState resetState = console.GetTiaState();
		EXPECT_EQ(resetState.AudioSampleCount, 0u);
		EXPECT_EQ(resetState.AudioMixAccumulator, 0u);
		EXPECT_EQ(resetState.LastMixedSample, 0u);
	}
}
