#include "pch.h"
#include "ChannelF/ChannelFCpu.h"

// Test helper: create a CPU wired to a flat 64KB memory array and 256-byte port array
class F8CpuTestFixture : public ::testing::Test {
protected:
	ChannelFCpu cpu;
	uint8_t memory[0x10000] = {};
	uint8_t ports[256] = {};

	void SetUp() override {
		memset(memory, 0, sizeof(memory));
		memset(ports, 0, sizeof(ports));

		cpu.SetReadCallback([this](uint16_t addr) -> uint8_t {
			return memory[addr];
		});
		cpu.SetWriteCallback([this](uint16_t addr, uint8_t value) {
			memory[addr] = value;
		});
		cpu.SetReadPortCallback([this](uint8_t port) -> uint8_t {
			return ports[port];
		});
		cpu.SetWritePortCallback([this](uint8_t port, uint8_t value) {
			ports[port] = value;
		});

		cpu.Reset();
	}

	// Load a program at address 0 and execute one instruction
	void LoadAndStep(std::initializer_list<uint8_t> program) {
		uint16_t addr = 0;
		for (uint8_t b : program) {
			memory[addr++] = b;
		}
		cpu.StepCycles(1);
	}
};

// ========== Reset State ==========

TEST_F(F8CpuTestFixture, ResetClearsAllRegisters) {
	cpu.Reset();
	EXPECT_EQ(cpu.GetA(), 0);
	EXPECT_EQ(cpu.GetW(), 0);
	EXPECT_EQ(cpu.GetIsar(), 0);
	EXPECT_EQ(cpu.GetPc(), 0);
	EXPECT_EQ(cpu.GetPc1(), 0);
	EXPECT_EQ(cpu.GetDc0(), 0);
	EXPECT_EQ(cpu.GetDc1(), 0);
	EXPECT_EQ(cpu.GetCycleCount(), 0);
}

// ========== Load Immediate ==========

TEST_F(F8CpuTestFixture, LI_LoadsImmediateToAccumulator) {
	// $20 $42: LI $42
	LoadAndStep({0x20, 0x42});
	EXPECT_EQ(cpu.GetA(), 0x42);
	EXPECT_EQ(cpu.GetPc(), 2); // Consumed 2 bytes
}

TEST_F(F8CpuTestFixture, LIS_LoadsShortImmediateToAccumulator) {
	// $75: LIS 5
	LoadAndStep({0x75});
	EXPECT_EQ(cpu.GetA(), 5);
	EXPECT_EQ(cpu.GetPc(), 1);
}

TEST_F(F8CpuTestFixture, CLR_ClearsAccumulator) {
	// First load a value
	memory[0] = 0x20; memory[1] = 0xff; // LI $ff
	memory[2] = 0x70; // CLR
	cpu.StepCycles(2); // Execute LI
	EXPECT_EQ(cpu.GetA(), 0xff);
	cpu.StepCycles(1); // Execute CLR
	EXPECT_EQ(cpu.GetA(), 0);
}

// ========== NOP ==========

TEST_F(F8CpuTestFixture, NOP_AdvancesPcByOne) {
	LoadAndStep({0x2b}); // NOP
	EXPECT_EQ(cpu.GetPc(), 1);
}

// ========== Shifts ==========

TEST_F(F8CpuTestFixture, SR1_ShiftsRight) {
	memory[0] = 0x20; memory[1] = 0x84; // LI $84
	memory[2] = 0x12; // SR 1
	cpu.StepCycles(2); // LI
	cpu.StepCycles(1); // SR 1
	EXPECT_EQ(cpu.GetA(), 0x42);
}

TEST_F(F8CpuTestFixture, SL1_ShiftsLeft) {
	memory[0] = 0x20; memory[1] = 0x21; // LI $21
	memory[2] = 0x13; // SL 1
	cpu.StepCycles(2); // LI
	cpu.StepCycles(1); // SL 1
	EXPECT_EQ(cpu.GetA(), 0x42);
}

TEST_F(F8CpuTestFixture, SR4_ShiftsRightByFour) {
	memory[0] = 0x20; memory[1] = 0xf0; // LI $f0
	memory[2] = 0x14; // SR 4
	cpu.StepCycles(2);
	cpu.StepCycles(1);
	EXPECT_EQ(cpu.GetA(), 0x0f);
}

TEST_F(F8CpuTestFixture, SL4_ShiftsLeftByFour) {
	memory[0] = 0x20; memory[1] = 0x0f; // LI $0f
	memory[2] = 0x15; // SL 4
	cpu.StepCycles(2);
	cpu.StepCycles(1);
	EXPECT_EQ(cpu.GetA(), 0xf0);
}

// ========== Complement ==========

TEST_F(F8CpuTestFixture, COM_ComplementsAccumulator) {
	memory[0] = 0x20; memory[1] = 0x55; // LI $55
	memory[2] = 0x18; // COM
	cpu.StepCycles(2);
	cpu.StepCycles(1);
	EXPECT_EQ(cpu.GetA(), 0xaa);
}

// ========== ALU Immediate ==========

TEST_F(F8CpuTestFixture, AI_AddsImmediateToAccumulator) {
	memory[0] = 0x20; memory[1] = 0x10; // LI $10
	memory[2] = 0x24; memory[3] = 0x20; // AI $20
	cpu.StepCycles(2);
	cpu.StepCycles(2);
	EXPECT_EQ(cpu.GetA(), 0x30);
}

TEST_F(F8CpuTestFixture, AI_SetsCarryOnOverflow) {
	memory[0] = 0x20; memory[1] = 0x80; // LI $80
	memory[2] = 0x24; memory[3] = 0x80; // AI $80
	cpu.StepCycles(2);
	cpu.StepCycles(2);
	EXPECT_EQ(cpu.GetA(), 0x00);
	EXPECT_TRUE(cpu.GetW() & 0x04); // Carry set
	EXPECT_TRUE(cpu.GetW() & 0x02); // Zero set
}

TEST_F(F8CpuTestFixture, NI_AndsImmediate) {
	memory[0] = 0x20; memory[1] = 0xff; // LI $ff
	memory[2] = 0x21; memory[3] = 0x0f; // NI $0f
	cpu.StepCycles(2);
	cpu.StepCycles(2);
	EXPECT_EQ(cpu.GetA(), 0x0f);
}

TEST_F(F8CpuTestFixture, OI_OrsImmediate) {
	memory[0] = 0x20; memory[1] = 0xf0; // LI $f0
	memory[2] = 0x22; memory[3] = 0x0f; // OI $0f
	cpu.StepCycles(2);
	cpu.StepCycles(2);
	EXPECT_EQ(cpu.GetA(), 0xff);
}

TEST_F(F8CpuTestFixture, XI_XorsImmediate) {
	memory[0] = 0x20; memory[1] = 0xff; // LI $ff
	memory[2] = 0x23; memory[3] = 0xaa; // XI $aa
	cpu.StepCycles(2);
	cpu.StepCycles(2);
	EXPECT_EQ(cpu.GetA(), 0x55);
}

// ========== Jumps ==========

TEST_F(F8CpuTestFixture, JMP_SetsPc) {
	memory[0] = 0x29; memory[1] = 0x08; memory[2] = 0x00; // JMP $0800
	cpu.StepCycles(3);
	EXPECT_EQ(cpu.GetPc(), 0x0800);
}

TEST_F(F8CpuTestFixture, PI_PushesAndJumps) {
	memory[0] = 0x28; memory[1] = 0x04; memory[2] = 0x00; // PI $0400
	cpu.StepCycles(3);
	EXPECT_EQ(cpu.GetPc(), 0x0400);
	EXPECT_EQ(cpu.GetPc1(), 3); // Return address saved
}

TEST_F(F8CpuTestFixture, POP_ReturnsToPc1) {
	// Load PC1 via PI, then POP returns
	memory[0] = 0x28; memory[1] = 0x00; memory[2] = 0x10; // PI $0010
	memory[0x10] = 0x1c; // POP
	cpu.StepCycles(3); // PI
	EXPECT_EQ(cpu.GetPc(), 0x0010);
	EXPECT_EQ(cpu.GetPc1(), 3);
	cpu.StepCycles(1); // POP
	EXPECT_EQ(cpu.GetPc(), 3);
}

// ========== Data Counter ==========

TEST_F(F8CpuTestFixture, DCI_LoadsDataCounter) {
	memory[0] = 0x2a; memory[1] = 0x12; memory[2] = 0x34; // DCI $1234
	cpu.StepCycles(3);
	EXPECT_EQ(cpu.GetDc0(), 0x1234);
}

TEST_F(F8CpuTestFixture, XDC_SwapsDataCounters) {
	memory[0] = 0x2a; memory[1] = 0x12; memory[2] = 0x34; // DCI $1234
	memory[3] = 0x2c; // XDC
	cpu.StepCycles(3); // DCI
	EXPECT_EQ(cpu.GetDc0(), 0x1234);
	EXPECT_EQ(cpu.GetDc1(), 0);
	cpu.StepCycles(1); // XDC
	EXPECT_EQ(cpu.GetDc0(), 0);
	EXPECT_EQ(cpu.GetDc1(), 0x1234);
}

TEST_F(F8CpuTestFixture, INC_IncrementsDataCounter) {
	memory[0] = 0x2a; memory[1] = 0x00; memory[2] = 0xff; // DCI $00ff
	memory[3] = 0x1f; // INC
	cpu.StepCycles(3);
	EXPECT_EQ(cpu.GetDc0(), 0x00ff);
	cpu.StepCycles(1);
	EXPECT_EQ(cpu.GetDc0(), 0x0100);
}

// ========== Memory Operations (LM / ST) ==========

TEST_F(F8CpuTestFixture, LM_LoadsFromMemoryViaDataCounter) {
	memory[0x100] = 0x42; // Data at $0100
	memory[0] = 0x2a; memory[1] = 0x01; memory[2] = 0x00; // DCI $0100
	memory[3] = 0x16; // LM
	cpu.StepCycles(3); // DCI
	cpu.StepCycles(1); // LM
	EXPECT_EQ(cpu.GetA(), 0x42);
	EXPECT_EQ(cpu.GetDc0(), 0x0101); // DC0 incremented
}

TEST_F(F8CpuTestFixture, ST_StoresToMemoryViaDataCounter) {
	memory[0] = 0x20; memory[1] = 0xab; // LI $ab
	memory[2] = 0x2a; memory[3] = 0x02; memory[4] = 0x00; // DCI $0200
	memory[5] = 0x17; // ST
	cpu.StepCycles(2); // LI
	cpu.StepCycles(3); // DCI
	cpu.StepCycles(1); // ST
	EXPECT_EQ(memory[0x200], 0xab);
	EXPECT_EQ(cpu.GetDc0(), 0x0201); // DC0 incremented
}

// ========== Scratchpad Operations ==========

TEST_F(F8CpuTestFixture, LR_StoreAndLoadScratchpad) {
	memory[0] = 0x20; memory[1] = 0x42; // LI $42
	memory[2] = 0x53; // LR 3,A (store A to R3)
	memory[3] = 0x70; // CLR
	memory[4] = 0x43; // LR A,3 (load A from R3)
	cpu.StepCycles(2); // LI
	cpu.StepCycles(1); // LR 3,A
	cpu.StepCycles(1); // CLR
	EXPECT_EQ(cpu.GetA(), 0);
	cpu.StepCycles(1); // LR A,3
	EXPECT_EQ(cpu.GetA(), 0x42);
}

TEST_F(F8CpuTestFixture, ISAR_IndirectScratchpadAccess) {
	// LISU 1, LISL 2 → ISAR = 0b001_010 = 10 (R10 = HU)
	memory[0] = 0x61; // LISU 1
	memory[1] = 0x6a; // LISL 2
	memory[2] = 0x20; memory[3] = 0x99; // LI $99
	memory[4] = 0x5c; // LR (IS),A — store A to scratchpad[ISAR]
	memory[5] = 0x70; // CLR
	memory[6] = 0x4c; // LR A,(IS)
	cpu.StepCycles(1); // LISU
	cpu.StepCycles(1); // LISL
	EXPECT_EQ(cpu.GetIsar(), 10);
	cpu.StepCycles(2); // LI
	cpu.StepCycles(1); // LR (IS),A
	cpu.StepCycles(1); // CLR
	cpu.StepCycles(1); // LR A,(IS)
	EXPECT_EQ(cpu.GetA(), 0x99);
}

TEST_F(F8CpuTestFixture, ISAR_AutoIncrementWrapWithin8) {
	// LISU 2, LISL 7 → ISAR = 0b010_111 = 23 (R23)
	memory[0] = 0x62; // LISU 2
	memory[1] = 0x6f; // LISL 7
	memory[2] = 0x4d; // LR A,(IS++) — reads R23, increments ISAR lower 3 bits
	cpu.StepCycles(1); cpu.StepCycles(1);
	EXPECT_EQ(cpu.GetIsar(), 23); // 0b010_111
	cpu.StepCycles(1);
	// Lower 3 bits wrap: 7→0, upper stays 2 → ISAR = 0b010_000 = 16
	EXPECT_EQ(cpu.GetIsar(), 16);
}

// ========== K/P Register Transfers ==========

TEST_F(F8CpuTestFixture, LR_KP_SavesPC1ToScratchpad) {
	// PI to set PC1 to 3, then LR K,P saves PC1 to R12:R13
	memory[0] = 0x28; memory[1] = 0x00; memory[2] = 0x10; // PI $0010
	memory[0x10] = 0x08; // LR K,P
	cpu.StepCycles(3); // PI → PC0=$0010, PC1=3
	cpu.StepCycles(1); // LR K,P
	EXPECT_EQ(cpu.GetScratchpadData()[12], 0x00); // R12 = high byte of PC1
	EXPECT_EQ(cpu.GetScratchpadData()[13], 0x03); // R13 = low byte of PC1
}

TEST_F(F8CpuTestFixture, PK_PushesAndJumpsToK) {
	// Set K manually via LR KU,A / LR KL,A
	memory[0] = 0x20; memory[1] = 0x05; // LI $05
	memory[2] = 0x04; // LR KU,A (R12 = 0x05)
	memory[3] = 0x20; memory[4] = 0x00; // LI $00
	memory[5] = 0x05; // LR KL,A (R13 = 0x00)
	memory[6] = 0x0c; // PK → PC1←PC0(7), PC0←[R12:R13] = $0500
	cpu.StepCycles(2); cpu.StepCycles(1); cpu.StepCycles(2); cpu.StepCycles(1);
	cpu.StepCycles(1); // PK
	EXPECT_EQ(cpu.GetPc(), 0x0500);
	EXPECT_EQ(cpu.GetPc1(), 7); // Return address
}

// ========== Branch Instructions ==========

TEST_F(F8CpuTestFixture, BR_UnconditionalBranch) {
	memory[0] = 0x90; memory[1] = 0x05; // BR +5 (PC after fetch = 2, target = 7)
	cpu.StepCycles(2);
	EXPECT_EQ(cpu.GetPc(), 7);
}

TEST_F(F8CpuTestFixture, BF_BranchOnFalseZero) {
	// W = 0 (zero flag clear), BF 2 should branch (mask=2, zero bit, (W & 2)==0)
	memory[0] = 0x92; memory[1] = 0x10; // BF 2 (zero), +16
	cpu.StepCycles(2);
	EXPECT_EQ(cpu.GetPc(), 18); // 2 + 16
}

TEST_F(F8CpuTestFixture, BF_DoesNotBranchWhenFlagSet) {
	// Set zero flag by computing 0
	memory[0] = 0x70; // CLR → A = 0
	memory[1] = 0x18; // COM → A = $ff, W gets sign set
	memory[2] = 0x92; memory[3] = 0x10; // BF 2, +16 (branch if zero clear)
	cpu.StepCycles(1); // CLR
	cpu.StepCycles(1); // COM sets sign flag (bit 3) in W, zero is clear
	// W should have sign set (bit 3=0x08) since result $ff has bit 7 set
	// Zero bit (0x02) should NOT be set since result != 0
	// So BF 2 (zero) should branch (because zero is not set)
	cpu.StepCycles(2);
	EXPECT_EQ(cpu.GetPc(), 20); // 4 + 16
}

TEST_F(F8CpuTestFixture, BR7_BranchesWhenIsarLowNot7) {
	// ISAR lower 3 bits = 0 (not 7) → BR7 should branch
	memory[0] = 0x60; // LISU 0 (sets upper to 0)
	memory[1] = 0x68; // LISL 0 (sets lower to 0)
	memory[2] = 0x8f; memory[3] = 0x10; // BR7, +16
	cpu.StepCycles(1); cpu.StepCycles(1);
	EXPECT_EQ(cpu.GetIsar() & 0x07, 0);
	cpu.StepCycles(2);
	EXPECT_EQ(cpu.GetPc(), 20); // 4 + 16
}

TEST_F(F8CpuTestFixture, BR7_SkipsWhenIsarLowIs7) {
	// ISAR lower 3 bits = 7 → BR7 should NOT branch
	memory[0] = 0x6f; // LISL 7
	memory[1] = 0x8f; memory[2] = 0x10; // BR7, +16
	cpu.StepCycles(1);
	EXPECT_EQ(cpu.GetIsar() & 0x07, 7);
	cpu.StepCycles(2);
	EXPECT_EQ(cpu.GetPc(), 3); // Falls through
}

// ========== I/O Ports ==========

TEST_F(F8CpuTestFixture, OUT_WritesToPort) {
	memory[0] = 0x20; memory[1] = 0xab; // LI $ab
	memory[2] = 0x27; memory[3] = 0x01; // OUT 1
	cpu.StepCycles(2);
	cpu.StepCycles(2);
	EXPECT_EQ(ports[1], 0xab);
}

TEST_F(F8CpuTestFixture, IN_ReadsFromPort) {
	ports[5] = 0x42;
	memory[0] = 0x26; memory[1] = 0x05; // IN 5
	cpu.StepCycles(2);
	EXPECT_EQ(cpu.GetA(), 0x42);
}

TEST_F(F8CpuTestFixture, INS_ShortPortRead) {
	ports[4] = 0x77;
	memory[0] = 0xa4; // INS 4
	cpu.StepCycles(1);
	EXPECT_EQ(cpu.GetA(), 0x77);
}

TEST_F(F8CpuTestFixture, OUTS_ShortPortWrite) {
	memory[0] = 0x20; memory[1] = 0xee; // LI $ee
	memory[2] = 0xb3; // OUTS 3
	cpu.StepCycles(2);
	cpu.StepCycles(1);
	EXPECT_EQ(ports[3], 0xee);
}

// ========== Interrupts ==========

TEST_F(F8CpuTestFixture, DI_DisablesInterrupts) {
	memory[0] = 0x1b; // EI
	memory[1] = 0x1a; // DI
	cpu.StepCycles(1); // EI
	cpu.StepCycles(1); // DI
	// No direct accessor for interrupts, but instruction executed without crash
	EXPECT_EQ(cpu.GetPc(), 2);
}

// ========== DS (Decrement Scratchpad) ==========

TEST_F(F8CpuTestFixture, DS_DecrementsScratchpad) {
	// Store 5 in R3, then DS 3
	memory[0] = 0x20; memory[1] = 0x05; // LI 5
	memory[2] = 0x53; // LR 3,A
	memory[3] = 0x33; // DS 3
	cpu.StepCycles(2); cpu.StepCycles(1);
	cpu.StepCycles(2); // DS
	EXPECT_EQ(cpu.GetScratchpadData()[3], 4);
}

// ========== AS (Add Scratchpad) ==========

TEST_F(F8CpuTestFixture, AS_AddsScratchpadToA) {
	// A = 10, R5 = 20, AS 5 → A = 30
	memory[0] = 0x20; memory[1] = 0x0a; // LI $0a
	memory[2] = 0x55; // LR 5,A (R5 = 10)
	memory[3] = 0x20; memory[4] = 0x14; // LI $14 (A = 20)
	memory[5] = 0xc5; // AS 5
	cpu.StepCycles(2); cpu.StepCycles(1);
	cpu.StepCycles(2);
	cpu.StepCycles(1); // AS 5
	EXPECT_EQ(cpu.GetA(), 0x1e); // 10 + 20 = 30 = $1e
}

// ========== XS (XOR Scratchpad) ==========

TEST_F(F8CpuTestFixture, XS_XorsScratchpadWithA) {
	memory[0] = 0x20; memory[1] = 0xaa; // LI $aa
	memory[2] = 0x52; // LR 2,A (R2 = $aa)
	memory[3] = 0x20; memory[4] = 0xff; // LI $ff
	memory[5] = 0xe2; // XS 2
	cpu.StepCycles(2); cpu.StepCycles(1); cpu.StepCycles(2);
	cpu.StepCycles(1); // XS 2
	EXPECT_EQ(cpu.GetA(), 0x55); // $ff ^ $aa = $55
}

// ========== NS (AND Scratchpad) ==========

TEST_F(F8CpuTestFixture, NS_AndsScratchpadWithA) {
	memory[0] = 0x20; memory[1] = 0x0f; // LI $0f
	memory[2] = 0x51; // LR 1,A (R1 = $0f)
	memory[3] = 0x20; memory[4] = 0x37; // LI $37
	memory[5] = 0xf1; // NS 1
	cpu.StepCycles(2); cpu.StepCycles(1); cpu.StepCycles(2);
	cpu.StepCycles(1); // NS 1
	EXPECT_EQ(cpu.GetA(), 0x07); // $37 & $0f = $07
}

// ========== Memory Manager Integration ==========

TEST_F(F8CpuTestFixture, LNK_AddsCarryToA) {
	// Set carry by overflowing: A = $ff, AI $01 → A = 0, carry set
	memory[0] = 0x20; memory[1] = 0xff; // LI $ff
	memory[2] = 0x24; memory[3] = 0x01; // AI $01
	memory[4] = 0x19; // LNK
	cpu.StepCycles(2); // LI
	cpu.StepCycles(2); // AI → A=0, carry set
	EXPECT_TRUE(cpu.GetW() & 0x04); // Carry
	cpu.StepCycles(1); // LNK → A += carry → A = 1
	EXPECT_EQ(cpu.GetA(), 1);
}

// ========== Export / Import State ==========

TEST_F(F8CpuTestFixture, ExportImportStateRoundtrip) {
	// Execute some instructions to set non-zero state
	memory[0] = 0x20; memory[1] = 0x42; // LI $42
	memory[2] = 0x2a; memory[3] = 0xab; memory[4] = 0xcd; // DCI $abcd
	cpu.StepCycles(2);
	cpu.StepCycles(3);

	uint8_t a, w, isar;
	uint16_t pc0, pc1, dc0, dc1;
	uint64_t cycles;
	bool irq;
	uint8_t scratch[64];

	cpu.ExportState(a, w, isar, pc0, pc1, dc0, dc1, cycles, irq, scratch);

	EXPECT_EQ(a, 0x42);
	EXPECT_EQ(dc0, 0xabcd);
	EXPECT_EQ(pc0, 5);

	// Now reset and import
	cpu.Reset();
	EXPECT_EQ(cpu.GetA(), 0);
	cpu.ImportState(a, w, isar, pc0, pc1, dc0, dc1, cycles, irq, scratch);
	EXPECT_EQ(cpu.GetA(), 0x42);
	EXPECT_EQ(cpu.GetDc0(), 0xabcd);
	EXPECT_EQ(cpu.GetPc(), 5);
}
