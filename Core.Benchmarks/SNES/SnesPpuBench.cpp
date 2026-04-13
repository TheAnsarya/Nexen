#include "pch.h"
#include <array>
#include <cstring>
#include "SNES/SnesPpuTypes.h"

// =============================================================================
// SNES PPU Rendering Hot-Path Benchmarks
// =============================================================================
// The SNES PPU (Picture Processing Unit) is one of the hottest code paths in
// SNES emulation. SnesPpu.cpp is 2,720 lines and processes 57,344 pixels per
// frame (256×224). These benchmarks isolate the individual per-pixel and
// per-scanline operations that run every frame.
//
// Key hot paths measured here:
//   1. 4bpp tile pixel extraction (VRAM reads per pixel)
//   2. Color math per pixel (add/subtract/halve with window masking)
//   3. Mode 7 affine transform (matrix multiply per pixel)
//   4. Window mask computation (6-layer OR/AND/XOR logic)
//   5. CGRAM palette lookup (cgram[colorIndex])
//   6. Per-scanline settings fetch cost
// =============================================================================

// -----------------------------------------------------------------------------
// 1. SNES 4bpp Tile Pixel Extraction
// -----------------------------------------------------------------------------
// SNES 4bpp format: each tile row uses 4 bitplanes.
// Planes 0-1 are interleaved as bytes in one 16-bit word per row.
// Planes 2-3 are interleaved as bytes in a second 16-bit word.
// Given a tile address in VRAM, extracting pixel X requires combining
// one bit from each of the 4 bitplanes.
// This loop simulates the inner tile-fetch hot path in RenderTilemap<>.

namespace {
	// Extract one 4bpp color index for pixel column `col` (0-7) from 4 bitplane bytes
	inline uint8_t Extract4bppPixel(uint8_t bp0, uint8_t bp1, uint8_t bp2, uint8_t bp3, uint8_t col) {
		uint8_t shift = 7 - col;
		return (uint8_t)(
			(((bp0 >> shift) & 1) << 0) |
			(((bp1 >> shift) & 1) << 1) |
			(((bp2 >> shift) & 1) << 2) |
			(((bp3 >> shift) & 1) << 3));
	}

	// Extract one 2bpp color index (BG Mode 0)
	inline uint8_t Extract2bppPixel(uint8_t bp0, uint8_t bp1, uint8_t col) {
		uint8_t shift = 7 - col;
		return (uint8_t)(
			(((bp0 >> shift) & 1) << 0) |
			(((bp1 >> shift) & 1) << 1));
	}

	// Extract one 8bpp color index (Mode 7)
	inline uint8_t Extract8bppPixel(const uint8_t* row8, uint8_t col) {
		return row8[col];
	}
} // anonymous namespace

// Benchmark: Extract one 4bpp pixel (single pixel, no loop) — baseline cost
static void BM_SnesPpu_TileExtract_4bpp_SinglePixel(benchmark::State& state) {
	uint8_t bp0 = 0xAA, bp1 = 0x55, bp2 = 0xCC, bp3 = 0x33;
	uint8_t col = 0;
	for (auto _ : state) {
		uint8_t px = Extract4bppPixel(bp0, bp1, bp2, bp3, col);
		benchmark::DoNotOptimize(px);
		col = (col + 1) & 7;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesPpu_TileExtract_4bpp_SinglePixel);

// Benchmark: Extract all 8 pixels of a 4bpp tile row
static void BM_SnesPpu_TileExtract_4bpp_FullRow(benchmark::State& state) {
	uint8_t bp0 = 0xAA, bp1 = 0x55, bp2 = 0xCC, bp3 = 0x33;
	std::array<uint8_t, 8> pixels{};
	for (auto _ : state) {
		for (uint8_t col = 0; col < 8; col++) {
			pixels[col] = Extract4bppPixel(bp0, bp1, bp2, bp3, col);
		}
		benchmark::DoNotOptimize(pixels);
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_SnesPpu_TileExtract_4bpp_FullRow);

// Benchmark: Extract 256-pixel scanline worth of 4bpp tiles (32 tiles)
static void BM_SnesPpu_TileExtract_4bpp_Scanline(benchmark::State& state) {
	// Simulate 32 tiles × 8 pixels = 256 pixels per BG scanline
	alignas(64) uint8_t vram[32 * 8 * 2] = {}; // 32 tiles, 8 rows, 2 words per row
	for (int i = 0; i < (int)sizeof(vram); i++) {
		vram[i] = static_cast<uint8_t>(i * 37 + 19);
	}

	std::array<uint8_t, 256> lineBuffer{};

	for (auto _ : state) {
		for (int tile = 0; tile < 32; tile++) {
			uint8_t bp0 = vram[tile * 4 + 0];
			uint8_t bp1 = vram[tile * 4 + 1];
			uint8_t bp2 = vram[tile * 4 + 2];
			uint8_t bp3 = vram[tile * 4 + 3];
			for (uint8_t col = 0; col < 8; col++) {
				lineBuffer[tile * 8 + col] = Extract4bppPixel(bp0, bp1, bp2, bp3, col);
			}
		}
		benchmark::DoNotOptimize(lineBuffer[0]);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_SnesPpu_TileExtract_4bpp_Scanline);

// Benchmark: 2bpp scanline (Mode 0 — most common BG mode outside JRPG games)
static void BM_SnesPpu_TileExtract_2bpp_Scanline(benchmark::State& state) {
	alignas(64) uint8_t vram[32 * 2] = {};
	for (int i = 0; i < (int)sizeof(vram); i++) vram[i] = static_cast<uint8_t>(i * 37);

	std::array<uint8_t, 256> lineBuffer{};
	for (auto _ : state) {
		for (int tile = 0; tile < 32; tile++) {
			uint8_t bp0 = vram[tile * 2 + 0];
			uint8_t bp1 = vram[tile * 2 + 1];
			for (uint8_t col = 0; col < 8; col++) {
				lineBuffer[tile * 8 + col] = Extract2bppPixel(bp0, bp1, col);
			}
		}
		benchmark::DoNotOptimize(lineBuffer[0]);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_SnesPpu_TileExtract_2bpp_Scanline);

// -----------------------------------------------------------------------------
// 2. SNES CGRAM Palette Lookup
// -----------------------------------------------------------------------------
// The CGRAM holds up to 256 15-bit BGR colors. Every non-transparent pixel
// requires a CGRAM lookup: paletteColor = _cgram[colorIndex].
// This is a simple array read but cache behavior matters at scale.

static void BM_SnesPpu_CgramLookup_Sequential(benchmark::State& state) {
	alignas(64) uint16_t cgram[256] = {};
	for (int i = 0; i < 256; i++) cgram[i] = static_cast<uint16_t>(i * 0x1F);

	uint8_t colorIndex = 0;
	for (auto _ : state) {
		uint16_t color = cgram[colorIndex];
		benchmark::DoNotOptimize(color);
		colorIndex++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesPpu_CgramLookup_Sequential);

// Benchmark: CGRAM lookup for a full 256-pixel scanline
static void BM_SnesPpu_CgramLookup_Scanline(benchmark::State& state) {
	alignas(64) uint16_t cgram[256] = {};
	for (int i = 0; i < 256; i++) cgram[i] = static_cast<uint16_t>(i * 0x1F);

	// Simulate pseudorandom pixel color indices (not sequential)
	std::array<uint8_t, 256> colorIndices{};
	for (int i = 0; i < 256; i++) colorIndices[i] = static_cast<uint8_t>((i * 37 + 13) & 0xFF);

	std::array<uint16_t, 256> lineOutput{};

	for (auto _ : state) {
		for (int x = 0; x < 256; x++) {
			lineOutput[x] = cgram[colorIndices[x]];
		}
		benchmark::DoNotOptimize(lineOutput[0]);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_SnesPpu_CgramLookup_Scanline);

// -----------------------------------------------------------------------------
// 3. SNES Color Math (ApplyColorMathToPixel)
// -----------------------------------------------------------------------------
// Called for every pixel in ApplyColorMath(). Two variations:
// - Standard: add or subtract subscreen color from main screen
// - Halved: result is divided by 2 (common "transparency" effect)
// With window masking: an extra lookup + branch per pixel

// Standalone color math implementations (matching ApplyColorMathToPixel logic)
namespace {
	// Add two 15-bit RGB555 colors (SNES hardware formula)
	inline uint16_t ColorAdd(uint16_t a, uint16_t b, uint8_t halfShift) {
		uint16_t r = ((a & 0x7C00) >> 10) + ((b & 0x7C00) >> 10);
		uint16_t g = ((a & 0x03E0) >> 5)  + ((b & 0x03E0) >> 5);
		uint16_t bl = ((a & 0x001F))       + ((b & 0x001F));
		if (r > 31) r = 31;
		if (g > 31) g = 31;
		if (bl > 31) bl = 31;
		r >>= halfShift;
		g >>= halfShift;
		bl >>= halfShift;
		return static_cast<uint16_t>((r << 10) | (g << 5) | bl);
	}

	// Subtract two 15-bit RGB555 colors (SNES hardware formula)
	inline uint16_t ColorSub(uint16_t a, uint16_t b, uint8_t halfShift) {
		int16_t r = (int16_t)((a & 0x7C00) >> 10) - (int16_t)((b & 0x7C00) >> 10);
		int16_t g = (int16_t)((a & 0x03E0) >> 5)  - (int16_t)((b & 0x03E0) >> 5);
		int16_t bl = (int16_t)(a & 0x001F)         - (int16_t)(b & 0x001F);
		if (r < 0) r = 0;
		if (g < 0) g = 0;
		if (bl < 0) bl = 0;
		r >>= halfShift;
		g >>= halfShift;
		bl >>= halfShift;
		return static_cast<uint16_t>((r << 10) | (g << 5) | bl);
	}
} // anonymous namespace

// Benchmark: Color addition for one pixel (baseline)
static void BM_SnesPpu_ColorMath_Add_SinglePixel(benchmark::State& state) {
	uint16_t mainPixel = 0x4A52; // Some BGR color
	uint16_t subPixel  = 0x1234;
	for (auto _ : state) {
		uint16_t result = ColorAdd(mainPixel, subPixel, 0);
		benchmark::DoNotOptimize(result);
		mainPixel += 7;
		subPixel  += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesPpu_ColorMath_Add_SinglePixel);

// Benchmark: Color addition with halving (divide by 2) for one pixel
static void BM_SnesPpu_ColorMath_AddHalf_SinglePixel(benchmark::State& state) {
	uint16_t mainPixel = 0x4A52;
	uint16_t subPixel  = 0x1234;
	for (auto _ : state) {
		uint16_t result = ColorAdd(mainPixel, subPixel, 1); // halfShift=1
		benchmark::DoNotOptimize(result);
		mainPixel += 7;
		subPixel  += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesPpu_ColorMath_AddHalf_SinglePixel);

// Benchmark: Color subtraction for one pixel
static void BM_SnesPpu_ColorMath_Sub_SinglePixel(benchmark::State& state) {
	uint16_t mainPixel = 0x7C1F;
	uint16_t subPixel  = 0x1234;
	for (auto _ : state) {
		uint16_t result = ColorSub(mainPixel, subPixel, 0);
		benchmark::DoNotOptimize(result);
		mainPixel += 7;
		subPixel  += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesPpu_ColorMath_Sub_SinglePixel);

// Benchmark: Full-scanline color math (256 pixels, no window)
// This is the innermost loop in ApplyColorMath() for non-windowed games
static void BM_SnesPpu_ColorMath_Add_Scanline(benchmark::State& state) {
	alignas(64) uint16_t mainScreen[256];
	alignas(64) uint16_t subScreen[256];
	for (int i = 0; i < 256; i++) {
		mainScreen[i] = static_cast<uint16_t>((i * 37 + 0x4A52) & 0x7FFF);
		subScreen[i]  = static_cast<uint16_t>((i * 13 + 0x1234) & 0x7FFF);
	}

	for (auto _ : state) {
		for (int x = 0; x < 256; x++) {
			mainScreen[x] = ColorAdd(mainScreen[x], subScreen[x], 0);
		}
		benchmark::DoNotOptimize(mainScreen[0]);
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_SnesPpu_ColorMath_Add_Scanline);

// Benchmark: Full-scanline color math with window mask check (branchy, per pixel)
static void BM_SnesPpu_ColorMath_Add_Scanline_Windowed(benchmark::State& state) {
	alignas(64) uint16_t mainScreen[256];
	alignas(64) uint16_t subScreen[256];
	alignas(64) bool windowMask[256];
	for (int i = 0; i < 256; i++) {
		mainScreen[i] = static_cast<uint16_t>((i * 37 + 0x4A52) & 0x7FFF);
		subScreen[i]  = static_cast<uint16_t>((i * 13 + 0x1234) & 0x7FFF);
		windowMask[i] = (i >= 64 && i < 192); // Window active over center 128 pixels
	}

	for (auto _ : state) {
		for (int x = 0; x < 256; x++) {
			if (windowMask[x]) {
				mainScreen[x] = ColorAdd(mainScreen[x], subScreen[x], 0);
			}
		}
		benchmark::DoNotOptimize(mainScreen[0]);
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_SnesPpu_ColorMath_Add_Scanline_Windowed);

// -----------------------------------------------------------------------------
// 4. SNES Mode 7 Affine Transform
// -----------------------------------------------------------------------------
// Mode 7 applies a 2×2 matrix transformation to map screen pixels to VRAM.
// Every pixel requires: xOffset = (xValue >> 8); yOffset = (yValue >> 8);
//                       xValue += xStep; yValue += yStep;
// This is 57,344 multiply-free steps per frame (the matrix is pre-computed
// at scanline start); only add/shift per pixel.
// The expensive part: VRAM access pattern (cache-unfriendly at high rotation).

static void BM_SnesPpu_Mode7_StepAccumulation(benchmark::State& state) {
	// Simulate the per-pixel position accumulation in RenderTilemapMode7
	int32_t xValue = 0x40000;  // Typical start position
	int32_t yValue = 0x20000;
	int16_t xStep  = 0x0100;   // Matrix[0] (no scaling, slight rotation)
	int16_t yStep  = 0x0010;   // Matrix[2]

	for (auto _ : state) {
		int32_t xOffset = xValue >> 8;
		int32_t yOffset = yValue >> 8;
		xValue += xStep;
		yValue += yStep;
		benchmark::DoNotOptimize(xOffset);
		benchmark::DoNotOptimize(yOffset);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesPpu_Mode7_StepAccumulation);

// Benchmark: Full Mode 7 scanline (256 pixels): step + tileIndex + color lookup
static void BM_SnesPpu_Mode7_FullScanline(benchmark::State& state) {
	// Simulate a 1024×1024 Mode 7 map tiled with dummy data
	alignas(64) uint16_t vram[128 * 128] = {};  // 128×128 map words (simplified)
	for (int i = 0; i < 128 * 128; i++) {
		// Each word: low byte = tileIndex, high byte = color (for layer 1 EXTBG)
		vram[i] = static_cast<uint16_t>((i & 0xFF) | ((i & 0x7F) << 8));
	}

	alignas(64) uint16_t cgram[256] = {};
	for (int i = 0; i < 256; i++) cgram[i] = static_cast<uint16_t>(i * 0x121);

	std::array<uint16_t, 256> lineBuffer{};

	int32_t xValue = 0x800000; // Center of 1024×1024 map
	int32_t yValue = 0x400000;
	int16_t xStep  = 0x0100;   // 1.0 = no horizontal scaling
	int16_t yStep  = 0x0010;   // Small vertical shear

	for (auto _ : state) {
		int32_t xv = xValue;
		int32_t yv = yValue;
		for (int x = 0; x < 256; x++) {
			int32_t xOff = (xv >> 8) & 0x3FF;
			int32_t yOff = (yv >> 8) & 0x3FF;
			xv += xStep;
			yv += yStep;

			// Tile lookup (map is 128×128 cells, 8 pixels each = 1024×1024)
			uint16_t mapIdx = static_cast<uint16_t>(((yOff >> 3) & 0x7F) * 128 + ((xOff >> 3) & 0x7F));
			uint8_t tileIndex = static_cast<uint8_t>(vram[mapIdx] & 0xFF);

			// Color from tile pixel data (simplified: use tileIndex + pixel offset)
			uint8_t colorIdx = static_cast<uint8_t>((tileIndex + (xOff & 7) + (yOff & 7)) & 0xFF);
			if (colorIdx != 0) {
				lineBuffer[x] = cgram[colorIdx];
			}
		}
		benchmark::DoNotOptimize(lineBuffer[0]);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_SnesPpu_Mode7_FullScanline);

// -----------------------------------------------------------------------------
// 5. SNES Window Mask Precomputation
// -----------------------------------------------------------------------------
// PrecomputeWindowMasks() is called once per scanline and populates 6 boolean
// arrays of 256 entries each (one per layer including Color and OBJ windows).
// The computation uses WindowConfig data and OR/AND/XOR logic.

// Standalone window mask fill simulation
static void BM_SnesPpu_WindowMask_Precompute(benchmark::State& state) {
	// Simulate two overlapping windows: window1 [left1..right1], window2 [left2..right2]
	uint8_t left1 = 32,  right1 = 160;  // Window 1: 32..160
	uint8_t left2 = 80,  right2 = 240;  // Window 2: 80..240
	// Logic: 0=OR, 1=AND, 2=XOR, 3=XNOR

	alignas(64) bool wMask[6][256] = {};

	for (auto _ : state) {
		// For each of 6 layers (BG1-4, OBJ, Color)
		for (int layer = 0; layer < 6; layer++) {
			for (int x = 0; x < 256; x++) {
				bool w1 = (x >= left1 && x <= right1);
				bool w2 = (x >= left2 && x <= right2);
				// OR logic (simplified: actual code uses MaskLogic enum)
				wMask[layer][x] = w1 || w2;
			}
		}
		benchmark::DoNotOptimize(wMask[0][0]);
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 6 * 256);
}
BENCHMARK(BM_SnesPpu_WindowMask_Precompute);

// Benchmark: Window mask lookup during pixel rendering (6 lookups per pixel)
static void BM_SnesPpu_WindowMask_Lookup_PerPixel(benchmark::State& state) {
	alignas(64) bool wMask[6][256] = {};
	for (int layer = 0; layer < 6; layer++) {
		for (int x = 0; x < 256; x++) {
			wMask[layer][x] = (x >= 32 && x <= 200);
		}
	}

	uint8_t x = 0;
	for (auto _ : state) {
		bool m0 = wMask[0][x];
		bool m1 = wMask[1][x];
		bool m2 = wMask[2][x];
		bool m3 = wMask[3][x];
		bool m4 = wMask[4][x];
		bool m5 = wMask[5][x];
		benchmark::DoNotOptimize(m0 || m1 || m2 || m3 || m4 || m5);
		x++;
	}
	state.SetItemsProcessed(state.iterations() * 6);
}
BENCHMARK(BM_SnesPpu_WindowMask_Lookup_PerPixel);

// Benchmark: Full scanline window mask lookups for all layers
static void BM_SnesPpu_WindowMask_Scanline_AllLayers(benchmark::State& state) {
	alignas(64) bool wMask[6][256] = {};
	for (int layer = 0; layer < 6; layer++) {
		for (int x = 0; x < 256; x++) {
			wMask[layer][x] = (x >= 32 && x <= 200) && (layer % 2 == 0);
		}
	}

	alignas(64) bool visibleMain[6][256] = {};

	for (auto _ : state) {
		for (int layer = 0; layer < 6; layer++) {
			for (int x = 0; x < 256; x++) {
				visibleMain[layer][x] = !wMask[layer][x];
			}
		}
		benchmark::DoNotOptimize(visibleMain[0][0]);
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 6 * 256);
}
BENCHMARK(BM_SnesPpu_WindowMask_Scanline_AllLayers);

// -----------------------------------------------------------------------------
// 6. Per-Scanline Settings Fetch Cost
// -----------------------------------------------------------------------------
// SnesPpu.cpp fetches `_settings->GetSnesConfig()` at lines 570 and 646,
// inside the per-scanline rendering path. This is a function-call + pointer
// dereference chain. We measure the cost of such a repeated non-inline call.

namespace {
	struct FakeSnesConfig {
		bool DisableFrameSkipping = false;
		bool EnableRandomPowerOnState = false;
		uint32_t OverclockRate = 100;
	};

	struct FakeSettings {
		FakeSnesConfig SnesConfig = {};

		__declspec(noinline) const FakeSnesConfig& GetSnesConfig() const {
			return SnesConfig;
		}
	};
} // anonymous namespace

static void BM_SnesPpu_SettingsFetch_PerScanline(benchmark::State& state) {
	FakeSettings settings;
	settings.SnesConfig.DisableFrameSkipping = false;
	settings.SnesConfig.OverclockRate = 100;

	for (auto _ : state) {
		// Simulate two fetches per scanline (as in SnesPpu.cpp lines 570, 646)
		const FakeSnesConfig& cfg1 = settings.GetSnesConfig();
		bool skipRender = cfg1.DisableFrameSkipping;
		const FakeSnesConfig& cfg2 = settings.GetSnesConfig();
		uint32_t rate = cfg2.OverclockRate;
		benchmark::DoNotOptimize(skipRender);
		benchmark::DoNotOptimize(rate);
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_SnesPpu_SettingsFetch_PerScanline);

// Comparison: inline struct member access (what caching the config would give)
static void BM_SnesPpu_SettingsFetch_Cached(benchmark::State& state) {
	bool cachedDisableFrameSkipping = false;
	uint32_t cachedOverclockRate = 100;

	for (auto _ : state) {
		bool skipRender = cachedDisableFrameSkipping;
		uint32_t rate = cachedOverclockRate;
		benchmark::DoNotOptimize(skipRender);
		benchmark::DoNotOptimize(rate);
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_SnesPpu_SettingsFetch_Cached);

// Benchmark: 224-scanline settings fetch (full frame per-scanline fetch cost)
static void BM_SnesPpu_SettingsFetch_FullFrame(benchmark::State& state) {
	FakeSettings settings;

	for (auto _ : state) {
		uint32_t checksum = 0;
		for (int scanline = 0; scanline < 224; scanline++) {
			const FakeSnesConfig& cfg = settings.GetSnesConfig();
			checksum += cfg.OverclockRate + (cfg.DisableFrameSkipping ? 1 : 0);
		}
		benchmark::DoNotOptimize(checksum);
	}
	state.SetItemsProcessed(state.iterations() * 224);
}
BENCHMARK(BM_SnesPpu_SettingsFetch_FullFrame);

// Comparison: 224 scanlines with a cached local copy (what caching avoids)
static void BM_SnesPpu_SettingsFetch_FullFrame_Cached(benchmark::State& state) {
	FakeSettings settings;

	for (auto _ : state) {
		const FakeSnesConfig cfg = settings.GetSnesConfig(); // cache once per frame
		uint32_t checksum = 0;
		for (int scanline = 0; scanline < 224; scanline++) {
			checksum += cfg.OverclockRate + (cfg.DisableFrameSkipping ? 1 : 0);
		}
		benchmark::DoNotOptimize(checksum);
	}
	state.SetItemsProcessed(state.iterations() * 224);
}
BENCHMARK(BM_SnesPpu_SettingsFetch_FullFrame_Cached);

// =============================================================================
// Phase 2: SNES BG Render Priority Resolution
// =============================================================================
// The SNES has up to 4 BG layers plus OBJ, each with two priority levels.
// Priority resolution selects the highest-priority non-transparent pixel
// for each screen position. The hot path checks priority flags per pixel.

static void BM_SnesPpu_PriorityResolution_MainScreen(benchmark::State& state) {
	// Simulate 4 BG layers + OBJ, each with a priority and color per pixel
	alignas(64) uint8_t priorities[5][256] = {};  // [layer][x]: priority flags (0..15)
	alignas(64) uint16_t colors[5][256] = {};     // [layer][x]: 15-bit BGR color

	for (int layer = 0; layer < 5; layer++) {
		for (int x = 0; x < 256; x++) {
			priorities[layer][x] = static_cast<uint8_t>((x * 3 + layer * 5) % 16);
			colors[layer][x]     = static_cast<uint16_t>((x * 37 + layer * 13) & 0x7FFF);
		}
	}

	alignas(64) uint16_t outputBuffer[256] = {};

	for (auto _ : state) {
		for (int x = 0; x < 256; x++) {
			uint8_t bestPrio = 0;
			uint16_t bestColor = 0;
			for (int layer = 0; layer < 5; layer++) {
				if (priorities[layer][x] > bestPrio) {
					bestPrio = priorities[layer][x];
					bestColor = colors[layer][x];
				}
			}
			outputBuffer[x] = bestColor;
		}
		benchmark::DoNotOptimize(outputBuffer[0]);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_SnesPpu_PriorityResolution_MainScreen);
