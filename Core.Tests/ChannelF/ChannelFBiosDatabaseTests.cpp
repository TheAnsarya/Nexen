#include "pch.h"
#include "ChannelF/ChannelFBiosDatabase.h"

TEST(ChannelFBiosDatabase, LookupLuxorBySha1_IsSystemII) {
	const ChannelFBiosDatabase::Entry* entry = ChannelFBiosDatabase::LookupBySha1("759E2ED31FBDE4A2D8DAF8B9F3E0DFFEBC90DAE2");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->Variant, ChannelFBiosVariant::SystemII);
}

TEST(ChannelFBiosDatabase, LookupLuxorByMd5_IsSystemII) {
	const ChannelFBiosDatabase::Entry* entry = ChannelFBiosDatabase::LookupByMd5("95D339631D867C8F1D15A5F2EC26069D");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->Variant, ChannelFBiosVariant::SystemII);
}

TEST(ChannelFBiosDatabase, DetectVariant_UsesSha1OrMd5) {
	EXPECT_EQ(
		ChannelFBiosDatabase::DetectVariant("759e2ed31fbde4a2d8daf8b9f3e0dffebc90dae2", ""),
		ChannelFBiosVariant::SystemII
	);
	EXPECT_EQ(
		ChannelFBiosDatabase::DetectVariant("", "95d339631d867c8f1d15a5f2ec26069d"),
		ChannelFBiosVariant::SystemII
	);
}

TEST(ChannelFBiosDatabase, IsKnownLuxorSystemII_DetectsExpectedHashes) {
	EXPECT_TRUE(ChannelFBiosDatabase::IsKnownLuxorSystemII("759e2ed31fbde4a2d8daf8b9f3e0dffebc90dae2", ""));
	EXPECT_TRUE(ChannelFBiosDatabase::IsKnownLuxorSystemII("", "95d339631d867c8f1d15a5f2ec26069d"));
	EXPECT_FALSE(ChannelFBiosDatabase::IsKnownLuxorSystemII("81193965a374d77b99b4743d317824b53c3e3c78", "ac9804d4c0e9d07e33472e3726ed15c3"));
}

TEST(ChannelFBiosDatabase, EntryCount_IsAtLeastFour) {
	EXPECT_GE(ChannelFBiosDatabase::GetEntryCount(), 4u);
}
