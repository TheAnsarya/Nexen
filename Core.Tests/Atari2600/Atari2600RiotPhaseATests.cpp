#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"

namespace {
	TEST(Atari2600RiotPhaseATests, PortDirectionMasksOutputAndInputReads) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x0082, 0x0F); // DDRA: low nibble output
		console.DebugWriteCartridge(0x0080, 0xA5); // SWCHA output latch

		EXPECT_EQ(console.DebugReadCartridge(0x0082), 0x0Fu);
		EXPECT_EQ(console.DebugReadCartridge(0x0080), 0x05u);

		Atari2600RiotState riotState = console.GetRiotState();
		EXPECT_EQ(riotState.PortA, 0xA5u);
		EXPECT_EQ(riotState.PortADirection, 0x0Fu);
	}

	TEST(Atari2600RiotPhaseATests, TimerDividerAndUnderflowEdgesAreDeterministic) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x0084, 0x02); // divider=1, timer=2

		console.StepCpuCycles(3);
		Atari2600RiotState firstUnderflow = console.GetRiotState();
		EXPECT_TRUE(firstUnderflow.TimerUnderflow);
		EXPECT_TRUE(firstUnderflow.InterruptFlag);
		EXPECT_EQ(firstUnderflow.InterruptEdgeCount, 1u);

		console.StepCpuCycles(256);
		Atari2600RiotState secondUnderflow = console.GetRiotState();
		EXPECT_EQ(secondUnderflow.InterruptEdgeCount, 2u);

		console.Reset();
		console.DebugWriteCartridge(0x0085, 0x01); // divider=8, timer=1
		console.StepCpuCycles(15);
		EXPECT_FALSE(console.GetRiotState().TimerUnderflow);
		console.StepCpuCycles(1);
		EXPECT_TRUE(console.GetRiotState().TimerUnderflow);
	}
}
