#include "pch.h"
#include "ChannelF/Debugger/ChannelFDisUtils.h"

/// <summary>
/// Tests for ChannelFDisUtils — the Fairchild F8 CPU disassembly utility.
///
/// Tests verify all pure static methods:
///   - GetOpSize: correct byte counts for all addressing modes
///   - GetOpName: opcode mnemonic lookup
///   - GetOpMode: addressing mode classification
///   - IsUnconditionalJump: PI/JMP/POP/LR P,K/PK/BR
///   - IsConditionalJump: BT $81-$87, BR7 $8F, BF $91-$9F
///   - IsJumpToSub: PI ($28) only
///   - IsReturnInstruction: POP/LR P,K/PK
///   - GetOpFlags: CDL SubEntryPoint for PI
///
/// References:
///   - ChannelFDisUtils.h/.cpp
///   - Fairchild F8 instruction set
/// </summary>

// =============================================================================
// GetOpSize Tests
// =============================================================================

class ChannelFDisUtilsOpSizeTest : public ::testing::Test {};

// 1-byte implied instructions ($00-$1F)
TEST_F(ChannelFDisUtilsOpSizeTest, LR_A_KU_0x00_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x00), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, LR_P_K_0x09_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x09), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, PK_0x0c_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x0c), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, SR1_0x12_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x12), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, POP_0x1c_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x1c), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, INC_0x1f_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x1f), 1);
}

// 2-byte immediate/port instructions ($20-$27)
TEST_F(ChannelFDisUtilsOpSizeTest, LI_0x20_Size2) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x20), 2);
}

TEST_F(ChannelFDisUtilsOpSizeTest, NI_0x21_Size2) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x21), 2);
}

TEST_F(ChannelFDisUtilsOpSizeTest, AI_0x24_Size2) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x24), 2);
}

TEST_F(ChannelFDisUtilsOpSizeTest, OUT_0x27_Size2) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x27), 2);
}

// 3-byte absolute instructions ($28-$2A)
TEST_F(ChannelFDisUtilsOpSizeTest, PI_0x28_Size3) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x28), 3);
}

TEST_F(ChannelFDisUtilsOpSizeTest, JMP_0x29_Size3) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x29), 3);
}

TEST_F(ChannelFDisUtilsOpSizeTest, DCI_0x2a_Size3) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x2a), 3);
}

// 1-byte misc ($2B-$2F)
TEST_F(ChannelFDisUtilsOpSizeTest, NOP_0x2b_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x2b), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, XDC_0x2c_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x2c), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, Undefined_0x2d_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x2d), 1);
}

// 1-byte register instructions ($30-$7F)
TEST_F(ChannelFDisUtilsOpSizeTest, DS_0x30_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x30), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, LR_A_r_0x40_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x40), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, LR_r_A_0x50_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x50), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, LISU_0x60_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x60), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, LIS_0x70_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x70), 1);
}

// 2-byte branch instructions ($80-$9F)
TEST_F(ChannelFDisUtilsOpSizeTest, BT_0x81_Size2) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x81), 2);
}

TEST_F(ChannelFDisUtilsOpSizeTest, BT_0x87_Size2) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x87), 2);
}

// 1-byte memory ALU instructions ($88-$8E)
TEST_F(ChannelFDisUtilsOpSizeTest, AM_0x88_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x88), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, AMD_0x89_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x89), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, NM_0x8a_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x8a), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, OM_0x8b_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x8b), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, XM_0x8c_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x8c), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, CM_0x8d_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x8d), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, ADC_0x8e_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x8e), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, BR7_0x8f_Size2) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x8f), 2);
}

TEST_F(ChannelFDisUtilsOpSizeTest, BR_0x90_Size2) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x90), 2);
}

TEST_F(ChannelFDisUtilsOpSizeTest, BF_0x91_Size2) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x91), 2);
}

TEST_F(ChannelFDisUtilsOpSizeTest, BF_0x9f_Size2) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0x9f), 2);
}

// 1-byte I/O ($A0-$BF)
TEST_F(ChannelFDisUtilsOpSizeTest, INS_0xa0_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0xa0), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, OUTS_0xb0_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0xb0), 1);
}

// 1-byte ALU ($C0-$FF)
TEST_F(ChannelFDisUtilsOpSizeTest, AS_0xc0_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0xc0), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, ASD_0xd0_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0xd0), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, XS_0xe0_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0xe0), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, NS_0xf0_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0xf0), 1);
}

TEST_F(ChannelFDisUtilsOpSizeTest, NS_0xff_Size1) {
	EXPECT_EQ(ChannelFDisUtils::GetOpSize(0xff), 1);
}

// Exhaustive: all 256 opcodes have size 1, 2, or 3
TEST_F(ChannelFDisUtilsOpSizeTest, AllOpcodes_SizeInValidRange) {
	for (int op = 0; op < 256; op++) {
		uint8_t size = ChannelFDisUtils::GetOpSize((uint8_t)op);
		EXPECT_GE(size, 1) << "Opcode $" << std::hex << op;
		EXPECT_LE(size, 3) << "Opcode $" << std::hex << op;
	}
}

// =============================================================================
// GetOpName Tests
// =============================================================================

class ChannelFDisUtilsGetOpNameTest : public ::testing::Test {};

TEST_F(ChannelFDisUtilsGetOpNameTest, LR_0x00) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x00), "LR");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, LR_0x09) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x09), "LR");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, PK_0x0c) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x0c), "PK");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, SR_0x12) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x12), "SR");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, SL_0x13) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x13), "SL");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, LM_0x16) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x16), "LM");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, ST_0x17) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x17), "ST");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, COM_0x18) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x18), "COM");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, DI_0x1a) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x1a), "DI");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, EI_0x1b) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x1b), "EI");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, POP_0x1c) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x1c), "POP");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, INC_0x1f) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x1f), "INC");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, LI_0x20) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x20), "LI");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, NI_0x21) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x21), "NI");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, OI_0x22) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x22), "OI");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, XI_0x23) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x23), "XI");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, AI_0x24) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x24), "AI");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, CI_0x25) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x25), "CI");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, IN_0x26) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x26), "IN");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, OUT_0x27) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x27), "OUT");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, PI_0x28) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x28), "PI");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, JMP_0x29) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x29), "JMP");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, DCI_0x2a) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x2a), "DCI");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, NOP_0x2b) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x2b), "NOP");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, XDC_0x2c) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x2c), "XDC");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, Undefined_0x2d) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x2d), "???");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, DS_0x30) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x30), "DS");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, LR_A_r_0x40) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x40), "LR");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, LR_r_A_0x50) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x50), "LR");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, LISU_0x60) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x60), "LISU");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, LISL_0x68) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x68), "LISL");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, LIS_0x70) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x70), "LIS");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, CLR_0x70_IsLIS) {
	// $70 is LIS 0, sometimes called CLR — but disasm shows LIS
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x70), "LIS");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, BT_0x81) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x81), "BT");
}

// Memory ALU opcodes $88-$8E
TEST_F(ChannelFDisUtilsGetOpNameTest, AM_0x88) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x88), "AM");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, AMD_0x89) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x89), "AMD");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, NM_0x8a) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x8a), "NM");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, OM_0x8b) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x8b), "OM");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, XM_0x8c) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x8c), "XM");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, CM_0x8d) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x8d), "CM");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, ADC_0x8e) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x8e), "ADC");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, BR7_0x8f) {
	// $8F is BR7 (branch if ISAR lower 3 bits != 7)
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x8f), "BR7");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, BR_0x90) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x90), "BR");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, BF_0x91) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0x91), "BF");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, INS_0xa0) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0xa0), "INS");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, OUTS_0xb0) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0xb0), "OUTS");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, AS_0xc0) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0xc0), "AS");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, ASD_0xd0) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0xd0), "ASD");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, XS_0xe0) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0xe0), "XS");
}

TEST_F(ChannelFDisUtilsGetOpNameTest, NS_0xf0) {
	EXPECT_STREQ(ChannelFDisUtils::GetOpName(0xf0), "NS");
}

// All 256 entries have a non-null name
TEST_F(ChannelFDisUtilsGetOpNameTest, AllOpcodes_NonNull) {
	for (int op = 0; op < 256; op++) {
		EXPECT_NE(ChannelFDisUtils::GetOpName((uint8_t)op), nullptr)
			<< "Opcode $" << std::hex << op;
	}
}

// =============================================================================
// GetOpMode Tests
// =============================================================================

class ChannelFDisUtilsGetOpModeTest : public ::testing::Test {};

// Implied mode ($00-$1F, $2B, $2C)
TEST_F(ChannelFDisUtilsGetOpModeTest, LR_A_KU_0x00_Imp) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x00), ChannelFAddrMode::Imp);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, POP_0x1c_Imp) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x1c), ChannelFAddrMode::Imp);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, NOP_0x2b_Imp) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x2b), ChannelFAddrMode::Imp);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, XDC_0x2c_Imp) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x2c), ChannelFAddrMode::Imp);
}

// Immediate mode ($20=LI, $21=NI, $22=OI, $23=XI, $24=AI, $25=CI)
TEST_F(ChannelFDisUtilsGetOpModeTest, LI_0x20_Imm) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x20), ChannelFAddrMode::Imm);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, CI_0x25_Imm) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x25), ChannelFAddrMode::Imm);
}

// Port mode ($26=IN, $27=OUT)
TEST_F(ChannelFDisUtilsGetOpModeTest, IN_0x26_Port) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x26), ChannelFAddrMode::Port);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, OUT_0x27_Port) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x27), ChannelFAddrMode::Port);
}

// Absolute mode ($28=PI, $29=JMP, $2A=DCI)
TEST_F(ChannelFDisUtilsGetOpModeTest, PI_0x28_Abs) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x28), ChannelFAddrMode::Abs);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, JMP_0x29_Abs) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x29), ChannelFAddrMode::Abs);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, DCI_0x2a_Abs) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x2a), ChannelFAddrMode::Abs);
}

// None mode ($2D-$2F undefined)
TEST_F(ChannelFDisUtilsGetOpModeTest, Undefined_0x2d_None) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x2d), ChannelFAddrMode::None);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, Undefined_0x2e_None) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x2e), ChannelFAddrMode::None);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, Undefined_0x2f_None) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x2f), ChannelFAddrMode::None);
}

// Register mode ($30-$3F DS, $40-$4F LR A,r, $50-$5F LR r,A, $A0-$AF INS, $B0-$BF OUTS, $C0-$FF ALU)
TEST_F(ChannelFDisUtilsGetOpModeTest, DS_0x30_Reg) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x30), ChannelFAddrMode::Reg);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, LR_A_r_0x4f_Reg) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x4f), ChannelFAddrMode::Reg);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, NS_0xff_Reg) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0xff), ChannelFAddrMode::Reg);
}

// Isar3 mode ($60-$6F LISU/LISL)
TEST_F(ChannelFDisUtilsGetOpModeTest, LISU_0x60_Isar3) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x60), ChannelFAddrMode::Isar3);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, LISL_0x68_Isar3) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x68), ChannelFAddrMode::Isar3);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, LISL_0x6f_Isar3) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x6f), ChannelFAddrMode::Isar3);
}

// Imm4 mode ($70-$7F LIS)
TEST_F(ChannelFDisUtilsGetOpModeTest, LIS_0x70_Imm4) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x70), ChannelFAddrMode::Imm4);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, LIS_0x7f_Imm4) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x7f), ChannelFAddrMode::Imm4);
}

// Relative mode ($80-$87 BT branches, $8F BR7, $90-$9F BF/BR)
TEST_F(ChannelFDisUtilsGetOpModeTest, BT_0x80_Rel) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x80), ChannelFAddrMode::Rel);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, BT_0x87_Rel) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x87), ChannelFAddrMode::Rel);
}

// Memory ALU opcodes $88-$8E are Implied (operate via DC0, no operand)
TEST_F(ChannelFDisUtilsGetOpModeTest, AM_0x88_Imp) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x88), ChannelFAddrMode::Imp);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, AMD_0x89_Imp) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x89), ChannelFAddrMode::Imp);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, NM_0x8a_Imp) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x8a), ChannelFAddrMode::Imp);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, OM_0x8b_Imp) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x8b), ChannelFAddrMode::Imp);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, XM_0x8c_Imp) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x8c), ChannelFAddrMode::Imp);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, CM_0x8d_Imp) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x8d), ChannelFAddrMode::Imp);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, ADC_0x8e_Imp) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x8e), ChannelFAddrMode::Imp);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, BR7_0x8f_Rel) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x8f), ChannelFAddrMode::Rel);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, BR_0x90_Rel) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x90), ChannelFAddrMode::Rel);
}

TEST_F(ChannelFDisUtilsGetOpModeTest, BF_0x9f_Rel) {
	EXPECT_EQ(ChannelFDisUtils::GetOpMode(0x9f), ChannelFAddrMode::Rel);
}

// =============================================================================
// IsUnconditionalJump Tests
// =============================================================================

class ChannelFDisUtilsUnconditionalJumpTest : public ::testing::Test {};

TEST_F(ChannelFDisUtilsUnconditionalJumpTest, PI_0x28) {
	EXPECT_TRUE(ChannelFDisUtils::IsUnconditionalJump(0x28));
}

TEST_F(ChannelFDisUtilsUnconditionalJumpTest, JMP_0x29) {
	EXPECT_TRUE(ChannelFDisUtils::IsUnconditionalJump(0x29));
}

TEST_F(ChannelFDisUtilsUnconditionalJumpTest, POP_0x1c) {
	EXPECT_TRUE(ChannelFDisUtils::IsUnconditionalJump(0x1c));
}

TEST_F(ChannelFDisUtilsUnconditionalJumpTest, LR_P_K_0x09) {
	EXPECT_TRUE(ChannelFDisUtils::IsUnconditionalJump(0x09));
}

TEST_F(ChannelFDisUtilsUnconditionalJumpTest, PK_0x0c) {
	EXPECT_TRUE(ChannelFDisUtils::IsUnconditionalJump(0x0c));
}

TEST_F(ChannelFDisUtilsUnconditionalJumpTest, BR_0x90) {
	EXPECT_TRUE(ChannelFDisUtils::IsUnconditionalJump(0x90));
}

TEST_F(ChannelFDisUtilsUnconditionalJumpTest, NOP_NotUnconditional) {
	EXPECT_FALSE(ChannelFDisUtils::IsUnconditionalJump(0x2b));
}

TEST_F(ChannelFDisUtilsUnconditionalJumpTest, LI_NotUnconditional) {
	EXPECT_FALSE(ChannelFDisUtils::IsUnconditionalJump(0x20));
}

TEST_F(ChannelFDisUtilsUnconditionalJumpTest, BT_NotUnconditional) {
	// Conditional branches are NOT unconditional
	EXPECT_FALSE(ChannelFDisUtils::IsUnconditionalJump(0x81));
}

TEST_F(ChannelFDisUtilsUnconditionalJumpTest, BF_NotUnconditional) {
	EXPECT_FALSE(ChannelFDisUtils::IsUnconditionalJump(0x91));
}

TEST_F(ChannelFDisUtilsUnconditionalJumpTest, DCI_NotUnconditional) {
	EXPECT_FALSE(ChannelFDisUtils::IsUnconditionalJump(0x2a));
}

TEST_F(ChannelFDisUtilsUnconditionalJumpTest, INS_NotUnconditional) {
	EXPECT_FALSE(ChannelFDisUtils::IsUnconditionalJump(0xa0));
}

// =============================================================================
// IsConditionalJump Tests
// =============================================================================

class ChannelFDisUtilsConditionalJumpTest : public ::testing::Test {};

// BT $81-$87 are conditional
TEST_F(ChannelFDisUtilsConditionalJumpTest, BT_0x81) {
	EXPECT_TRUE(ChannelFDisUtils::IsConditionalJump(0x81));
}

TEST_F(ChannelFDisUtilsConditionalJumpTest, BT_0x84) {
	EXPECT_TRUE(ChannelFDisUtils::IsConditionalJump(0x84));
}

TEST_F(ChannelFDisUtilsConditionalJumpTest, BT_0x87) {
	EXPECT_TRUE(ChannelFDisUtils::IsConditionalJump(0x87));
}

// Memory ALU $88-$8E are NOT conditional jumps
TEST_F(ChannelFDisUtilsConditionalJumpTest, AM_0x88_NotConditional) {
	EXPECT_FALSE(ChannelFDisUtils::IsConditionalJump(0x88));
}

TEST_F(ChannelFDisUtilsConditionalJumpTest, ADC_0x8e_NotConditional) {
	EXPECT_FALSE(ChannelFDisUtils::IsConditionalJump(0x8e));
}

// BR7 ($8F) is conditional
TEST_F(ChannelFDisUtilsConditionalJumpTest, BR7_0x8f) {
	EXPECT_TRUE(ChannelFDisUtils::IsConditionalJump(0x8f));
}

// BF $91-$9F are conditional
TEST_F(ChannelFDisUtilsConditionalJumpTest, BF_0x91) {
	EXPECT_TRUE(ChannelFDisUtils::IsConditionalJump(0x91));
}

TEST_F(ChannelFDisUtilsConditionalJumpTest, BF_0x98) {
	EXPECT_TRUE(ChannelFDisUtils::IsConditionalJump(0x98));
}

TEST_F(ChannelFDisUtilsConditionalJumpTest, BF_0x9f) {
	EXPECT_TRUE(ChannelFDisUtils::IsConditionalJump(0x9f));
}

// BT $80 — "branch on all true" with mask 0 never branches, excluded
TEST_F(ChannelFDisUtilsConditionalJumpTest, BT_0x80_NotConditional) {
	EXPECT_FALSE(ChannelFDisUtils::IsConditionalJump(0x80));
}

// BR $90 is unconditional, NOT conditional
TEST_F(ChannelFDisUtilsConditionalJumpTest, BR_0x90_NotConditional) {
	EXPECT_FALSE(ChannelFDisUtils::IsConditionalJump(0x90));
}

TEST_F(ChannelFDisUtilsConditionalJumpTest, JMP_NotConditional) {
	EXPECT_FALSE(ChannelFDisUtils::IsConditionalJump(0x29));
}

TEST_F(ChannelFDisUtilsConditionalJumpTest, PI_NotConditional) {
	EXPECT_FALSE(ChannelFDisUtils::IsConditionalJump(0x28));
}

TEST_F(ChannelFDisUtilsConditionalJumpTest, NOP_NotConditional) {
	EXPECT_FALSE(ChannelFDisUtils::IsConditionalJump(0x2b));
}

// Exhaustive: BT $81-$87 and BR7 $8F are conditional, $88-$8E (memory ALU) are NOT
TEST_F(ChannelFDisUtilsConditionalJumpTest, AllBT_81_to_87_AreConditional) {
	for (int op = 0x81; op <= 0x87; op++) {
		EXPECT_TRUE(ChannelFDisUtils::IsConditionalJump((uint8_t)op))
			<< "BT $" << std::hex << op;
	}
	// BR7 $8F is also conditional
	EXPECT_TRUE(ChannelFDisUtils::IsConditionalJump(0x8f));
}

TEST_F(ChannelFDisUtilsConditionalJumpTest, MemoryALU_88_to_8E_NotConditional) {
	for (int op = 0x88; op <= 0x8e; op++) {
		EXPECT_FALSE(ChannelFDisUtils::IsConditionalJump((uint8_t)op))
			<< "Memory ALU $" << std::hex << op << " should NOT be conditional";
	}
}

TEST_F(ChannelFDisUtilsConditionalJumpTest, AllBF_91_to_9F_AreConditional) {
	for (int op = 0x91; op <= 0x9f; op++) {
		EXPECT_TRUE(ChannelFDisUtils::IsConditionalJump((uint8_t)op))
			<< "BF $" << std::hex << op;
	}
}

// =============================================================================
// IsJumpToSub Tests
// =============================================================================

class ChannelFDisUtilsJumpToSubTest : public ::testing::Test {};

TEST_F(ChannelFDisUtilsJumpToSubTest, PI_0x28) {
	EXPECT_TRUE(ChannelFDisUtils::IsJumpToSub(0x28));
}

TEST_F(ChannelFDisUtilsJumpToSubTest, JMP_NotJumpToSub) {
	EXPECT_FALSE(ChannelFDisUtils::IsJumpToSub(0x29));
}

TEST_F(ChannelFDisUtilsJumpToSubTest, POP_NotJumpToSub) {
	EXPECT_FALSE(ChannelFDisUtils::IsJumpToSub(0x1c));
}

TEST_F(ChannelFDisUtilsJumpToSubTest, BR_NotJumpToSub) {
	EXPECT_FALSE(ChannelFDisUtils::IsJumpToSub(0x90));
}

TEST_F(ChannelFDisUtilsJumpToSubTest, PK_NotJumpToSub) {
	// PK is listed in IsReturnInstruction, not IsJumpToSub
	EXPECT_FALSE(ChannelFDisUtils::IsJumpToSub(0x0c));
}

TEST_F(ChannelFDisUtilsJumpToSubTest, NOP_NotJumpToSub) {
	EXPECT_FALSE(ChannelFDisUtils::IsJumpToSub(0x2b));
}

// =============================================================================
// IsReturnInstruction Tests
// =============================================================================

class ChannelFDisUtilsReturnTest : public ::testing::Test {};

TEST_F(ChannelFDisUtilsReturnTest, POP_0x1c) {
	EXPECT_TRUE(ChannelFDisUtils::IsReturnInstruction(0x1c));
}

TEST_F(ChannelFDisUtilsReturnTest, LR_P_K_0x09) {
	EXPECT_TRUE(ChannelFDisUtils::IsReturnInstruction(0x09));
}

TEST_F(ChannelFDisUtilsReturnTest, PK_0x0c) {
	EXPECT_TRUE(ChannelFDisUtils::IsReturnInstruction(0x0c));
}

TEST_F(ChannelFDisUtilsReturnTest, PI_NotReturn) {
	EXPECT_FALSE(ChannelFDisUtils::IsReturnInstruction(0x28));
}

TEST_F(ChannelFDisUtilsReturnTest, JMP_NotReturn) {
	EXPECT_FALSE(ChannelFDisUtils::IsReturnInstruction(0x29));
}

TEST_F(ChannelFDisUtilsReturnTest, BR_NotReturn) {
	EXPECT_FALSE(ChannelFDisUtils::IsReturnInstruction(0x90));
}

TEST_F(ChannelFDisUtilsReturnTest, NOP_NotReturn) {
	EXPECT_FALSE(ChannelFDisUtils::IsReturnInstruction(0x2b));
}

// =============================================================================
// GetOpFlags Tests (CDL flags)
// =============================================================================

class ChannelFDisUtilsGetOpFlagsTest : public ::testing::Test {};

TEST_F(ChannelFDisUtilsGetOpFlagsTest, PI_0x28_SubEntryPoint) {
	EXPECT_EQ(ChannelFDisUtils::GetOpFlags(0x28, 0, 0), CdlFlags::SubEntryPoint);
}

TEST_F(ChannelFDisUtilsGetOpFlagsTest, JMP_0x29_None) {
	EXPECT_EQ(ChannelFDisUtils::GetOpFlags(0x29, 0, 0), CdlFlags::None);
}

TEST_F(ChannelFDisUtilsGetOpFlagsTest, NOP_0x2b_None) {
	EXPECT_EQ(ChannelFDisUtils::GetOpFlags(0x2b, 0, 0), CdlFlags::None);
}

TEST_F(ChannelFDisUtilsGetOpFlagsTest, BR_0x90_None) {
	EXPECT_EQ(ChannelFDisUtils::GetOpFlags(0x90, 0, 0), CdlFlags::None);
}

TEST_F(ChannelFDisUtilsGetOpFlagsTest, POP_0x1c_None) {
	// POP is return, not sub entry point
	EXPECT_EQ(ChannelFDisUtils::GetOpFlags(0x1c, 0, 0), CdlFlags::None);
}

// =============================================================================
// Table Consistency Tests
// =============================================================================

class ChannelFDisUtilsTableConsistencyTest : public ::testing::Test {};

// OpSize should match addressing mode expectations
TEST_F(ChannelFDisUtilsTableConsistencyTest, ImpliedOpcodes_Size1) {
	// All implied-mode opcodes should be 1 byte
	for (int op = 0; op < 256; op++) {
		if (ChannelFDisUtils::GetOpMode((uint8_t)op) == ChannelFAddrMode::Imp) {
			EXPECT_EQ(ChannelFDisUtils::GetOpSize((uint8_t)op), 1)
				<< "Implied opcode $" << std::hex << op << " should be 1 byte";
		}
	}
}

TEST_F(ChannelFDisUtilsTableConsistencyTest, ImmOpcodes_Size2) {
	for (int op = 0; op < 256; op++) {
		if (ChannelFDisUtils::GetOpMode((uint8_t)op) == ChannelFAddrMode::Imm) {
			EXPECT_EQ(ChannelFDisUtils::GetOpSize((uint8_t)op), 2)
				<< "Immediate opcode $" << std::hex << op << " should be 2 bytes";
		}
	}
}

TEST_F(ChannelFDisUtilsTableConsistencyTest, PortOpcodes_Size2) {
	for (int op = 0; op < 256; op++) {
		if (ChannelFDisUtils::GetOpMode((uint8_t)op) == ChannelFAddrMode::Port) {
			EXPECT_EQ(ChannelFDisUtils::GetOpSize((uint8_t)op), 2)
				<< "Port opcode $" << std::hex << op << " should be 2 bytes";
		}
	}
}

TEST_F(ChannelFDisUtilsTableConsistencyTest, RelOpcodes_Size2) {
	for (int op = 0; op < 256; op++) {
		if (ChannelFDisUtils::GetOpMode((uint8_t)op) == ChannelFAddrMode::Rel) {
			EXPECT_EQ(ChannelFDisUtils::GetOpSize((uint8_t)op), 2)
				<< "Relative opcode $" << std::hex << op << " should be 2 bytes";
		}
	}
}

TEST_F(ChannelFDisUtilsTableConsistencyTest, AbsOpcodes_Size3) {
	for (int op = 0; op < 256; op++) {
		if (ChannelFDisUtils::GetOpMode((uint8_t)op) == ChannelFAddrMode::Abs) {
			EXPECT_EQ(ChannelFDisUtils::GetOpSize((uint8_t)op), 3)
				<< "Absolute opcode $" << std::hex << op << " should be 3 bytes";
		}
	}
}

TEST_F(ChannelFDisUtilsTableConsistencyTest, RegOpcodes_Size1) {
	for (int op = 0; op < 256; op++) {
		if (ChannelFDisUtils::GetOpMode((uint8_t)op) == ChannelFAddrMode::Reg) {
			EXPECT_EQ(ChannelFDisUtils::GetOpSize((uint8_t)op), 1)
				<< "Register opcode $" << std::hex << op << " should be 1 byte";
		}
	}
}

TEST_F(ChannelFDisUtilsTableConsistencyTest, Isar3Opcodes_Size1) {
	for (int op = 0; op < 256; op++) {
		if (ChannelFDisUtils::GetOpMode((uint8_t)op) == ChannelFAddrMode::Isar3) {
			EXPECT_EQ(ChannelFDisUtils::GetOpSize((uint8_t)op), 1)
				<< "Isar3 opcode $" << std::hex << op << " should be 1 byte";
		}
	}
}

TEST_F(ChannelFDisUtilsTableConsistencyTest, Imm4Opcodes_Size1) {
	for (int op = 0; op < 256; op++) {
		if (ChannelFDisUtils::GetOpMode((uint8_t)op) == ChannelFAddrMode::Imm4) {
			EXPECT_EQ(ChannelFDisUtils::GetOpSize((uint8_t)op), 1)
				<< "Imm4 opcode $" << std::hex << op << " should be 1 byte";
		}
	}
}

// Unconditional jumps should include all return instructions
TEST_F(ChannelFDisUtilsTableConsistencyTest, ReturnInstructions_AreAlsoUnconditionalJumps) {
	for (int op = 0; op < 256; op++) {
		if (ChannelFDisUtils::IsReturnInstruction((uint8_t)op)) {
			EXPECT_TRUE(ChannelFDisUtils::IsUnconditionalJump((uint8_t)op))
				<< "Return instruction $" << std::hex << op
				<< " should also be an unconditional jump";
		}
	}
}

// Jump-to-sub should be a subset of unconditional jumps
TEST_F(ChannelFDisUtilsTableConsistencyTest, JumpToSub_AreAlsoUnconditionalJumps) {
	for (int op = 0; op < 256; op++) {
		if (ChannelFDisUtils::IsJumpToSub((uint8_t)op)) {
			EXPECT_TRUE(ChannelFDisUtils::IsUnconditionalJump((uint8_t)op))
				<< "Jump-to-sub $" << std::hex << op
				<< " should also be an unconditional jump";
		}
	}
}

// Conditional and unconditional should be mutually exclusive
TEST_F(ChannelFDisUtilsTableConsistencyTest, ConditionalAndUnconditional_MutuallyExclusive) {
	for (int op = 0; op < 256; op++) {
		if (ChannelFDisUtils::IsConditionalJump((uint8_t)op)) {
			EXPECT_FALSE(ChannelFDisUtils::IsUnconditionalJump((uint8_t)op))
				<< "Opcode $" << std::hex << op
				<< " should not be both conditional and unconditional";
		}
	}
}

// Exactly 3 undefined opcodes ($2D, $2E, $2F)
TEST_F(ChannelFDisUtilsTableConsistencyTest, ExactlyThreeUndefinedOpcodes) {
	int undefinedCount = 0;
	for (int op = 0; op < 256; op++) {
		if (ChannelFDisUtils::GetOpMode((uint8_t)op) == ChannelFAddrMode::None) {
			undefinedCount++;
		}
	}
	EXPECT_EQ(undefinedCount, 3);
}
