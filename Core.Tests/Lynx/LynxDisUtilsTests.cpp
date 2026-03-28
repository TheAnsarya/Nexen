#include "pch.h"
#include "Lynx/Debugger/LynxDisUtils.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Tests for LynxDisUtils — the Lynx (65C02) disassembly utility functions.
///
/// Tests verify:
///   - GetOpSize: correct byte counts for all addressing modes
///   - IsUnconditionalJump: BRK/JSR/RTI/JMP/RTS/JMP(ind)/JMP(abs,X)/BRA
///   - IsConditionalJump: BPL/BMI/BVC/BVS/BCC/BCS/BNE/BEQ
///   - IsJumpToSub: BRK/JSR
///   - IsReturnInstruction: RTI/RTS
///   - GetOpName: opcode mnemonic lookup
///
/// References:
///   - LynxDisUtils.h/.cpp
///   - WDC 65C02 instruction set
/// </summary>

// =============================================================================
// Addressing Mode → OpSize Tests
// =============================================================================

class LynxDisUtilsOpSizeTest : public ::testing::Test {};

// Single-byte instructions (implied, accumulator, none)
TEST_F(LynxDisUtilsOpSizeTest, Implied_NOP_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xea), 1); // NOP
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_CLC_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x18), 1); // CLC
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_SEC_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x38), 1); // SEC
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_CLI_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x58), 1); // CLI
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_SEI_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x78), 1); // SEI
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_CLV_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xb8), 1); // CLV
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_CLD_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xd8), 1); // CLD
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_SED_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xf8), 1); // SED
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_TAX_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xaa), 1); // TAX
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_TAY_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xa8), 1); // TAY
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_TXA_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x8a), 1); // TXA
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_TYA_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x98), 1); // TYA
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_TXS_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x9a), 1); // TXS
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_TSX_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xba), 1); // TSX
}

TEST_F(LynxDisUtilsOpSizeTest, Accumulator_ASL_A_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x0a), 1); // ASL A
}

TEST_F(LynxDisUtilsOpSizeTest, Accumulator_ROL_A_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x2a), 1); // ROL A
}

TEST_F(LynxDisUtilsOpSizeTest, Accumulator_LSR_A_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x4a), 1); // LSR A
}

TEST_F(LynxDisUtilsOpSizeTest, Accumulator_ROR_A_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x6a), 1); // ROR A
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_INX_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xe8), 1); // INX
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_DEX_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xca), 1); // DEX
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_INY_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xc8), 1); // INY
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_DEY_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x88), 1); // DEY
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_PHA_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x48), 1); // PHA
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_PLA_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x68), 1); // PLA
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_PHP_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x08), 1); // PHP
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_PLP_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x28), 1); // PLP
}

// 65C02 extensions — single-byte
TEST_F(LynxDisUtilsOpSizeTest, Implied_PHX_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xda), 1); // PHX (65C02)
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_PHY_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x5a), 1); // PHY (65C02)
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_PLX_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xfa), 1); // PLX (65C02)
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_PLY_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x7a), 1); // PLY (65C02)
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_INC_A_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x1a), 1); // INC A (65C02)
}

TEST_F(LynxDisUtilsOpSizeTest, Implied_DEC_A_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x3a), 1); // DEC A (65C02)
}

// 65C02: WAI, STP, BRK, RTI, RTS are single-byte
TEST_F(LynxDisUtilsOpSizeTest, BRK_Size1) {
	// BRK on the 65C02 is 1-byte (technically 2 but size=1 for disassembly)
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x00), 1);
}

TEST_F(LynxDisUtilsOpSizeTest, RTI_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x40), 1); // RTI
}

TEST_F(LynxDisUtilsOpSizeTest, RTS_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x60), 1); // RTS
}

TEST_F(LynxDisUtilsOpSizeTest, WAI_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xcb), 1); // WAI (65C02)
}

TEST_F(LynxDisUtilsOpSizeTest, STP_Size1) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xdb), 1); // STP (65C02)
}

// Two-byte instructions (immediate, zero-page, relative, etc.)
TEST_F(LynxDisUtilsOpSizeTest, Immediate_LDA_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xa9), 2); // LDA #imm
}

TEST_F(LynxDisUtilsOpSizeTest, Immediate_LDX_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xa2), 2); // LDX #imm
}

TEST_F(LynxDisUtilsOpSizeTest, Immediate_LDY_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xa0), 2); // LDY #imm
}

TEST_F(LynxDisUtilsOpSizeTest, Immediate_CMP_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xc9), 2); // CMP #imm
}

TEST_F(LynxDisUtilsOpSizeTest, Immediate_CPX_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xe0), 2); // CPX #imm
}

TEST_F(LynxDisUtilsOpSizeTest, Immediate_CPY_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xc0), 2); // CPY #imm
}

TEST_F(LynxDisUtilsOpSizeTest, ZeroPage_LDA_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xa5), 2); // LDA zp
}

TEST_F(LynxDisUtilsOpSizeTest, ZeroPage_STA_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x85), 2); // STA zp
}

TEST_F(LynxDisUtilsOpSizeTest, ZeroPageX_LDA_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xb5), 2); // LDA zp,X
}

TEST_F(LynxDisUtilsOpSizeTest, ZeroPageY_LDX_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xb6), 2); // LDX zp,Y
}

TEST_F(LynxDisUtilsOpSizeTest, IndX_LDA_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xa1), 2); // LDA (zp,X)
}

TEST_F(LynxDisUtilsOpSizeTest, IndY_LDA_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xb1), 2); // LDA (zp),Y
}

TEST_F(LynxDisUtilsOpSizeTest, ZpgInd_65C02_LDA_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xb2), 2); // LDA (zp) — 65C02
}

TEST_F(LynxDisUtilsOpSizeTest, Relative_BEQ_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xf0), 2); // BEQ rel
}

TEST_F(LynxDisUtilsOpSizeTest, Relative_BNE_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xd0), 2); // BNE rel
}

TEST_F(LynxDisUtilsOpSizeTest, Relative_BRA_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x80), 2); // BRA rel (65C02)
}

TEST_F(LynxDisUtilsOpSizeTest, Immediate_BIT_65C02_Size2) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x89), 2); // BIT #imm (65C02)
}

// Three-byte instructions (absolute, absolute,X, absolute,Y, etc.)
TEST_F(LynxDisUtilsOpSizeTest, Absolute_LDA_Size3) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xad), 3); // LDA abs
}

TEST_F(LynxDisUtilsOpSizeTest, Absolute_STA_Size3) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x8d), 3); // STA abs
}

TEST_F(LynxDisUtilsOpSizeTest, AbsoluteX_LDA_Size3) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xbd), 3); // LDA abs,X
}

TEST_F(LynxDisUtilsOpSizeTest, AbsoluteY_LDA_Size3) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0xb9), 3); // LDA abs,Y
}

TEST_F(LynxDisUtilsOpSizeTest, Absolute_JMP_Size3) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x4c), 3); // JMP abs
}

TEST_F(LynxDisUtilsOpSizeTest, Absolute_JSR_Size3) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x20), 3); // JSR abs
}

TEST_F(LynxDisUtilsOpSizeTest, Indirect_JMP_Size3) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x6c), 3); // JMP (abs)
}

TEST_F(LynxDisUtilsOpSizeTest, AbsIndX_JMP_65C02_Size3) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x7c), 3); // JMP (abs,X) — 65C02
}

TEST_F(LynxDisUtilsOpSizeTest, Absolute_STZ_65C02_Size3) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x9c), 3); // STZ abs (65C02)
}

TEST_F(LynxDisUtilsOpSizeTest, Absolute_BIT_Size3) {
	EXPECT_EQ(LynxDisUtils::GetOpSize(0x2c), 3); // BIT abs
}

// =============================================================================
// IsUnconditionalJump Tests
// =============================================================================

class LynxDisUtilsUnconditionalJumpTest : public ::testing::Test {};

TEST_F(LynxDisUtilsUnconditionalJumpTest, JSR_0x20) {
	EXPECT_TRUE(LynxDisUtils::IsUnconditionalJump(0x20));
}

TEST_F(LynxDisUtilsUnconditionalJumpTest, RTI_0x40) {
	EXPECT_TRUE(LynxDisUtils::IsUnconditionalJump(0x40));
}

TEST_F(LynxDisUtilsUnconditionalJumpTest, JMP_abs_0x4c) {
	EXPECT_TRUE(LynxDisUtils::IsUnconditionalJump(0x4c));
}

TEST_F(LynxDisUtilsUnconditionalJumpTest, RTS_0x60) {
	EXPECT_TRUE(LynxDisUtils::IsUnconditionalJump(0x60));
}

TEST_F(LynxDisUtilsUnconditionalJumpTest, JMP_ind_0x6c) {
	EXPECT_TRUE(LynxDisUtils::IsUnconditionalJump(0x6c));
}

TEST_F(LynxDisUtilsUnconditionalJumpTest, JMP_absX_0x7c) {
	EXPECT_TRUE(LynxDisUtils::IsUnconditionalJump(0x7c));
}

TEST_F(LynxDisUtilsUnconditionalJumpTest, BRA_0x80) {
	EXPECT_TRUE(LynxDisUtils::IsUnconditionalJump(0x80));
}

TEST_F(LynxDisUtilsUnconditionalJumpTest, NOP_NotUnconditional) {
	EXPECT_FALSE(LynxDisUtils::IsUnconditionalJump(0xea));
}

TEST_F(LynxDisUtilsUnconditionalJumpTest, LDA_imm_NotUnconditional) {
	EXPECT_FALSE(LynxDisUtils::IsUnconditionalJump(0xa9));
}

TEST_F(LynxDisUtilsUnconditionalJumpTest, BEQ_NotUnconditional) {
	// Conditional branches are NOT unconditional
	EXPECT_FALSE(LynxDisUtils::IsUnconditionalJump(0xf0));
}

// =============================================================================
// IsConditionalJump Tests
// =============================================================================

class LynxDisUtilsConditionalJumpTest : public ::testing::Test {};

TEST_F(LynxDisUtilsConditionalJumpTest, BPL_0x10) {
	EXPECT_TRUE(LynxDisUtils::IsConditionalJump(0x10));
}

TEST_F(LynxDisUtilsConditionalJumpTest, BMI_0x30) {
	EXPECT_TRUE(LynxDisUtils::IsConditionalJump(0x30));
}

TEST_F(LynxDisUtilsConditionalJumpTest, BVC_0x50) {
	EXPECT_TRUE(LynxDisUtils::IsConditionalJump(0x50));
}

TEST_F(LynxDisUtilsConditionalJumpTest, BVS_0x70) {
	EXPECT_TRUE(LynxDisUtils::IsConditionalJump(0x70));
}

TEST_F(LynxDisUtilsConditionalJumpTest, BCC_0x90) {
	EXPECT_TRUE(LynxDisUtils::IsConditionalJump(0x90));
}

TEST_F(LynxDisUtilsConditionalJumpTest, BCS_0xb0) {
	EXPECT_TRUE(LynxDisUtils::IsConditionalJump(0xb0));
}

TEST_F(LynxDisUtilsConditionalJumpTest, BNE_0xd0) {
	EXPECT_TRUE(LynxDisUtils::IsConditionalJump(0xd0));
}

TEST_F(LynxDisUtilsConditionalJumpTest, BEQ_0xf0) {
	EXPECT_TRUE(LynxDisUtils::IsConditionalJump(0xf0));
}

TEST_F(LynxDisUtilsConditionalJumpTest, JMP_abs_NotConditional) {
	EXPECT_FALSE(LynxDisUtils::IsConditionalJump(0x4c));
}

TEST_F(LynxDisUtilsConditionalJumpTest, BRA_NotConditional) {
	// BRA is unconditional, not conditional
	EXPECT_FALSE(LynxDisUtils::IsConditionalJump(0x80));
}

TEST_F(LynxDisUtilsConditionalJumpTest, NOP_NotConditional) {
	EXPECT_FALSE(LynxDisUtils::IsConditionalJump(0xea));
}

// =============================================================================
// IsJumpToSub Tests
// =============================================================================

class LynxDisUtilsJumpToSubTest : public ::testing::Test {};

TEST_F(LynxDisUtilsJumpToSubTest, BRK_0x00) {
	EXPECT_TRUE(LynxDisUtils::IsJumpToSub(0x00));
}

TEST_F(LynxDisUtilsJumpToSubTest, JSR_0x20) {
	EXPECT_TRUE(LynxDisUtils::IsJumpToSub(0x20));
}

TEST_F(LynxDisUtilsJumpToSubTest, JMP_abs_NotJumpToSub) {
	EXPECT_FALSE(LynxDisUtils::IsJumpToSub(0x4c));
}

TEST_F(LynxDisUtilsJumpToSubTest, RTS_NotJumpToSub) {
	EXPECT_FALSE(LynxDisUtils::IsJumpToSub(0x60));
}

TEST_F(LynxDisUtilsJumpToSubTest, RTI_NotJumpToSub) {
	EXPECT_FALSE(LynxDisUtils::IsJumpToSub(0x40));
}

TEST_F(LynxDisUtilsJumpToSubTest, NOP_NotJumpToSub) {
	EXPECT_FALSE(LynxDisUtils::IsJumpToSub(0xea));
}

// =============================================================================
// IsReturnInstruction Tests
// =============================================================================

class LynxDisUtilsReturnTest : public ::testing::Test {};

TEST_F(LynxDisUtilsReturnTest, RTI_0x40) {
	EXPECT_TRUE(LynxDisUtils::IsReturnInstruction(0x40));
}

TEST_F(LynxDisUtilsReturnTest, RTS_0x60) {
	EXPECT_TRUE(LynxDisUtils::IsReturnInstruction(0x60));
}

TEST_F(LynxDisUtilsReturnTest, JMP_NotReturn) {
	EXPECT_FALSE(LynxDisUtils::IsReturnInstruction(0x4c));
}

TEST_F(LynxDisUtilsReturnTest, JSR_NotReturn) {
	EXPECT_FALSE(LynxDisUtils::IsReturnInstruction(0x20));
}

TEST_F(LynxDisUtilsReturnTest, BRK_NotReturn) {
	EXPECT_FALSE(LynxDisUtils::IsReturnInstruction(0x00));
}

TEST_F(LynxDisUtilsReturnTest, NOP_NotReturn) {
	EXPECT_FALSE(LynxDisUtils::IsReturnInstruction(0xea));
}

// =============================================================================
// GetOpName Tests
// =============================================================================

class LynxDisUtilsGetOpNameTest : public ::testing::Test {};

TEST_F(LynxDisUtilsGetOpNameTest, BRK_0x00) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0x00), "BRK");
}

TEST_F(LynxDisUtilsGetOpNameTest, JSR_0x20) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0x20), "JSR");
}

TEST_F(LynxDisUtilsGetOpNameTest, RTI_0x40) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0x40), "RTI");
}

TEST_F(LynxDisUtilsGetOpNameTest, JMP_abs_0x4c) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0x4c), "JMP");
}

TEST_F(LynxDisUtilsGetOpNameTest, RTS_0x60) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0x60), "RTS");
}

TEST_F(LynxDisUtilsGetOpNameTest, BRA_0x80) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0x80), "BRA");
}

TEST_F(LynxDisUtilsGetOpNameTest, LDA_imm_0xa9) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0xa9), "LDA");
}

TEST_F(LynxDisUtilsGetOpNameTest, STA_abs_0x8d) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0x8d), "STA");
}

TEST_F(LynxDisUtilsGetOpNameTest, NOP_0xea) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0xea), "NOP");
}

TEST_F(LynxDisUtilsGetOpNameTest, WAI_0xcb) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0xcb), "WAI");
}

TEST_F(LynxDisUtilsGetOpNameTest, STP_0xdb) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0xdb), "STP");
}

// 65C02 zero-page indirect
TEST_F(LynxDisUtilsGetOpNameTest, LDA_zpind_0xb2) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0xb2), "LDA");
}

// STZ (65C02)
TEST_F(LynxDisUtilsGetOpNameTest, STZ_zp_0x64) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0x64), "STZ");
}

// TRB/TSB (65C02)
TEST_F(LynxDisUtilsGetOpNameTest, TRB_zp_0x14) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0x14), "TRB");
}

TEST_F(LynxDisUtilsGetOpNameTest, TSB_zp_0x04) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0x04), "TSB");
}

// BBR/BBS/RMB/SMB — Lynx 65SC02 does NOT have Rockwell extensions;
// these opcodes are treated as 1-byte NOP
TEST_F(LynxDisUtilsGetOpNameTest, BBR0_0x0f_IsNop) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0x0f), "NOP");
}

TEST_F(LynxDisUtilsGetOpNameTest, BBS7_0xff_IsNop) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0xff), "NOP");
}

TEST_F(LynxDisUtilsGetOpNameTest, RMB0_0x07_IsNop) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0x07), "NOP");
}

TEST_F(LynxDisUtilsGetOpNameTest, SMB7_0xf7_IsNop) {
	EXPECT_STREQ(LynxDisUtils::GetOpName(0xf7), "NOP");
}

// =============================================================================
// Exhaustive OpSize — all 256 opcodes produce valid sizes (1, 2, or 3)
// =============================================================================

class LynxDisUtilsExhaustiveTest : public ::testing::Test {};

TEST_F(LynxDisUtilsExhaustiveTest, AllOpcodes_ValidSize) {
	for (int op = 0; op < 256; op++) {
		uint8_t size = LynxDisUtils::GetOpSize(static_cast<uint8_t>(op));
		EXPECT_GE(size, 1) << "Opcode 0x" << std::hex << op << " has size < 1";
		EXPECT_LE(size, 3) << "Opcode 0x" << std::hex << op << " has size > 3";
	}
}

TEST_F(LynxDisUtilsExhaustiveTest, AllOpcodes_HaveName) {
	for (int op = 0; op < 256; op++) {
		const char* name = LynxDisUtils::GetOpName(static_cast<uint8_t>(op));
		EXPECT_NE(name, nullptr) << "Opcode 0x" << std::hex << op << " has null name";
		EXPECT_GT(strlen(name), 0u) << "Opcode 0x" << std::hex << op << " has empty name";
	}
}

TEST_F(LynxDisUtilsExhaustiveTest, ConditionalAndUnconditional_Disjoint) {
	// No opcode should be both conditional and unconditional
	for (int op = 0; op < 256; op++) {
		uint8_t opcode = static_cast<uint8_t>(op);
		if (LynxDisUtils::IsConditionalJump(opcode)) {
			EXPECT_FALSE(LynxDisUtils::IsUnconditionalJump(opcode))
				<< "Opcode 0x" << std::hex << op << " is both conditional and unconditional";
		}
	}
}

TEST_F(LynxDisUtilsExhaustiveTest, ReturnInstructions_AreUnconditional) {
	// RTI/RTS should be unconditional jumps
	for (int op = 0; op < 256; op++) {
		uint8_t opcode = static_cast<uint8_t>(op);
		if (LynxDisUtils::IsReturnInstruction(opcode)) {
			EXPECT_TRUE(LynxDisUtils::IsUnconditionalJump(opcode))
				<< "Return instruction 0x" << std::hex << op << " is not marked unconditional";
		}
	}
}

TEST_F(LynxDisUtilsExhaustiveTest, ConditionalBranches_AllSize2) {
	// All conditional branches are relative (2 bytes)
	for (int op = 0; op < 256; op++) {
		uint8_t opcode = static_cast<uint8_t>(op);
		if (LynxDisUtils::IsConditionalJump(opcode)) {
			EXPECT_EQ(LynxDisUtils::GetOpSize(opcode), 2)
				<< "Conditional branch 0x" << std::hex << op << " is not size 2";
		}
	}
}

// BBR/BBS/RMB/SMB — Lynx 65SC02 does NOT have Rockwell extensions.
// These opcodes are 1-byte NOPs (Implied addressing mode).
TEST_F(LynxDisUtilsExhaustiveTest, BBR_BBS_Are1ByteNop) {
	// BBR0-BBR7 slots: 0x0f, 0x1f, 0x2f, 0x3f, 0x4f, 0x5f, 0x6f, 0x7f
	// BBS0-BBS7 slots: 0x8f, 0x9f, 0xaf, 0xbf, 0xcf, 0xdf, 0xef, 0xff
	uint8_t bbrOps[] = { 0x0f, 0x1f, 0x2f, 0x3f, 0x4f, 0x5f, 0x6f, 0x7f };
	uint8_t bbsOps[] = { 0x8f, 0x9f, 0xaf, 0xbf, 0xcf, 0xdf, 0xef, 0xff };

	for (uint8_t op : bbrOps) {
		EXPECT_EQ(LynxDisUtils::GetOpSize(op), 1)
			<< "BBR slot 0x" << std::hex << (int)op << " should be 1-byte NOP";
		EXPECT_STREQ(LynxDisUtils::GetOpName(op), "NOP");
	}
	for (uint8_t op : bbsOps) {
		EXPECT_EQ(LynxDisUtils::GetOpSize(op), 1)
			<< "BBS slot 0x" << std::hex << (int)op << " should be 1-byte NOP";
		EXPECT_STREQ(LynxDisUtils::GetOpName(op), "NOP");
	}
}

TEST_F(LynxDisUtilsExhaustiveTest, RMB_SMB_Are1ByteNop) {
	// RMB0-RMB7 slots: 0x07, 0x17, 0x27, 0x37, 0x47, 0x57, 0x67, 0x77
	// SMB0-SMB7 slots: 0x87, 0x97, 0xa7, 0xb7, 0xc7, 0xd7, 0xe7, 0xf7
	uint8_t rmbOps[] = { 0x07, 0x17, 0x27, 0x37, 0x47, 0x57, 0x67, 0x77 };
	uint8_t smbOps[] = { 0x87, 0x97, 0xa7, 0xb7, 0xc7, 0xd7, 0xe7, 0xf7 };

	for (uint8_t op : rmbOps) {
		EXPECT_EQ(LynxDisUtils::GetOpSize(op), 1)
			<< "RMB slot 0x" << std::hex << (int)op << " should be 1-byte NOP";
		EXPECT_STREQ(LynxDisUtils::GetOpName(op), "NOP");
	}
	for (uint8_t op : smbOps) {
		EXPECT_EQ(LynxDisUtils::GetOpSize(op), 1)
			<< "SMB slot 0x" << std::hex << (int)op << " should be 1-byte NOP";
		EXPECT_STREQ(LynxDisUtils::GetOpName(op), "NOP");
	}
}
