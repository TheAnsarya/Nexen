#include "pch.h"
#include "Utilities/Base64.h"

// Test fixture for Base64
class Base64Test : public ::testing::Test {};

// ===== Encode Tests =====

TEST_F(Base64Test, Encode_Empty) {
	std::vector<uint8_t> data = {};
	EXPECT_EQ(Base64::Encode(data), "");
}

TEST_F(Base64Test, Encode_SingleByte) {
	std::vector<uint8_t> data = {0x66}; // 'f'
	EXPECT_EQ(Base64::Encode(data), "Zg==");
}

TEST_F(Base64Test, Encode_TwoBytes) {
	std::vector<uint8_t> data = {0x66, 0x6F}; // "fo"
	EXPECT_EQ(Base64::Encode(data), "Zm8=");
}

TEST_F(Base64Test, Encode_ThreeBytes) {
	std::vector<uint8_t> data = {0x66, 0x6F, 0x6F}; // "foo"
	EXPECT_EQ(Base64::Encode(data), "Zm9v");
}

TEST_F(Base64Test, Encode_KnownVector_foobar) {
	// "foobar" → "Zm9vYmFy"
	std::vector<uint8_t> data = {0x66, 0x6F, 0x6F, 0x62, 0x61, 0x72};
	EXPECT_EQ(Base64::Encode(data), "Zm9vYmFy");
}

TEST_F(Base64Test, Encode_AllZeros) {
	std::vector<uint8_t> data = {0x00, 0x00, 0x00};
	EXPECT_EQ(Base64::Encode(data), "AAAA");
}

TEST_F(Base64Test, Encode_AllFF) {
	std::vector<uint8_t> data = {0xFF, 0xFF, 0xFF};
	EXPECT_EQ(Base64::Encode(data), "////");
}

TEST_F(Base64Test, Encode_PaddingAlignment) {
	// 1 byte → 4 chars with ==
	// 2 bytes → 4 chars with =
	// 3 bytes → 4 chars no padding
	std::vector<uint8_t> d1 = {0x41};
	std::vector<uint8_t> d2 = {0x41, 0x42};
	std::vector<uint8_t> d3 = {0x41, 0x42, 0x43};

	std::string e1 = Base64::Encode(d1);
	std::string e2 = Base64::Encode(d2);
	std::string e3 = Base64::Encode(d3);

	EXPECT_EQ(e1.size() % 4, 0u);
	EXPECT_EQ(e2.size() % 4, 0u);
	EXPECT_EQ(e3.size() % 4, 0u);

	// Check padding chars
	EXPECT_EQ(e1.back(), '=');
	EXPECT_NE(e3.back(), '=');
}

// ===== Decode Tests =====

TEST_F(Base64Test, Decode_Empty) {
	auto result = Base64::Decode("");
	EXPECT_TRUE(result.empty());
}

TEST_F(Base64Test, Decode_KnownVector_Zg) {
	auto result = Base64::Decode("Zg==");
	ASSERT_EQ(result.size(), 1u);
	EXPECT_EQ(result[0], 0x66);
}

TEST_F(Base64Test, Decode_KnownVector_foobar) {
	auto result = Base64::Decode("Zm9vYmFy");
	ASSERT_EQ(result.size(), 6u);
	EXPECT_EQ(result[0], 'f');
	EXPECT_EQ(result[1], 'o');
	EXPECT_EQ(result[2], 'o');
	EXPECT_EQ(result[3], 'b');
	EXPECT_EQ(result[4], 'a');
	EXPECT_EQ(result[5], 'r');
}

TEST_F(Base64Test, Decode_AllZeros) {
	auto result = Base64::Decode("AAAA");
	ASSERT_EQ(result.size(), 3u);
	EXPECT_EQ(result[0], 0);
	EXPECT_EQ(result[1], 0);
	EXPECT_EQ(result[2], 0);
}

// ===== Roundtrip Tests =====

TEST_F(Base64Test, Roundtrip_SmallData) {
	std::vector<uint8_t> original = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
	std::string encoded = Base64::Encode(original);
	std::vector<uint8_t> decoded = Base64::Decode(encoded);
	EXPECT_EQ(original, decoded);
}

TEST_F(Base64Test, Roundtrip_BinaryData) {
	// All byte values 0-255
	std::vector<uint8_t> original(256);
	for (int i = 0; i < 256; i++) {
		original[i] = static_cast<uint8_t>(i);
	}
	std::string encoded = Base64::Encode(original);
	std::vector<uint8_t> decoded = Base64::Decode(encoded);
	EXPECT_EQ(original, decoded);
}

TEST_F(Base64Test, Roundtrip_LargeData) {
	// 4KB of data
	std::vector<uint8_t> original(4096);
	for (size_t i = 0; i < original.size(); i++) {
		original[i] = static_cast<uint8_t>((i * 37) & 0xFF);
	}
	std::string encoded = Base64::Encode(original);
	std::vector<uint8_t> decoded = Base64::Decode(encoded);
	EXPECT_EQ(original, decoded);
}

TEST_F(Base64Test, Roundtrip_VariousLengths) {
	// Test lengths 0-20 to cover all padding cases
	for (size_t len = 0; len <= 20; len++) {
		std::vector<uint8_t> original(len);
		for (size_t i = 0; i < len; i++) {
			original[i] = static_cast<uint8_t>(i + 0x41);
		}
		std::string encoded = Base64::Encode(original);
		std::vector<uint8_t> decoded = Base64::Decode(encoded);
		EXPECT_EQ(original, decoded) << "Failed at length " << len;
	}
}
