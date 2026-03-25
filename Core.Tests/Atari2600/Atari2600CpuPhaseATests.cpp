#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Utilities/VirtualFile.h"

namespace {
	TEST(Atari2600CpuPhaseATests, ExecutesLoadAndStoreAcrossInstructionCycleBoundaries) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		rom[0x0000] = 0xA9; // lda #$05
		rom[0x0001] = 0x05;
		rom[0x0002] = 0x8D; // sta $0280 (riot porta)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "cpu-phase-a-load-store.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(2);
		EXPECT_EQ(console.GetMasterClock(), 2u);
		EXPECT_EQ(console.GetRiotState().PortA, 0u);

		console.StepCpuCycles(4);
		Atari2600RiotState riotState = console.GetRiotState();
		EXPECT_EQ(console.GetMasterClock(), 6u);
		EXPECT_EQ(riotState.PortA, 0x05u);
	}

	TEST(Atari2600CpuPhaseATests, BranchFlowSelectsExpectedStorePathAndCycles) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		rom[0x0000] = 0xA9; // lda #$01
		rom[0x0001] = 0x01;
		rom[0x0002] = 0xD0; // bne +2 (taken)
		rom[0x0003] = 0x02;
		rom[0x0004] = 0xA9; // skipped when branch is taken
		rom[0x0005] = 0x00;
		rom[0x0006] = 0x8D; // sta $0280 (riot porta)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x02;
		rom[0x0009] = 0xA9; // lda #$07
		rom[0x000A] = 0x07;
		rom[0x000B] = 0xF0; // beq +2 (not taken)
		rom[0x000C] = 0x02;
		rom[0x000D] = 0xA9; // lda #$03
		rom[0x000E] = 0x03;
		rom[0x000F] = 0x8D; // sta $0281 (riot portb)
		rom[0x0010] = 0x81;
		rom[0x0011] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "cpu-phase-a-branching.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(19);
		Atari2600RiotState riotState = console.GetRiotState();

		EXPECT_EQ(console.GetMasterClock(), 19u);
		EXPECT_EQ(riotState.PortA, 0x01u);
		EXPECT_EQ(riotState.PortB, 0x03u);
	}

	TEST(Atari2600CpuPhaseATests, StackPushPullRoundTripPreservesAccumulatorValue) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		rom[0x0000] = 0xA9; // lda #$2A
		rom[0x0001] = 0x2A;
		rom[0x0002] = 0x48; // pha
		rom[0x0003] = 0xA9; // lda #$00
		rom[0x0004] = 0x00;
		rom[0x0005] = 0x68; // pla
		rom[0x0006] = 0x8D; // sta $0280 (riot porta)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "cpu-phase-a-stack-roundtrip.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(15);
		Atari2600RiotState riotState = console.GetRiotState();

		EXPECT_EQ(console.GetMasterClock(), 15u);
		EXPECT_EQ(riotState.PortA, 0x2Au);
	}

	TEST(Atari2600CpuPhaseATests, JsrRtsFlowExecutesSubroutineAndReturnsToCaller) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		rom[0x0000] = 0xA9; // lda #$11
		rom[0x0001] = 0x11;
		rom[0x0002] = 0x20; // jsr $1010
		rom[0x0003] = 0x10;
		rom[0x0004] = 0x10;
		rom[0x0005] = 0x8D; // sta $0280 (riot porta)
		rom[0x0006] = 0x80;
		rom[0x0007] = 0x02;

		rom[0x0010] = 0xA9; // lda #$22
		rom[0x0011] = 0x22;
		rom[0x0012] = 0x60; // rts

		VirtualFile testRom(rom.data(), rom.size(), "cpu-phase-a-jsr-rts.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(20);
		Atari2600RiotState riotState = console.GetRiotState();

		EXPECT_EQ(console.GetMasterClock(), 20u);
		EXPECT_EQ(riotState.PortA, 0x22u);
	}
}
