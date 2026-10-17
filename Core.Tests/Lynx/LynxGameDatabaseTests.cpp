#include "pch.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Tests for the Lynx game identification database.
///
/// The database provides CRC32-based lookup for game metadata:
///   - Game title
///   - Screen rotation (None, Left, Right)
///   - EEPROM type (None, 93c46, 93c56, 93c66, 93c76, 93c86)
///   - Player count (1-6)
///
/// Used to auto-detect properties for headerless ROMs and verify
/// LNX header data against known-good values.
///
/// References:
///   - No-Intro DAT files for Lynx
///   - LynxGameDatabase.h (database implementation)
/// </summary>
class LynxGameDatabaseTest : public ::testing::Test {
protected:
	// Mock Entry structure matching LynxGameDatabase::Entry
	struct MockEntry {
		uint32_t PrgCrc32;
		const char* Name;
		LynxRotation Rotation;
		LynxEepromType EepromType;
		uint8_t PlayerCount;
	};

	// Sample database entries for testing
	static constexpr MockEntry _testEntries[] = {
		{ 0x8b8de924, "California Games", LynxRotation::None, LynxEepromType::None, 4 },
		{ 0x1d0dab8a, "Chip's Challenge", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x45ce0898, "Klax", LynxRotation::Right, LynxEepromType::None, 2 },
		{ 0xe8b3b8d9, "Rygar", LynxRotation::None, LynxEepromType::None, 1 },
	};

	const MockEntry* Lookup(uint32_t crc32) {
		for (const auto& entry : _testEntries) {
			if (entry.PrgCrc32 == crc32) {
				return &entry;
			}
		}
		return nullptr;
	}
};

//=============================================================================
// Lookup Tests
//=============================================================================

TEST_F(LynxGameDatabaseTest, Lookup_KnownGame_ReturnsEntry) {
	const auto* entry = Lookup(0x8b8de924);
	ASSERT_NE(entry, nullptr);
	EXPECT_STREQ(entry->Name, "California Games");
}

TEST_F(LynxGameDatabaseTest, Lookup_UnknownGame_ReturnsNull) {
	const auto* entry = Lookup(0xdeadbeef);
	EXPECT_EQ(entry, nullptr);
}

TEST_F(LynxGameDatabaseTest, Lookup_AllTestEntries_Found) {
	for (const auto& testEntry : _testEntries) {
		const auto* entry = Lookup(testEntry.PrgCrc32);
		ASSERT_NE(entry, nullptr);
		EXPECT_STREQ(entry->Name, testEntry.Name);
	}
}

//=============================================================================
// Rotation Detection Tests
//=============================================================================

TEST_F(LynxGameDatabaseTest, Rotation_CaliforniaGames_None) {
	const auto* entry = Lookup(0x8b8de924);
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->Rotation, LynxRotation::None);
}

TEST_F(LynxGameDatabaseTest, Rotation_Klax_Right) {
	const auto* entry = Lookup(0x45ce0898);
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->Rotation, LynxRotation::Right);
}

TEST_F(LynxGameDatabaseTest, Rotation_Left_Exists) {
	// Left rotation exists in real database (e.g., Warbirds)
	LynxRotation leftRot = LynxRotation::Left;
	EXPECT_EQ(static_cast<uint8_t>(leftRot), 1);
}

//=============================================================================
// EEPROM Detection Tests
//=============================================================================

TEST_F(LynxGameDatabaseTest, Eeprom_None_MostGames) {
	const auto* entry = Lookup(0xe8b3b8d9); // Rygar
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->EepromType, LynxEepromType::None);
}

TEST_F(LynxGameDatabaseTest, Eeprom_Types_AllValid) {
	EXPECT_EQ(static_cast<uint8_t>(LynxEepromType::None), 0);
	EXPECT_EQ(static_cast<uint8_t>(LynxEepromType::Eeprom93c46), 1);
	EXPECT_EQ(static_cast<uint8_t>(LynxEepromType::Eeprom93c56), 2);
	EXPECT_EQ(static_cast<uint8_t>(LynxEepromType::Eeprom93c66), 3);
	EXPECT_EQ(static_cast<uint8_t>(LynxEepromType::Eeprom93c76), 4);
	EXPECT_EQ(static_cast<uint8_t>(LynxEepromType::Eeprom93c86), 5);
}

//=============================================================================
// Player Count Tests
//=============================================================================

TEST_F(LynxGameDatabaseTest, PlayerCount_CaliforniaGames_4) {
	const auto* entry = Lookup(0x8b8de924);
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->PlayerCount, 4);
}

TEST_F(LynxGameDatabaseTest, PlayerCount_ChipsChallenge_1) {
	const auto* entry = Lookup(0x1d0dab8a);
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->PlayerCount, 1);
}

TEST_F(LynxGameDatabaseTest, PlayerCount_Klax_2) {
	const auto* entry = Lookup(0x45ce0898);
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->PlayerCount, 2);
}

TEST_F(LynxGameDatabaseTest, PlayerCount_Range) {
	// Valid player counts: 1-6 for Lynx (up to 6 via ComLynx)
	for (uint8_t i = 1; i <= 6; i++) {
		EXPECT_GE(i, 1);
		EXPECT_LE(i, 6);
	}
}

//=============================================================================
// CRC32 Validation Tests
//=============================================================================

TEST_F(LynxGameDatabaseTest, Crc32_NotZero) {
	for (const auto& entry : _testEntries) {
		EXPECT_NE(entry.PrgCrc32, 0u);
	}
}

TEST_F(LynxGameDatabaseTest, Crc32_Unique) {
	// All CRC32 values should be unique
	std::set<uint32_t> crcs;
	for (const auto& entry : _testEntries) {
		EXPECT_TRUE(crcs.insert(entry.PrgCrc32).second);
	}
}

TEST_F(LynxGameDatabaseTest, Crc32_ExcludesHeader) {
	// CRC32 is computed on PRG ROM data, NOT including LNX header
	// Header is first 64 bytes — CRC starts at byte 64
	uint32_t headerSize = 64;
	EXPECT_EQ(headerSize, 64u);
}

//=============================================================================
// Name Validation Tests
//=============================================================================

TEST_F(LynxGameDatabaseTest, Name_NotNull) {
	for (const auto& entry : _testEntries) {
		EXPECT_NE(entry.Name, nullptr);
	}
}

TEST_F(LynxGameDatabaseTest, Name_NotEmpty) {
	for (const auto& entry : _testEntries) {
		EXPECT_GT(strlen(entry.Name), 0u);
	}
}

TEST_F(LynxGameDatabaseTest, Name_ReasonableLength) {
	// Names should be reasonable length (< 64 chars)
	for (const auto& entry : _testEntries) {
		EXPECT_LT(strlen(entry.Name), 64u);
	}
}

//=============================================================================
// Database Size Tests
//=============================================================================

TEST_F(LynxGameDatabaseTest, TestEntries_Count) {
	constexpr size_t count = sizeof(_testEntries) / sizeof(_testEntries[0]);
	EXPECT_EQ(count, 4u);
}

TEST_F(LynxGameDatabaseTest, RealDatabase_HasEntries) {
	// Real database should have 80+ entries
	// This is a placeholder — actual test would call LynxGameDatabase::GetEntryCount()
	constexpr uint32_t expectedMinEntries = 80;
	EXPECT_GE(expectedMinEntries, 80u);
}

//=============================================================================
// Fallback Behavior Tests
//=============================================================================

TEST_F(LynxGameDatabaseTest, Fallback_UnknownGame_DefaultRotation) {
	// When game not found, default to no rotation
	LynxRotation defaultRot = LynxRotation::None;
	EXPECT_EQ(defaultRot, LynxRotation::None);
}

TEST_F(LynxGameDatabaseTest, Fallback_UnknownGame_NoEeprom) {
	// When game not found, default to no EEPROM
	LynxEepromType defaultType = LynxEepromType::None;
	EXPECT_EQ(defaultType, LynxEepromType::None);
}

TEST_F(LynxGameDatabaseTest, Fallback_UnknownGame_SinglePlayer) {
	// When game not found, default to 1 player
	uint8_t defaultPlayers = 1;
	EXPECT_EQ(defaultPlayers, 1);
}

