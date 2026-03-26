#include "pch.h"
#include "ChannelF/ChannelFCoreScaffold.h"
#include "ChannelF/ChannelFOpcodeTable.h"
#include "ChannelF/ChannelFBiosDatabase.h"

TEST(ChannelFCoreScaffold, ResetClearsState) {
	ChannelFCoreScaffold core;
	core.SetPanelButtons(0x0f);
	core.SetRightController(0xaa);
	core.SetLeftController(0x55);
	core.RunFrame();

	core.Reset();

	EXPECT_EQ(core.GetFrameCount(), 0u);
	EXPECT_EQ(core.GetMasterClock(), 0u);
	EXPECT_EQ(core.GetDeterministicState(), 0x6d2b79f5u);
}

TEST(ChannelFCoreScaffold, RunFrameAdvancesDeterministically) {
	ChannelFCoreScaffold a;
	ChannelFCoreScaffold b;

	a.SetPanelButtons(0x03);
	a.SetRightController(0x21);
	a.SetLeftController(0x84);

	b.SetPanelButtons(0x03);
	b.SetRightController(0x21);
	b.SetLeftController(0x84);

	for(int i = 0; i < 120; i++) {
		a.RunFrame();
		b.RunFrame();
	}

	EXPECT_EQ(a.GetFrameCount(), 120u);
	EXPECT_EQ(b.GetFrameCount(), 120u);
	EXPECT_EQ(a.GetMasterClock(), 120ull * ChannelFCoreScaffold::CyclesPerFrame);
	EXPECT_EQ(a.GetMasterClock(), b.GetMasterClock());
	EXPECT_EQ(a.GetDeterministicState(), b.GetDeterministicState());
}

TEST(ChannelFCoreScaffold, SerializeRoundtripPreservesState) {
	ChannelFCoreScaffold original;
	original.SetPanelButtons(0x0a);
	original.SetRightController(0x40);
	original.SetLeftController(0x80);
	for(int i = 0; i < 20; i++) {
		original.RunFrame();
	}

	Serializer saver(1, true, SerializeFormat::Binary);
	original.Serialize(saver);

	std::stringstream ss;
	saver.SaveTo(ss);
	ss.seekg(0);

	ChannelFCoreScaffold loaded;
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loaded.Serialize(loader);

	EXPECT_EQ(loaded.GetFrameCount(), original.GetFrameCount());
	EXPECT_EQ(loaded.GetMasterClock(), original.GetMasterClock());
	EXPECT_EQ(loaded.GetDeterministicState(), original.GetDeterministicState());
}

TEST(ChannelFCoreScaffold, SerializeRoundtripContinuesDeterministically) {
	ChannelFCoreScaffold original;
	original.SetPanelButtons(0x06);
	original.SetRightController(0x12);
	original.SetLeftController(0x48);
	for(int i = 0; i < 40; i++) {
		original.RunFrame();
	}

	Serializer saver(1, true, SerializeFormat::Binary);
	original.Serialize(saver);

	std::stringstream ss;
	saver.SaveTo(ss);
	ss.seekg(0);

	ChannelFCoreScaffold restored;
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	restored.Serialize(loader);

	for(int i = 0; i < 120; i++) {
		original.RunFrame();
		restored.RunFrame();
	}

	EXPECT_EQ(restored.GetFrameCount(), original.GetFrameCount());
	EXPECT_EQ(restored.GetMasterClock(), original.GetMasterClock());
	EXPECT_EQ(restored.GetDeterministicState(), original.GetDeterministicState());
}

TEST(ChannelFCoreScaffold, LuxorHashesSelectSystemIIVariant) {
	ChannelFCoreScaffold core;
	core.DetectVariantFromHashes(
		"759e2ed31fbde4a2d8daf8b9f3e0dffebc90dae2",
		"95d339631d867c8f1d15a5f2ec26069d"
	);

	EXPECT_EQ(core.GetVariant(), ChannelFBiosVariant::SystemII);
}

// ── ChannelFBiosDatabase Tests ──

TEST(ChannelFBiosDatabase, EntryCount_Returns4) {
	EXPECT_EQ(ChannelFBiosDatabase::GetEntryCount(), 4u);
}

TEST(ChannelFBiosDatabase, LookupBySha1_FairchildSL31253) {
	auto* entry = ChannelFBiosDatabase::LookupBySha1("81193965a374d77b99b4743d317824b53c3e3c78");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->Variant, ChannelFBiosVariant::SystemI);
	EXPECT_STREQ(entry->Name, "Channel F BIOS SL31253 (Fairchild)");
}

TEST(ChannelFBiosDatabase, LookupBySha1_FairchildSL31254) {
	auto* entry = ChannelFBiosDatabase::LookupBySha1("8f70d1b74483ba3a37e86cf16c849d601a8c3d2c");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->Variant, ChannelFBiosVariant::SystemI);
	EXPECT_STREQ(entry->Name, "Channel F BIOS SL31254 (Fairchild)");
}

TEST(ChannelFBiosDatabase, LookupBySha1_LuxorSL90025) {
	auto* entry = ChannelFBiosDatabase::LookupBySha1("759e2ed31fbde4a2d8daf8b9f3e0dffebc90dae2");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->Variant, ChannelFBiosVariant::SystemII);
}

TEST(ChannelFBiosDatabase, LookupBySha1_HockeyTennis) {
	auto* entry = ChannelFBiosDatabase::LookupBySha1("4b0a38b5af525aa598907683f0dcfeaa90d242e0");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->Variant, ChannelFBiosVariant::SystemI);
}

TEST(ChannelFBiosDatabase, LookupByMd5_LuxorSL90025) {
	auto* entry = ChannelFBiosDatabase::LookupByMd5("95d339631d867c8f1d15a5f2ec26069d");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->Variant, ChannelFBiosVariant::SystemII);
}

TEST(ChannelFBiosDatabase, LookupBySha1_UnknownHash_ReturnsNull) {
	auto* entry = ChannelFBiosDatabase::LookupBySha1("0000000000000000000000000000000000000000");
	EXPECT_EQ(entry, nullptr);
}

TEST(ChannelFBiosDatabase, LookupByMd5_UnknownHash_ReturnsNull) {
	auto* entry = ChannelFBiosDatabase::LookupByMd5("00000000000000000000000000000000");
	EXPECT_EQ(entry, nullptr);
}

TEST(ChannelFBiosDatabase, DetectVariant_UnknownHashes_ReturnsUnknown) {
	auto variant = ChannelFBiosDatabase::DetectVariant("bad_sha1", "bad_md5");
	EXPECT_EQ(variant, ChannelFBiosVariant::Unknown);
}

TEST(ChannelFBiosDatabase, IsKnownLuxorSystemII_ValidHashes) {
	EXPECT_TRUE(ChannelFBiosDatabase::IsKnownLuxorSystemII(
		"759e2ed31fbde4a2d8daf8b9f3e0dffebc90dae2",
		"95d339631d867c8f1d15a5f2ec26069d"
	));
}

TEST(ChannelFBiosDatabase, IsKnownLuxorSystemII_SystemIHashes_ReturnsFalse) {
	EXPECT_FALSE(ChannelFBiosDatabase::IsKnownLuxorSystemII(
		"81193965a374d77b99b4743d317824b53c3e3c78",
		"ac9804d4c0e9d07e33472e3726ed15c3"
	));
}

// ── ChannelFOpcodeTable Tests ──

TEST(ChannelFOpcodeTable, DefinedOpcodeCount_IsPositive) {
	EXPECT_GT(ChannelFOpcodeTable::GetDefinedOpcodeCount(), 0u);
}

TEST(ChannelFOpcodeTable, Get_DefinedOpcode_HasMnemonic) {
	const auto& info = ChannelFOpcodeTable::Get(0x2b);
	EXPECT_TRUE(info.IsDefined);
	EXPECT_NE(std::string(info.Mnemonic), "ill");
	EXPECT_GE(info.Size, 1);
	EXPECT_GE(info.Cycles, 1);
}

TEST(ChannelFOpcodeTable, Get_UndefinedOpcode_ReturnsIll) {
	const auto& info = ChannelFOpcodeTable::Get(0xff);
	EXPECT_FALSE(info.IsDefined);
	EXPECT_STREQ(info.Mnemonic, "ill");
	EXPECT_EQ(info.Size, 1);
}

TEST(ChannelFOpcodeTable, IsDefined_ConsistentWithGet) {
	for(uint16_t op = 0; op < 256; op++) {
		const auto& info = ChannelFOpcodeTable::Get(static_cast<uint8_t>(op));
		EXPECT_EQ(ChannelFOpcodeTable::IsDefined(static_cast<uint8_t>(op)), info.IsDefined);
	}
}

TEST(ChannelFOpcodeTable, AllOpcodes_HaveValidMetadata) {
	for(uint16_t op = 0; op < 256; op++) {
		const auto& info = ChannelFOpcodeTable::Get(static_cast<uint8_t>(op));
		EXPECT_EQ(info.Opcode, static_cast<uint8_t>(op));
		EXPECT_NE(info.Mnemonic, nullptr);
		EXPECT_GE(info.Size, 1);
		EXPECT_GE(info.Cycles, 1);
	}
}
