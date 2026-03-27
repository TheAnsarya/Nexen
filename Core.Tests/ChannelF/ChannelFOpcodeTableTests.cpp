#include "pch.h"
#include "ChannelF/ChannelFOpcodeTable.h"

TEST(ChannelFOpcodeTable, ReturnsEntryForAllOpcodes) {
	for (uint32_t opcode = 0; opcode < ChannelFOpcodeTable::OpcodeCount; opcode++) {
		const ChannelFOpcodeInfo& info = ChannelFOpcodeTable::Get((uint8_t)opcode);
		EXPECT_EQ(info.Opcode, opcode);
		EXPECT_NE(info.Mnemonic, nullptr);
		EXPECT_GE(info.Size, 1);
		EXPECT_GE(info.Cycles, 1);
	}
}

TEST(ChannelFOpcodeTable, KnownBaselineEntriesAreDefined) {
	EXPECT_TRUE(ChannelFOpcodeTable::IsDefined(0x00)); // lr a,ku
	EXPECT_TRUE(ChannelFOpcodeTable::IsDefined(0x08)); // lr k,p
	EXPECT_TRUE(ChannelFOpcodeTable::IsDefined(0x2b)); // nop
	EXPECT_TRUE(ChannelFOpcodeTable::IsDefined(0x90)); // br
	EXPECT_TRUE(ChannelFOpcodeTable::IsDefined(0xa0)); // ins 0
	EXPECT_TRUE(ChannelFOpcodeTable::IsDefined(0xff)); // ns 15
}

TEST(ChannelFOpcodeTable, UndefinedOpcodeEntriesAreIll) {
	// $2D-$2F are undefined in the F8 instruction set
	const ChannelFOpcodeInfo& info = ChannelFOpcodeTable::Get(0x2d);
	EXPECT_STREQ(info.Mnemonic, "ill");
	EXPECT_FALSE(info.IsDefined);
	EXPECT_EQ(info.Size, 1);
	EXPECT_EQ(info.Cycles, 1);
}

TEST(ChannelFOpcodeTable, DefinedOpcodeCountIsComplete) {
	// Full F8 instruction set: 253 defined opcodes ($00-$FF minus $2D, $2E, $2F)
	EXPECT_EQ(ChannelFOpcodeTable::GetDefinedOpcodeCount(), 253u);
}

TEST(ChannelFOpcodeTable, ImmediateOpcodesSizeAndCycles) {
	// 2-byte immediate instructions
	EXPECT_EQ(ChannelFOpcodeTable::Get(0x20).Size, 2); // LI
	EXPECT_EQ(ChannelFOpcodeTable::Get(0x21).Size, 2); // NI
	EXPECT_EQ(ChannelFOpcodeTable::Get(0x24).Size, 2); // AI
	// 3-byte absolute instructions
	EXPECT_EQ(ChannelFOpcodeTable::Get(0x28).Size, 3); // PI
	EXPECT_EQ(ChannelFOpcodeTable::Get(0x29).Size, 3); // JMP
	EXPECT_EQ(ChannelFOpcodeTable::Get(0x2a).Size, 3); // DCI
}

TEST(ChannelFOpcodeTable, BranchOpcodesSizeIs2) {
	EXPECT_EQ(ChannelFOpcodeTable::Get(0x8f).Size, 2); // BR7
	EXPECT_EQ(ChannelFOpcodeTable::Get(0x90).Size, 2); // BR
	EXPECT_EQ(ChannelFOpcodeTable::Get(0x91).Size, 2); // BF 1
	EXPECT_EQ(ChannelFOpcodeTable::Get(0x81).Size, 2); // BP
}

TEST(ChannelFOpcodeTable, CorrectMnemonics) {
	EXPECT_STREQ(ChannelFOpcodeTable::Get(0x00).Mnemonic, "lr a,ku");
	EXPECT_STREQ(ChannelFOpcodeTable::Get(0x2b).Mnemonic, "nop");
	EXPECT_STREQ(ChannelFOpcodeTable::Get(0x70).Mnemonic, "clr");
	EXPECT_STREQ(ChannelFOpcodeTable::Get(0x8f).Mnemonic, "br7");
	EXPECT_STREQ(ChannelFOpcodeTable::Get(0x90).Mnemonic, "br");
	EXPECT_STREQ(ChannelFOpcodeTable::Get(0x2a).Mnemonic, "dci");
}
