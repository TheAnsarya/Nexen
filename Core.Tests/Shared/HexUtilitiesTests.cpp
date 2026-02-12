#include "pch.h"
#include "Utilities/HexUtilities.h"

// Test fixture for HexUtilities
class HexUtilitiesTest : public ::testing::Test {};

// ===== ToHex(uint8_t) Tests =====

TEST_F(HexUtilitiesTest, ToHex_Uint8_Zero) {
	EXPECT_EQ(HexUtilities::ToHex((uint8_t)0), "00");
}

TEST_F(HexUtilitiesTest, ToHex_Uint8_Max) {
	EXPECT_EQ(HexUtilities::ToHex((uint8_t)0xFF), "FF");
}

TEST_F(HexUtilitiesTest, ToHex_Uint8_MidValues) {
	EXPECT_EQ(HexUtilities::ToHex((uint8_t)0x0A), "0A");
	EXPECT_EQ(HexUtilities::ToHex((uint8_t)0x42), "42");
	EXPECT_EQ(HexUtilities::ToHex((uint8_t)0x80), "80");
	EXPECT_EQ(HexUtilities::ToHex((uint8_t)0xAB), "AB");
}

// ===== ToHex(uint16_t) Tests =====

TEST_F(HexUtilitiesTest, ToHex_Uint16_Zero) {
	EXPECT_EQ(HexUtilities::ToHex((uint16_t)0), "0000");
}

TEST_F(HexUtilitiesTest, ToHex_Uint16_Max) {
	EXPECT_EQ(HexUtilities::ToHex((uint16_t)0xFFFF), "FFFF");
}

TEST_F(HexUtilitiesTest, ToHex_Uint16_MidValues) {
	EXPECT_EQ(HexUtilities::ToHex((uint16_t)0x1234), "1234");
	EXPECT_EQ(HexUtilities::ToHex((uint16_t)0xABCD), "ABCD");
	EXPECT_EQ(HexUtilities::ToHex((uint16_t)0x00FF), "00FF");
}

// ===== ToHex(uint32_t) Tests =====

TEST_F(HexUtilitiesTest, ToHex_Uint32_SmallValue_ReturnsShort) {
	// Without fullSize, small values use minimal digits
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0x42, false), "42");
}

TEST_F(HexUtilitiesTest, ToHex_Uint32_16BitValue_Returns4Digits) {
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0x1234, false), "1234");
}

TEST_F(HexUtilitiesTest, ToHex_Uint32_24BitValue_Returns6Digits) {
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0x123456, false), "123456");
}

TEST_F(HexUtilitiesTest, ToHex_Uint32_FullValue_Returns8Digits) {
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0x12345678, false), "12345678");
}

TEST_F(HexUtilitiesTest, ToHex_Uint32_FullSize_AlwaysReturns8Digits) {
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0x42, true), "00000042");
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0x1234, true), "00001234");
}

// ===== ToHex32 Tests =====

TEST_F(HexUtilitiesTest, ToHex32_Zero) {
	EXPECT_EQ(HexUtilities::ToHex32(0), "00000000");
}

TEST_F(HexUtilitiesTest, ToHex32_Max) {
	EXPECT_EQ(HexUtilities::ToHex32(0xFFFFFFFF), "FFFFFFFF");
}

TEST_F(HexUtilitiesTest, ToHex32_KnownValue) {
	EXPECT_EQ(HexUtilities::ToHex32(0xDEADBEEF), "DEADBEEF");
}

// ===== ToHex24 Tests =====

TEST_F(HexUtilitiesTest, ToHex24_Zero) {
	EXPECT_EQ(HexUtilities::ToHex24(0), "000000");
}

TEST_F(HexUtilitiesTest, ToHex24_KnownValue) {
	EXPECT_EQ(HexUtilities::ToHex24(0x7E2000), "7E2000");
}

// ===== ToHex20 Tests =====

TEST_F(HexUtilitiesTest, ToHex20_Zero) {
	EXPECT_EQ(HexUtilities::ToHex20(0), "00000");
}

TEST_F(HexUtilitiesTest, ToHex20_KnownValue) {
	EXPECT_EQ(HexUtilities::ToHex20(0xFFFFF), "FFFFF");
}

// ===== ToHex(uint64_t) Tests =====

TEST_F(HexUtilitiesTest, ToHex_Uint64_Zero) {
	EXPECT_EQ(HexUtilities::ToHex((uint64_t)0), "0000000000000000");
}

TEST_F(HexUtilitiesTest, ToHex_Uint64_Max) {
	EXPECT_EQ(HexUtilities::ToHex((uint64_t)0xFFFFFFFFFFFFFFFF), "FFFFFFFFFFFFFFFF");
}

TEST_F(HexUtilitiesTest, ToHex_Uint64_KnownValue) {
	EXPECT_EQ(HexUtilities::ToHex((uint64_t)0x0123456789ABCDEF), "0123456789ABCDEF");
}

// ===== ToHex(vector<uint8_t>&) Tests =====

TEST_F(HexUtilitiesTest, ToHex_Vector_NoDelimiter) {
	std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
	EXPECT_EQ(HexUtilities::ToHex(data), "DEADBEEF");
}

TEST_F(HexUtilitiesTest, ToHex_Vector_WithDelimiter) {
	std::vector<uint8_t> data = {0xCA, 0xFE};
	EXPECT_EQ(HexUtilities::ToHex(data, ' '), "CA FE ");
}

TEST_F(HexUtilitiesTest, ToHex_Vector_Empty) {
	std::vector<uint8_t> data = {};
	EXPECT_EQ(HexUtilities::ToHex(data), "");
}

TEST_F(HexUtilitiesTest, ToHex_Vector_SingleByte) {
	std::vector<uint8_t> data = {0x42};
	EXPECT_EQ(HexUtilities::ToHex(data), "42");
}

// ===== FromHex Tests =====

TEST_F(HexUtilitiesTest, FromHex_Uppercase) {
	EXPECT_EQ(HexUtilities::FromHex("FF"), 0xFF);
	EXPECT_EQ(HexUtilities::FromHex("DEADBEEF"), 0xDEADBEEF);
}

TEST_F(HexUtilitiesTest, FromHex_Lowercase) {
	EXPECT_EQ(HexUtilities::FromHex("ff"), 0xFF);
	EXPECT_EQ(HexUtilities::FromHex("abcd"), 0xABCD);
}

TEST_F(HexUtilitiesTest, FromHex_MixedCase) {
	EXPECT_EQ(HexUtilities::FromHex("AbCd"), 0xABCD);
}

TEST_F(HexUtilitiesTest, FromHex_SingleDigit) {
	EXPECT_EQ(HexUtilities::FromHex("0"), 0);
	EXPECT_EQ(HexUtilities::FromHex("F"), 0xF);
}

TEST_F(HexUtilitiesTest, FromHex_Zero) {
	EXPECT_EQ(HexUtilities::FromHex("00"), 0);
	EXPECT_EQ(HexUtilities::FromHex("0000"), 0);
}

TEST_F(HexUtilitiesTest, FromHex_EmptyString) {
	EXPECT_EQ(HexUtilities::FromHex(""), 0);
}

// ===== ToHexChar Tests =====

TEST_F(HexUtilitiesTest, ToHexChar_ReturnsValidCString) {
	const char* result = HexUtilities::ToHexChar(0xAB);
	EXPECT_STREQ(result, "AB");
}

TEST_F(HexUtilitiesTest, ToHexChar_Zero) {
	EXPECT_STREQ(HexUtilities::ToHexChar(0), "00");
}

TEST_F(HexUtilitiesTest, ToHexChar_Max) {
	EXPECT_STREQ(HexUtilities::ToHexChar(0xFF), "FF");
}

// ===== Roundtrip Tests =====

TEST_F(HexUtilitiesTest, Roundtrip_Uint8) {
	for (int i = 0; i <= 255; i++) {
		std::string hex = HexUtilities::ToHex((uint8_t)i);
		int parsed = HexUtilities::FromHex(hex);
		EXPECT_EQ(parsed, i) << "Failed roundtrip for value " << i;
	}
}

TEST_F(HexUtilitiesTest, Roundtrip_Uint16) {
	// Test boundary values and a few mid-range
	uint16_t values[] = {0, 1, 0xFF, 0x100, 0x1234, 0x7FFF, 0x8000, 0xFFFF};
	for (uint16_t val : values) {
		std::string hex = HexUtilities::ToHex(val);
		int parsed = HexUtilities::FromHex(hex);
		EXPECT_EQ(parsed, val) << "Failed roundtrip for " << val;
	}
}

// ===== Exhaustive Optimized Function Tests =====

TEST_F(HexUtilitiesTest, ToHex20_AllBoundaries) {
	// ToHex20 uses direct char buf[5] construction — verify all boundaries
	struct TestCase { uint32_t input; const char* expected; };
	TestCase cases[] = {
		{0x00000, "00000"},
		{0x00001, "00001"},
		{0x0000F, "0000F"},
		{0x00010, "00010"},
		{0x000FF, "000FF"},
		{0x00100, "00100"},
		{0x00FFF, "00FFF"},
		{0x01000, "01000"},
		{0x0FFFF, "0FFFF"},
		{0x10000, "10000"},
		{0x12345, "12345"},
		{0x7FFFF, "7FFFF"},
		{0x80000, "80000"},
		{0xABCDE, "ABCDE"},
		{0xFFFFF, "FFFFF"},
	};
	for (const auto& tc : cases) {
		EXPECT_EQ(HexUtilities::ToHex20(tc.input), tc.expected)
			<< "ToHex20 failed for 0x" << std::hex << tc.input;
	}
}

TEST_F(HexUtilitiesTest, ToHex24_AllBoundaries) {
	// ToHex24 uses direct char buf[6] construction from 3 byte pairs
	struct TestCase { uint32_t input; const char* expected; };
	TestCase cases[] = {
		{0x000000, "000000"},
		{0x000001, "000001"},
		{0x00000F, "00000F"},
		{0x0000FF, "0000FF"},
		{0x000100, "000100"},
		{0x00FFFF, "00FFFF"},
		{0x010000, "010000"},
		{0x123456, "123456"},
		{0x7FFFFF, "7FFFFF"},
		{0x800000, "800000"},
		{0xABCDEF, "ABCDEF"},
		{0xFFFFFF, "FFFFFF"},
	};
	for (const auto& tc : cases) {
		EXPECT_EQ(HexUtilities::ToHex24(tc.input), tc.expected)
			<< "ToHex24 failed for 0x" << std::hex << tc.input;
	}
}

TEST_F(HexUtilitiesTest, ToHex32_AllBoundaries) {
	// ToHex32 uses direct char buf[8] construction from 4 byte pairs
	struct TestCase { uint32_t input; const char* expected; };
	TestCase cases[] = {
		{0x00000000, "00000000"},
		{0x00000001, "00000001"},
		{0x000000FF, "000000FF"},
		{0x0000FF00, "0000FF00"},
		{0x00FF0000, "00FF0000"},
		{0xFF000000, "FF000000"},
		{0x12345678, "12345678"},
		{0xDEADBEEF, "DEADBEEF"},
		{0x7FFFFFFF, "7FFFFFFF"},
		{0x80000000, "80000000"},
		{0xFFFFFFFF, "FFFFFFFF"},
	};
	for (const auto& tc : cases) {
		EXPECT_EQ(HexUtilities::ToHex32(tc.input), tc.expected)
			<< "ToHex32 failed for 0x" << std::hex << tc.input;
	}
}

TEST_F(HexUtilitiesTest, ToHex64_AllBoundaries) {
	// ToHex(uint64_t) uses loop with char buf[16]
	struct TestCase { uint64_t input; const char* expected; };
	TestCase cases[] = {
		{0x0000000000000000ULL, "0000000000000000"},
		{0x0000000000000001ULL, "0000000000000001"},
		{0x00000000000000FFULL, "00000000000000FF"},
		{0x000000000000FF00ULL, "000000000000FF00"},
		{0x0000000000FF0000ULL, "0000000000FF0000"},
		{0x00000000FF000000ULL, "00000000FF000000"},
		{0x000000FF00000000ULL, "000000FF00000000"},
		{0x0000FF0000000000ULL, "0000FF0000000000"},
		{0x00FF000000000000ULL, "00FF000000000000"},
		{0xFF00000000000000ULL, "FF00000000000000"},
		{0x0123456789ABCDEFULL, "0123456789ABCDEF"},
		{0xDEADBEEFCAFEBABEULL, "DEADBEEFCAFEBABE"},
		{0x7FFFFFFFFFFFFFFFULL, "7FFFFFFFFFFFFFFF"},
		{0x8000000000000000ULL, "8000000000000000"},
		{0xFFFFFFFFFFFFFFFFULL, "FFFFFFFFFFFFFFFF"},
	};
	for (const auto& tc : cases) {
		EXPECT_EQ(HexUtilities::ToHex(tc.input), tc.expected)
			<< "ToHex(uint64_t) failed for input";
	}
}

TEST_F(HexUtilitiesTest, ToHexAutoSize_DispatchesCorrectly) {
	// ToHex(uint32_t, bool fullSize) delegates to different widths
	// fullSize=false: auto-selects smallest representation
	// fullSize=true: always returns 8 chars (ToHex32)

	// fullSize=true always gives 8 chars
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0x00, true), "00000000");
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0xFF, true), "000000FF");
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0xFFFF, true), "0000FFFF");
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0xFFFFFFFF, true), "FFFFFFFF");

	// fullSize=false auto-sizes
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0x00, false), "00");       // 8-bit
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0xFF, false), "FF");       // 8-bit
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0x100, false), "0100");    // 16-bit
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0xFFFF, false), "FFFF");   // 16-bit
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0x10000, false), "010000"); // 24-bit
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0xFFFFFF, false), "FFFFFF"); // 24-bit
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0x1000000, false), "01000000"); // 32-bit
	EXPECT_EQ(HexUtilities::ToHex((uint32_t)0xFFFFFFFF, false), "FFFFFFFF"); // 32-bit
}

TEST_F(HexUtilitiesTest, ToHex_ConsistentWithFromHex) {
	// Comprehensive roundtrip for all ToHex widths
	uint32_t values32[] = {0, 1, 127, 128, 255, 256, 1000, 0xFFFF, 0x10000, 0xFFFFFF, 0x1000000, 0xFFFFFFFF};
	for (uint32_t val : values32) {
		std::string hex = HexUtilities::ToHex(val, true);
		int parsed = HexUtilities::FromHex(hex);
		EXPECT_EQ(static_cast<uint32_t>(parsed), val) << "Roundtrip failed for " << val;
	}
}

// ===== Before/After Comparison: String Concat vs Buffer Construction =====

/// <summary>
/// These tests verify the optimized buffer-based ToHex functions produce
/// identical output to a reference implementation using string concatenation
/// (the old approach). This proves the optimization is safe.
/// </summary>
class HexUtilitiesComparisonTest : public ::testing::Test {
protected:
	/// Reference implementation: old string concatenation approach for 16-bit
	static std::string ToHex16_OldConcat(uint16_t value) {
		return HexUtilities::ToHex((uint8_t)(value >> 8))
			 + HexUtilities::ToHex((uint8_t)(value & 0xFF));
	}

	/// Reference implementation: old string concatenation approach for 24-bit
	static std::string ToHex24_OldConcat(int32_t value) {
		return HexUtilities::ToHex((uint8_t)((value >> 16) & 0xFF))
			 + HexUtilities::ToHex((uint8_t)((value >> 8) & 0xFF))
			 + HexUtilities::ToHex((uint8_t)(value & 0xFF));
	}

	/// Reference implementation: old string concatenation approach for 32-bit
	static std::string ToHex32_OldConcat(uint32_t value) {
		return HexUtilities::ToHex((uint8_t)(value >> 24))
			 + HexUtilities::ToHex((uint8_t)((value >> 16) & 0xFF))
			 + HexUtilities::ToHex((uint8_t)((value >> 8) & 0xFF))
			 + HexUtilities::ToHex((uint8_t)(value & 0xFF));
	}

	/// Reference implementation: old string concatenation approach for 64-bit
	static std::string ToHex64_OldConcat(uint64_t value) {
		std::string result;
		for (int i = 7; i >= 0; i--) {
			result += HexUtilities::ToHex((uint8_t)((value >> (i * 8)) & 0xFF));
		}
		return result;
	}
};

TEST_F(HexUtilitiesComparisonTest, ToHex16_Exhaustive_AllValues) {
	// Compare optimized buffer construction vs old string concat for ALL 65536 uint16_t values
	for (int v = 0; v <= 0xFFFF; v++) {
		uint16_t value = static_cast<uint16_t>(v);
		std::string optimized = HexUtilities::ToHex(value);
		std::string reference = ToHex16_OldConcat(value);
		EXPECT_EQ(optimized, reference)
			<< "ToHex(uint16_t) mismatch for 0x" << std::hex << v;
	}
}

TEST_F(HexUtilitiesComparisonTest, ToHex24_AllByteBoundaries) {
	// Test every byte boundary combination for 24-bit
	for (int hi = 0; hi < 256; hi += 17) {
		for (int mid = 0; mid < 256; mid += 17) {
			for (int lo = 0; lo < 256; lo += 17) {
				int32_t value = (hi << 16) | (mid << 8) | lo;
				std::string optimized = HexUtilities::ToHex24(value);
				std::string reference = ToHex24_OldConcat(value);
				EXPECT_EQ(optimized, reference)
					<< "ToHex24 mismatch for 0x" << std::hex << value;
			}
		}
	}
}

TEST_F(HexUtilitiesComparisonTest, ToHex32_AllByteBoundaries) {
	// Test each byte position independently with all 256 values
	uint32_t baseValues[] = {0x00000000, 0x12345678, 0xDEADBEEF, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF};
	for (uint32_t base : baseValues) {
		std::string optimized = HexUtilities::ToHex32(base);
		std::string reference = ToHex32_OldConcat(base);
		EXPECT_EQ(optimized, reference) << "ToHex32 mismatch for 0x" << std::hex << base;
	}

	// Sweep each byte position
	for (int bytePos = 0; bytePos < 4; bytePos++) {
		for (int b = 0; b < 256; b++) {
			uint32_t value = static_cast<uint32_t>(b) << (bytePos * 8);
			std::string optimized = HexUtilities::ToHex32(value);
			std::string reference = ToHex32_OldConcat(value);
			EXPECT_EQ(optimized, reference)
				<< "ToHex32 byte sweep mismatch at pos " << bytePos << " byte 0x" << std::hex << b;
		}
	}
}

TEST_F(HexUtilitiesComparisonTest, ToHex64_AllByteBoundaries) {
	// Test each byte position independently for 64-bit
	uint64_t baseValues[] = {
		0x0000000000000000ULL, 0x0123456789ABCDEFULL,
		0xDEADBEEFCAFEBABEULL, 0x7FFFFFFFFFFFFFFFULL,
		0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL
	};
	for (uint64_t base : baseValues) {
		std::string optimized = HexUtilities::ToHex(base);
		std::string reference = ToHex64_OldConcat(base);
		EXPECT_EQ(optimized, reference) << "ToHex(uint64_t) mismatch";
	}

	// Sweep each byte position
	for (int bytePos = 0; bytePos < 8; bytePos++) {
		for (int b = 0; b < 256; b++) {
			uint64_t value = static_cast<uint64_t>(b) << (bytePos * 8);
			std::string optimized = HexUtilities::ToHex(value);
			std::string reference = ToHex64_OldConcat(value);
			EXPECT_EQ(optimized, reference)
				<< "ToHex(uint64_t) byte sweep mismatch at pos " << bytePos << " byte 0x" << std::hex << b;
		}
	}
}

TEST_F(HexUtilitiesComparisonTest, ToHex20_VsManualConstruction) {
	// ToHex20 produces 5-char output: hi nibble + 2 byte pairs
	// Reference: build from individual byte ToHex + nibble extraction
	uint32_t values[] = {
		0x00000, 0x00001, 0x0000F, 0x00010, 0x000FF, 0x00100,
		0x00FFF, 0x01000, 0x0FFFF, 0x10000, 0x12345, 0x7FFFF,
		0x80000, 0xABCDE, 0xFFFFF
	};
	for (uint32_t val : values) {
		std::string optimized = HexUtilities::ToHex20(val);
		// Reference: manual hex construction
		char ref[6];
		const char hexDigits[] = "0123456789ABCDEF";
		ref[0] = hexDigits[(val >> 16) & 0xF];
		ref[1] = hexDigits[(val >> 12) & 0xF];
		ref[2] = hexDigits[(val >> 8) & 0xF];
		ref[3] = hexDigits[(val >> 4) & 0xF];
		ref[4] = hexDigits[val & 0xF];
		ref[5] = '\0';
		EXPECT_EQ(optimized, std::string(ref))
			<< "ToHex20 mismatch for 0x" << std::hex << val;
	}
}

// ===== FromHex LUT Correctness Tests =====
// Verify the constexpr nibble LUT produces identical results for all input chars

TEST_F(HexUtilitiesTest, FromHex_LUT_AllValidHexDigits) {
	// Every valid hex digit should parse correctly
	for (char c = '0'; c <= '9'; c++) {
		std::string s(1, c);
		EXPECT_EQ(HexUtilities::FromHex(s), c - '0')
			<< "Failed for digit '" << c << "'";
	}
	for (char c = 'A'; c <= 'F'; c++) {
		std::string s(1, c);
		EXPECT_EQ(HexUtilities::FromHex(s), 10 + c - 'A')
			<< "Failed for char '" << c << "'";
	}
	for (char c = 'a'; c <= 'f'; c++) {
		std::string s(1, c);
		EXPECT_EQ(HexUtilities::FromHex(s), 10 + c - 'a')
			<< "Failed for char '" << c << "'";
	}
}

TEST_F(HexUtilitiesTest, FromHex_LUT_Exhaustive_AllTwoDigitValues) {
	// Verify all 256 possible two-digit hex values parse correctly
	const char hexDigits[] = "0123456789ABCDEF";
	for (int hi = 0; hi < 16; hi++) {
		for (int lo = 0; lo < 16; lo++) {
			char buf[3] = {hexDigits[hi], hexDigits[lo], '\0'};
			int expected = (hi << 4) | lo;
			EXPECT_EQ(HexUtilities::FromHex(buf), expected)
				<< "Failed for " << buf;
		}
	}
}

TEST_F(HexUtilitiesTest, FromHex_LUT_Exhaustive_AllTwoDigitValues_Lowercase) {
	const char hexDigits[] = "0123456789abcdef";
	for (int hi = 0; hi < 16; hi++) {
		for (int lo = 0; lo < 16; lo++) {
			char buf[3] = {hexDigits[hi], hexDigits[lo], '\0'};
			int expected = (hi << 4) | lo;
			EXPECT_EQ(HexUtilities::FromHex(buf), expected)
				<< "Failed for " << buf;
		}
	}
}