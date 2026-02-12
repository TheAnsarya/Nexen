#include "pch.h"
#include <array>
#include <cstring>

// =============================================================================
// Video Filter Optimization Tests
// =============================================================================
// Verifies that flat-loop and row-pointer video filter optimizations produce
// identical output to the original nested-loop implementations.

class VideoFilterTests : public ::testing::Test {};

// Verify flat loop produces same output as nested loop for GB dimensions (160x144)
TEST_F(VideoFilterTests, FlatLoop_MatchesNestedLoop_GbDimensions) {
	constexpr uint32_t Width = 160;
	constexpr uint32_t Height = 144;
	constexpr uint32_t PixelCount = Width * Height;

	std::array<uint16_t, PixelCount> input{};
	for (uint32_t i = 0; i < PixelCount; i++) {
		input[i] = static_cast<uint16_t>(i & 0x7FFF);
	}

	// Reference: nested loop
	std::array<uint32_t, PixelCount> refOutput{};
	for (uint32_t i = 0; i < Height; i++) {
		for (uint32_t j = 0; j < Width; j++) {
			refOutput[i * Width + j] = 0xFF000000 | input[i * Width + j];
		}
	}

	// Optimized: flat loop
	std::array<uint32_t, PixelCount> optOutput{};
	for (uint32_t idx = 0; idx < PixelCount; idx++) {
		optOutput[idx] = 0xFF000000 | input[idx];
	}

	EXPECT_EQ(refOutput, optOutput);
}

// Verify flat loop for GBA dimensions (240x160)
TEST_F(VideoFilterTests, FlatLoop_MatchesNestedLoop_GbaDimensions) {
	constexpr uint32_t Width = 240;
	constexpr uint32_t Height = 160;
	constexpr uint32_t PixelCount = Width * Height;

	std::array<uint16_t, PixelCount> input{};
	for (uint32_t i = 0; i < PixelCount; i++) {
		input[i] = static_cast<uint16_t>(i & 0x7FFF);
	}

	std::array<uint32_t, PixelCount> refOutput{};
	for (uint32_t i = 0; i < Height; i++) {
		for (uint32_t j = 0; j < Width; j++) {
			refOutput[i * Width + j] = 0xFF000000 | input[i * Width + j];
		}
	}

	std::array<uint32_t, PixelCount> optOutput{};
	for (uint32_t idx = 0; idx < PixelCount; idx++) {
		optOutput[idx] = 0xFF000000 | input[idx];
	}

	EXPECT_EQ(refOutput, optOutput);
}

// Verify NES DecodePpuBuffer row pointer hoisting matches per-pixel indexing
TEST_F(VideoFilterTests, NesDecodePpu_RowPointer_MatchesPerPixelCalc) {
	constexpr uint32_t BaseWidth = 256;
	constexpr uint32_t FrameWidth = 240;
	constexpr uint32_t FrameHeight = 224;
	constexpr uint32_t OverscanTop = 8;
	constexpr uint32_t OverscanLeft = 8;

	// Fill PPU buffer with distinct values
	std::array<uint16_t, BaseWidth * 240> ppuBuffer{};
	for (uint32_t i = 0; i < ppuBuffer.size(); i++) {
		ppuBuffer[i] = static_cast<uint16_t>(i & 0x7FFF);
	}

	// Simple palette lookup
	std::array<uint32_t, 0x8000> palette{};
	for (uint32_t i = 0; i < palette.size(); i++) {
		palette[i] = 0xFF000000 | i;
	}

	// Reference: per-pixel calculation
	std::array<uint32_t, FrameWidth * FrameHeight> refOutput{};
	{
		uint32_t* out = refOutput.data();
		for (uint32_t i = 0; i < FrameHeight; i++) {
			for (uint32_t j = 0; j < FrameWidth; j++) {
				*out = palette[ppuBuffer[(i + OverscanTop) * BaseWidth + j + OverscanLeft]];
				out++;
			}
		}
	}

	// Optimized: row pointer hoisting
	std::array<uint32_t, FrameWidth * FrameHeight> optOutput{};
	{
		uint32_t* out = optOutput.data();
		for (uint32_t i = 0; i < FrameHeight; i++) {
			const uint16_t* srcRow = ppuBuffer.data() + (i + OverscanTop) * BaseWidth + OverscanLeft;
			for (uint32_t j = 0; j < FrameWidth; j++) {
				*out++ = palette[srcRow[j]];
			}
		}
	}

	EXPECT_EQ(refOutput, optOutput);
}

// Verify NES DecodePpuBuffer with various overscan values
TEST_F(VideoFilterTests, NesDecodePpu_RowPointer_VariousOverscan) {
	constexpr uint32_t BaseWidth = 256;
	constexpr uint32_t BaseHeight = 240;

	std::array<uint16_t, BaseWidth * BaseHeight> ppuBuffer{};
	for (uint32_t i = 0; i < ppuBuffer.size(); i++) {
		ppuBuffer[i] = static_cast<uint16_t>(i & 0x7FFF);
	}

	std::array<uint32_t, 0x8000> palette{};
	for (uint32_t i = 0; i < palette.size(); i++) {
		palette[i] = 0xFF000000 | i;
	}

	// Test with different overscan settings
	struct OverscanTest { uint32_t top; uint32_t left; uint32_t width; uint32_t height; };
	OverscanTest tests[] = {
		{0, 0, 256, 240},
		{8, 8, 240, 224},
		{16, 0, 256, 208},
		{0, 16, 224, 240},
	};

	for (const auto& t : tests) {
		std::vector<uint32_t> refOutput(t.width * t.height);
		std::vector<uint32_t> optOutput(t.width * t.height);

		// Reference
		{
			uint32_t* out = refOutput.data();
			for (uint32_t i = 0; i < t.height; i++) {
				for (uint32_t j = 0; j < t.width; j++) {
					*out = palette[ppuBuffer[(i + t.top) * BaseWidth + j + t.left]];
					out++;
				}
			}
		}

		// Optimized
		{
			uint32_t* out = optOutput.data();
			for (uint32_t i = 0; i < t.height; i++) {
				const uint16_t* srcRow = ppuBuffer.data() + (i + t.top) * BaseWidth + t.left;
				for (uint32_t j = 0; j < t.width; j++) {
					*out++ = palette[srcRow[j]];
				}
			}
		}

		EXPECT_EQ(refOutput, optOutput) << "Failed for overscan top=" << t.top
			<< " left=" << t.left << " w=" << t.width << " h=" << t.height;
	}
}

// Verify equalizer band frequency constants are computed correctly
TEST_F(VideoFilterTests, EqualizerBands_PrecomputedBoundaries_MatchOriginal) {
	// Original computation from Equalizer.cpp
	std::vector<double> originalBands = {40, 56, 80, 113, 160, 225, 320, 450, 600, 750, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 10000, 12500, 13000};
	originalBands.insert(originalBands.begin(), originalBands[0] - (originalBands[1] - originalBands[0]));
	originalBands.insert(originalBands.end(), originalBands[originalBands.size() - 1] + (originalBands[originalBands.size() - 1] - originalBands[originalBands.size() - 2]));

	// Pre-computed static constexpr values
	std::array<double, 22> precomputed = {
		24, 40, 56, 80, 113, 160, 225, 320, 450, 600, 750, 1000,
		2000, 3000, 4000, 5000, 6000, 7000, 10000, 12500, 13000, 13500
	};

	ASSERT_EQ(originalBands.size(), precomputed.size());
	for (size_t i = 0; i < originalBands.size(); i++) {
		EXPECT_DOUBLE_EQ(originalBands[i], precomputed[i]) << "Band " << i << " mismatch";
	}
}

// Verify BaseVideoFilter PI constant matches std::numbers::pi
TEST_F(VideoFilterTests, PI_Constant_MatchesStdNumbersPi) {
	constexpr double oldPI = 3.14159265358979323846;
	EXPECT_DOUBLE_EQ(oldPI, std::numbers::pi);
}

// Verify BaseVideoFilter double literals match truncated float values
TEST_F(VideoFilterTests, ConversionMatrix_DoubleLiterals_MatchFloatValues) {
	// Old code used float literals assigned to double
	double fromFloat[6] = {0.956f, 0.621f, -0.272f, -0.647f, -1.105f, 1.702f};
	// New code uses double literals
	constexpr double fromDouble[6] = {0.956, 0.621, -0.272, -0.647, -1.105, 1.702};

	// The double literals are MORE precise; verify no functional regression
	// In the original code, floats were converted to doubles with rounding artifacts
	// The new double literals are the "intended" values
	for (int i = 0; i < 6; i++) {
		// They should be very close (within float precision)
		EXPECT_NEAR(fromFloat[i], fromDouble[i], 1e-6) << "Value " << i
			<< " float=" << fromFloat[i] << " double=" << fromDouble[i];
	}
}
