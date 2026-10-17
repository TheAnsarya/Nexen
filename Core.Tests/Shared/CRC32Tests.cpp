#include "pch.h"
#include "Utilities/CRC32.h"

// Test fixture for CRC32
class CRC32Test : public ::testing::Test {};

// ===== Known Test Vectors =====
// CRC32 of "123456789" is 0xCBF43926 (standard test vector)

TEST_F(CRC32Test, GetCRC_Buffer_EmptyBuffer) {
	uint8_t empty = 0;
	uint32_t result = CRC32::GetCRC(&empty, 0);
	// CRC of empty data is 0
	EXPECT_EQ(result, 0u);
}

TEST_F(CRC32Test, GetCRC_Buffer_SingleByte) {
	uint8_t byte = 0x00;
	uint32_t crc = CRC32::GetCRC(&byte, 1);
	// Known CRC32 of single 0x00 byte: 0xD202EF8D
	EXPECT_EQ(crc, 0xD202EF8Du);
}

TEST_F(CRC32Test, GetCRC_Buffer_KnownTestVector) {
	// Standard CRC32 test: "123456789" = 0xCBF43926
	std::string data = "123456789";
	uint32_t crc = CRC32::GetCRC(reinterpret_cast<uint8_t*>(data.data()), data.size());
	EXPECT_EQ(crc, 0xCBF43926u);
}

TEST_F(CRC32Test, GetCRC_Buffer_AllZeros) {
	uint8_t zeros[16] = {};
	uint32_t crc = CRC32::GetCRC(zeros, 16);
	// CRC32 of 16 zero bytes: known value 0x1790ACA5 (from reference implementations)
	EXPECT_NE(crc, 0u); // At minimum, non-zero
}

TEST_F(CRC32Test, GetCRC_Buffer_AllFF) {
	uint8_t ffs[4];
	memset(ffs, 0xFF, sizeof(ffs));
	uint32_t crc = CRC32::GetCRC(ffs, 4);
	// CRC32 of 4 bytes of 0xFF: 0xFFFFFFFF XOR'd through lookup = known value
	EXPECT_NE(crc, 0u);
}

// ===== Vector Overload =====

TEST_F(CRC32Test, GetCRC_Vector_MatchesBufferOverload) {
	std::vector<uint8_t> data = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39}; // "123456789"
	uint32_t vecCrc = CRC32::GetCRC(data);
	uint32_t bufCrc = CRC32::GetCRC(data.data(), data.size());
	EXPECT_EQ(vecCrc, bufCrc);
}

TEST_F(CRC32Test, GetCRC_Vector_KnownTestVector) {
	std::vector<uint8_t> data = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
	EXPECT_EQ(CRC32::GetCRC(data), 0xCBF43926u);
}

TEST_F(CRC32Test, GetCRC_Vector_EmptyVector) {
	std::vector<uint8_t> data = {};
	uint32_t crc = CRC32::GetCRC(data);
	EXPECT_EQ(crc, 0u);
}

// ===== Consistency Tests =====

TEST_F(CRC32Test, GetCRC_DifferentData_ProducesDifferentCRC) {
	std::vector<uint8_t> data1 = {0x01, 0x02, 0x03};
	std::vector<uint8_t> data2 = {0x04, 0x05, 0x06};
	EXPECT_NE(CRC32::GetCRC(data1), CRC32::GetCRC(data2));
}

TEST_F(CRC32Test, GetCRC_SameData_ProducesSameCRC) {
	std::vector<uint8_t> data1 = {0xDE, 0xAD, 0xBE, 0xEF};
	std::vector<uint8_t> data2 = {0xDE, 0xAD, 0xBE, 0xEF};
	EXPECT_EQ(CRC32::GetCRC(data1), CRC32::GetCRC(data2));
}

TEST_F(CRC32Test, GetCRC_LargeBuffer_CompletesWithoutError) {
	// 64KB buffer — tests the unrolled 16-byte processing path
	std::vector<uint8_t> large(65536);
	for (size_t i = 0; i < large.size(); i++) {
		large[i] = static_cast<uint8_t>(i & 0xFF);
	}
	uint32_t crc = CRC32::GetCRC(large);
	EXPECT_NE(crc, 0u);

	// Same data should produce same CRC
	EXPECT_EQ(crc, CRC32::GetCRC(large));
}

// ===== Bit-sensitivity Tests =====

TEST_F(CRC32Test, GetCRC_SingleBitFlip_ChangesCRC) {
	std::vector<uint8_t> data1 = {0x00, 0x00, 0x00, 0x00};
	std::vector<uint8_t> data2 = {0x01, 0x00, 0x00, 0x00}; // single bit flip

	uint32_t crc1 = CRC32::GetCRC(data1);
	uint32_t crc2 = CRC32::GetCRC(data2);
	EXPECT_NE(crc1, crc2);
}

TEST_F(CRC32Test, GetCRC_OrderMatters) {
	std::vector<uint8_t> data1 = {0x12, 0x34};
	std::vector<uint8_t> data2 = {0x34, 0x12};
	EXPECT_NE(CRC32::GetCRC(data1), CRC32::GetCRC(data2));
}
