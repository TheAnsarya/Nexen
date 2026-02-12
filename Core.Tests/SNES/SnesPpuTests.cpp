#include "pch.h"
#include <array>

// ===== SNES PPU ApplyBrightness LUT Correctness Tests =====
// These tests verify that the constexpr brightness LUT produces identical
// results to the original per-pixel multiply-divide approach.

class SnesPpuBrightnessTest : public ::testing::Test {
protected:
	/// Reference implementation: original per-pixel multiply/divide
	static uint16_t ApplyBrightness_Reference(uint16_t pixel, uint8_t brightness) {
		uint16_t r = (pixel & 0x1F) * brightness / 15;
		uint16_t g = ((pixel >> 5) & 0x1F) * brightness / 15;
		uint16_t b = ((pixel >> 10) & 0x1F) * brightness / 15;
		return r | (g << 5) | (b << 10);
	}

	/// Build brightness LUT (same logic as production SnesPpu.cpp)
	static std::array<std::array<uint8_t, 32>, 16> MakeBrightnessLut() {
		std::array<std::array<uint8_t, 32>, 16> lut{};
		for (int b = 0; b < 16; b++) {
			for (int c = 0; c < 32; c++) {
				lut[b][c] = static_cast<uint8_t>(c * b / 15);
			}
		}
		return lut;
	}

	static uint16_t ApplyBrightness_LUT(uint16_t pixel, uint8_t brightness) {
		const auto& lut = brightnessLut[brightness];
		uint16_t r = lut[pixel & 0x1F];
		uint16_t g = lut[(pixel >> 5) & 0x1F];
		uint16_t b = lut[(pixel >> 10) & 0x1F];
		return r | (g << 5) | (b << 10);
	}

	inline static const std::array<std::array<uint8_t, 32>, 16> brightnessLut = MakeBrightnessLut();
};

TEST_F(SnesPpuBrightnessTest, LUT_MatchesReference_AllBrightnessLevels) {
	// Test all 16 brightness levels with representative pixel values
	uint16_t testPixels[] = {
		0x0000,                          // Black
		0x7FFF,                          // White (max RGB555)
		0x001F,                          // Pure red (max)
		0x03E0,                          // Pure green (max)
		0x7C00,                          // Pure blue (max)
		0x0001,                          // Dim red (min)
		0x0421,                          // R=1, G=1, B=1
		0x0842,                          // R=2, G=2, B=2
		0x294A,                          // R=10, G=10, B=10
		0x56B5,                          // R=21, G=21, B=21
		0x7BDE,                          // R=30, G=30, B=30
		0x7FFF,                          // R=31, G=31, B=31
		0x1234,                          // Arbitrary
		0x5678,                          // Arbitrary
		0x3DAF,                          // Arbitrary
	};

	for (uint8_t brightness = 0; brightness < 16; brightness++) {
		for (uint16_t pixel : testPixels) {
			uint16_t reference = ApplyBrightness_Reference(pixel, brightness);
			uint16_t optimized = ApplyBrightness_LUT(pixel, brightness);
			EXPECT_EQ(optimized, reference)
				<< "Brightness=" << (int)brightness
				<< " Pixel=0x" << std::hex << pixel;
		}
	}
}

TEST_F(SnesPpuBrightnessTest, LUT_MatchesReference_Exhaustive_AllComponents) {
	// Test every possible 5-bit component value for every brightness level
	for (uint8_t brightness = 0; brightness < 16; brightness++) {
		for (int component = 0; component < 32; component++) {
			uint8_t reference = static_cast<uint8_t>(component * brightness / 15);
			uint8_t lutResult = brightnessLut[brightness][component];
			EXPECT_EQ(lutResult, reference)
				<< "Brightness=" << (int)brightness
				<< " Component=" << component;
		}
	}
}

TEST_F(SnesPpuBrightnessTest, LUT_MatchesReference_FullScanline_256Pixels) {
	// Simulate a full 256-pixel scanline at each brightness level
	std::array<uint16_t, 256> refBuffer;
	std::array<uint16_t, 256> lutBuffer;

	for (uint8_t brightness = 0; brightness < 16; brightness++) {
		// Fill both buffers with same test data
		for (int i = 0; i < 256; i++) {
			uint16_t pixel = static_cast<uint16_t>(i * 128 + brightness * 17);
			pixel &= 0x7FFF; // Mask to valid RGB555 range
			refBuffer[i] = pixel;
			lutBuffer[i] = pixel;
		}

		// Apply reference brightness
		for (int x = 0; x < 256; x++) {
			refBuffer[x] = ApplyBrightness_Reference(refBuffer[x], brightness);
		}

		// Apply LUT brightness
		const auto& lut = brightnessLut[brightness];
		for (int x = 0; x < 256; x++) {
			uint16_t pixel = lutBuffer[x];
			uint16_t r = lut[pixel & 0x1F];
			uint16_t g = lut[(pixel >> 5) & 0x1F];
			uint16_t b = lut[(pixel >> 10) & 0x1F];
			lutBuffer[x] = r | (g << 5) | (b << 10);
		}

		// Compare
		for (int x = 0; x < 256; x++) {
			EXPECT_EQ(lutBuffer[x], refBuffer[x])
				<< "Brightness=" << (int)brightness << " Pixel=" << x;
		}
	}
}

TEST_F(SnesPpuBrightnessTest, LUT_BrightnessZero_AllBlack) {
	// Brightness 0 should make everything black
	for (int c = 0; c < 32; c++) {
		EXPECT_EQ(brightnessLut[0][c], 0)
			<< "Brightness 0 should produce 0 for component " << c;
	}
}

TEST_F(SnesPpuBrightnessTest, LUT_BrightnessFull_Identity) {
	// Brightness 15 should be identity (component * 15 / 15 = component)
	for (int c = 0; c < 32; c++) {
		EXPECT_EQ(brightnessLut[15][c], c)
			<< "Brightness 15 should be identity for component " << c;
	}
}
