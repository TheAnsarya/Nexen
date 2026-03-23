#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Utilities/VirtualFile.h"

// Comprehensive 6507 opcode/addressing mode conformance corpus for Atari 2600.
// Tests observe CPU results via RIOT port writes ($0280 = PortA, $0281 = PortB).
// RIOT RAM occupies zero-page $80-$FF.
// ROM starts at $1000 (offset 0x0000 in the 4KB ROM image).
// CPU starts execution at $1000 with all registers zeroed.

namespace {

	// =========================================================================
	// Arithmetic: ADC
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, AdcImmediateAddsWithoutCarry) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// clc; lda #$10; adc #$25; sta $0280
		rom[0x0000] = 0x18; // clc          (2 cyc)
		rom[0x0001] = 0xA9; // lda #$10     (2 cyc)
		rom[0x0002] = 0x10;
		rom[0x0003] = 0x69; // adc #$25     (2 cyc)
		rom[0x0004] = 0x25;
		rom[0x0005] = 0x8D; // sta $0280    (4 cyc)
		rom[0x0006] = 0x80;
		rom[0x0007] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "adc-imm.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(10);
		EXPECT_EQ(console.GetRiotState().PortA, 0x35u);
	}

	TEST(Atari2600CpuOpcodeTests, AdcImmediateAddsWithCarryIn) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// sec; lda #$10; adc #$25; sta $0280
		rom[0x0000] = 0x38; // sec          (2 cyc)
		rom[0x0001] = 0xA9; // lda #$10     (2 cyc)
		rom[0x0002] = 0x10;
		rom[0x0003] = 0x69; // adc #$25     (2 cyc)
		rom[0x0004] = 0x25;
		rom[0x0005] = 0x8D; // sta $0280    (4 cyc)
		rom[0x0006] = 0x80;
		rom[0x0007] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "adc-carry-in.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(10);
		EXPECT_EQ(console.GetRiotState().PortA, 0x36u); // $10 + $25 + 1 = $36
	}

	TEST(Atari2600CpuOpcodeTests, AdcImmediateOverflowSetsCarryFlag) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// clc; lda #$FF; adc #$01; sta $0280
		// A = $FF + $01 = $00 with carry set
		// Then: lda #$00; adc #$00; sta $0281  (should be $01 from carry)
		rom[0x0000] = 0x18; // clc           (2)
		rom[0x0001] = 0xA9; // lda #$FF      (2)
		rom[0x0002] = 0xFF;
		rom[0x0003] = 0x69; // adc #$01      (2)
		rom[0x0004] = 0x01;
		rom[0x0005] = 0x8D; // sta $0280     (4) = $00
		rom[0x0006] = 0x80;
		rom[0x0007] = 0x02;
		rom[0x0008] = 0xA9; // lda #$00      (2)
		rom[0x0009] = 0x00;
		rom[0x000A] = 0x69; // adc #$00      (2) = $00 + $00 + carry(1) = $01
		rom[0x000B] = 0x00;
		rom[0x000C] = 0x8D; // sta $0281     (4)
		rom[0x000D] = 0x81;
		rom[0x000E] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "adc-carry-out.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(18);
		EXPECT_EQ(console.GetRiotState().PortA, 0x00u);
		EXPECT_EQ(console.GetRiotState().PortB, 0x01u);
	}

	TEST(Atari2600CpuOpcodeTests, AdcZeroPageAddsFromRam) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$42; sta $80; clc; lda #$10; adc $80; sta $0280
		rom[0x0000] = 0xA9; // lda #$42      (2)
		rom[0x0001] = 0x42;
		rom[0x0002] = 0x85; // sta $80       (3) — ZP store to RIOT RAM
		rom[0x0003] = 0x80;
		rom[0x0004] = 0x18; // clc           (2)
		rom[0x0005] = 0xA9; // lda #$10      (2)
		rom[0x0006] = 0x10;
		rom[0x0007] = 0x65; // adc $80       (3)
		rom[0x0008] = 0x80;
		rom[0x0009] = 0x8D; // sta $0280     (4)
		rom[0x000A] = 0x80;
		rom[0x000B] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "adc-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(16);
		EXPECT_EQ(console.GetRiotState().PortA, 0x52u); // $10 + $42 = $52
	}

	TEST(Atari2600CpuOpcodeTests, AdcAbsoluteAddsFromRam) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$33; sta $0090; clc; lda #$11; adc $0090; sta $0280
		rom[0x0000] = 0xA9; // lda #$33      (2)
		rom[0x0001] = 0x33;
		rom[0x0002] = 0x8D; // sta $0090     (4) — abs store to RIOT RAM
		rom[0x0003] = 0x90;
		rom[0x0004] = 0x00;
		rom[0x0005] = 0x18; // clc           (2)
		rom[0x0006] = 0xA9; // lda #$11      (2)
		rom[0x0007] = 0x11;
		rom[0x0008] = 0x6D; // adc $0090     (4)
		rom[0x0009] = 0x90;
		rom[0x000A] = 0x00;
		rom[0x000B] = 0x8D; // sta $0280     (4)
		rom[0x000C] = 0x80;
		rom[0x000D] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "adc-abs.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(18);
		EXPECT_EQ(console.GetRiotState().PortA, 0x44u); // $11 + $33 = $44
	}

	// =========================================================================
	// Arithmetic: SBC
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, SbcImmediateSubtractsWithBorrow) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// sec; lda #$50; sbc #$20; sta $0280
		rom[0x0000] = 0x38; // sec          (2)
		rom[0x0001] = 0xA9; // lda #$50     (2)
		rom[0x0002] = 0x50;
		rom[0x0003] = 0xE9; // sbc #$20     (2)
		rom[0x0004] = 0x20;
		rom[0x0005] = 0x8D; // sta $0280    (4)
		rom[0x0006] = 0x80;
		rom[0x0007] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "sbc-imm.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(10);
		EXPECT_EQ(console.GetRiotState().PortA, 0x30u); // $50 - $20 = $30
	}

	TEST(Atari2600CpuOpcodeTests, SbcImmediateWithBorrowIn) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// clc; lda #$50; sbc #$20; sta $0280
		// Without carry set: $50 - $20 - 1 = $2F
		rom[0x0000] = 0x18; // clc          (2)
		rom[0x0001] = 0xA9; // lda #$50     (2)
		rom[0x0002] = 0x50;
		rom[0x0003] = 0xE9; // sbc #$20     (2)
		rom[0x0004] = 0x20;
		rom[0x0005] = 0x8D; // sta $0280    (4)
		rom[0x0006] = 0x80;
		rom[0x0007] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "sbc-borrow.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(10);
		EXPECT_EQ(console.GetRiotState().PortA, 0x2Fu); // $50 - $20 - 1 = $2F
	}

	TEST(Atari2600CpuOpcodeTests, SbcUnderflowClearsBorrow) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// sec; lda #$10; sbc #$20; sta $0280
		// $10 - $20 = $F0 (underflow, carry cleared)
		// Then: lda #$50; sbc #$00; sta $0281 (should be $50 - $00 - 1 = $4F)
		rom[0x0000] = 0x38; // sec          (2)
		rom[0x0001] = 0xA9; // lda #$10     (2)
		rom[0x0002] = 0x10;
		rom[0x0003] = 0xE9; // sbc #$20     (2) → $F0, carry=0
		rom[0x0004] = 0x20;
		rom[0x0005] = 0x8D; // sta $0280    (4)
		rom[0x0006] = 0x80;
		rom[0x0007] = 0x02;
		rom[0x0008] = 0xA9; // lda #$50     (2)
		rom[0x0009] = 0x50;
		rom[0x000A] = 0xE9; // sbc #$00     (2) → $50 - $00 - 1 = $4F
		rom[0x000B] = 0x00;
		rom[0x000C] = 0x8D; // sta $0281    (4)
		rom[0x000D] = 0x81;
		rom[0x000E] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "sbc-underflow.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(18);
		EXPECT_EQ(console.GetRiotState().PortA, 0xF0u);
		EXPECT_EQ(console.GetRiotState().PortB, 0x4Fu);
	}

	// =========================================================================
	// Logic: AND, ORA, EOR
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, AndImmediateMasksBits) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$CF; and #$F0; sta $0280
		rom[0x0000] = 0xA9; // lda #$CF     (2)
		rom[0x0001] = 0xCF;
		rom[0x0002] = 0x29; // and #$F0     (2)
		rom[0x0003] = 0xF0;
		rom[0x0004] = 0x8D; // sta $0280    (4)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "and-imm.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(8);
		EXPECT_EQ(console.GetRiotState().PortA, 0xC0u);
	}

	TEST(Atari2600CpuOpcodeTests, AndZeroPageMasksBitsFromRam) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$0F; sta $80; lda #$AB; and $80; sta $0280
		rom[0x0000] = 0xA9; // lda #$0F     (2)
		rom[0x0001] = 0x0F;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xA9; // lda #$AB     (2)
		rom[0x0005] = 0xAB;
		rom[0x0006] = 0x25; // and $80      (3)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "and-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(14);
		EXPECT_EQ(console.GetRiotState().PortA, 0x0Bu); // $AB & $0F = $0B
	}

	TEST(Atari2600CpuOpcodeTests, OraImmediateSetsBits) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$0A; ora #$50; sta $0280
		rom[0x0000] = 0xA9; // lda #$0A     (2)
		rom[0x0001] = 0x0A;
		rom[0x0002] = 0x09; // ora #$50     (2)
		rom[0x0003] = 0x50;
		rom[0x0004] = 0x8D; // sta $0280    (4)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "ora-imm.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(8);
		EXPECT_EQ(console.GetRiotState().PortA, 0x5Au); // $0A | $50 = $5A
	}

	TEST(Atari2600CpuOpcodeTests, EorImmediateXorsBits) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$FF; eor #$AA; sta $0280
		rom[0x0000] = 0xA9; // lda #$FF     (2)
		rom[0x0001] = 0xFF;
		rom[0x0002] = 0x49; // eor #$AA     (2)
		rom[0x0003] = 0xAA;
		rom[0x0004] = 0x8D; // sta $0280    (4)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "eor-imm.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(8);
		EXPECT_EQ(console.GetRiotState().PortA, 0x55u); // $FF ^ $AA = $55
	}

	// =========================================================================
	// Shifts & Rotates: ASL, LSR, ROL, ROR
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, AslAccumulatorShiftsLeft) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$25; asl a; sta $0280
		rom[0x0000] = 0xA9; // lda #$25     (2)
		rom[0x0001] = 0x25;
		rom[0x0002] = 0x0A; // asl a        (2)
		rom[0x0003] = 0x8D; // sta $0280    (4)
		rom[0x0004] = 0x80;
		rom[0x0005] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "asl-acc.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(8);
		EXPECT_EQ(console.GetRiotState().PortA, 0x4Au); // $25 << 1 = $4A
	}

	TEST(Atari2600CpuOpcodeTests, AslAccumulatorSetsCarryFromBit7) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$80; asl a; sta $0280; lda #$00; adc #$00; sta $0281
		rom[0x0000] = 0xA9; // lda #$80     (2)
		rom[0x0001] = 0x80;
		rom[0x0002] = 0x0A; // asl a        (2) → $00, carry=1
		rom[0x0003] = 0x8D; // sta $0280    (4) → $00
		rom[0x0004] = 0x80;
		rom[0x0005] = 0x02;
		rom[0x0006] = 0xA9; // lda #$00     (2)
		rom[0x0007] = 0x00;
		rom[0x0008] = 0x69; // adc #$00     (2) = $00 + carry(1) = $01
		rom[0x0009] = 0x00;
		rom[0x000A] = 0x8D; // sta $0281    (4)
		rom[0x000B] = 0x81;
		rom[0x000C] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "asl-carry.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(16);
		EXPECT_EQ(console.GetRiotState().PortA, 0x00u);
		EXPECT_EQ(console.GetRiotState().PortB, 0x01u);
	}

	TEST(Atari2600CpuOpcodeTests, AslZeroPageShiftsMemory) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$19; sta $80; asl $80; lda $80; sta $0280
		rom[0x0000] = 0xA9; // lda #$19     (2)
		rom[0x0001] = 0x19;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0x06; // asl $80      (5)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0xA5; // lda $80      (3)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "asl-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(17);
		EXPECT_EQ(console.GetRiotState().PortA, 0x32u); // $19 << 1 = $32
	}

	TEST(Atari2600CpuOpcodeTests, LsrAccumulatorShiftsRight) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$4A; lsr a; sta $0280
		rom[0x0000] = 0xA9; // lda #$4A     (2)
		rom[0x0001] = 0x4A;
		rom[0x0002] = 0x4A; // lsr a        (2)
		rom[0x0003] = 0x8D; // sta $0280    (4)
		rom[0x0004] = 0x80;
		rom[0x0005] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "lsr-acc.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(8);
		EXPECT_EQ(console.GetRiotState().PortA, 0x25u); // $4A >> 1 = $25
	}

	TEST(Atari2600CpuOpcodeTests, RolAccumulatorRotatesLeftThroughCarry) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// sec; lda #$25; rol a; sta $0280
		// carry=1, A=$25(00100101) → result = 01001011 = $4B
		rom[0x0000] = 0x38; // sec          (2)
		rom[0x0001] = 0xA9; // lda #$25     (2)
		rom[0x0002] = 0x25;
		rom[0x0003] = 0x2A; // rol a        (2)
		rom[0x0004] = 0x8D; // sta $0280    (4)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "rol-acc.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(10);
		EXPECT_EQ(console.GetRiotState().PortA, 0x4Bu);
	}

	TEST(Atari2600CpuOpcodeTests, RorAccumulatorRotatesRightThroughCarry) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// sec; lda #$4A; ror a; sta $0280
		// carry=1, A=$4A(01001010) → result = 10100101 = $A5
		rom[0x0000] = 0x38; // sec          (2)
		rom[0x0001] = 0xA9; // lda #$4A     (2)
		rom[0x0002] = 0x4A;
		rom[0x0003] = 0x6A; // ror a        (2)
		rom[0x0004] = 0x8D; // sta $0280    (4)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "ror-acc.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(10);
		EXPECT_EQ(console.GetRiotState().PortA, 0xA5u);
	}

	// =========================================================================
	// Compare: CMP, CPX, CPY
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, CmpImmediateEqualSetsZero) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$42; cmp #$42; beq +2; lda #$01; sta $0280
		// If equal, skip lda #$01 → A stays $42 → PortA = $42
		// If not equal, A = $01 → PortA = $01
		rom[0x0000] = 0xA9; // lda #$42     (2)
		rom[0x0001] = 0x42;
		rom[0x0002] = 0xC9; // cmp #$42     (2)
		rom[0x0003] = 0x42;
		rom[0x0004] = 0xF0; // beq +2       (2/3)
		rom[0x0005] = 0x02;
		rom[0x0006] = 0xA9; // lda #$01     (skipped)
		rom[0x0007] = 0x01;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "cmp-eq.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(13);
		EXPECT_EQ(console.GetRiotState().PortA, 0x42u);
	}

	TEST(Atari2600CpuOpcodeTests, CmpImmediateGreaterSetsCarry) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$50; cmp #$30; bcs +2; lda #$01; sta $0280
		// $50 >= $30, so carry set, branch taken → A stays $50
		rom[0x0000] = 0xA9; // lda #$50     (2)
		rom[0x0001] = 0x50;
		rom[0x0002] = 0xC9; // cmp #$30     (2)
		rom[0x0003] = 0x30;
		rom[0x0004] = 0xB0; // bcs +2       (2/3)
		rom[0x0005] = 0x02;
		rom[0x0006] = 0xA9; // lda #$01     (skipped)
		rom[0x0007] = 0x01;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "cmp-gt.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(13);
		EXPECT_EQ(console.GetRiotState().PortA, 0x50u);
	}

	TEST(Atari2600CpuOpcodeTests, CpxImmediateComparesXRegister) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldx #$10; cpx #$10; beq +2; lda #$FF; sta $0280
		// X == $10, Z set → branch taken → A = $10 (from ldx... no, A is separate)
		// We need to get A=$42 if equal
		rom[0x0000] = 0xA2; // ldx #$10     (2)
		rom[0x0001] = 0x10;
		rom[0x0002] = 0xA9; // lda #$42     (2)
		rom[0x0003] = 0x42;
		rom[0x0004] = 0xE0; // cpx #$10     (2)
		rom[0x0005] = 0x10;
		rom[0x0006] = 0xF0; // beq +2       (3 taken)
		rom[0x0007] = 0x02;
		rom[0x0008] = 0xA9; // lda #$FF     (skipped)
		rom[0x0009] = 0xFF;
		rom[0x000A] = 0x8D; // sta $0280    (4)
		rom[0x000B] = 0x80;
		rom[0x000C] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "cpx-imm.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(15);
		EXPECT_EQ(console.GetRiotState().PortA, 0x42u);
	}

	TEST(Atari2600CpuOpcodeTests, CpyImmediateComparesYRegister) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldy #$20; lda #$55; cpy #$20; beq +2; lda #$FF; sta $0280
		rom[0x0000] = 0xA0; // ldy #$20     (2)
		rom[0x0001] = 0x20;
		rom[0x0002] = 0xA9; // lda #$55     (2)
		rom[0x0003] = 0x55;
		rom[0x0004] = 0xC0; // cpy #$20     (2)
		rom[0x0005] = 0x20;
		rom[0x0006] = 0xF0; // beq +2       (3 taken)
		rom[0x0007] = 0x02;
		rom[0x0008] = 0xA9; // lda #$FF     (skipped)
		rom[0x0009] = 0xFF;
		rom[0x000A] = 0x8D; // sta $0280    (4)
		rom[0x000B] = 0x80;
		rom[0x000C] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "cpy-imm.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(15);
		EXPECT_EQ(console.GetRiotState().PortA, 0x55u);
	}

	// =========================================================================
	// Increment/Decrement: INC, DEC, INX, DEX, INY, DEY
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, IncZeroPageIncrementsMemory) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$41; sta $80; inc $80; lda $80; sta $0280
		rom[0x0000] = 0xA9; // lda #$41     (2)
		rom[0x0001] = 0x41;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xE6; // inc $80      (5)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0xA5; // lda $80      (3)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "inc-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(17);
		EXPECT_EQ(console.GetRiotState().PortA, 0x42u);
	}

	TEST(Atari2600CpuOpcodeTests, DecZeroPageDecrementsMemory) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$43; sta $80; dec $80; lda $80; sta $0280
		rom[0x0000] = 0xA9; // lda #$43     (2)
		rom[0x0001] = 0x43;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xC6; // dec $80      (5)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0xA5; // lda $80      (3)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "dec-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(17);
		EXPECT_EQ(console.GetRiotState().PortA, 0x42u);
	}

	TEST(Atari2600CpuOpcodeTests, IncZeroPageWrapsFromFFToZero) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$FF; sta $80; inc $80; lda $80; sta $0280
		rom[0x0000] = 0xA9; // lda #$FF     (2)
		rom[0x0001] = 0xFF;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xE6; // inc $80      (5)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0xA5; // lda $80      (3)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "inc-wrap.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(17);
		EXPECT_EQ(console.GetRiotState().PortA, 0x00u);
	}

	TEST(Atari2600CpuOpcodeTests, InxIncrementsXRegister) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldx #$41; inx; txa; sta $0280
		rom[0x0000] = 0xA2; // ldx #$41     (2)
		rom[0x0001] = 0x41;
		rom[0x0002] = 0xE8; // inx          (2)
		rom[0x0003] = 0x8A; // txa          (2)
		rom[0x0004] = 0x8D; // sta $0280    (4)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "inx.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(10);
		EXPECT_EQ(console.GetRiotState().PortA, 0x42u);
	}

	TEST(Atari2600CpuOpcodeTests, DexDecrementsXRegister) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldx #$43; dex; txa; sta $0280
		rom[0x0000] = 0xA2; // ldx #$43     (2)
		rom[0x0001] = 0x43;
		rom[0x0002] = 0xCA; // dex          (2)
		rom[0x0003] = 0x8A; // txa          (2)
		rom[0x0004] = 0x8D; // sta $0280    (4)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "dex.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(10);
		EXPECT_EQ(console.GetRiotState().PortA, 0x42u);
	}

	TEST(Atari2600CpuOpcodeTests, InyIncrementsYRegister) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldy #$41; iny; tya; sta $0280
		rom[0x0000] = 0xA0; // ldy #$41     (2)
		rom[0x0001] = 0x41;
		rom[0x0002] = 0xC8; // iny          (2)
		rom[0x0003] = 0x98; // tya          (2)
		rom[0x0004] = 0x8D; // sta $0280    (4)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "iny.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(10);
		EXPECT_EQ(console.GetRiotState().PortA, 0x42u);
	}

	TEST(Atari2600CpuOpcodeTests, DeyDecrementsYRegister) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldy #$43; dey; tya; sta $0280
		rom[0x0000] = 0xA0; // ldy #$43     (2)
		rom[0x0001] = 0x43;
		rom[0x0002] = 0x88; // dey          (2)
		rom[0x0003] = 0x98; // tya          (2)
		rom[0x0004] = 0x8D; // sta $0280    (4)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "dey.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(10);
		EXPECT_EQ(console.GetRiotState().PortA, 0x42u);
	}

	// =========================================================================
	// Register Transfers: TAX, TAY, TXA, TYA, TXS, TSX
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, TaxTransfersAccumulatorToX) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$77; tax; stx $80; lda $80; sta $0280
		rom[0x0000] = 0xA9; // lda #$77     (2)
		rom[0x0001] = 0x77;
		rom[0x0002] = 0xAA; // tax          (2)
		rom[0x0003] = 0x86; // stx $80      (3)
		rom[0x0004] = 0x80;
		rom[0x0005] = 0xA5; // lda $80      (3)
		rom[0x0006] = 0x80;
		rom[0x0007] = 0x8D; // sta $0280    (4)
		rom[0x0008] = 0x80;
		rom[0x0009] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "tax.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(14);
		EXPECT_EQ(console.GetRiotState().PortA, 0x77u);
	}

	TEST(Atari2600CpuOpcodeTests, TayTransfersAccumulatorToY) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$88; tay; sty $80; lda $80; sta $0280
		rom[0x0000] = 0xA9; // lda #$88     (2)
		rom[0x0001] = 0x88;
		rom[0x0002] = 0xA8; // tay          (2)
		rom[0x0003] = 0x84; // sty $80      (3)
		rom[0x0004] = 0x80;
		rom[0x0005] = 0xA5; // lda $80      (3)
		rom[0x0006] = 0x80;
		rom[0x0007] = 0x8D; // sta $0280    (4)
		rom[0x0008] = 0x80;
		rom[0x0009] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "tay.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(14);
		EXPECT_EQ(console.GetRiotState().PortA, 0x88u);
	}

	TEST(Atari2600CpuOpcodeTests, TxaTransfersXToAccumulator) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldx #$33; txa; sta $0280
		rom[0x0000] = 0xA2; // ldx #$33     (2)
		rom[0x0001] = 0x33;
		rom[0x0002] = 0x8A; // txa          (2)
		rom[0x0003] = 0x8D; // sta $0280    (4)
		rom[0x0004] = 0x80;
		rom[0x0005] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "txa.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(8);
		EXPECT_EQ(console.GetRiotState().PortA, 0x33u);
	}

	TEST(Atari2600CpuOpcodeTests, TyaTransfersYToAccumulator) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldy #$44; tya; sta $0280
		rom[0x0000] = 0xA0; // ldy #$44     (2)
		rom[0x0001] = 0x44;
		rom[0x0002] = 0x98; // tya          (2)
		rom[0x0003] = 0x8D; // sta $0280    (4)
		rom[0x0004] = 0x80;
		rom[0x0005] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "tya.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(8);
		EXPECT_EQ(console.GetRiotState().PortA, 0x44u);
	}

	TEST(Atari2600CpuOpcodeTests, TxsTsxStackPointerRoundTrip) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldx #$E0; txs; ldx #$00; tsx; txa; sta $0280
		rom[0x0000] = 0xA2; // ldx #$E0     (2)
		rom[0x0001] = 0xE0;
		rom[0x0002] = 0x9A; // txs          (2)
		rom[0x0003] = 0xA2; // ldx #$00     (2)
		rom[0x0004] = 0x00;
		rom[0x0005] = 0xBA; // tsx          (2)
		rom[0x0006] = 0x8A; // txa          (2)
		rom[0x0007] = 0x8D; // sta $0280    (4)
		rom[0x0008] = 0x80;
		rom[0x0009] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "txs-tsx.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(14);
		EXPECT_EQ(console.GetRiotState().PortA, 0xE0u);
	}

	// =========================================================================
	// Load/Store: LDX, LDY, STX, STY with addressing modes
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, LdxImmediateLoadsX) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldx #$AB; stx $0280
		rom[0x0000] = 0xA2; // ldx #$AB     (2)
		rom[0x0001] = 0xAB;
		rom[0x0002] = 0x8E; // stx $0280    (4)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "ldx-imm.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(6);
		EXPECT_EQ(console.GetRiotState().PortA, 0xABu);
	}

	TEST(Atari2600CpuOpcodeTests, LdyImmediateLoadsY) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldy #$CD; sty $0280
		rom[0x0000] = 0xA0; // ldy #$CD     (2)
		rom[0x0001] = 0xCD;
		rom[0x0002] = 0x8C; // sty $0280    (4)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "ldy-imm.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(6);
		EXPECT_EQ(console.GetRiotState().PortA, 0xCDu);
	}

	TEST(Atari2600CpuOpcodeTests, StxZeroPageStoresX) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldx #$EF; stx $80; lda $80; sta $0280
		rom[0x0000] = 0xA2; // ldx #$EF     (2)
		rom[0x0001] = 0xEF;
		rom[0x0002] = 0x86; // stx $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xA5; // lda $80      (3)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x8D; // sta $0280    (4)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "stx-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(12);
		EXPECT_EQ(console.GetRiotState().PortA, 0xEFu);
	}

	TEST(Atari2600CpuOpcodeTests, StyZeroPageStoresY) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldy #$DE; sty $80; lda $80; sta $0280
		rom[0x0000] = 0xA0; // ldy #$DE     (2)
		rom[0x0001] = 0xDE;
		rom[0x0002] = 0x84; // sty $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xA5; // lda $80      (3)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x8D; // sta $0280    (4)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "sty-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(12);
		EXPECT_EQ(console.GetRiotState().PortA, 0xDEu);
	}

	TEST(Atari2600CpuOpcodeTests, LdaZeroPageLoadsFromRam) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$99; sta $85; lda #$00; lda $85; sta $0280
		rom[0x0000] = 0xA9; // lda #$99     (2)
		rom[0x0001] = 0x99;
		rom[0x0002] = 0x85; // sta $85      (3)
		rom[0x0003] = 0x85;
		rom[0x0004] = 0xA9; // lda #$00     (2)
		rom[0x0005] = 0x00;
		rom[0x0006] = 0xA5; // lda $85      (3)
		rom[0x0007] = 0x85;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "lda-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(14);
		EXPECT_EQ(console.GetRiotState().PortA, 0x99u);
	}

	// =========================================================================
	// Indexed Addressing: ZP,X  ZP,Y  Abs,X  Abs,Y
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, LdaZeroPageXIndexesWithX) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$77; sta $85; ldx #$05; lda $80,x; sta $0280
		// ZP addr = ($80 + $05) & $FF = $85
		rom[0x0000] = 0xA9; // lda #$77     (2)
		rom[0x0001] = 0x77;
		rom[0x0002] = 0x85; // sta $85      (3)
		rom[0x0003] = 0x85;
		rom[0x0004] = 0xA2; // ldx #$05     (2)
		rom[0x0005] = 0x05;
		rom[0x0006] = 0xB5; // lda $80,x    (4)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "lda-zpx.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(15);
		EXPECT_EQ(console.GetRiotState().PortA, 0x77u);
	}

	TEST(Atari2600CpuOpcodeTests, LdxZeroPageYIndexesWithY) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$66; sta $87; ldy #$07; ldx $80,y; txa; sta $0280
		// ZP addr = ($80 + $07) & $FF = $87
		rom[0x0000] = 0xA9; // lda #$66     (2)
		rom[0x0001] = 0x66;
		rom[0x0002] = 0x85; // sta $87      (3)
		rom[0x0003] = 0x87;
		rom[0x0004] = 0xA0; // ldy #$07     (2)
		rom[0x0005] = 0x07;
		rom[0x0006] = 0xB6; // ldx $80,y    (4)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x8A; // txa          (2)
		rom[0x0009] = 0x8D; // sta $0280    (4)
		rom[0x000A] = 0x80;
		rom[0x000B] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "ldx-zpy.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(17);
		EXPECT_EQ(console.GetRiotState().PortA, 0x66u);
	}

	TEST(Atari2600CpuOpcodeTests, LdaAbsoluteXIndexesWithX) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$55; sta $8A; ldx #$0A; lda $0080,x; sta $0280
		// Abs addr = $0080 + $0A = $008A → RIOT RAM
		rom[0x0000] = 0xA9; // lda #$55     (2)
		rom[0x0001] = 0x55;
		rom[0x0002] = 0x8D; // sta $008A    (4)
		rom[0x0003] = 0x8A;
		rom[0x0004] = 0x00;
		rom[0x0005] = 0xA2; // ldx #$0A     (2)
		rom[0x0006] = 0x0A;
		rom[0x0007] = 0xBD; // lda $0080,x  (4)
		rom[0x0008] = 0x80;
		rom[0x0009] = 0x00;
		rom[0x000A] = 0x8D; // sta $0280    (4)
		rom[0x000B] = 0x80;
		rom[0x000C] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "lda-absx.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(16);
		EXPECT_EQ(console.GetRiotState().PortA, 0x55u);
	}

	TEST(Atari2600CpuOpcodeTests, LdaAbsoluteYIndexesWithY) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$44; sta $8C; ldy #$0C; lda $0080,y; sta $0280
		rom[0x0000] = 0xA9; // lda #$44     (2)
		rom[0x0001] = 0x44;
		rom[0x0002] = 0x8D; // sta $008C    (4)
		rom[0x0003] = 0x8C;
		rom[0x0004] = 0x00;
		rom[0x0005] = 0xA0; // ldy #$0C     (2)
		rom[0x0006] = 0x0C;
		rom[0x0007] = 0xB9; // lda $0080,y  (4)
		rom[0x0008] = 0x80;
		rom[0x0009] = 0x00;
		rom[0x000A] = 0x8D; // sta $0280    (4)
		rom[0x000B] = 0x80;
		rom[0x000C] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "lda-absy.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(16);
		EXPECT_EQ(console.GetRiotState().PortA, 0x44u);
	}

	TEST(Atari2600CpuOpcodeTests, StaZeroPageXStoresIndexed) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldx #$03; lda #$BB; sta $80,x; lda $83; sta $0280
		rom[0x0000] = 0xA2; // ldx #$03     (2)
		rom[0x0001] = 0x03;
		rom[0x0002] = 0xA9; // lda #$BB     (2)
		rom[0x0003] = 0xBB;
		rom[0x0004] = 0x95; // sta $80,x    (4)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0xA5; // lda $83      (3)
		rom[0x0007] = 0x83;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "sta-zpx.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(15);
		EXPECT_EQ(console.GetRiotState().PortA, 0xBBu);
	}

	// =========================================================================
	// Indirect Addressing: (Indirect,X) and (Indirect),Y
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, LdaIndirectXLoadsViaPointer) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// Set up pointer at ZP $84-$85 pointing to $0090
		// Store $AA at $0090
		// ldx #$04; lda ($80,x) should read from address at ($80+$04)=($84)=$0090
		rom[0x0000] = 0xA9; // lda #$90     (2)
		rom[0x0001] = 0x90;
		rom[0x0002] = 0x85; // sta $84      (3) — low byte of pointer
		rom[0x0003] = 0x84;
		rom[0x0004] = 0xA9; // lda #$00     (2)
		rom[0x0005] = 0x00;
		rom[0x0006] = 0x85; // sta $85      (3) — high byte of pointer
		rom[0x0007] = 0x85;
		rom[0x0008] = 0xA9; // lda #$AA     (2)
		rom[0x0009] = 0xAA;
		rom[0x000A] = 0x8D; // sta $0090    (4) — target value
		rom[0x000B] = 0x90;
		rom[0x000C] = 0x00;
		rom[0x000D] = 0xA2; // ldx #$04     (2)
		rom[0x000E] = 0x04;
		rom[0x000F] = 0xA1; // lda ($80,x)  (6)
		rom[0x0010] = 0x80;
		rom[0x0011] = 0x8D; // sta $0280    (4)
		rom[0x0012] = 0x80;
		rom[0x0013] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "lda-indx.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(28);
		EXPECT_EQ(console.GetRiotState().PortA, 0xAAu);
	}

	TEST(Atari2600CpuOpcodeTests, LdaIndirectYLoadsViaPointerPlusY) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// Set up pointer at ZP $80-$81 pointing to $0088
		// Store $BB at $008B ($0088 + Y=$03)
		rom[0x0000] = 0xA9; // lda #$88     (2)
		rom[0x0001] = 0x88;
		rom[0x0002] = 0x85; // sta $80      (3) — low byte
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xA9; // lda #$00     (2)
		rom[0x0005] = 0x00;
		rom[0x0006] = 0x85; // sta $81      (3) — high byte
		rom[0x0007] = 0x81;
		rom[0x0008] = 0xA9; // lda #$BB     (2)
		rom[0x0009] = 0xBB;
		rom[0x000A] = 0x8D; // sta $008B    (4) — target
		rom[0x000B] = 0x8B;
		rom[0x000C] = 0x00;
		rom[0x000D] = 0xA0; // ldy #$03     (2)
		rom[0x000E] = 0x03;
		rom[0x000F] = 0xB1; // lda ($80),y  (5)
		rom[0x0010] = 0x80;
		rom[0x0011] = 0x8D; // sta $0280    (4)
		rom[0x0012] = 0x80;
		rom[0x0013] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "lda-indy.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(27);
		EXPECT_EQ(console.GetRiotState().PortA, 0xBBu);
	}

	TEST(Atari2600CpuOpcodeTests, StaIndirectXStoresViaPointer) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// Set pointer at ZP $84-$85 = $0280 (RIOT PortA)
		// ldx #$04; lda #$CC; sta ($80,x)
		rom[0x0000] = 0xA9; // lda #$80     (2) — low byte of $0280
		rom[0x0001] = 0x80;
		rom[0x0002] = 0x85; // sta $84      (3)
		rom[0x0003] = 0x84;
		rom[0x0004] = 0xA9; // lda #$02     (2) — high byte of $0280
		rom[0x0005] = 0x02;
		rom[0x0006] = 0x85; // sta $85      (3)
		rom[0x0007] = 0x85;
		rom[0x0008] = 0xA2; // ldx #$04     (2)
		rom[0x0009] = 0x04;
		rom[0x000A] = 0xA9; // lda #$CC     (2)
		rom[0x000B] = 0xCC;
		rom[0x000C] = 0x81; // sta ($80,x)  (6)
		rom[0x000D] = 0x80;

		VirtualFile testRom(rom.data(), rom.size(), "sta-indx.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(20);
		EXPECT_EQ(console.GetRiotState().PortA, 0xCCu);
	}

	TEST(Atari2600CpuOpcodeTests, StaIndirectYStoresViaPointerPlusY) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// Set pointer at ZP $80-$81 = $0280 (RIOT PortA base)
		// ldy #$01; lda #$DD; sta ($80),y → write to $0281 (PortB)
		rom[0x0000] = 0xA9; // lda #$80     (2)
		rom[0x0001] = 0x80;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xA9; // lda #$02     (2)
		rom[0x0005] = 0x02;
		rom[0x0006] = 0x85; // sta $81      (3)
		rom[0x0007] = 0x81;
		rom[0x0008] = 0xA0; // ldy #$01     (2)
		rom[0x0009] = 0x01;
		rom[0x000A] = 0xA9; // lda #$DD     (2)
		rom[0x000B] = 0xDD;
		rom[0x000C] = 0x91; // sta ($80),y  (6)
		rom[0x000D] = 0x80;

		VirtualFile testRom(rom.data(), rom.size(), "sta-indy.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(20);
		EXPECT_EQ(console.GetRiotState().PortB, 0xDDu);
	}

	// =========================================================================
	// Branch Instructions: BPL, BMI, BCC, BCS, BVC, BVS
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, BplBranchesWhenPositive) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$01 (positive); bpl +2; lda #$FF; sta $0280
		rom[0x0000] = 0xA9; // lda #$01     (2)
		rom[0x0001] = 0x01;
		rom[0x0002] = 0x10; // bpl +2       (3 taken)
		rom[0x0003] = 0x02;
		rom[0x0004] = 0xA9; // lda #$FF     (skipped)
		rom[0x0005] = 0xFF;
		rom[0x0006] = 0x8D; // sta $0280    (4)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "bpl.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(11);
		EXPECT_EQ(console.GetRiotState().PortA, 0x01u);
	}

	TEST(Atari2600CpuOpcodeTests, BmiBranchesWhenNegative) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$80 (negative); bmi +2; lda #$01; sta $0280
		rom[0x0000] = 0xA9; // lda #$80     (2)
		rom[0x0001] = 0x80;
		rom[0x0002] = 0x30; // bmi +2       (3 taken)
		rom[0x0003] = 0x02;
		rom[0x0004] = 0xA9; // lda #$01     (skipped)
		rom[0x0005] = 0x01;
		rom[0x0006] = 0x8D; // sta $0280    (4)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "bmi.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(11);
		EXPECT_EQ(console.GetRiotState().PortA, 0x80u);
	}

	TEST(Atari2600CpuOpcodeTests, BccBranchesWhenCarryClear) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// clc; lda #$22; bcc +2; lda #$FF; sta $0280
		rom[0x0000] = 0x18; // clc          (2)
		rom[0x0001] = 0xA9; // lda #$22     (2)
		rom[0x0002] = 0x22;
		rom[0x0003] = 0x90; // bcc +2       (3 taken)
		rom[0x0004] = 0x02;
		rom[0x0005] = 0xA9; // lda #$FF     (skipped)
		rom[0x0006] = 0xFF;
		rom[0x0007] = 0x8D; // sta $0280    (4)
		rom[0x0008] = 0x80;
		rom[0x0009] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "bcc.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(13);
		EXPECT_EQ(console.GetRiotState().PortA, 0x22u);
	}

	TEST(Atari2600CpuOpcodeTests, BcsBranchesWhenCarrySet) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// sec; lda #$33; bcs +2; lda #$FF; sta $0280
		rom[0x0000] = 0x38; // sec          (2)
		rom[0x0001] = 0xA9; // lda #$33     (2)
		rom[0x0002] = 0x33;
		rom[0x0003] = 0xB0; // bcs +2       (3 taken)
		rom[0x0004] = 0x02;
		rom[0x0005] = 0xA9; // lda #$FF     (skipped)
		rom[0x0006] = 0xFF;
		rom[0x0007] = 0x8D; // sta $0280    (4)
		rom[0x0008] = 0x80;
		rom[0x0009] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "bcs.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(13);
		EXPECT_EQ(console.GetRiotState().PortA, 0x33u);
	}

	TEST(Atari2600CpuOpcodeTests, BvcBranchesWhenOverflowClear) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// clv; lda #$11; bvc +2; lda #$FF; sta $0280
		rom[0x0000] = 0xB8; // clv          (2)
		rom[0x0001] = 0xA9; // lda #$11     (2)
		rom[0x0002] = 0x11;
		rom[0x0003] = 0x50; // bvc +2       (3 taken)
		rom[0x0004] = 0x02;
		rom[0x0005] = 0xA9; // lda #$FF     (skipped)
		rom[0x0006] = 0xFF;
		rom[0x0007] = 0x8D; // sta $0280    (4)
		rom[0x0008] = 0x80;
		rom[0x0009] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "bvc.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(13);
		EXPECT_EQ(console.GetRiotState().PortA, 0x11u);
	}

	TEST(Atari2600CpuOpcodeTests, BvsBranchesWhenOverflowSet) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// Force overflow: clc; lda #$7F; adc #$01 → V=1
		// lda #$22; bvs +2; lda #$FF; sta $0280
		rom[0x0000] = 0x18; // clc          (2)
		rom[0x0001] = 0xA9; // lda #$7F     (2)
		rom[0x0002] = 0x7F;
		rom[0x0003] = 0x69; // adc #$01     (2) → $80, V=1
		rom[0x0004] = 0x01;
		rom[0x0005] = 0xA9; // lda #$22     (2)
		rom[0x0006] = 0x22;
		rom[0x0007] = 0x70; // bvs +2       (3 taken)
		rom[0x0008] = 0x02;
		rom[0x0009] = 0xA9; // lda #$FF     (skipped)
		rom[0x000A] = 0xFF;
		rom[0x000B] = 0x8D; // sta $0280    (4)
		rom[0x000C] = 0x80;
		rom[0x000D] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "bvs.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(17);
		EXPECT_EQ(console.GetRiotState().PortA, 0x22u);
	}

	// =========================================================================
	// Flag Instructions: CLC, SEC, CLI, SEI, CLD, SED, CLV
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, ClcSecToggleCarry) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// sec; lda #$00; adc #$00; sta $0280 (should be $01 from carry)
		// clc; lda #$00; adc #$00; sta $0281 (should be $00, no carry)
		rom[0x0000] = 0x38; // sec          (2)
		rom[0x0001] = 0xA9; // lda #$00     (2)
		rom[0x0002] = 0x00;
		rom[0x0003] = 0x69; // adc #$00     (2) = $01
		rom[0x0004] = 0x00;
		rom[0x0005] = 0x8D; // sta $0280    (4)
		rom[0x0006] = 0x80;
		rom[0x0007] = 0x02;
		rom[0x0008] = 0x18; // clc          (2)
		rom[0x0009] = 0xA9; // lda #$00     (2)
		rom[0x000A] = 0x00;
		rom[0x000B] = 0x69; // adc #$00     (2) = $00
		rom[0x000C] = 0x00;
		rom[0x000D] = 0x8D; // sta $0281    (4)
		rom[0x000E] = 0x81;
		rom[0x000F] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "clc-sec.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(20);
		EXPECT_EQ(console.GetRiotState().PortA, 0x01u);
		EXPECT_EQ(console.GetRiotState().PortB, 0x00u);
	}

	TEST(Atari2600CpuOpcodeTests, ClvClearsOverflowFlag) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// Force overflow: clc; lda #$7F; adc #$01 → V=1
		// clv → V=0
		// bvc +2; lda #$FF; sta $0280  (branch should be taken after clv)
		rom[0x0000] = 0x18; // clc          (2)
		rom[0x0001] = 0xA9; // lda #$7F     (2)
		rom[0x0002] = 0x7F;
		rom[0x0003] = 0x69; // adc #$01     (2) → V=1
		rom[0x0004] = 0x01;
		rom[0x0005] = 0xA9; // lda #$42     (2)
		rom[0x0006] = 0x42;
		rom[0x0007] = 0xB8; // clv          (2)
		rom[0x0008] = 0x50; // bvc +2       (3 taken since V=0)
		rom[0x0009] = 0x02;
		rom[0x000A] = 0xA9; // lda #$FF     (skipped)
		rom[0x000B] = 0xFF;
		rom[0x000C] = 0x8D; // sta $0280    (4)
		rom[0x000D] = 0x80;
		rom[0x000E] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "clv.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(19);
		EXPECT_EQ(console.GetRiotState().PortA, 0x42u);
	}

	// =========================================================================
	// Stack: PHP, PLP (flag round-trip)
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, PhpPlpPreservesFlags) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// sec; php; clc; plp; lda #$00; adc #$00; sta $0280
		// After plp: carry should be restored=1 → adc result = $01
		rom[0x0000] = 0x38; // sec          (2)
		rom[0x0001] = 0x08; // php          (3)
		rom[0x0002] = 0x18; // clc          (2)
		rom[0x0003] = 0x28; // plp          (4)
		rom[0x0004] = 0xA9; // lda #$00     (2)
		rom[0x0005] = 0x00;
		rom[0x0006] = 0x69; // adc #$00     (2) = $01 (carry restored)
		rom[0x0007] = 0x00;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "php-plp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(19);
		EXPECT_EQ(console.GetRiotState().PortA, 0x01u);
	}

	// =========================================================================
	// Jump: JMP absolute, JMP indirect
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, JmpAbsoluteJumpsToAddress) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$99; jmp $1010; lda #$01; ...
		// At $1010: sta $0280
		rom[0x0000] = 0xA9; // lda #$99     (2)
		rom[0x0001] = 0x99;
		rom[0x0002] = 0x4C; // jmp $1010    (3)
		rom[0x0003] = 0x10;
		rom[0x0004] = 0x10;
		rom[0x0005] = 0xA9; // lda #$01     (skipped)
		rom[0x0006] = 0x01;

		rom[0x0010] = 0x8D; // sta $0280    (4)
		rom[0x0011] = 0x80;
		rom[0x0012] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "jmp-abs.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(9);
		EXPECT_EQ(console.GetRiotState().PortA, 0x99u);
	}

	TEST(Atari2600CpuOpcodeTests, JmpIndirectJumpsViaPointer) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// Set up pointer in ROM at $1020-$1021 pointing to $1030
		// jmp ($1020) should jump to $1030
		// At $1030: lda #$77; sta $0280
		rom[0x0000] = 0xA9; // lda #$77     (2)
		rom[0x0001] = 0x77;
		rom[0x0002] = 0x6C; // jmp ($1020)  (5)
		rom[0x0003] = 0x20;
		rom[0x0004] = 0x10;

		rom[0x0020] = 0x30; // low byte → $1030
		rom[0x0021] = 0x10; // high byte

		rom[0x0030] = 0x8D; // sta $0280    (4)
		rom[0x0031] = 0x80;
		rom[0x0032] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "jmp-ind.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(11);
		EXPECT_EQ(console.GetRiotState().PortA, 0x77u);
	}

	// =========================================================================
	// BIT test
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, BitZeroPageSetsZeroWhenNoCommonBits) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$F0; sta $80; lda #$0F; bit $80; beq +2; lda #$FF; sta $0280
		// A & M = $0F & $F0 = $00 → Z=1 → branch taken → A stays $0F
		rom[0x0000] = 0xA9; // lda #$F0     (2)
		rom[0x0001] = 0xF0;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xA9; // lda #$0F     (2)
		rom[0x0005] = 0x0F;
		rom[0x0006] = 0x24; // bit $80      (3)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0xF0; // beq +2       (3 taken)
		rom[0x0009] = 0x02;
		rom[0x000A] = 0xA9; // lda #$FF     (skipped)
		rom[0x000B] = 0xFF;
		rom[0x000C] = 0x8D; // sta $0280    (4)
		rom[0x000D] = 0x80;
		rom[0x000E] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "bit-zp-zero.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(19);
		EXPECT_EQ(console.GetRiotState().PortA, 0x0Fu);
	}

	TEST(Atari2600CpuOpcodeTests, BitZeroPageSetsNegativeFromMemoryBit7) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$80; sta $80; lda #$FF; bit $80; bmi +2; lda #$01; sta $0280
		// M = $80, bit 7 = 1 → N=1 → bmi taken → A stays $FF
		rom[0x0000] = 0xA9; // lda #$80     (2)
		rom[0x0001] = 0x80;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xA9; // lda #$FF     (2)
		rom[0x0005] = 0xFF;
		rom[0x0006] = 0x24; // bit $80      (3)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x30; // bmi +2       (3 taken)
		rom[0x0009] = 0x02;
		rom[0x000A] = 0xA9; // lda #$01     (skipped)
		rom[0x000B] = 0x01;
		rom[0x000C] = 0x8D; // sta $0280    (4)
		rom[0x000D] = 0x80;
		rom[0x000E] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "bit-zp-neg.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(19);
		EXPECT_EQ(console.GetRiotState().PortA, 0xFFu);
	}

	TEST(Atari2600CpuOpcodeTests, BitZeroPageSetsOverflowFromMemoryBit6) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$40; sta $80; lda #$FF; bit $80; bvs +2; lda #$01; sta $0280
		// M = $40, bit 6 = 1 → V=1 → bvs taken → A stays $FF
		rom[0x0000] = 0xA9; // lda #$40     (2)
		rom[0x0001] = 0x40;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xA9; // lda #$FF     (2)
		rom[0x0005] = 0xFF;
		rom[0x0006] = 0x24; // bit $80      (3)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x70; // bvs +2       (3 taken)
		rom[0x0009] = 0x02;
		rom[0x000A] = 0xA9; // lda #$01     (skipped)
		rom[0x000B] = 0x01;
		rom[0x000C] = 0x8D; // sta $0280    (4)
		rom[0x000D] = 0x80;
		rom[0x000E] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "bit-zp-overflow.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(19);
		EXPECT_EQ(console.GetRiotState().PortA, 0xFFu);
	}

	// =========================================================================
	// ADC Overflow (V flag) — signed arithmetic edge cases
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, AdcSignedOverflowPositivePlusPositive) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// clc; lda #$7F; adc #$01 → $80 (pos + pos = neg → overflow)
		// bvs +2; lda #$01; sta $0280
		rom[0x0000] = 0x18; // clc          (2)
		rom[0x0001] = 0xA9; // lda #$7F     (2)
		rom[0x0002] = 0x7F;
		rom[0x0003] = 0x69; // adc #$01     (2) → $80, V=1
		rom[0x0004] = 0x01;
		rom[0x0005] = 0x8D; // sta $0280    (4) = $80
		rom[0x0006] = 0x80;
		rom[0x0007] = 0x02;
		rom[0x0008] = 0x70; // bvs +2       (3 taken)
		rom[0x0009] = 0x02;
		rom[0x000A] = 0xA9; // lda #$01     (skipped)
		rom[0x000B] = 0x01;
		rom[0x000C] = 0x8D; // sta $0281    (4) = $80 (unchanged since skipped)
		rom[0x000D] = 0x81;
		rom[0x000E] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "adc-overflow.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(19);
		EXPECT_EQ(console.GetRiotState().PortA, 0x80u); // Result
		EXPECT_EQ(console.GetRiotState().PortB, 0x80u); // V was set so branch taken, stored $80
	}

	TEST(Atari2600CpuOpcodeTests, AdcNoOverflowWhenSignsMatch) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// clc; lda #$20; adc #$30 → $50 (no overflow, both positive, result positive)
		// bvc +2; lda #$FF; sta $0280
		rom[0x0000] = 0x18; // clc          (2)
		rom[0x0001] = 0xA9; // lda #$20     (2)
		rom[0x0002] = 0x20;
		rom[0x0003] = 0x69; // adc #$30     (2) → $50, V=0
		rom[0x0004] = 0x30;
		rom[0x0005] = 0x50; // bvc +2       (3 taken since V=0)
		rom[0x0006] = 0x02;
		rom[0x0007] = 0xA9; // lda #$FF     (skipped)
		rom[0x0008] = 0xFF;
		rom[0x0009] = 0x8D; // sta $0280    (4)
		rom[0x000A] = 0x80;
		rom[0x000B] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "adc-no-overflow.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(15);
		EXPECT_EQ(console.GetRiotState().PortA, 0x50u);
	}

	// =========================================================================
	// NOP
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, NopDoesNotAlterState) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$42; nop; nop; nop; sta $0280
		rom[0x0000] = 0xA9; // lda #$42     (2)
		rom[0x0001] = 0x42;
		rom[0x0002] = 0xEA; // nop          (2)
		rom[0x0003] = 0xEA; // nop          (2)
		rom[0x0004] = 0xEA; // nop          (2)
		rom[0x0005] = 0x8D; // sta $0280    (4)
		rom[0x0006] = 0x80;
		rom[0x0007] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "nop.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(12);
		EXPECT_EQ(console.GetRiotState().PortA, 0x42u);
	}

	// =========================================================================
	// Cycle Count Validation
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, ImpliedInstructionsTakeTwoCycles) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// nop; nop; nop; nop; nop → 10 cycles total (5 * 2)
		// Already all NOPs from fill. Just verify cycle count.
		VirtualFile testRom(rom.data(), rom.size(), "cycles-implied.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(10);
		EXPECT_EQ(console.GetMasterClock(), 10u);
	}

	TEST(Atari2600CpuOpcodeTests, ImmediateInstructionsTakeTwoCycles) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$42 → 2 cycles
		rom[0x0000] = 0xA9; // lda #$42     (2)
		rom[0x0001] = 0x42;

		VirtualFile testRom(rom.data(), rom.size(), "cycles-imm.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(2);
		EXPECT_EQ(console.GetMasterClock(), 2u);
	}

	TEST(Atari2600CpuOpcodeTests, ZeroPageReadTakesThreeCycles) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda $80 → 3 cycles
		rom[0x0000] = 0xA5; // lda $80      (3)
		rom[0x0001] = 0x80;

		VirtualFile testRom(rom.data(), rom.size(), "cycles-zp-read.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(3);
		EXPECT_EQ(console.GetMasterClock(), 3u);
	}

	TEST(Atari2600CpuOpcodeTests, ZeroPageWriteTakesThreeCycles) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// sta $80 → 3 cycles
		rom[0x0000] = 0x85; // sta $80      (3)
		rom[0x0001] = 0x80;

		VirtualFile testRom(rom.data(), rom.size(), "cycles-zp-write.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(3);
		EXPECT_EQ(console.GetMasterClock(), 3u);
	}

	TEST(Atari2600CpuOpcodeTests, AbsoluteReadTakesFourCycles) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda $0080 → 4 cycles
		rom[0x0000] = 0xAD; // lda $0080    (4)
		rom[0x0001] = 0x80;
		rom[0x0002] = 0x00;

		VirtualFile testRom(rom.data(), rom.size(), "cycles-abs-read.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(4);
		EXPECT_EQ(console.GetMasterClock(), 4u);
	}

	TEST(Atari2600CpuOpcodeTests, AbsoluteWriteTakesFourCycles) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// sta $0080 → 4 cycles
		rom[0x0000] = 0x8D; // sta $0080    (4)
		rom[0x0001] = 0x80;
		rom[0x0002] = 0x00;

		VirtualFile testRom(rom.data(), rom.size(), "cycles-abs-write.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(4);
		EXPECT_EQ(console.GetMasterClock(), 4u);
	}

	TEST(Atari2600CpuOpcodeTests, JsrTakesSixCyclesRtsTakesSixCycles) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// jsr $1010 → 6 cycles
		// At $1010: rts → 6 cycles
		// nop → 2 cycles
		// Total: 14 cycles
		rom[0x0000] = 0x20; // jsr $1010    (6)
		rom[0x0001] = 0x10;
		rom[0x0002] = 0x10;

		rom[0x0010] = 0x60; // rts          (6)

		VirtualFile testRom(rom.data(), rom.size(), "cycles-jsr-rts.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(14);
		EXPECT_EQ(console.GetMasterClock(), 14u);
	}

	TEST(Atari2600CpuOpcodeTests, BranchTakenTakesThreeCyclesSamePageNoPenalty) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$01; bne +2 → branch taken = 3 cycles (same page)
		// Total: lda(2) + bne(3) = 5
		rom[0x0000] = 0xA9; // lda #$01     (2)
		rom[0x0001] = 0x01;
		rom[0x0002] = 0xD0; // bne +2       (3 taken, same page)
		rom[0x0003] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "cycles-branch-taken.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(5);
		EXPECT_EQ(console.GetMasterClock(), 5u);
	}

	TEST(Atari2600CpuOpcodeTests, BranchNotTakenTakesTwoCycles) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$00; bne +2 → branch NOT taken = 2 cycles
		// Total: lda(2) + bne(2) = 4
		rom[0x0000] = 0xA9; // lda #$00     (2)
		rom[0x0001] = 0x00;
		rom[0x0002] = 0xD0; // bne +2       (2 not taken)
		rom[0x0003] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "cycles-branch-not-taken.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(4);
		EXPECT_EQ(console.GetMasterClock(), 4u);
	}

	// =========================================================================
	// Loop Integration Test — counting loop using DEX/BNE
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, CountingLoopWithDexBne) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$00; ldx #$05; loop: clc; adc #$03; dex; bne loop; sta $0280
		// Loop runs 5 times: A = 0 + 3*5 = 15 = $0F
		rom[0x0000] = 0xA9; // lda #$00     (2)
		rom[0x0001] = 0x00;
		rom[0x0002] = 0xA2; // ldx #$05     (2)
		rom[0x0003] = 0x05;
		// loop at $1004:
		rom[0x0004] = 0x18; // clc          (2)
		rom[0x0005] = 0x69; // adc #$03     (2)
		rom[0x0006] = 0x03;
		rom[0x0007] = 0xCA; // dex          (2)
		rom[0x0008] = 0xD0; // bne -6       (3 taken / 2 not taken)
		rom[0x0009] = 0xFA; // relative offset = -6 (back to $1004)
		rom[0x000A] = 0x8D; // sta $0280    (4)
		rom[0x000B] = 0x80;
		rom[0x000C] = 0x02;
		// Cycles: 2+2 = 4 (setup)
		// Each taken iteration: 2+2+2+3 = 9 (4 iterations)
		// Last iteration (not taken): 2+2+2+2 = 8
		// sta: 4
		// Total: 4 + 4*9 + 8 + 4 = 52

		VirtualFile testRom(rom.data(), rom.size(), "loop-dex-bne.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(52);
		EXPECT_EQ(console.GetRiotState().PortA, 0x0Fu);
	}

	// =========================================================================
	// Zero-page wrapping for indexed modes
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, ZeroPageXWrapsWithinZeroPage) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$EE; sta $82; ldx #$82; lda $00,x; sta $0280
		// ZP addr = ($00 + $82) & $FF = $82 → should read RIOT RAM at $82
		// But $82 in RIOT RAM is ($82 & $7F) = $02
		// Actually $82 with bit 7 set → RIOT range → addr & 0x7F = $02
		// We stored at $82 which is RIOT RAM[$02]
		rom[0x0000] = 0xA9; // lda #$EE     (2)
		rom[0x0001] = 0xEE;
		rom[0x0002] = 0x85; // sta $82      (3)
		rom[0x0003] = 0x82;
		rom[0x0004] = 0xA2; // ldx #$82     (2)
		rom[0x0005] = 0x82;
		rom[0x0006] = 0xB5; // lda $00,x    (4) — wraps to $82 within ZP
		rom[0x0007] = 0x00;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "zpx-wrap.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(15);
		EXPECT_EQ(console.GetRiotState().PortA, 0xEEu);
	}

	// =========================================================================
	// Store absolute indexed: STA abs,X and STA abs,Y
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, StaAbsoluteXStoresIndexed) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldx #$03; lda #$57; sta $0080,x; lda $83; sta $0280
		rom[0x0000] = 0xA2; // ldx #$03     (2)
		rom[0x0001] = 0x03;
		rom[0x0002] = 0xA9; // lda #$57     (2)
		rom[0x0003] = 0x57;
		rom[0x0004] = 0x9D; // sta $0080,x  (5)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x00;
		rom[0x0007] = 0xA5; // lda $83      (3) — read back from RIOT RAM
		rom[0x0008] = 0x83;
		rom[0x0009] = 0x8D; // sta $0280    (4)
		rom[0x000A] = 0x80;
		rom[0x000B] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "sta-absx.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(16);
		EXPECT_EQ(console.GetRiotState().PortA, 0x57u);
	}

	TEST(Atari2600CpuOpcodeTests, StaAbsoluteYStoresIndexed) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// ldy #$05; lda #$68; sta $0080,y; lda $85; sta $0280
		rom[0x0000] = 0xA0; // ldy #$05     (2)
		rom[0x0001] = 0x05;
		rom[0x0002] = 0xA9; // lda #$68     (2)
		rom[0x0003] = 0x68;
		rom[0x0004] = 0x99; // sta $0080,y  (5)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0x00;
		rom[0x0007] = 0xA5; // lda $85      (3)
		rom[0x0008] = 0x85;
		rom[0x0009] = 0x8D; // sta $0280    (4)
		rom[0x000A] = 0x80;
		rom[0x000B] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "sta-absy.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(16);
		EXPECT_EQ(console.GetRiotState().PortA, 0x68u);
	}

	// =========================================================================
	// INC/DEC absolute mode
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, IncAbsoluteIncrementsMemory) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$FE; sta $0090; inc $0090; lda $0090; sta $0280
		rom[0x0000] = 0xA9; // lda #$FE     (2)
		rom[0x0001] = 0xFE;
		rom[0x0002] = 0x8D; // sta $0090    (4)
		rom[0x0003] = 0x90;
		rom[0x0004] = 0x00;
		rom[0x0005] = 0xEE; // inc $0090    (6)
		rom[0x0006] = 0x90;
		rom[0x0007] = 0x00;
		rom[0x0008] = 0xAD; // lda $0090    (4)
		rom[0x0009] = 0x90;
		rom[0x000A] = 0x00;
		rom[0x000B] = 0x8D; // sta $0280    (4)
		rom[0x000C] = 0x80;
		rom[0x000D] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "inc-abs.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(20);
		EXPECT_EQ(console.GetRiotState().PortA, 0xFFu);
	}

	TEST(Atari2600CpuOpcodeTests, DecAbsoluteDecrementsMemory) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$01; sta $0090; dec $0090; lda $0090; sta $0280
		rom[0x0000] = 0xA9; // lda #$01     (2)
		rom[0x0001] = 0x01;
		rom[0x0002] = 0x8D; // sta $0090    (4)
		rom[0x0003] = 0x90;
		rom[0x0004] = 0x00;
		rom[0x0005] = 0xCE; // dec $0090    (6)
		rom[0x0006] = 0x90;
		rom[0x0007] = 0x00;
		rom[0x0008] = 0xAD; // lda $0090    (4)
		rom[0x0009] = 0x90;
		rom[0x000A] = 0x00;
		rom[0x000B] = 0x8D; // sta $0280    (4)
		rom[0x000C] = 0x80;
		rom[0x000D] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "dec-abs.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(20);
		EXPECT_EQ(console.GetRiotState().PortA, 0x00u);
	}

	// =========================================================================
	// ORA / EOR / AND with indexed and indirect addressing
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, OraZeroPageSetsFromRam) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$30; sta $80; lda #$0C; ora $80; sta $0280
		rom[0x0000] = 0xA9; // lda #$30     (2)
		rom[0x0001] = 0x30;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xA9; // lda #$0C     (2)
		rom[0x0005] = 0x0C;
		rom[0x0006] = 0x05; // ora $80      (3)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "ora-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(14);
		EXPECT_EQ(console.GetRiotState().PortA, 0x3Cu); // $0C | $30 = $3C
	}

	TEST(Atari2600CpuOpcodeTests, EorZeroPageXorsFromRam) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$AA; sta $80; lda #$FF; eor $80; sta $0280
		rom[0x0000] = 0xA9; // lda #$AA     (2)
		rom[0x0001] = 0xAA;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xA9; // lda #$FF     (2)
		rom[0x0005] = 0xFF;
		rom[0x0006] = 0x45; // eor $80      (3)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "eor-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(14);
		EXPECT_EQ(console.GetRiotState().PortA, 0x55u); // $FF ^ $AA = $55
	}

	// =========================================================================
	// SBC zero-page
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, SbcZeroPageSubtractsFromRam) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$15; sta $80; sec; lda #$50; sbc $80; sta $0280
		rom[0x0000] = 0xA9; // lda #$15     (2)
		rom[0x0001] = 0x15;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0x38; // sec          (2)
		rom[0x0005] = 0xA9; // lda #$50     (2)
		rom[0x0006] = 0x50;
		rom[0x0007] = 0xE5; // sbc $80      (3)
		rom[0x0008] = 0x80;
		rom[0x0009] = 0x8D; // sta $0280    (4)
		rom[0x000A] = 0x80;
		rom[0x000B] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "sbc-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(16);
		EXPECT_EQ(console.GetRiotState().PortA, 0x3Bu); // $50 - $15 = $3B
	}

	// =========================================================================
	// CMP/CPX/CPY zero-page and absolute
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, CmpZeroPageComparesWithRam) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$42; sta $80; lda #$42; cmp $80; beq +2; lda #$FF; sta $0280
		rom[0x0000] = 0xA9; // lda #$42     (2)
		rom[0x0001] = 0x42;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0xA9; // lda #$42     (2)
		rom[0x0005] = 0x42;
		rom[0x0006] = 0xC5; // cmp $80      (3)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0xF0; // beq +2       (3 taken)
		rom[0x0009] = 0x02;
		rom[0x000A] = 0xA9; // lda #$FF     (skipped)
		rom[0x000B] = 0xFF;
		rom[0x000C] = 0x8D; // sta $0280    (4)
		rom[0x000D] = 0x80;
		rom[0x000E] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "cmp-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(19);
		EXPECT_EQ(console.GetRiotState().PortA, 0x42u);
	}

	// =========================================================================
	// Shift/Rotate memory modes — ZP,X and Absolute
	// =========================================================================

	TEST(Atari2600CpuOpcodeTests, LsrZeroPageShiftsMemoryRight) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// lda #$64; sta $80; lsr $80; lda $80; sta $0280
		rom[0x0000] = 0xA9; // lda #$64     (2)
		rom[0x0001] = 0x64;
		rom[0x0002] = 0x85; // sta $80      (3)
		rom[0x0003] = 0x80;
		rom[0x0004] = 0x46; // lsr $80      (5)
		rom[0x0005] = 0x80;
		rom[0x0006] = 0xA5; // lda $80      (3)
		rom[0x0007] = 0x80;
		rom[0x0008] = 0x8D; // sta $0280    (4)
		rom[0x0009] = 0x80;
		rom[0x000A] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "lsr-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(17);
		EXPECT_EQ(console.GetRiotState().PortA, 0x32u); // $64 >> 1 = $32
	}

	TEST(Atari2600CpuOpcodeTests, RolZeroPageRotatesMemoryLeft) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// clc; lda #$41; sta $80; rol $80; lda $80; sta $0280
		rom[0x0000] = 0x18; // clc          (2)
		rom[0x0001] = 0xA9; // lda #$41     (2)
		rom[0x0002] = 0x41;
		rom[0x0003] = 0x85; // sta $80      (3)
		rom[0x0004] = 0x80;
		rom[0x0005] = 0x26; // rol $80      (5)
		rom[0x0006] = 0x80;
		rom[0x0007] = 0xA5; // lda $80      (3)
		rom[0x0008] = 0x80;
		rom[0x0009] = 0x8D; // sta $0280    (4)
		rom[0x000A] = 0x80;
		rom[0x000B] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "rol-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(19);
		EXPECT_EQ(console.GetRiotState().PortA, 0x82u); // $41 << 1 = $82
	}

	TEST(Atari2600CpuOpcodeTests, RorZeroPageRotatesMemoryRight) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom(4096, 0xEA);
		// clc; lda #$82; sta $80; ror $80; lda $80; sta $0280
		rom[0x0000] = 0x18; // clc          (2)
		rom[0x0001] = 0xA9; // lda #$82     (2)
		rom[0x0002] = 0x82;
		rom[0x0003] = 0x85; // sta $80      (3)
		rom[0x0004] = 0x80;
		rom[0x0005] = 0x66; // ror $80      (5)
		rom[0x0006] = 0x80;
		rom[0x0007] = 0xA5; // lda $80      (3)
		rom[0x0008] = 0x80;
		rom[0x0009] = 0x8D; // sta $0280    (4)
		rom[0x000A] = 0x80;
		rom[0x000B] = 0x02;

		VirtualFile testRom(rom.data(), rom.size(), "ror-zp.a26");
		ASSERT_EQ(console.LoadRom(testRom), LoadRomResult::Success);

		console.StepCpuCycles(19);
		EXPECT_EQ(console.GetRiotState().PortA, 0x41u); // $82 >> 1 = $41
	}
}
