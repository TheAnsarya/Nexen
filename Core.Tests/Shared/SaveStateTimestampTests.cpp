#include "pch.h"
#include <gtest/gtest.h>
#include "Shared/SaveStateManager.h"

// =============================================================================
// SaveStateManager::ParseTimestampFromFilename Tests
// =============================================================================
// Tests for the static timestamp parser that extracts Unix timestamps from
// timestamped save state filenames. Covers valid formats, edge cases, and
// malformed inputs. Regression tests for #1188 (DST offset fix).

// --- Valid .nexen-save filenames ---

TEST(SaveStateTimestampTests, ValidNexenSave_ReturnsNonZero) {
	time_t result = SaveStateManager::ParseTimestampFromFilename(
		"TestRom_2026-04-08_14-30-00.nexen-save");
	EXPECT_NE(result, 0);
}

TEST(SaveStateTimestampTests, ValidNexenSave_ParsesCorrectDate) {
	time_t result = SaveStateManager::ParseTimestampFromFilename(
		"TestRom_2026-01-15_12-00-00.nexen-save");
	EXPECT_NE(result, 0);

	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &result);
#else
	localtime_r(&result, &tm);
#endif
	EXPECT_EQ(tm.tm_year + 1900, 2026);
	EXPECT_EQ(tm.tm_mon + 1, 1);   // January
	EXPECT_EQ(tm.tm_mday, 15);
	EXPECT_EQ(tm.tm_hour, 12);
	EXPECT_EQ(tm.tm_min, 0);
	EXPECT_EQ(tm.tm_sec, 0);
}

TEST(SaveStateTimestampTests, ValidNexenSave_Midnight) {
	time_t result = SaveStateManager::ParseTimestampFromFilename(
		"Game_2026-03-01_00-00-00.nexen-save");
	EXPECT_NE(result, 0);

	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &result);
#else
	localtime_r(&result, &tm);
#endif
	EXPECT_EQ(tm.tm_hour, 0);
	EXPECT_EQ(tm.tm_min, 0);
	EXPECT_EQ(tm.tm_sec, 0);
}

TEST(SaveStateTimestampTests, ValidNexenSave_EndOfDay) {
	time_t result = SaveStateManager::ParseTimestampFromFilename(
		"Rom_2026-12-31_23-59-59.nexen-save");
	EXPECT_NE(result, 0);

	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &result);
#else
	localtime_r(&result, &tm);
#endif
	EXPECT_EQ(tm.tm_hour, 23);
	EXPECT_EQ(tm.tm_min, 59);
	EXPECT_EQ(tm.tm_sec, 59);
}

// --- Valid .mss filenames (legacy format) ---

TEST(SaveStateTimestampTests, ValidMss_ReturnsNonZero) {
	time_t result = SaveStateManager::ParseTimestampFromFilename(
		"TestRom_2026-04-08_14-30-00.mss");
	EXPECT_NE(result, 0);
}

TEST(SaveStateTimestampTests, ValidMss_ParsesCorrectDate) {
	time_t result = SaveStateManager::ParseTimestampFromFilename(
		"LegacyRom_2025-06-15_09-45-30.mss");
	EXPECT_NE(result, 0);

	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &result);
#else
	localtime_r(&result, &tm);
#endif
	EXPECT_EQ(tm.tm_year + 1900, 2025);
	EXPECT_EQ(tm.tm_mon + 1, 6);
	EXPECT_EQ(tm.tm_mday, 15);
	EXPECT_EQ(tm.tm_hour, 9);
	EXPECT_EQ(tm.tm_min, 45);
	EXPECT_EQ(tm.tm_sec, 30);
}

// --- DST regression tests (#1188) ---

TEST(SaveStateTimestampTests, DSTMonth_RoundTripsCorrectly) {
	// Timestamps created during DST (e.g., July) should round-trip correctly.
	// Before #1188 fix, tm_isdst=0 caused mktime to assume standard time,
	// producing a 1-hour offset for DST-period timestamps.
	time_t result = SaveStateManager::ParseTimestampFromFilename(
		"Game_2026-07-15_14-30-00.nexen-save");
	EXPECT_NE(result, 0);

	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &result);
#else
	localtime_r(&result, &tm);
#endif
	EXPECT_EQ(tm.tm_hour, 14);  // Must be 14, not 13 or 15
	EXPECT_EQ(tm.tm_min, 30);
}

TEST(SaveStateTimestampTests, StandardTimeMonth_RoundTripsCorrectly) {
	// January is always standard time in most timezones
	time_t result = SaveStateManager::ParseTimestampFromFilename(
		"Game_2026-01-10_08-15-00.nexen-save");
	EXPECT_NE(result, 0);

	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &result);
#else
	localtime_r(&result, &tm);
#endif
	EXPECT_EQ(tm.tm_hour, 8);
	EXPECT_EQ(tm.tm_min, 15);
}

TEST(SaveStateTimestampTests, DSTTransitionMonth_March) {
	// March — DST transition month in many timezones
	time_t result = SaveStateManager::ParseTimestampFromFilename(
		"Game_2026-03-15_02-30-00.nexen-save");
	EXPECT_NE(result, 0);

	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &result);
#else
	localtime_r(&result, &tm);
#endif
	// Should round-trip; exact hour may shift by 1 near DST boundary
	// but the key invariant is that it doesn't return 0
	EXPECT_EQ(tm.tm_min, 30);
}

TEST(SaveStateTimestampTests, DSTTransitionMonth_November) {
	// November — DST fall-back month in many timezones
	time_t result = SaveStateManager::ParseTimestampFromFilename(
		"Game_2026-11-01_01-30-00.nexen-save");
	EXPECT_NE(result, 0);

	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &result);
#else
	localtime_r(&result, &tm);
#endif
	EXPECT_EQ(tm.tm_min, 30);
}

// --- Malformed / edge-case filenames ---

TEST(SaveStateTimestampTests, EmptyString_ReturnsZero) {
	EXPECT_EQ(SaveStateManager::ParseTimestampFromFilename(""), 0);
}

TEST(SaveStateTimestampTests, NoExtension_ReturnsZero) {
	EXPECT_EQ(SaveStateManager::ParseTimestampFromFilename(
		"TestRom_2026-04-08_14-30-00"), 0);
}

TEST(SaveStateTimestampTests, WrongExtension_ReturnsZero) {
	EXPECT_EQ(SaveStateManager::ParseTimestampFromFilename(
		"TestRom_2026-04-08_14-30-00.sav"), 0);
}

TEST(SaveStateTimestampTests, TooShort_ReturnsZero) {
	EXPECT_EQ(SaveStateManager::ParseTimestampFromFilename("short.nexen-save"), 0);
}

TEST(SaveStateTimestampTests, NoTimestamp_ReturnsZero) {
	EXPECT_EQ(SaveStateManager::ParseTimestampFromFilename(
		"TestRom_1.nexen-save"), 0);
}

TEST(SaveStateTimestampTests, GarbageTimestamp_ReturnsZero) {
	EXPECT_EQ(SaveStateManager::ParseTimestampFromFilename(
		"TestRom_XXXX-XX-XX_XX-XX-XX.nexen-save"), 0);
}

TEST(SaveStateTimestampTests, MissingUnderscore_ReturnsZero) {
	// Missing the separator underscore before the date
	EXPECT_EQ(SaveStateManager::ParseTimestampFromFilename(
		"TestRom2026-04-08_14-30-00.nexen-save"), 0);
}

// --- Long ROM names ---

TEST(SaveStateTimestampTests, LongRomName_ParsesCorrectly) {
	time_t result = SaveStateManager::ParseTimestampFromFilename(
		"Super_Long_Game_Name_With_Many_Words_2026-06-15_10-20-30.nexen-save");
	EXPECT_NE(result, 0);

	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &result);
#else
	localtime_r(&result, &tm);
#endif
	EXPECT_EQ(tm.tm_year + 1900, 2026);
	EXPECT_EQ(tm.tm_mon + 1, 6);
	EXPECT_EQ(tm.tm_mday, 15);
}

TEST(SaveStateTimestampTests, RomNameWithUnderscores_ParsesCorrectly) {
	// ROM names often contain underscores — parser must find the right one
	time_t result = SaveStateManager::ParseTimestampFromFilename(
		"Dragon_Quest_III_2026-08-20_16-45-10.nexen-save");
	EXPECT_NE(result, 0);

	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &result);
#else
	localtime_r(&result, &tm);
#endif
	EXPECT_EQ(tm.tm_hour, 16);
	EXPECT_EQ(tm.tm_min, 45);
	EXPECT_EQ(tm.tm_sec, 10);
}

// --- Consistency: same filename always produces same timestamp ---

TEST(SaveStateTimestampTests, DeterministicParsing) {
	const std::string filename = "TestRom_2026-05-20_18-00-00.nexen-save";
	time_t first = SaveStateManager::ParseTimestampFromFilename(filename);
	time_t second = SaveStateManager::ParseTimestampFromFilename(filename);
	EXPECT_EQ(first, second);
	EXPECT_NE(first, 0);
}
