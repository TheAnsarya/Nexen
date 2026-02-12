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

