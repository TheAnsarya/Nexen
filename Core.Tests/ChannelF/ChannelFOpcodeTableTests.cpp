#include "pch.h"
#include "ChannelF/ChannelFOpcodeTable.h"

TEST(ChannelFOpcodeTable, ReturnsEntryForAllOpcodes) {
	for(uint32_t opcode = 0; opcode < ChannelFOpcodeTable::OpcodeCount; opcode++) {
		const ChannelFOpcodeInfo& info = ChannelFOpcodeTable::Get((uint8_t)opcode);
		EXPECT_EQ(info.Opcode, opcode);
		EXPECT_NE(info.Mnemonic, nullptr);
		EXPECT_GE(info.Size, 1);
		EXPECT_GE(info.Cycles, 1);
	}
}

TEST(ChannelFOpcodeTable, KnownBaselineEntriesAreDefined) {
	EXPECT_TRUE(ChannelFOpcodeTable::IsDefined(0x00));
	EXPECT_TRUE(ChannelFOpcodeTable::IsDefined(0x08));
	EXPECT_TRUE(ChannelFOpcodeTable::IsDefined(0x2b));
	EXPECT_TRUE(ChannelFOpcodeTable::IsDefined(0x90));
	EXPECT_TRUE(ChannelFOpcodeTable::IsDefined(0xf0));
}

TEST(ChannelFOpcodeTable, UnknownEntriesDefaultToIll) {
	const ChannelFOpcodeInfo& info = ChannelFOpcodeTable::Get(0xff);
	EXPECT_STREQ(info.Mnemonic, "ill");
	EXPECT_FALSE(info.IsDefined);
	EXPECT_EQ(info.Size, 1);
	EXPECT_EQ(info.Cycles, 1);
}

TEST(ChannelFOpcodeTable, DefinedOpcodeCountMatchesScaffold) {
	EXPECT_EQ(ChannelFOpcodeTable::GetDefinedOpcodeCount(), 5u);
}
