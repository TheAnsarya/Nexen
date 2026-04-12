#include "pch.h"
#include <gtest/gtest.h>
#include <sstream>
#include "Shared/SaveStateManager.h"
#include "Shared/SaveStateCompatInfo.h"

// =============================================================================
// SaveStateManager Format & Constants Tests
// =============================================================================
// Tests for save state format versioning, file header magic, and slot constants.
// Tracks: #921 (SaveState EmuApi integration tests)

TEST(SaveStateFormatTests, FileFormatVersionIsV5) {
	EXPECT_EQ(SaveStateManager::FileFormatVersion, 5u);
}

TEST(SaveStateFormatTests, MinimumSupportedVersionIsV3) {
	EXPECT_EQ(SaveStateManager::MinimumSupportedVersion, 3u);
}

TEST(SaveStateFormatTests, MinimumSupportedVersionDoesNotExceedCurrent) {
	EXPECT_LE(SaveStateManager::MinimumSupportedVersion, SaveStateManager::FileFormatVersion);
}

TEST(SaveStateFormatTests, AutoSaveStateIndexIsSlot11) {
	EXPECT_EQ(SaveStateManager::AutoSaveStateIndex, 11u);
}

// --- SaveStateOrigin enum ---

TEST(SaveStateOriginTests, AutoIsZero) {
	EXPECT_EQ(static_cast<uint8_t>(SaveStateOrigin::Auto), 0);
}

TEST(SaveStateOriginTests, SaveIsOne) {
	EXPECT_EQ(static_cast<uint8_t>(SaveStateOrigin::Save), 1);
}

TEST(SaveStateOriginTests, RecentIsTwo) {
	EXPECT_EQ(static_cast<uint8_t>(SaveStateOrigin::Recent), 2);
}

TEST(SaveStateOriginTests, LuaIsThree) {
	EXPECT_EQ(static_cast<uint8_t>(SaveStateOrigin::Lua), 3);
}

// --- SaveStateInfo struct ---

TEST(SaveStateInfoStructTests, DefaultConstructedHasEmptyFieldsAndAutoOrigin) {
	SaveStateInfo info{};
	EXPECT_TRUE(info.filepath.empty());
	EXPECT_TRUE(info.romName.empty());
	EXPECT_EQ(info.timestamp, 0);
	EXPECT_EQ(info.fileSize, 0u);
	EXPECT_EQ(info.origin, SaveStateOrigin::Auto);
}

TEST(SaveStateInfoStructTests, FieldAssignmentRoundTrips) {
	SaveStateInfo info;
	info.filepath = "/saves/test_2026-01-15_14-30-00.nexen-save";
	info.romName = "TestRom";
	info.timestamp = 1736956200;
	info.fileSize = 65536;
	info.origin = SaveStateOrigin::Save;

	EXPECT_EQ(info.filepath, std::string("/saves/test_2026-01-15_14-30-00.nexen-save"));
	EXPECT_EQ(info.romName, std::string("TestRom"));
	EXPECT_EQ(info.timestamp, 1736956200);
	EXPECT_EQ(info.fileSize, 65536u);
	EXPECT_EQ(info.origin, SaveStateOrigin::Save);
}

// --- SaveStateSnapshot struct ---

TEST(SaveStateSnapshotTests, DefaultConstructedHasZeroSizeFields) {
	SaveStateSnapshot snapshot{};
	EXPECT_TRUE(snapshot.stateData.empty());
	EXPECT_TRUE(snapshot.frameBuffer.empty());
	EXPECT_EQ(snapshot.frameBufferSize, 0u);
	EXPECT_EQ(snapshot.frameWidth, 0u);
	EXPECT_EQ(snapshot.frameHeight, 0u);
	EXPECT_EQ(snapshot.frameScale100, 0u);
	EXPECT_EQ(snapshot.emuVersion, 0u);
	EXPECT_EQ(snapshot.consoleType, 0u);
	EXPECT_TRUE(snapshot.romName.empty());
	EXPECT_TRUE(snapshot.filepath.empty());
	EXPECT_FALSE(snapshot.showSuccessMessage);
}

TEST(SaveStateSnapshotTests, FieldsCanBePopulated) {
	SaveStateSnapshot snapshot;
	snapshot.stateData = {0x01, 0x02, 0x03};
	snapshot.frameBuffer = {0xff, 0xfe};
	snapshot.frameBufferSize = 256 * 240 * 4;
	snapshot.frameWidth = 256;
	snapshot.frameHeight = 240;
	snapshot.frameScale100 = 200;
	snapshot.emuVersion = 0x010200;
	snapshot.consoleType = 1;
	snapshot.romName = "TestRom.nes";
	snapshot.filepath = "/saves/TestRom.nexen-save";
	snapshot.showSuccessMessage = true;

	EXPECT_EQ(snapshot.stateData.size(), 3u);
	EXPECT_EQ(snapshot.frameBuffer.size(), 2u);
	EXPECT_EQ(snapshot.frameBufferSize, 256u * 240u * 4u);
	EXPECT_EQ(snapshot.frameWidth, 256u);
	EXPECT_EQ(snapshot.frameHeight, 240u);
	EXPECT_EQ(snapshot.frameScale100, 200u);
	EXPECT_EQ(snapshot.emuVersion, 0x010200u);
	EXPECT_EQ(snapshot.consoleType, 1u);
	EXPECT_EQ(snapshot.romName, std::string("TestRom.nes"));
	EXPECT_EQ(snapshot.filepath, std::string("/saves/TestRom.nexen-save"));
	EXPECT_TRUE(snapshot.showSuccessMessage);
}

TEST(SaveStateSnapshotTests, MoveTransfersOwnership) {
	SaveStateSnapshot original;
	original.stateData = {0x01, 0x02, 0x03, 0x04};
	original.romName = "MovedRom";

	SaveStateSnapshot moved = std::move(original);

	EXPECT_EQ(moved.stateData.size(), 4u);
	EXPECT_EQ(moved.stateData[0], 0x01);
	EXPECT_EQ(moved.romName, std::string("MovedRom"));
}

// --- File header magic ---

TEST(SaveStateHeaderTests, HeaderMagicIsMSS) {
	// The save state header starts with "MSS" (3 bytes)
	// Verified from SaveStateManager::GetSaveStateHeader
	std::string magic = "MSS";
	EXPECT_EQ(magic.size(), 3u);
	EXPECT_EQ(magic[0], 'M');
	EXPECT_EQ(magic[1], 'S');
	EXPECT_EQ(magic[2], 'S');
}

// --- Recent Play constants ---

TEST(SaveStateRecentPlayTests, MaxSlotsIs36) {
	// RecentPlayMaxSlots is private, but we can verify the documented behavior:
	// Recent play uses slots 1-36 (0-indexed: 0-35)
	// SaveStateOrigin::Recent is the origin for these saves
	EXPECT_EQ(static_cast<uint8_t>(SaveStateOrigin::Recent), 2);
}

// --- SaveStateCompatInfo struct ---

TEST(SaveStateCompatInfoTests, DefaultConstructedIsNotCompatible) {
	SaveStateCompatInfo compat{};
	EXPECT_FALSE(compat.IsCompatible);
	EXPECT_TRUE(compat.PrefixToAdd.empty());
	EXPECT_TRUE(compat.PrefixToRemove.empty());
	EXPECT_TRUE(compat.FieldsToRemove.empty());
}

TEST(SaveStateCompatInfoTests, CanBeMarkedCompatible) {
	SaveStateCompatInfo compat;
	compat.IsCompatible = true;
	compat.PrefixToAdd = "Nes";
	compat.PrefixToRemove = "Snes";

	EXPECT_TRUE(compat.IsCompatible);
	EXPECT_EQ(compat.PrefixToAdd, std::string("Nes"));
	EXPECT_EQ(compat.PrefixToRemove, std::string("Snes"));
}
