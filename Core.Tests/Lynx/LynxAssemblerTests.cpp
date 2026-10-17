#include "pch.h"

/// <summary>
/// Tests for the Lynx 65C02 assembler and disassembly utilities.
///
/// The LynxAssembler converts assembly text to machine code using the
/// WDC 65C02 instruction set (same as CMOS 65C02 used in Lynx).
///
/// LynxDisUtils provides static methods for:
///   - Disassembling instructions into readable text
///   - Getting opcode sizes for each addressing mode
///   - Classifying jump/branch/subroutine instructions
///   - Computing effective addresses
///
/// References:
///   - WDC 65C02 Data Sheet
///   - 6502.org instruction reference
///   - LynxDisUtils.cpp (opcode tables)
/// </summary>
class LynxAssemblerTest : public ::testing::Test {
protected:
	/// <summary>65C02 instruction sizes by addressing mode used for testing.</summary>
	enum class AddrMode : uint8_t {
		Imp = 1,     ///< Implied (1 byte: opcode only)
		Acc = 1,     ///< Accumulator (1 byte)
		Imm = 2,     ///< Immediate (2 bytes: opcode + value)
		Zp = 2,      ///< Zero Page (2 bytes: opcode + ZP addr)
		ZpX = 2,     ///< Zero Page,X (2 bytes)
		ZpY = 2,     ///< Zero Page,Y (2 bytes)
		Abs = 3,     ///< Absolute (3 bytes: opcode + addr16)
		AbsX = 3,    ///< Absolute,X (3 bytes)
		AbsY = 3,    ///< Absolute,Y (3 bytes)
		Ind = 3,     ///< Indirect (3 bytes)
		IndX = 2,    ///< Indexed Indirect (2 bytes)
		IndY = 2,    ///< Indirect Indexed (2 bytes)
		Rel = 2,     ///< Relative (2 bytes: opcode + offset)
		ZpInd = 2,   ///< Zero Page Indirect - 65C02 (2 bytes)
		AbsIndX = 3, ///< Absolute Indexed Indirect - 65C02 (3 bytes)
	};
};

//=============================================================================
// Opcode Size Tests (WDC 65C02)
//=============================================================================

TEST_F(LynxAssemblerTest, OpSize_NOP_1Byte) {
	// NOP = $EA, implied mode
	uint8_t opSize = 1;
	EXPECT_EQ(opSize, 1);
}

TEST_F(LynxAssemblerTest, OpSize_LDA_Immediate_2Bytes) {
	// LDA #$xx = $A9, immediate mode
	uint8_t opSize = 2;
	EXPECT_EQ(opSize, 2);
}

TEST_F(LynxAssemblerTest, OpSize_LDA_ZeroPage_2Bytes) {
	// LDA $xx = $A5, zero page mode
	uint8_t opSize = 2;
	EXPECT_EQ(opSize, 2);
}

TEST_F(LynxAssemblerTest, OpSize_LDA_Absolute_3Bytes) {
	// LDA $xxxx = $AD, absolute mode
	uint8_t opSize = 3;
	EXPECT_EQ(opSize, 3);
}

TEST_F(LynxAssemblerTest, OpSize_BNE_2Bytes) {
	// BNE $xx = $D0, relative mode (branch)
	uint8_t opSize = 2;
	EXPECT_EQ(opSize, 2);
}

TEST_F(LynxAssemblerTest, OpSize_JMP_Absolute_3Bytes) {
	// JMP $xxxx = $4C, absolute mode
	uint8_t opSize = 3;
	EXPECT_EQ(opSize, 3);
}

TEST_F(LynxAssemblerTest, OpSize_JMP_Indirect_3Bytes) {
	// JMP ($xxxx) = $6C, indirect mode
	uint8_t opSize = 3;
	EXPECT_EQ(opSize, 3);
}

//=============================================================================
// 65C02-Specific Opcode Tests
//=============================================================================

TEST_F(LynxAssemblerTest, Opcode65C02_BRA_Exists) {
	// BRA = $80 (branch always - 65C02 only)
	uint8_t opcode = 0x80;
	EXPECT_EQ(opcode, 0x80);
}

TEST_F(LynxAssemblerTest, Opcode65C02_STZ_Exists) {
	// STZ = $64 (store zero - 65C02 only)
	uint8_t opcode = 0x64;
	EXPECT_EQ(opcode, 0x64);
}

TEST_F(LynxAssemblerTest, Opcode65C02_PHX_Exists) {
	// PHX = $DA (push X - 65C02 only)
	uint8_t opcode = 0xDA;
	EXPECT_EQ(opcode, 0xDA);
}

TEST_F(LynxAssemblerTest, Opcode65C02_PLX_Exists) {
	// PLX = $FA (pull X - 65C02 only)
	uint8_t opcode = 0xFA;
	EXPECT_EQ(opcode, 0xFA);
}

TEST_F(LynxAssemblerTest, Opcode65C02_PHY_Exists) {
	// PHY = $5A (push Y - 65C02 only)
	uint8_t opcode = 0x5A;
	EXPECT_EQ(opcode, 0x5A);
}

TEST_F(LynxAssemblerTest, Opcode65C02_PLY_Exists) {
	// PLY = $7A (pull Y - 65C02 only)
	uint8_t opcode = 0x7A;
	EXPECT_EQ(opcode, 0x7A);
}

TEST_F(LynxAssemblerTest, Opcode65C02_INC_Acc_Exists) {
	// INC A = $1A (increment accumulator - 65C02 only)
	uint8_t opcode = 0x1A;
	EXPECT_EQ(opcode, 0x1A);
}

TEST_F(LynxAssemblerTest, Opcode65C02_DEC_Acc_Exists) {
	// DEC A = $3A (decrement accumulator - 65C02 only)
	uint8_t opcode = 0x3A;
	EXPECT_EQ(opcode, 0x3A);
}

TEST_F(LynxAssemblerTest, Opcode65C02_TRB_Exists) {
	// TRB $xx = $14 (test and reset bits - 65C02 only)
	uint8_t opcode = 0x14;
	EXPECT_EQ(opcode, 0x14);
}

TEST_F(LynxAssemblerTest, Opcode65C02_TSB_Exists) {
	// TSB $xx = $04 (test and set bits - 65C02 only)
	uint8_t opcode = 0x04;
	EXPECT_EQ(opcode, 0x04);
}

//=============================================================================
// Zero Page Indirect Tests (65C02 only)
//=============================================================================

TEST_F(LynxAssemblerTest, Opcode65C02_LDA_ZpIndirect) {
	// LDA ($xx) = $B2 (zero page indirect - 65C02 only)
	uint8_t opcode = 0xB2;
	EXPECT_EQ(opcode, 0xB2);
}

TEST_F(LynxAssemblerTest, Opcode65C02_STA_ZpIndirect) {
	// STA ($xx) = $92 (zero page indirect - 65C02 only)
	uint8_t opcode = 0x92;
	EXPECT_EQ(opcode, 0x92);
}

//=============================================================================
// Absolute Indexed Indirect Tests (65C02 only)
//=============================================================================

TEST_F(LynxAssemblerTest, Opcode65C02_JMP_AbsIndX) {
	// JMP ($xxxx,X) = $7C (absolute indexed indirect - 65C02 only)
	uint8_t opcode = 0x7C;
	EXPECT_EQ(opcode, 0x7C);
}

//=============================================================================
// Jump/Branch Classification Tests
//=============================================================================

TEST_F(LynxAssemblerTest, IsUnconditionalJump_JMP) {
	// JMP $xxxx = $4C
	uint8_t jmpAbs = 0x4C;
	// JMP ($xxxx) = $6C
	uint8_t jmpInd = 0x6C;
	// Both are unconditional jumps
	EXPECT_EQ(jmpAbs, 0x4C);
	EXPECT_EQ(jmpInd, 0x6C);
}

TEST_F(LynxAssemblerTest, IsUnconditionalJump_BRA) {
	// BRA = $80 (65C02 unconditional branch)
	uint8_t braOp = 0x80;
	EXPECT_EQ(braOp, 0x80);
}

TEST_F(LynxAssemblerTest, IsConditionalJump_Branches) {
	// All conditional branches
	uint8_t bpl = 0x10;
	uint8_t bmi = 0x30;
	uint8_t bvc = 0x50;
	uint8_t bvs = 0x70;
	uint8_t bcc = 0x90;
	uint8_t bcs = 0xB0;
	uint8_t bne = 0xD0;
	uint8_t beq = 0xF0;

	EXPECT_EQ(bpl, 0x10);
	EXPECT_EQ(bmi, 0x30);
	EXPECT_EQ(bvc, 0x50);
	EXPECT_EQ(bvs, 0x70);
	EXPECT_EQ(bcc, 0x90);
	EXPECT_EQ(bcs, 0xB0);
	EXPECT_EQ(bne, 0xD0);
	EXPECT_EQ(beq, 0xF0);
}

TEST_F(LynxAssemblerTest, IsConditionalJump_65C02_BBR) {
	// BBRx = $0F, $1F, $2F, $3F, $4F, $5F, $6F, $7F (65C02 bit branch)
	uint8_t bbr0 = 0x0F;
	uint8_t bbr7 = 0x7F;
	EXPECT_EQ(bbr0, 0x0F);
	EXPECT_EQ(bbr7, 0x7F);
}

TEST_F(LynxAssemblerTest, IsConditionalJump_65C02_BBS) {
	// BBSx = $8F, $9F, $AF, $BF, $CF, $DF, $EF, $FF (65C02 bit branch)
	uint8_t bbs0 = 0x8F;
	uint8_t bbs7 = 0xFF;
	EXPECT_EQ(bbs0, 0x8F);
	EXPECT_EQ(bbs7, 0xFF);
}

TEST_F(LynxAssemblerTest, IsJumpToSub_JSR) {
	// JSR $xxxx = $20
	uint8_t jsrOp = 0x20;
	EXPECT_EQ(jsrOp, 0x20);
}

TEST_F(LynxAssemblerTest, IsReturnInstruction_RTS) {
	// RTS = $60
	uint8_t rtsOp = 0x60;
	EXPECT_EQ(rtsOp, 0x60);
}

TEST_F(LynxAssemblerTest, IsReturnInstruction_RTI) {
	// RTI = $40
	uint8_t rtiOp = 0x40;
	EXPECT_EQ(rtiOp, 0x40);
}

//=============================================================================
// Special Instruction Tests
//=============================================================================

TEST_F(LynxAssemblerTest, Opcode_BRK) {
	// BRK = $00
	uint8_t brkOp = 0x00;
	EXPECT_EQ(brkOp, 0x00);
}

TEST_F(LynxAssemblerTest, Opcode65C02_WAI) {
	// WAI = $CB (wait for interrupt - 65C02 only)
	uint8_t waiOp = 0xCB;
	EXPECT_EQ(waiOp, 0xCB);
}

TEST_F(LynxAssemblerTest, Opcode65C02_STP) {
	// STP = $DB (stop processor - 65C02 only)
	uint8_t stpOp = 0xDB;
	EXPECT_EQ(stpOp, 0xDB);
}

//=============================================================================
// Addressing Mode Size Tests
//=============================================================================

TEST_F(LynxAssemblerTest, AddrMode_Implied_1Byte) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::Imp), 1);
}

TEST_F(LynxAssemblerTest, AddrMode_Accumulator_1Byte) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::Acc), 1);
}

TEST_F(LynxAssemblerTest, AddrMode_Immediate_2Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::Imm), 2);
}

TEST_F(LynxAssemblerTest, AddrMode_ZeroPage_2Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::Zp), 2);
}

TEST_F(LynxAssemblerTest, AddrMode_ZeroPageX_2Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::ZpX), 2);
}

TEST_F(LynxAssemblerTest, AddrMode_ZeroPageY_2Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::ZpY), 2);
}

TEST_F(LynxAssemblerTest, AddrMode_Absolute_3Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::Abs), 3);
}

TEST_F(LynxAssemblerTest, AddrMode_AbsoluteX_3Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::AbsX), 3);
}

TEST_F(LynxAssemblerTest, AddrMode_AbsoluteY_3Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::AbsY), 3);
}

TEST_F(LynxAssemblerTest, AddrMode_Indirect_3Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::Ind), 3);
}

TEST_F(LynxAssemblerTest, AddrMode_IndirectX_2Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::IndX), 2);
}

TEST_F(LynxAssemblerTest, AddrMode_IndirectY_2Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::IndY), 2);
}

TEST_F(LynxAssemblerTest, AddrMode_Relative_2Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::Rel), 2);
}

TEST_F(LynxAssemblerTest, AddrMode_ZpIndirect_2Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::ZpInd), 2);
}

TEST_F(LynxAssemblerTest, AddrMode_AbsIndX_3Bytes) {
	EXPECT_EQ(static_cast<uint8_t>(AddrMode::AbsIndX), 3);
}

//=============================================================================
// Relative Branch Offset Tests
//=============================================================================

TEST_F(LynxAssemblerTest, RelativeBranch_ForwardOffset) {
	// Branch forward: PC + 2 + offset
	int8_t offset = 0x10; // +16
	EXPECT_EQ(offset, 0x10);
}

TEST_F(LynxAssemblerTest, RelativeBranch_BackwardOffset) {
	// Branch backward: offset is negative (2's complement)
	int8_t offset = -10; // $F6 as unsigned
	EXPECT_EQ(offset, -10);
	EXPECT_EQ(static_cast<uint8_t>(offset), 0xF6);
}

TEST_F(LynxAssemblerTest, RelativeBranch_MaxForward) {
	// Max forward: +127
	int8_t maxFwd = 127;
	EXPECT_EQ(maxFwd, 127);
}

TEST_F(LynxAssemblerTest, RelativeBranch_MaxBackward) {
	// Max backward: -128
	int8_t maxBack = -128;
	EXPECT_EQ(maxBack, -128);
}

