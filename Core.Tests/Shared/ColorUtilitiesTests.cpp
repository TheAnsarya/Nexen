#include "pch.h"
#include "Shared/ColorUtilities.h"

// Test fixture for ColorUtilities
class ColorUtilitiesTest : public ::testing::Test {
protected:
	void SetUp() override {
		// Setup code if needed
	}

	void TearDown() override {
		// Cleanup code if needed
	}
};

// Test RGB555 to ARGB conversion
TEST_F(ColorUtilitiesTest, Rgb555ToArgb_Black_ReturnsBlack) {
	uint32_t result = ColorUtilities::Rgb555ToArgb(0x0000);
	EXPECT_EQ(result, 0xFF000000);
}

TEST_F(ColorUtilitiesTest, Rgb555ToArgb_White_ReturnsWhite) {
	uint32_t result = ColorUtilities::Rgb555ToArgb(0x7FFF);
	EXPECT_EQ(result, 0xFFFFFFFF);  // 31 -> 255 for all channels
}

TEST_F(ColorUtilitiesTest, Rgb555ToArgb_Red_ReturnsRed) {
	uint32_t result = ColorUtilities::Rgb555ToArgb(0x001F);
	EXPECT_EQ(result, 0xFFFF0000);  // Red channel only: 31 -> 255
}

TEST_F(ColorUtilitiesTest, Rgb555ToArgb_Green_ReturnsGreen) {
	uint32_t result = ColorUtilities::Rgb555ToArgb(0x03E0);
	EXPECT_EQ(result, 0xFF00FF00);  // Green channel only: 31 -> 255
}

TEST_F(ColorUtilitiesTest, Rgb555ToArgb_Blue_ReturnsBlue) {
	uint32_t result = ColorUtilities::Rgb555ToArgb(0x7C00);
	EXPECT_EQ(result, 0xFF0000FF);  // Blue channel only: 31 -> 255
}

// Test constexpr evaluation
TEST_F(ColorUtilitiesTest, Rgb555ToArgb_Constexpr_CompilesAndWorks) {
	// This should compile as constexpr
	constexpr uint32_t black = ColorUtilities::Rgb555ToArgb(0x0000);
	constexpr uint32_t white = ColorUtilities::Rgb555ToArgb(0x7FFF);
	constexpr uint32_t red = ColorUtilities::Rgb555ToArgb(0x001F);
	
	EXPECT_EQ(black, 0xFF000000);
	EXPECT_EQ(white, 0xFFFFFFFF);  // 31 -> 255 for all
	EXPECT_EQ(red, 0xFFFF0000);    // Red only
}

// Test RGB555 to RGB conversion
TEST_F(ColorUtilitiesTest, Rgb555ToRgb_ValidColor_ReturnsCorrect) {
	uint8_t r, g, b;
	ColorUtilities::Rgb555ToRgb(0x7FFF, r, g, b);
	// 31 -> 255: (31 << 3) + (31 >> 2) = 248 + 7 = 255
	EXPECT_EQ(r, 255);
	EXPECT_EQ(g, 255);
	EXPECT_EQ(b, 255);
}

// Parameterized test for multiple color conversions
class ColorConversionTest : public ::testing::TestWithParam<std::tuple<uint16_t, uint32_t>> {};

TEST_P(ColorConversionTest, Rgb555ToArgb_MultipleColors) {
	auto [input, expected] = GetParam();
	uint32_t result = ColorUtilities::Rgb555ToArgb(input);
	EXPECT_EQ(result, expected);
}

INSTANTIATE_TEST_SUITE_P(
	ValidColors,
	ColorConversionTest,
	::testing::Values(
		std::make_tuple(0x0000, 0xFF000000),  // Black (0->0)
		std::make_tuple(0x7FFF, 0xFFFFFFFF),  // White (31->255 all channels)
		std::make_tuple(0x001F, 0xFFFF0000),  // Red (31->255 R)
		std::make_tuple(0x03E0, 0xFF00FF00),  // Green (31->255 G)
		std::make_tuple(0x7C00, 0xFF0000FF),  // Blue (31->255 B)
		std::make_tuple(0x4210, 0xFF848484),  // Mid gray (16->132)
		std::make_tuple(0x1084, 0xFF212121)   // Dark (4->33)
	)
);
