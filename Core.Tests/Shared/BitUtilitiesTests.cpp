#include "pch.h"
#include "Utilities/BitUtilities.h"

// =============================================================================
// BitUtilities Unit Tests
// =============================================================================
// Tests for compile-time templated bit manipulation (SetBits / GetBits).

class BitUtilitiesTest : public ::testing::Test {};

// =============================================================================
// GetBits — Extract 8-bit value from specific bit position
// =============================================================================

TEST_F(BitUtilitiesTest, GetBits_Bit0_LowByte) {
	uint16_t value = 0x1234;
	EXPECT_EQ(BitUtilities::GetBits<0>(value), 0x34);
}

TEST_F(BitUtilitiesTest, GetBits_Bit8_HighByte) {
	uint16_t value = 0x1234;
	EXPECT_EQ(BitUtilities::GetBits<8>(value), 0x12);
}

TEST_F(BitUtilitiesTest, GetBits_Bit0_32bit) {
	uint32_t value = 0xaabbccdd;
	EXPECT_EQ(BitUtilities::GetBits<0>(value), 0xdd);
}

TEST_F(BitUtilitiesTest, GetBits_Bit8_32bit) {
	uint32_t value = 0xaabbccdd;
	EXPECT_EQ(BitUtilities::GetBits<8>(value), 0xcc);
}

TEST_F(BitUtilitiesTest, GetBits_Bit16_32bit) {
	uint32_t value = 0xaabbccdd;
	EXPECT_EQ(BitUtilities::GetBits<16>(value), 0xbb);
}

TEST_F(BitUtilitiesTest, GetBits_Bit24_32bit) {
	uint32_t value = 0xaabbccdd;
	EXPECT_EQ(BitUtilities::GetBits<24>(value), 0xaa);
}

TEST_F(BitUtilitiesTest, GetBits_AllZeros) {
	uint32_t value = 0x00000000;
	EXPECT_EQ(BitUtilities::GetBits<0>(value), 0x00);
	EXPECT_EQ(BitUtilities::GetBits<8>(value), 0x00);
	EXPECT_EQ(BitUtilities::GetBits<16>(value), 0x00);
	EXPECT_EQ(BitUtilities::GetBits<24>(value), 0x00);
}

TEST_F(BitUtilitiesTest, GetBits_AllOnes) {
	uint32_t value = 0xffffffff;
	EXPECT_EQ(BitUtilities::GetBits<0>(value), 0xff);
	EXPECT_EQ(BitUtilities::GetBits<8>(value), 0xff);
	EXPECT_EQ(BitUtilities::GetBits<16>(value), 0xff);
	EXPECT_EQ(BitUtilities::GetBits<24>(value), 0xff);
}

TEST_F(BitUtilitiesTest, GetBits_SingleBitPatterns) {
	uint32_t value = 0x80402010;
	EXPECT_EQ(BitUtilities::GetBits<0>(value), 0x10);
	EXPECT_EQ(BitUtilities::GetBits<8>(value), 0x20);
	EXPECT_EQ(BitUtilities::GetBits<16>(value), 0x40);
	EXPECT_EQ(BitUtilities::GetBits<24>(value), 0x80);
}

TEST_F(BitUtilitiesTest, GetBits_Uint8t_Bit0) {
	uint8_t value = 0xab;
	EXPECT_EQ(BitUtilities::GetBits<0>(value), 0xab);
}

// =============================================================================
// SetBits — Set 8-bit value at specific bit position
// =============================================================================

TEST_F(BitUtilitiesTest, SetBits_Bit0_16bit) {
	uint16_t dst = 0x0000;
	BitUtilities::SetBits<0>(dst, 0x42);
	EXPECT_EQ(dst, 0x0042);
}

TEST_F(BitUtilitiesTest, SetBits_Bit8_16bit) {
	uint16_t dst = 0x0000;
	BitUtilities::SetBits<8>(dst, 0x42);
	EXPECT_EQ(dst, 0x4200);
}

TEST_F(BitUtilitiesTest, SetBits_Bit0_PreservesHighByte) {
	uint16_t dst = 0xff00;
	BitUtilities::SetBits<0>(dst, 0x42);
	EXPECT_EQ(dst, 0xff42);
}

TEST_F(BitUtilitiesTest, SetBits_Bit8_PreservesLowByte) {
	uint16_t dst = 0x00ff;
	BitUtilities::SetBits<8>(dst, 0x42);
	EXPECT_EQ(dst, 0x42ff);
}

TEST_F(BitUtilitiesTest, SetBits_32bit_AllPositions) {
	uint32_t dst = 0x00000000;

	BitUtilities::SetBits<0>(dst, 0x11);
	EXPECT_EQ(dst, 0x00000011u);

	BitUtilities::SetBits<8>(dst, 0x22);
	EXPECT_EQ(dst, 0x00002211u);

	BitUtilities::SetBits<16>(dst, 0x33);
	EXPECT_EQ(dst, 0x00332211u);

	BitUtilities::SetBits<24>(dst, 0x44);
	EXPECT_EQ(dst, 0x44332211u);
}

TEST_F(BitUtilitiesTest, SetBits_Overwrite) {
	uint32_t dst = 0xffffffff;

	BitUtilities::SetBits<0>(dst, 0x00);
	EXPECT_EQ(dst, 0xffffff00u);

	BitUtilities::SetBits<8>(dst, 0x00);
	EXPECT_EQ(dst, 0xffff0000u);

	BitUtilities::SetBits<16>(dst, 0x00);
	EXPECT_EQ(dst, 0xff000000u);

	BitUtilities::SetBits<24>(dst, 0x00);
	EXPECT_EQ(dst, 0x00000000u);
}

TEST_F(BitUtilitiesTest, SetBits_DoesNotAffectOtherPositions) {
	uint32_t dst = 0xaabbccdd;
	BitUtilities::SetBits<8>(dst, 0xff);
	// Only byte 1 (bits 8-15) changed: 0xcc → 0xff
	EXPECT_EQ(dst, 0xaabbffdd);
}

// =============================================================================
// SetBits/GetBits Roundtrip
// =============================================================================

TEST_F(BitUtilitiesTest, Roundtrip_Set_Then_Get) {
	uint32_t dst = 0;

	BitUtilities::SetBits<0>(dst, 0xaa);
	BitUtilities::SetBits<8>(dst, 0xbb);
	BitUtilities::SetBits<16>(dst, 0xcc);
	BitUtilities::SetBits<24>(dst, 0xdd);

	EXPECT_EQ(BitUtilities::GetBits<0>(dst), 0xaa);
	EXPECT_EQ(BitUtilities::GetBits<8>(dst), 0xbb);
	EXPECT_EQ(BitUtilities::GetBits<16>(dst), 0xcc);
	EXPECT_EQ(BitUtilities::GetBits<24>(dst), 0xdd);
}

TEST_F(BitUtilitiesTest, Roundtrip_AllValues_Bit0) {
	// Test all 256 byte values at bit position 0
	for (int v = 0; v < 256; v++) {
		uint16_t dst = 0xff00;
		BitUtilities::SetBits<0>(dst, static_cast<uint8_t>(v));
		EXPECT_EQ(BitUtilities::GetBits<0>(dst), static_cast<uint8_t>(v));
		EXPECT_EQ(BitUtilities::GetBits<8>(dst), 0xff); // High byte preserved
	}
}

TEST_F(BitUtilitiesTest, Roundtrip_AllValues_Bit8) {
	for (int v = 0; v < 256; v++) {
		uint16_t dst = 0x00ff;
		BitUtilities::SetBits<8>(dst, static_cast<uint8_t>(v));
		EXPECT_EQ(BitUtilities::GetBits<8>(dst), static_cast<uint8_t>(v));
		EXPECT_EQ(BitUtilities::GetBits<0>(dst), 0xff); // Low byte preserved
	}
}

// =============================================================================
// Register emulation patterns
// =============================================================================

TEST_F(BitUtilitiesTest, RegisterPacking_SnesCpuStyle) {
	// Simulates packing A (8-bit) + B (8-bit) into 16-bit accumulator
	uint16_t accumulator = 0;
	uint8_t regA = 0x42;
	uint8_t regB = 0xff;

	BitUtilities::SetBits<0>(accumulator, regA);
	BitUtilities::SetBits<8>(accumulator, regB);

	EXPECT_EQ(accumulator, 0xff42);
	EXPECT_EQ(BitUtilities::GetBits<0>(accumulator), regA);
	EXPECT_EQ(BitUtilities::GetBits<8>(accumulator), regB);
}

TEST_F(BitUtilitiesTest, FlagPacking_StatusRegister) {
	// Simulates extracting flag bytes from 32-bit status register
	uint32_t status = 0;
	BitUtilities::SetBits<0>(status, 0x30);  // NV-BDIZC flags
	BitUtilities::SetBits<8>(status, 0x01);  // Emulation mode
	BitUtilities::SetBits<16>(status, 0xff); // Bank byte

	EXPECT_EQ(BitUtilities::GetBits<0>(status), 0x30);
	EXPECT_EQ(BitUtilities::GetBits<8>(status), 0x01);
	EXPECT_EQ(BitUtilities::GetBits<16>(status), 0xff);
	EXPECT_EQ(BitUtilities::GetBits<24>(status), 0x00);
}
