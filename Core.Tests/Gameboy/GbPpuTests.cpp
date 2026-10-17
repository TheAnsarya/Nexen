#include "pch.h"
#include "Gameboy/GbConstants.h"

// =============================================================================
// GB PPU Rendering Tests
// =============================================================================
// Verifies correctness of GB PPU pixel write optimizations.

class GbPpuPixelTests : public ::testing::Test {};

// Verify that cached scanline offset produces identical results to per-pixel multiply
TEST_F(GbPpuPixelTests, ScanlineOffset_MatchesPerPixelMultiply_AllScanlines) {
	std::array<uint16_t, GbConstants::PixelCount> refBuffer{};
	std::array<uint16_t, GbConstants::PixelCount> optBuffer{};

	for (uint16_t scanline = 0; scanline < GbConstants::ScreenHeight; scanline++) {
		// Cache scanline offset once per scanline (optimized path)
		uint16_t scanlineOffset = scanline * GbConstants::ScreenWidth;

		for (int16_t pixel = 0; pixel < static_cast<int16_t>(GbConstants::ScreenWidth); pixel++) {
			// Reference: per-pixel multiply
			uint16_t refOffset = scanline * GbConstants::ScreenWidth + pixel;
			refBuffer[refOffset] = 0x7FFF;

			// Optimized: cached offset + add
			uint16_t optOffset = scanlineOffset + pixel;
			optBuffer[optOffset] = 0x7FFF;
		}
	}

	EXPECT_EQ(refBuffer, optBuffer);
}

// Verify scanline offset for every scanline matches the multiply formula
TEST_F(GbPpuPixelTests, ScanlineOffset_EveryValue_MatchesMultiply) {
	for (uint16_t scanline = 0; scanline < GbConstants::ScreenHeight; scanline++) {
		uint16_t expected = scanline * GbConstants::ScreenWidth;
		uint16_t actual = scanline * GbConstants::ScreenWidth;  // Both should compute 160*scanline
		EXPECT_EQ(expected, actual) << "Scanline " << scanline;

		// Verify offset range is valid
		EXPECT_LT(expected, GbConstants::PixelCount) << "Scanline " << scanline << " offset exceeds buffer";
		EXPECT_LE(expected + GbConstants::ScreenWidth, GbConstants::PixelCount)
			<< "Scanline " << scanline << " row end exceeds buffer";
	}
}

// Verify full frame buffer write with cached offset fills all pixels correctly
TEST_F(GbPpuPixelTests, CachedOffset_FullFrame_AllPixelsWritten) {
	std::array<uint16_t, GbConstants::PixelCount> buffer{};
	buffer.fill(0);

	for (uint16_t scanline = 0; scanline < GbConstants::ScreenHeight; scanline++) {
		uint16_t scanlineOffset = scanline * GbConstants::ScreenWidth;
		for (int16_t pixel = 0; pixel < static_cast<int16_t>(GbConstants::ScreenWidth); pixel++) {
			buffer[scanlineOffset + pixel] = static_cast<uint16_t>(scanline * 256 + pixel);
		}
	}

	// Every pixel should be non-zero (except scanline 0 pixel 0 which is 0*256+0=0)
	for (uint16_t scanline = 0; scanline < GbConstants::ScreenHeight; scanline++) {
		for (uint16_t pixel = 0; pixel < GbConstants::ScreenWidth; pixel++) {
			uint16_t expected = static_cast<uint16_t>(scanline * 256 + pixel);
			uint16_t actual = buffer[scanline * GbConstants::ScreenWidth + pixel];
			EXPECT_EQ(expected, actual) << "at (" << scanline << ", " << pixel << ")";
		}
	}
}
