#include "pch.h"
#include <array>
#include <cstring>
#include "NES/NesTypes.h"
#include "SNES/SnesPpuTypes.h"
#include "Shared/ColorUtilities.h"

// =============================================================================
// PPU Rendering Benchmarks
// =============================================================================
// These benchmarks measure performance-critical PPU operations across platforms.
// PPU rendering is often the most CPU-intensive part of emulation.

// -----------------------------------------------------------------------------
// NES PPU Benchmarks
// -----------------------------------------------------------------------------

// Benchmark NES tile pixel extraction (2bpp planar)
static void BM_NesPpu_TilePixelExtraction(benchmark::State& state) {
	uint8_t lowByte = 0x55;   // 01010101
	uint8_t highByte = 0xAA;  // 10101010

	for (auto _ : state) {
		// Extract 8 pixels from 2bpp tile data
		for (int i = 7; i >= 0; i--) {
			uint8_t pixel = ((lowByte >> i) & 1) | (((highByte >> i) & 1) << 1);
			benchmark::DoNotOptimize(pixel);
		}
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_NesPpu_TilePixelExtraction);

// Benchmark NES tile pixel extraction (unrolled, common optimization)
static void BM_NesPpu_TilePixelExtraction_Unrolled(benchmark::State& state) {
	uint8_t lowByte = 0x55;
	uint8_t highByte = 0xAA;
	std::array<uint8_t, 8> pixels{};

	for (auto _ : state) {
		// Unrolled extraction - often found in optimized emulators
		pixels[0] = ((lowByte >> 7) & 1) | (((highByte >> 7) & 1) << 1);
		pixels[1] = ((lowByte >> 6) & 1) | (((highByte >> 6) & 1) << 1);
		pixels[2] = ((lowByte >> 5) & 1) | (((highByte >> 5) & 1) << 1);
		pixels[3] = ((lowByte >> 4) & 1) | (((highByte >> 4) & 1) << 1);
		pixels[4] = ((lowByte >> 3) & 1) | (((highByte >> 3) & 1) << 1);
		pixels[5] = ((lowByte >> 2) & 1) | (((highByte >> 2) & 1) << 1);
		pixels[6] = ((lowByte >> 1) & 1) | (((highByte >> 1) & 1) << 1);
		pixels[7] = (lowByte & 1) | ((highByte & 1) << 1);
		benchmark::DoNotOptimize(pixels);
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_NesPpu_TilePixelExtraction_Unrolled);

// Benchmark NES palette lookup
static void BM_NesPpu_PaletteLookup(benchmark::State& state) {
	std::array<uint8_t, 32> palette{};
	for (int i = 0; i < 32; i++) palette[i] = static_cast<uint8_t>(i * 2);

	uint8_t paletteOffset = 0;  // 0, 4, 8, or 12 for BG; 16, 20, 24, 28 for sprites
	uint8_t pixelColor = 0;

	for (auto _ : state) {
		uint8_t paletteIndex = paletteOffset | pixelColor;
		uint8_t nesColor = palette[paletteIndex & 0x1F];
		benchmark::DoNotOptimize(nesColor);
		pixelColor = (pixelColor + 1) & 3;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesPpu_PaletteLookup);

// Benchmark NES sprite priority/transparency check
static void BM_NesPpu_SpritePriorityCheck(benchmark::State& state) {
	uint8_t bgPixel = 0;
	uint8_t spritePixel = 2;
	bool spritePriority = false;  // 0 = in front, 1 = behind

	for (auto _ : state) {
		uint8_t finalPixel;
		// Sprite 0 hit detection and priority logic
		if (spritePixel == 0) {
			// Transparent sprite pixel - use BG
			finalPixel = bgPixel;
		} else if (bgPixel == 0) {
			// Transparent BG pixel - use sprite
			finalPixel = spritePixel;
		} else if (spritePriority) {
			// Sprite behind BG
			finalPixel = bgPixel;
		} else {
			// Sprite in front of BG
			finalPixel = spritePixel;
		}
		benchmark::DoNotOptimize(finalPixel);
		bgPixel = (bgPixel + 1) & 3;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesPpu_SpritePriorityCheck);

// Benchmark NES scanline rendering (256 pixels)
static void BM_NesPpu_ScanlineRender(benchmark::State& state) {
	std::array<uint32_t, 256> scanlineBuffer{};
	std::array<uint32_t, 64> nesPalette{};
	// Initialize with typical NES RGB values
	for (int i = 0; i < 64; i++) {
		nesPalette[i] = 0xFF000000 | (i * 4) | ((i * 4) << 8) | ((i * 4) << 16);
	}

	for (auto _ : state) {
		for (int x = 0; x < 256; x++) {
			uint8_t paletteIndex = static_cast<uint8_t>(x & 0x3F);
			scanlineBuffer[x] = nesPalette[paletteIndex];
		}
		benchmark::DoNotOptimize(scanlineBuffer);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_NesPpu_ScanlineRender);

// -----------------------------------------------------------------------------
// SNES PPU Benchmarks
// -----------------------------------------------------------------------------

// Benchmark SNES 4bpp tile pixel extraction
static void BM_SnesPpu_TilePixelExtraction_4bpp(benchmark::State& state) {
	std::array<uint8_t, 8> tileRow{};  // 4 bitplanes * 2 = 8 bytes per row
	for (int i = 0; i < 8; i++) tileRow[i] = static_cast<uint8_t>(0x55 + i);

	for (auto _ : state) {
		for (int i = 7; i >= 0; i--) {
			// 4bpp: 4 bitplanes interleaved
			uint8_t pixel = ((tileRow[0] >> i) & 1) |
							(((tileRow[1] >> i) & 1) << 1) |
							(((tileRow[2] >> i) & 1) << 2) |
							(((tileRow[3] >> i) & 1) << 3);
			benchmark::DoNotOptimize(pixel);
		}
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_SnesPpu_TilePixelExtraction_4bpp);

// Benchmark SNES 8bpp tile pixel extraction (Mode 7, BG layers in some modes)
static void BM_SnesPpu_TilePixelExtraction_8bpp(benchmark::State& state) {
	std::array<uint8_t, 16> tileRow{};  // 8 bitplanes * 2 = 16 bytes per row
	for (int i = 0; i < 16; i++) tileRow[i] = static_cast<uint8_t>(0x11 * (i + 1));

	for (auto _ : state) {
		for (int i = 7; i >= 0; i--) {
			uint8_t pixel = ((tileRow[0] >> i) & 1) |
							(((tileRow[1] >> i) & 1) << 1) |
							(((tileRow[2] >> i) & 1) << 2) |
							(((tileRow[3] >> i) & 1) << 3) |
							(((tileRow[4] >> i) & 1) << 4) |
							(((tileRow[5] >> i) & 1) << 5) |
							(((tileRow[6] >> i) & 1) << 6) |
							(((tileRow[7] >> i) & 1) << 7);
			benchmark::DoNotOptimize(pixel);
		}
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_SnesPpu_TilePixelExtraction_8bpp);

// Benchmark SNES Mode 7 coordinate transformation
static void BM_SnesPpu_Mode7Transform(benchmark::State& state) {
	// Mode 7 matrix parameters
	int16_t a = 0x0100;  // cos(0) * scale
	int16_t b = 0x0000;  // sin(0) * scale
	int16_t c = 0x0000;  // -sin(0) * scale
	int16_t d = 0x0100;  // cos(0) * scale
	int16_t cx = 128;    // center X
	int16_t cy = 128;    // center Y
	int16_t hofs = 0;
	int16_t vofs = 0;

	for (auto _ : state) {
		for (int screenX = 0; screenX < 256; screenX++) {
			// Mode 7 affine transformation
			int32_t x = screenX - cx;
			int32_t y = 128 - cy;  // Current scanline

			int32_t vramX = (a * x + b * y + (cx << 8) + hofs) >> 8;
			int32_t vramY = (c * x + d * y + (cy << 8) + vofs) >> 8;

			benchmark::DoNotOptimize(vramX);
			benchmark::DoNotOptimize(vramY);
		}
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_SnesPpu_Mode7Transform);

// Benchmark SNES color math (add/subtract with optional half)
static void BM_SnesPpu_ColorMath(benchmark::State& state) {
	uint16_t mainColor = 0x1F << 5;   // Green
	uint16_t subColor = 0x1F;         // Red
	bool subtract = false;
	bool half = true;

	for (auto _ : state) {
		uint16_t result;
		if (subtract) {
			int r = ((mainColor & 0x1F) - (subColor & 0x1F));
			int g = (((mainColor >> 5) & 0x1F) - ((subColor >> 5) & 0x1F));
			int b = (((mainColor >> 10) & 0x1F) - ((subColor >> 10) & 0x1F));
			r = (r < 0) ? 0 : r;
			g = (g < 0) ? 0 : g;
			b = (b < 0) ? 0 : b;
			if (half) { r >>= 1; g >>= 1; b >>= 1; }
			result = static_cast<uint16_t>(r | (g << 5) | (b << 10));
		} else {
			int r = ((mainColor & 0x1F) + (subColor & 0x1F));
			int g = (((mainColor >> 5) & 0x1F) + ((subColor >> 5) & 0x1F));
			int b = (((mainColor >> 10) & 0x1F) + ((subColor >> 10) & 0x1F));
			r = (r > 31) ? 31 : r;
			g = (g > 31) ? 31 : g;
			b = (b > 31) ? 31 : b;
			if (half) { r >>= 1; g >>= 1; b >>= 1; }
			result = static_cast<uint16_t>(r | (g << 5) | (b << 10));
		}
		benchmark::DoNotOptimize(result);
		mainColor++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesPpu_ColorMath);

// Benchmark SNES window mask calculation
static void BM_SnesPpu_WindowMask(benchmark::State& state) {
	uint8_t window1Left = 32;
	uint8_t window1Right = 224;
	uint8_t window2Left = 64;
	uint8_t window2Right = 192;
	bool window1Enabled = true;
	bool window2Enabled = true;
	bool window1Invert = false;
	bool window2Invert = false;
	uint8_t maskLogic = 0;  // OR

	for (auto _ : state) {
		for (int x = 0; x < 256; x++) {
			bool w1 = window1Enabled && (x >= window1Left && x <= window1Right);
			bool w2 = window2Enabled && (x >= window2Left && x <= window2Right);
			if (window1Invert) w1 = !w1;
			if (window2Invert) w2 = !w2;

			bool masked = false;
			switch (maskLogic) {
				case 0: masked = w1 | w2; break;  // OR
				case 1: masked = w1 & w2; break;  // AND
				case 2: masked = w1 ^ w2; break;  // XOR
				case 3: masked = !(w1 ^ w2); break;  // XNOR
			}
			benchmark::DoNotOptimize(masked);
		}
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_SnesPpu_WindowMask);

// Benchmark SNES OAM sprite evaluation
static void BM_SnesPpu_OamEvaluation(benchmark::State& state) {
	// Simulate OAM (512 bytes main + 32 bytes high table)
	std::array<uint8_t, 544> oam{};
	// Set up some sprites
	for (int i = 0; i < 128; i++) {
		oam[i * 4 + 0] = static_cast<uint8_t>(i * 2);      // X position
		oam[i * 4 + 1] = static_cast<uint8_t>(100 + (i % 10));  // Y position
		oam[i * 4 + 2] = static_cast<uint8_t>(i);  // Tile
		oam[i * 4 + 3] = 0;  // Attributes
	}

	uint16_t scanline = 100;
	uint8_t spriteHeight = 8;

	for (auto _ : state) {
		int spritesOnLine = 0;
		for (int i = 0; i < 128 && spritesOnLine < 32; i++) {
			uint8_t y = oam[i * 4 + 1];
			if (scanline >= y && scanline < y + spriteHeight) {
				spritesOnLine++;
			}
		}
		benchmark::DoNotOptimize(spritesOnLine);
	}
	state.SetItemsProcessed(state.iterations() * 128);
}
BENCHMARK(BM_SnesPpu_OamEvaluation);

// -----------------------------------------------------------------------------
// Common PPU Benchmarks (All Platforms)
// -----------------------------------------------------------------------------

// Benchmark RGB555 to RGB888 conversion
static void BM_Ppu_Rgb555ToRgb888(benchmark::State& state) {
	uint16_t color555 = 0x1F | (0x1F << 5) | (0x1F << 10);  // White

	for (auto _ : state) {
		uint8_t r = ColorUtilities::Convert5BitTo8Bit(color555 & 0x1F);
		uint8_t g = ColorUtilities::Convert5BitTo8Bit((color555 >> 5) & 0x1F);
		uint8_t b = ColorUtilities::Convert5BitTo8Bit((color555 >> 10) & 0x1F);
		uint32_t rgb888 = 0xFF000000 | r | (g << 8) | (b << 16);
		benchmark::DoNotOptimize(rgb888);
		color555++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Ppu_Rgb555ToRgb888);

// Benchmark scanline buffer copy to frame buffer
static void BM_Ppu_ScanlineBufferCopy(benchmark::State& state) {
	std::array<uint32_t, 256> scanline{};
	std::array<uint32_t, 256 * 240> frameBuffer{};
	std::fill(scanline.begin(), scanline.end(), 0xFFFFFFFF);
	uint32_t scanlineIndex = 0;

	for (auto _ : state) {
		std::memcpy(&frameBuffer[scanlineIndex * 256], scanline.data(), 256 * sizeof(uint32_t));
		scanlineIndex = (scanlineIndex + 1) % 240;
		benchmark::DoNotOptimize(frameBuffer);
	}
	state.SetBytesProcessed(state.iterations() * 256 * sizeof(uint32_t));
}
BENCHMARK(BM_Ppu_ScanlineBufferCopy);

// Benchmark SNES hi-res mode (512 pixels wide)
static void BM_SnesPpu_HiResRender(benchmark::State& state) {
	std::array<uint32_t, 512> scanlineBuffer{};
	std::array<uint32_t, 256> palette{};
	for (int i = 0; i < 256; i++) {
		palette[i] = 0xFF000000 | (i << 16) | (i << 8) | i;
	}

	for (auto _ : state) {
		for (int x = 0; x < 512; x++) {
			uint8_t colorIndex = static_cast<uint8_t>(x & 0xFF);
			scanlineBuffer[x] = palette[colorIndex];
		}
		benchmark::DoNotOptimize(scanlineBuffer);
	}
	state.SetItemsProcessed(state.iterations() * 512);
}
BENCHMARK(BM_SnesPpu_HiResRender);

// Benchmark mosaic effect calculation
static void BM_Ppu_MosaicEffect(benchmark::State& state) {
	std::array<uint32_t, 256> inputScanline{};
	std::array<uint32_t, 256> outputScanline{};
	for (int i = 0; i < 256; i++) {
		inputScanline[i] = static_cast<uint32_t>(i * 0x010101);
	}
	uint8_t mosaicSize = 4;  // 4x4 mosaic

	for (auto _ : state) {
		for (int x = 0; x < 256; x++) {
			int blockX = (x / mosaicSize) * mosaicSize;
			outputScanline[x] = inputScanline[blockX];
		}
		benchmark::DoNotOptimize(outputScanline);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_Ppu_MosaicEffect);

// Benchmark sprite tile lookup with VRAM addressing
static void BM_Ppu_SpriteTileLookup(benchmark::State& state) {
	std::array<uint8_t, 0x10000> vram{};
	// Initialize with pattern
	for (int i = 0; i < 0x10000; i++) {
		vram[i] = static_cast<uint8_t>(i & 0xFF);
	}

	uint16_t baseAddress = 0x4000;
	uint8_t tileIndex = 0;
	uint8_t tileY = 0;

	for (auto _ : state) {
		// Calculate VRAM address for sprite tile row
		uint16_t tileAddr = baseAddress + (tileIndex * 16) + (tileY * 2);
		uint8_t lowByte = vram[tileAddr];
		uint8_t highByte = vram[tileAddr + 1];
		benchmark::DoNotOptimize(lowByte);
		benchmark::DoNotOptimize(highByte);
		tileIndex++;
		tileY = (tileY + 1) & 7;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Ppu_SpriteTileLookup);

// Benchmark background scrolling with wrapping
static void BM_Ppu_BackgroundScrolling(benchmark::State& state) {
	uint16_t scrollX = 0;
	uint16_t scrollY = 0;

	for (auto _ : state) {
		for (int screenX = 0; screenX < 256; screenX++) {
			// Calculate tile position with wrapping (32x32 tile map)
			uint16_t bgX = (scrollX + screenX) & 0x1FF;  // 512 pixel wrap
			uint16_t bgY = scrollY & 0x1FF;

			uint8_t tileX = static_cast<uint8_t>((bgX >> 3) & 0x3F);
			uint8_t tileY = static_cast<uint8_t>((bgY >> 3) & 0x3F);
			uint8_t fineX = static_cast<uint8_t>(bgX & 7);
			uint8_t fineY = static_cast<uint8_t>(bgY & 7);

			benchmark::DoNotOptimize(tileX);
			benchmark::DoNotOptimize(tileY);
			benchmark::DoNotOptimize(fineX);
			benchmark::DoNotOptimize(fineY);
		}
		scrollX++;
		scrollY++;
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_Ppu_BackgroundScrolling);

// ===== SNES Brightness Optimization Benchmarks =====

// Reference: Old per-pixel multiply approach
static void BM_SnesPpu_ApplyBrightness_Multiply(benchmark::State& state) {
	std::array<uint16_t, 256> buffer{};
	for (int i = 0; i < 256; i++) {
		buffer[i] = static_cast<uint16_t>((i & 0x1F) | (((i >> 3) & 0x1F) << 5) | (((i >> 6) & 0x1F) << 10));
	}
	uint8_t brightness = 10;

	for (auto _ : state) {
		for (int x = 0; x < 256; x++) {
			uint16_t pixel = buffer[x];
			uint16_t r = (pixel & 0x1F) * brightness / 15;
			uint16_t g = ((pixel >> 5) & 0x1F) * brightness / 15;
			uint16_t b = ((pixel >> 10) & 0x1F) * brightness / 15;
			buffer[x] = r | (g << 5) | (b << 10);
		}
		benchmark::DoNotOptimize(buffer);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_SnesPpu_ApplyBrightness_Multiply);

// Optimized: LUT-based approach (eliminates 3 multiplies + 3 divides per pixel)
static void BM_SnesPpu_ApplyBrightness_LUT(benchmark::State& state) {
	// Precompute brightness LUT (same as production code)
	uint8_t brightnessLut[16][32]{};
	for (int b = 0; b < 16; b++) {
		for (int c = 0; c < 32; c++) {
			brightnessLut[b][c] = static_cast<uint8_t>(c * b / 15);
		}
	}

	std::array<uint16_t, 256> buffer{};
	for (int i = 0; i < 256; i++) {
		buffer[i] = static_cast<uint16_t>((i & 0x1F) | (((i >> 3) & 0x1F) << 5) | (((i >> 6) & 0x1F) << 10));
	}
	uint8_t brightness = 10;

	for (auto _ : state) {
		const auto& lut = brightnessLut[brightness];
		for (int x = 0; x < 256; x++) {
			uint16_t pixel = buffer[x];
			uint16_t r = lut[pixel & 0x1F];
			uint16_t g = lut[(pixel >> 5) & 0x1F];
			uint16_t b = lut[(pixel >> 10) & 0x1F];
			buffer[x] = r | (g << 5) | (b << 10);
		}
		benchmark::DoNotOptimize(buffer);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_SnesPpu_ApplyBrightness_LUT);

// ===== Video Filter Loop Benchmarks =====

// Reference: Nested loop with per-pixel multiply (old GB/GBA filter pattern)
static void BM_VideoFilter_NestedLoop(benchmark::State& state) {
	constexpr uint32_t Width = 160;
	constexpr uint32_t Height = 144;
	constexpr uint32_t PixelCount = Width * Height;
	std::array<uint32_t, PixelCount> out{};
	std::array<uint16_t, PixelCount> input{};
	for (uint32_t i = 0; i < PixelCount; i++) input[i] = static_cast<uint16_t>(i & 0x7FFF);

	for (auto _ : state) {
		for (uint32_t i = 0; i < Height; i++) {
			for (uint32_t j = 0; j < Width; j++) {
				out[i * Width + j] = 0xFF000000 | input[i * Width + j];
			}
		}
		benchmark::DoNotOptimize(out);
	}
	state.SetItemsProcessed(state.iterations() * PixelCount);
}
BENCHMARK(BM_VideoFilter_NestedLoop);

// Optimized: Flat loop (eliminates per-pixel multiply)
static void BM_VideoFilter_FlatLoop(benchmark::State& state) {
	constexpr uint32_t Width = 160;
	constexpr uint32_t Height = 144;
	constexpr uint32_t PixelCount = Width * Height;
	std::array<uint32_t, PixelCount> out{};
	std::array<uint16_t, PixelCount> input{};
	for (uint32_t i = 0; i < PixelCount; i++) input[i] = static_cast<uint16_t>(i & 0x7FFF);

	for (auto _ : state) {
		for (uint32_t idx = 0; idx < PixelCount; idx++) {
			out[idx] = 0xFF000000 | input[idx];
		}
		benchmark::DoNotOptimize(out);
	}
	state.SetItemsProcessed(state.iterations() * PixelCount);
}
BENCHMARK(BM_VideoFilter_FlatLoop);

// ===== NES DecodePpuBuffer Benchmarks =====

// Reference: Per-pixel source index calculation
static void BM_NesPpu_DecodePpu_PerPixelCalc(benchmark::State& state) {
	constexpr uint32_t BaseWidth = 256;
	constexpr uint32_t FrameWidth = 240;
	constexpr uint32_t FrameHeight = 224;
	constexpr uint32_t OverscanTop = 8;
	constexpr uint32_t OverscanLeft = 8;
	std::array<uint16_t, BaseWidth * 240> ppuBuffer{};
	std::array<uint32_t, 0x8000> palette{};
	std::array<uint32_t, FrameWidth * FrameHeight> output{};
	for (auto& p : palette) p = 0xFF000000;
	for (auto& p : ppuBuffer) p = 0;

	for (auto _ : state) {
		uint32_t* out = output.data();
		for (uint32_t i = 0; i < FrameHeight; i++) {
			for (uint32_t j = 0; j < FrameWidth; j++) {
				*out = palette[ppuBuffer[(i + OverscanTop) * BaseWidth + j + OverscanLeft]];
				out++;
			}
		}
		benchmark::DoNotOptimize(output);
	}
	state.SetItemsProcessed(state.iterations() * FrameWidth * FrameHeight);
}
BENCHMARK(BM_NesPpu_DecodePpu_PerPixelCalc);

// Optimized: Row pointer hoisted outside inner loop
static void BM_NesPpu_DecodePpu_RowPointer(benchmark::State& state) {
	constexpr uint32_t BaseWidth = 256;
	constexpr uint32_t FrameWidth = 240;
	constexpr uint32_t FrameHeight = 224;
	constexpr uint32_t OverscanTop = 8;
	constexpr uint32_t OverscanLeft = 8;
	std::array<uint16_t, BaseWidth * 240> ppuBuffer{};
	std::array<uint32_t, 0x8000> palette{};
	std::array<uint32_t, FrameWidth * FrameHeight> output{};
	for (auto& p : palette) p = 0xFF000000;
	for (auto& p : ppuBuffer) p = 0;

	for (auto _ : state) {
		uint32_t* out = output.data();
		for (uint32_t i = 0; i < FrameHeight; i++) {
			const uint16_t* srcRow = ppuBuffer.data() + (i + OverscanTop) * BaseWidth + OverscanLeft;
			for (uint32_t j = 0; j < FrameWidth; j++) {
				*out++ = palette[srcRow[j]];
			}
		}
		benchmark::DoNotOptimize(output);
	}
	state.SetItemsProcessed(state.iterations() * FrameWidth * FrameHeight);
}
BENCHMARK(BM_NesPpu_DecodePpu_RowPointer);

// ===== GB PPU Pixel Write Benchmarks =====

// Reference: Per-pixel multiply (scanline * width + drawnPixels)
static void BM_GbPpu_WritePixel_Multiply(benchmark::State& state) {
	std::array<uint16_t, 160 * 144> buffer{};
	uint16_t scanline = 72;

	for (auto _ : state) {
		for (int16_t pixel = 0; pixel < 160; pixel++) {
			uint16_t outOffset = scanline * 160 + pixel;
			buffer[outOffset] = 0x7FFF;
		}
		benchmark::DoNotOptimize(buffer);
	}
	state.SetItemsProcessed(state.iterations() * 160);
}
BENCHMARK(BM_GbPpu_WritePixel_Multiply);

// Optimized: Cached scanline offset (add only)
static void BM_GbPpu_WritePixel_CachedOffset(benchmark::State& state) {
	std::array<uint16_t, 160 * 144> buffer{};
	uint16_t scanline = 72;
	uint16_t scanlineOffset = scanline * 160;

	for (auto _ : state) {
		for (int16_t pixel = 0; pixel < 160; pixel++) {
			uint16_t outOffset = scanlineOffset + pixel;
			buffer[outOffset] = 0x7FFF;
		}
		benchmark::DoNotOptimize(buffer);
	}
	state.SetItemsProcessed(state.iterations() * 160);
}
BENCHMARK(BM_GbPpu_WritePixel_CachedOffset);

// =============================================================================
// SNES Video Filter Benchmarks (Phase 6)
// =============================================================================

// Reference: Nested loop with per-pixel multiply (original SNES video filter)
static void BM_SnesVideoFilter_NestedMultiply(benchmark::State& state) {
	constexpr uint32_t BaseWidth = 512;
	constexpr uint32_t FrameWidth = 488;
	constexpr uint32_t FrameHeight = 448;
	constexpr uint32_t OverscanLeft = 12;
	constexpr uint32_t OverscanTop = 15;

	std::vector<uint16_t> ppuBuffer(BaseWidth * 512, 0);
	for (auto& p : ppuBuffer) p = static_cast<uint16_t>(rand() & 0x7FFF);
	std::array<uint32_t, 0x8000> palette{};
	for (int i = 0; i < 0x8000; i++) palette[i] = 0xFF000000 | i;
	std::vector<uint32_t> output(FrameWidth * FrameHeight, 0);

	uint32_t xOffset = OverscanLeft;
	uint32_t yOffset = OverscanTop * BaseWidth;

	for (auto _ : state) {
		for (uint32_t i = 0; i < FrameHeight; i++) {
			for (uint32_t j = 0; j < FrameWidth; j++) {
				output[i * FrameWidth + j] = palette[ppuBuffer[i * BaseWidth + j + yOffset + xOffset]];
			}
		}
		benchmark::DoNotOptimize(output);
	}
	state.SetItemsProcessed(state.iterations() * FrameWidth * FrameHeight);
}
BENCHMARK(BM_SnesVideoFilter_NestedMultiply);

// Optimized: Row pointer hoisting with flat output index
static void BM_SnesVideoFilter_RowPointer(benchmark::State& state) {
	constexpr uint32_t BaseWidth = 512;
	constexpr uint32_t FrameWidth = 488;
	constexpr uint32_t FrameHeight = 448;
	constexpr uint32_t OverscanLeft = 12;
	constexpr uint32_t OverscanTop = 15;

	std::vector<uint16_t> ppuBuffer(BaseWidth * 512, 0);
	for (auto& p : ppuBuffer) p = static_cast<uint16_t>(rand() & 0x7FFF);
	std::array<uint32_t, 0x8000> palette{};
	for (int i = 0; i < 0x8000; i++) palette[i] = 0xFF000000 | i;
	std::vector<uint32_t> output(FrameWidth * FrameHeight, 0);

	uint32_t xOffset = OverscanLeft;
	uint32_t yOffset = OverscanTop * BaseWidth;

	for (auto _ : state) {
		uint32_t outIdx = 0;
		uint32_t srcOffset = yOffset + xOffset;
		for (uint32_t i = 0; i < FrameHeight; i++) {
			for (uint32_t j = 0; j < FrameWidth; j++) {
				output[outIdx++] = palette[ppuBuffer[srcOffset + j]];
			}
			srcOffset += BaseWidth;
		}
		benchmark::DoNotOptimize(output);
	}
	state.SetItemsProcessed(state.iterations() * FrameWidth * FrameHeight);
}
BENCHMARK(BM_SnesVideoFilter_RowPointer);

// Reference: SNES ConvertToHiRes with double memory read per pixel
static void BM_SnesConvertToHiRes_DoubleRead(benchmark::State& state) {
	constexpr size_t BufSize = 240 * 1024;
	std::vector<uint16_t> buffer(BufSize, 0);
	for (int i = 0; i < 240; i++)
		for (int x = 0; x < 256; x++)
			buffer[(i << 8) + x] = static_cast<uint16_t>((i * 256 + x) & 0x7FFF);

	for (auto _ : state) {
		for (int i = 239; i >= 0; i--) {
			for (int x = 0; x < 256; x++) {
				buffer[(i << 10) + (x << 1)] = buffer[(i << 8) + x];
				buffer[(i << 10) + (x << 1) + 1] = buffer[(i << 8) + x];
			}
		}
		benchmark::DoNotOptimize(buffer);
	}
	state.SetItemsProcessed(state.iterations() * 240 * 256);
}
BENCHMARK(BM_SnesConvertToHiRes_DoubleRead);

// Optimized: SNES ConvertToHiRes with cached pixel read
static void BM_SnesConvertToHiRes_CachedPixel(benchmark::State& state) {
	constexpr size_t BufSize = 240 * 1024;
	std::vector<uint16_t> buffer(BufSize, 0);
	for (int i = 0; i < 240; i++)
		for (int x = 0; x < 256; x++)
			buffer[(i << 8) + x] = static_cast<uint16_t>((i * 256 + x) & 0x7FFF);

	for (auto _ : state) {
		for (int i = 239; i >= 0; i--) {
			uint16_t* src = buffer.data() + (i << 8);
			uint16_t* dst = buffer.data() + (i << 10);
			for (int x = 0; x < 256; x++) {
				uint16_t pixel = src[x];
				dst[x << 1] = pixel;
				dst[(x << 1) + 1] = pixel;
			}
		}
		benchmark::DoNotOptimize(buffer);
	}
	state.SetItemsProcessed(state.iterations() * 240 * 256);
}
BENCHMARK(BM_SnesConvertToHiRes_CachedPixel);

// =============================================================================
// NES PPU Sprite Evaluation Loop
// =============================================================================
// The NES PPU evaluates all 64 OAM sprites every scanline to find the 8
// visible sprites for the next scanline. This is a tight state-machine loop
// called on cycles 65-256 (192 cycles per scanline).
//
// The evaluation logic in NesPpu<T>::ProcessSpriteEvaluation():
//   - Cycles 1-64: Clear secondary OAM (32 bytes)
//   - Cycles 65-256: Walk all 64 sprites, copy Y+tile+attr+X for visible ones
//   - Each visible sprite requires 4 bytes copied to secondary OAM
//   - Sprite overflow flag set if more than 8 are visible
//
// This benchmark simulates the core evaluation loop without PPU state machine
// complexity, measuring the Y-range check + copy throughput.
// =============================================================================

namespace {
	// Sprite Y-range check (the hot path in ProcessSpriteEvaluation even cycles)
	inline bool SpriteInRange(uint8_t spriteY, uint8_t scanline, bool largeSprites) {
		return scanline >= spriteY && scanline < spriteY + (largeSprites ? 16u : 8u);
	}
} // anonymous namespace

// Benchmark: NES sprite evaluation — all 64 sprites, zero visible (best case)
// This is the "idle" cost when the scanline has no sprites.
static void BM_NesPpu_SpriteEval_NoneVisible(benchmark::State& state) {
	// 64 sprites all at Y=0xFF (off-screen, below scanline 239)
	alignas(64) uint8_t oam[256];
	for (int i = 0; i < 64; i++) {
		oam[i * 4 + 0] = 0xFF; // Y: below visible range
		oam[i * 4 + 1] = static_cast<uint8_t>(i); // Tile
		oam[i * 4 + 2] = 0x00; // Attributes
		oam[i * 4 + 3] = static_cast<uint8_t>(i * 4); // X
	}

	alignas(64) uint8_t secondaryOam[32];
	memset(secondaryOam, 0xFF, sizeof(secondaryOam));

	uint8_t scanline = 100;

	for (auto _ : state) {
		int spriteCount = 0;
		for (int i = 0; i < 64; i++) {
			uint8_t spriteY = oam[i * 4];
			if (SpriteInRange(spriteY, scanline, false)) {
				if (spriteCount < 8) {
					memcpy(&secondaryOam[spriteCount * 4], &oam[i * 4], 4);
					spriteCount++;
				} else {
					break; // Overflow: NES stops evaluating after 8 sprites
				}
			}
		}
		benchmark::DoNotOptimize(spriteCount);
		scanline = (scanline + 1) % 240;
	}
	state.SetItemsProcessed(state.iterations() * 64); // All 64 sprites scanned
}
BENCHMARK(BM_NesPpu_SpriteEval_NoneVisible);

// Benchmark: NES sprite evaluation — 8 sprites visible (full secondary OAM fill)
// This is the common case in typical NES games.
static void BM_NesPpu_SpriteEval_8Visible(benchmark::State& state) {
	// Place 8 sprites on scanline 100, rest off-screen
	alignas(64) uint8_t oam[256];
	memset(oam, 0xFF, sizeof(oam));
	for (int i = 0; i < 8; i++) {
		oam[i * 4 + 0] = 96;  // Y=96: visible on scanlines 96-103
		oam[i * 4 + 1] = static_cast<uint8_t>(i * 16); // Tile
		oam[i * 4 + 2] = 0x00; // Attributes
		oam[i * 4 + 3] = static_cast<uint8_t>(i * 30); // X
	}

	alignas(64) uint8_t secondaryOam[32];

	for (auto _ : state) {
		memset(secondaryOam, 0xFF, 32); // Clear secondary OAM first
		int spriteCount = 0;
		for (int i = 0; i < 64; i++) {
			uint8_t spriteY = oam[i * 4];
			if (SpriteInRange(spriteY, 100, false)) {
				if (spriteCount < 8) {
					memcpy(&secondaryOam[spriteCount * 4], &oam[i * 4], 4);
					spriteCount++;
				} else {
					break;
				}
			}
		}
		benchmark::DoNotOptimize(spriteCount);
	}
	state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_NesPpu_SpriteEval_8Visible);

// Benchmark: NES sprite evaluation — max density (64 sprites across screen)
// Worst case: many sprites visible, forces full 64-sprite scan every scanline.
static void BM_NesPpu_SpriteEval_MaxDensity(benchmark::State& state) {
	// Spread 64 sprites evenly: Y values 0, 4, 8, ... 252 (8-pixel separation)
	alignas(64) uint8_t oam[256];
	for (int i = 0; i < 64; i++) {
		oam[i * 4 + 0] = static_cast<uint8_t>(i * 4);  // Y: 0..252
		oam[i * 4 + 1] = static_cast<uint8_t>(i);       // Tile
		oam[i * 4 + 2] = 0x00;                          // Attributes
		oam[i * 4 + 3] = static_cast<uint8_t>(i * 4);  // X
	}

	alignas(64) uint8_t secondaryOam[32];
	uint8_t scanline = 0;

	for (auto _ : state) {
		memset(secondaryOam, 0xFF, 32);
		int spriteCount = 0;
		bool overflow = false;
		for (int i = 0; i < 64; i++) {
			uint8_t spriteY = oam[i * 4];
			if (SpriteInRange(spriteY, scanline, false)) {
				if (spriteCount < 8) {
					memcpy(&secondaryOam[spriteCount * 4], &oam[i * 4], 4);
					spriteCount++;
				} else {
					overflow = true;
					break;
				}
			}
		}
		benchmark::DoNotOptimize(spriteCount);
		benchmark::DoNotOptimize(overflow);
		scanline = (scanline + 1) % 240;
	}
	state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_NesPpu_SpriteEval_MaxDensity);

// Benchmark: NES sprite evaluation with large sprites (8×16) — harder range check
static void BM_NesPpu_SpriteEval_LargeSprites_8x16(benchmark::State& state) {
	alignas(64) uint8_t oam[256];
	// 8 large (8×16) sprites covering half the screen
	memset(oam, 0xFF, sizeof(oam));
	for (int i = 0; i < 8; i++) {
		oam[i * 4 + 0] = static_cast<uint8_t>(i * 20); // Y: 0,20,40,...140
		oam[i * 4 + 1] = static_cast<uint8_t>(i * 2);  // Tile
		oam[i * 4 + 2] = 0x00;
		oam[i * 4 + 3] = static_cast<uint8_t>(i * 28);
	}

	alignas(64) uint8_t secondaryOam[32];
	uint8_t scanline = 50;

	for (auto _ : state) {
		memset(secondaryOam, 0xFF, 32);
		int spriteCount = 0;
		for (int i = 0; i < 64; i++) {
			uint8_t spriteY = oam[i * 4];
			if (SpriteInRange(spriteY, scanline, true)) { // 8×16 sprites
				if (spriteCount < 8) {
					memcpy(&secondaryOam[spriteCount * 4], &oam[i * 4], 4);
					spriteCount++;
				} else {
					break;
				}
			}
		}
		benchmark::DoNotOptimize(spriteCount);
		scanline = (scanline + 1) % 240;
	}
	state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_NesPpu_SpriteEval_LargeSprites_8x16);

// =============================================================================
// NES PPU GetPixelColor — Per-Pixel Priority Resolution
// =============================================================================
// GetPixelColor() is called 256 times per visible scanline (61,440/frame).
// For each pixel it:
//   1. Reads background shift registers to get BG color
//   2. Scans up to 8 sprite tiles to find the first covering this pixel
//   3. Checks sprite priority vs BG priority
//   4. Sets sprite-0 hit flag if applicable
//
// The hot path has [[unlikely]] on sprite checks but still scans up to 8 sprites.
// =============================================================================

// Benchmark: GetPixelColor with 0 sprites (BG-only path — fastest)
static void BM_NesPpu_PixelColor_BgOnly(benchmark::State& state) {
	// Simulate shift register state for BG tiles
	uint16_t lowBitShift  = 0xA5A5;
	uint16_t highBitShift = 0x5A5A;
	uint8_t  xScroll = 3;

	for (auto _ : state) {
		uint8_t offset = xScroll;
		// Simulate reading BG color bits from shift registers
		uint8_t bgColor = (uint8_t)(
			(((lowBitShift  << offset) & 0x8000) >> 15) |
			(((highBitShift << offset) & 0x8000) >> 14));

		benchmark::DoNotOptimize(bgColor);
		lowBitShift  = (lowBitShift  << 1) | (lowBitShift  >> 15);
		highBitShift = (highBitShift << 1) | (highBitShift >> 15);
		xScroll = (xScroll + 1) & 7;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesPpu_PixelColor_BgOnly);

// Benchmark: GetPixelColor with 8 sprites — worst-case sprite scan per pixel
// Simulates the full sprite priority loop from NesPpu<T>::GetPixelColor()
static void BM_NesPpu_PixelColor_8Sprites_PriorityResolution(benchmark::State& state) {
	// Set up 8 sprite tiles at various X positions
	// Uses NesSpriteInfo-compatible layout
	struct SpriteTileData {
		bool HorizontalMirror = false;
		bool BackgroundPriority = false;
		uint8_t SpriteX = 0;
		uint8_t LowByte = 0;
		uint8_t HighByte = 0;
		uint8_t PaletteOffset = 16;
	};

	std::array<SpriteTileData, 8> spriteTiles{};
	for (int i = 0; i < 8; i++) {
		spriteTiles[i].SpriteX        = static_cast<uint8_t>(i * 32);
		spriteTiles[i].LowByte        = static_cast<uint8_t>(0xAA >> i);
		spriteTiles[i].HighByte       = static_cast<uint8_t>(0x55 << i);
		spriteTiles[i].BackgroundPriority = (i % 3 == 0);
		spriteTiles[i].HorizontalMirror   = (i % 2 == 0);
		spriteTiles[i].PaletteOffset      = static_cast<uint8_t>(16 + i * 4);
	}

	uint16_t lowBitShift  = 0xA5A5;
	uint16_t highBitShift = 0x5A5A;
	uint8_t xScroll = 0;
	int cycle = 1;

	for (auto _ : state) {
		uint8_t offset = xScroll;
		// BG color
		uint8_t bgColor = (uint8_t)(
			(((lowBitShift  << offset) & 0x8000) >> 15) |
			(((highBitShift << offset) & 0x8000) >> 14));

		// Sprite scan (mirrors GetPixelColor inner loop)
		uint8_t resultColor = bgColor;
		bool sprite0Hit = false;
		for (int i = 0; i < 8; i++) {
			int32_t shift = cycle - (int32_t)spriteTiles[i].SpriteX - 1;
			if (shift >= 0 && shift < 8) {
				uint8_t spriteColor;
				if (spriteTiles[i].HorizontalMirror) {
					spriteColor = ((spriteTiles[i].LowByte >> shift) & 0x01) |
					              (((spriteTiles[i].HighByte >> shift) & 0x01) << 1);
				} else {
					spriteColor = (((spriteTiles[i].LowByte << shift) & 0x80) >> 7) |
					              (((spriteTiles[i].HighByte << shift) & 0x80) >> 6);
				}
				if (spriteColor != 0) {
					if (i == 0 && bgColor != 0) [[unlikely]] {
						sprite0Hit = true;
					}
					if (bgColor == 0 || !spriteTiles[i].BackgroundPriority) {
						resultColor = spriteTiles[i].PaletteOffset + spriteColor;
					}
					break;
				}
			}
		}
		benchmark::DoNotOptimize(resultColor);
		benchmark::DoNotOptimize(sprite0Hit);
		cycle++;
		if (cycle > 256) cycle = 1;
		lowBitShift = (lowBitShift << 1) | (lowBitShift >> 15);
		highBitShift = (highBitShift << 1) | (highBitShift >> 15);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesPpu_PixelColor_8Sprites_PriorityResolution);

// Benchmark: Full 256-pixel scanline with 8 sprites (57,344 iterations/frame)
static void BM_NesPpu_PixelColor_FullScanline_8Sprites(benchmark::State& state) {
	struct SpriteTile {
		bool HorizontalMirror;
		bool BackgroundPriority;
		uint8_t SpriteX;
		uint8_t LowByte;
		uint8_t HighByte;
		uint8_t PaletteOffset;
	};

	std::array<SpriteTile, 8> sprites{};
	for (int i = 0; i < 8; i++) {
		sprites[i] = {
			.HorizontalMirror  = (i % 2 == 0),
			.BackgroundPriority = (i % 3 == 0),
			.SpriteX           = static_cast<uint8_t>(i * 30 + 8),
			.LowByte           = static_cast<uint8_t>(0b10110101),
			.HighByte          = static_cast<uint8_t>(0b01001010),
			.PaletteOffset     = static_cast<uint8_t>(16 + i * 4)
		};
	}

	alignas(64) uint8_t scanlineOutput[256] = {};
	uint16_t lowBitShift  = 0xC3C3;
	uint16_t highBitShift = 0x3C3C;

	for (auto _ : state) {
		for (int cycle = 1; cycle <= 256; cycle++) {
			uint8_t offset = 0; // xScroll = 0 for bench
			uint8_t bgColor = (uint8_t)(
				(((lowBitShift  << offset) & 0x8000) >> 15) |
				(((highBitShift << offset) & 0x8000) >> 14));
			lowBitShift  <<= 1;
			highBitShift <<= 1;

			uint8_t resultColor = bgColor;
			for (int i = 0; i < 8; i++) {
				int32_t shift = cycle - (int32_t)sprites[i].SpriteX - 1;
				if (shift >= 0 && shift < 8) {
					uint8_t sc = sprites[i].HorizontalMirror
						? ((sprites[i].LowByte >> shift) & 1) | (((sprites[i].HighByte >> shift) & 1) << 1)
						: (((sprites[i].LowByte << shift) & 0x80) >> 7) | (((sprites[i].HighByte << shift) & 0x80) >> 6);
					if (sc != 0) {
						if (bgColor == 0 || !sprites[i].BackgroundPriority) {
							resultColor = sprites[i].PaletteOffset + sc;
						}
						break;
					}
				}
			}
			scanlineOutput[cycle - 1] = resultColor;
		}
		benchmark::DoNotOptimize(scanlineOutput[0]);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_NesPpu_PixelColor_FullScanline_8Sprites);
