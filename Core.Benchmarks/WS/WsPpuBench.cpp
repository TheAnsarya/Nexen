#include "pch.h"
#include <benchmark/benchmark.h>
#include <array>
#include <algorithm>
#include <cstring>
#include "WS/WsTypes.h"

// =============================================================================
// WonderSwan PPU Benchmarks
// =============================================================================
// Benchmarks for WS PPU rendering hot paths: pixel color extraction, palette
// lookup, window checks, and scanline patterns.

// --- Pixel Color Extraction (hot path in DrawBackground/DrawSprites) ---

namespace {
	// Monochrome / Color 2bpp extraction
	uint16_t GetPixelColor_Mono(const uint8_t* tileData, uint8_t column) {
		return (
			((tileData[0] << column) & 0x80) >> 7 |
			((tileData[1] << column) & 0x80) >> 6);
	}

	// Color 4bpp extraction
	uint16_t GetPixelColor_4bpp(const uint8_t* tileData, uint8_t column) {
		return (
			((tileData[0] << column) & 0x80) >> 7 |
			((tileData[1] << column) & 0x80) >> 6 |
			((tileData[2] << column) & 0x80) >> 5 |
			((tileData[3] << column) & 0x80) >> 4);
	}

	// Color 4bpp packed extraction
	uint16_t GetPixelColor_4bppPacked(const uint8_t* tileData, uint8_t column) {
		return (tileData[column / 2] >> (column & 0x01 ? 0 : 4)) & 0x0f;
	}

	// Monochrome palette → shade LUT → RGB
	uint16_t MonoPaletteToRgb(uint8_t color, uint8_t palette,
		const uint8_t bwPalettes[0x40], const uint8_t bwShades[8]) {
		uint8_t shadeValue = bwPalettes[(palette << 2) | color];
		uint8_t brightness = bwShades[shadeValue] ^ 0x0f;
		return brightness | (brightness << 4) | (brightness << 8);
	}

	// Color palette RAM lookup
	uint16_t ColorPaletteToRgb(uint8_t color, uint8_t palette, const uint8_t* vram) {
		uint16_t addr = 0xfe00 | (palette << 5) | (color << 1);
		return vram[addr] | ((vram[addr + 1] & 0x0f) << 8);
	}

	// Window bounds check
	bool IsInsideWindow(uint8_t x, uint8_t y,
		uint8_t left, uint8_t right, uint8_t top, uint8_t bottom) {
		return x >= left && x <= right && y >= top && y <= bottom;
	}

	// ares-compatible frame end scanline count
	uint16_t GetFrameEndScanlineAres(uint8_t vtotal) {
		uint16_t visibleCount = static_cast<uint16_t>(vtotal) + 1;
		return visibleCount >= WsConstants::ScreenHeight ? visibleCount : WsConstants::ScreenHeight;
	}

	// Nexen current render-Y wrapping model for line rendering
	uint8_t GetRenderYWrapped(uint16_t scanline, uint8_t vtotal) {
		uint16_t visibleCount = static_cast<uint16_t>(vtotal) + 1;
		return visibleCount >= WsConstants::ScreenHeight
			? static_cast<uint8_t>(scanline)
			: static_cast<uint8_t>(scanline % visibleCount);
	}
} // anonymous namespace

// =============================================================================
// Pixel Color Extraction Benchmarks
// =============================================================================

static void BM_WsPpu_PixelColor_Mono(benchmark::State& state) {
	uint8_t tileData[2] = {0xaa, 0x55};
	for (auto _ : state) {
		for (uint8_t col = 0; col < 8; col++) {
			benchmark::DoNotOptimize(GetPixelColor_Mono(tileData, col));
		}
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_WsPpu_PixelColor_Mono);

static void BM_WsPpu_PixelColor_4bpp(benchmark::State& state) {
	uint8_t tileData[4] = {0xaa, 0x55, 0xcc, 0x33};
	for (auto _ : state) {
		for (uint8_t col = 0; col < 8; col++) {
			benchmark::DoNotOptimize(GetPixelColor_4bpp(tileData, col));
		}
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_WsPpu_PixelColor_4bpp);

static void BM_WsPpu_PixelColor_4bppPacked(benchmark::State& state) {
	uint8_t tileData[4] = {0x12, 0x34, 0x56, 0x78};
	for (auto _ : state) {
		for (uint8_t col = 0; col < 8; col++) {
			benchmark::DoNotOptimize(GetPixelColor_4bppPacked(tileData, col));
		}
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_WsPpu_PixelColor_4bppPacked);

// =============================================================================
// Palette Lookup Benchmarks
// =============================================================================

static void BM_WsPpu_Palette_Mono(benchmark::State& state) {
	uint8_t bwPalettes[0x40] = {};
	uint8_t bwShades[8] = {};
	// Realistic shade LUT
	for (int i = 0; i < 8; i++) bwShades[i] = static_cast<uint8_t>(i * 2);
	for (int i = 0; i < 0x40; i++) bwPalettes[i] = static_cast<uint8_t>(i & 0x07);

	for (auto _ : state) {
		for (uint8_t pal = 0; pal < 8; pal++) {
			for (uint8_t col = 0; col < 4; col++) {
				benchmark::DoNotOptimize(MonoPaletteToRgb(col, pal, bwPalettes, bwShades));
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * 32);
}
BENCHMARK(BM_WsPpu_Palette_Mono);

static void BM_WsPpu_Palette_Color(benchmark::State& state) {
	alignas(16) uint8_t vram[0x10000] = {};
	// Fill palette RAM with test values
	for (uint16_t i = 0xfe00; i < 0x10000; i++) {
		vram[i] = static_cast<uint8_t>(i & 0xff);
	}

	for (auto _ : state) {
		for (uint8_t pal = 0; pal < 16; pal++) {
			for (uint8_t col = 0; col < 16; col++) {
				benchmark::DoNotOptimize(ColorPaletteToRgb(col, pal, vram));
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_WsPpu_Palette_Color);

// =============================================================================
// Full Scanline Pixel Color Extraction (224 pixels)
// =============================================================================

static void BM_WsPpu_Scanline_Mono224(benchmark::State& state) {
	// Simulate 224-pixel scanline: 28 tiles × 8 pixels
	uint8_t tileRows[28][2];
	for (int i = 0; i < 28; i++) {
		tileRows[i][0] = static_cast<uint8_t>(i * 7);
		tileRows[i][1] = static_cast<uint8_t>(i * 13);
	}

	for (auto _ : state) {
		for (int tile = 0; tile < 28; tile++) {
			for (uint8_t col = 0; col < 8; col++) {
				benchmark::DoNotOptimize(GetPixelColor_Mono(tileRows[tile], col));
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * 224);
}
BENCHMARK(BM_WsPpu_Scanline_Mono224);

static void BM_WsPpu_Scanline_4bpp224(benchmark::State& state) {
	uint8_t tileRows[28][4];
	for (int i = 0; i < 28; i++) {
		tileRows[i][0] = static_cast<uint8_t>(i * 7);
		tileRows[i][1] = static_cast<uint8_t>(i * 13);
		tileRows[i][2] = static_cast<uint8_t>(i * 19);
		tileRows[i][3] = static_cast<uint8_t>(i * 23);
	}

	for (auto _ : state) {
		for (int tile = 0; tile < 28; tile++) {
			for (uint8_t col = 0; col < 8; col++) {
				benchmark::DoNotOptimize(GetPixelColor_4bpp(tileRows[tile], col));
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * 224);
}
BENCHMARK(BM_WsPpu_Scanline_4bpp224);

static void BM_WsPpu_Scanline_4bppPacked224(benchmark::State& state) {
	uint8_t tileRows[28][4];
	for (int i = 0; i < 28; i++) {
		tileRows[i][0] = static_cast<uint8_t>(i * 7);
		tileRows[i][1] = static_cast<uint8_t>(i * 13);
		tileRows[i][2] = static_cast<uint8_t>(i * 23);
		tileRows[i][3] = static_cast<uint8_t>(i * 29);
	}

	for (auto _ : state) {
		for (int tile = 0; tile < 28; tile++) {
			for (uint8_t col = 0; col < 8; col++) {
				benchmark::DoNotOptimize(GetPixelColor_4bppPacked(tileRows[tile], col));
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * 224);
}
BENCHMARK(BM_WsPpu_Scanline_4bppPacked224);

// =============================================================================
// Window Check Benchmarks
// =============================================================================

static void BM_WsPpu_WindowCheck_FullScreen(benchmark::State& state) {
	for (auto _ : state) {
		int insideCount = 0;
		for (uint8_t y = 0; y < 144; y++) {
			for (uint8_t x = 0; x < 224; x++) {
				insideCount += IsInsideWindow(x, y, 0, 223, 0, 143);
			}
		}
		benchmark::DoNotOptimize(insideCount);
	}
	state.SetItemsProcessed(state.iterations() * 224 * 144);
}
BENCHMARK(BM_WsPpu_WindowCheck_FullScreen);

static void BM_WsPpu_WindowCheck_SmallWindow(benchmark::State& state) {
	// Small centered window: 50% hit rate roughly
	for (auto _ : state) {
		int insideCount = 0;
		for (uint8_t y = 0; y < 144; y++) {
			for (uint8_t x = 0; x < 224; x++) {
				insideCount += IsInsideWindow(x, y, 56, 167, 36, 107);
			}
		}
		benchmark::DoNotOptimize(insideCount);
	}
	state.SetItemsProcessed(state.iterations() * 224 * 144);
}
BENCHMARK(BM_WsPpu_WindowCheck_SmallWindow);

// =============================================================================
// Frame Timing / Wrap Benchmarks (Issue #1076 parity work)
// =============================================================================

static void BM_WsPpu_FrameEndScanline_AresModel(benchmark::State& state) {
	for (auto _ : state) {
		for (uint16_t vtotal = 0; vtotal <= 255; vtotal++) {
			benchmark::DoNotOptimize(GetFrameEndScanlineAres(static_cast<uint8_t>(vtotal)));
		}
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_WsPpu_FrameEndScanline_AresModel);

static void BM_WsPpu_RenderYWrap_LowVtotal(benchmark::State& state) {
	constexpr uint8_t vtotal = 49;
	for (auto _ : state) {
		for (uint16_t scanline = 0; scanline < WsConstants::ScreenHeight; scanline++) {
			benchmark::DoNotOptimize(GetRenderYWrapped(scanline, vtotal));
		}
	}
	state.SetItemsProcessed(state.iterations() * WsConstants::ScreenHeight);
}
BENCHMARK(BM_WsPpu_RenderYWrap_LowVtotal);

// =============================================================================
// Combined Scanline: Pixel Extract + Palette Lookup (mono)
// =============================================================================

static void BM_WsPpu_ScanlineFull_MonoExtractAndPalette(benchmark::State& state) {
	uint8_t tileRows[28][2];
	uint8_t bwPalettes[0x40] = {};
	uint8_t bwShades[8] = {};
	for (int i = 0; i < 8; i++) bwShades[i] = static_cast<uint8_t>(i * 2);
	for (int i = 0; i < 0x40; i++) bwPalettes[i] = static_cast<uint8_t>(i & 0x07);
	for (int i = 0; i < 28; i++) {
		tileRows[i][0] = static_cast<uint8_t>(i * 7);
		tileRows[i][1] = static_cast<uint8_t>(i * 13);
	}

	for (auto _ : state) {
		for (int tile = 0; tile < 28; tile++) {
			for (uint8_t col = 0; col < 8; col++) {
				uint16_t color = GetPixelColor_Mono(tileRows[tile], col);
				uint16_t rgb = MonoPaletteToRgb(static_cast<uint8_t>(color), static_cast<uint8_t>(tile & 7), bwPalettes, bwShades);
				benchmark::DoNotOptimize(rgb);
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * 224);
}
BENCHMARK(BM_WsPpu_ScanlineFull_MonoExtractAndPalette);

// =============================================================================
// Combined Scanline: Pixel Extract + Palette Lookup (color 4bpp)
// =============================================================================

static void BM_WsPpu_ScanlineFull_Color4bppExtractAndPalette(benchmark::State& state) {
	uint8_t tileRows[28][4];
	alignas(16) uint8_t vram[0x10000] = {};
	for (uint16_t i = 0xfe00; i < 0x10000; i++) {
		vram[i] = static_cast<uint8_t>(i & 0xff);
	}
	for (int i = 0; i < 28; i++) {
		tileRows[i][0] = static_cast<uint8_t>(i * 7);
		tileRows[i][1] = static_cast<uint8_t>(i * 13);
		tileRows[i][2] = static_cast<uint8_t>(i * 19);
		tileRows[i][3] = static_cast<uint8_t>(i * 23);
	}

	for (auto _ : state) {
		for (int tile = 0; tile < 28; tile++) {
			for (uint8_t col = 0; col < 8; col++) {
				uint16_t color = GetPixelColor_4bpp(tileRows[tile], col);
				uint16_t rgb = ColorPaletteToRgb(static_cast<uint8_t>(color), static_cast<uint8_t>(tile & 15), vram);
				benchmark::DoNotOptimize(rgb);
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * 224);
}
BENCHMARK(BM_WsPpu_ScanlineFull_Color4bppExtractAndPalette);

// =============================================================================
// Latch Operations Benchmark
// =============================================================================

static void BM_WsPpu_BgLayerLatch(benchmark::State& state) {
	WsBgLayer layer = {};
	layer.Enabled = true;
	layer.ScrollX = 42;
	layer.ScrollY = 99;
	layer.MapAddress = 0x1800;

	for (auto _ : state) {
		layer.Latch();
		benchmark::DoNotOptimize(layer.EnabledLatch);
		benchmark::DoNotOptimize(layer.ScrollXLatch);
		benchmark::DoNotOptimize(layer.ScrollYLatch);
		benchmark::DoNotOptimize(layer.MapAddressLatch);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_WsPpu_BgLayerLatch);

static void BM_WsPpu_WindowLatch(benchmark::State& state) {
	WsWindow win = {};
	win.Enabled = true;
	win.Left = 10;
	win.Right = 200;
	win.Top = 20;
	win.Bottom = 130;

	for (auto _ : state) {
		win.Latch();
		benchmark::DoNotOptimize(win.EnabledLatch);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_WsPpu_WindowLatch);

// =============================================================================
// CPU Flags Pack/Unpack Benchmark
// =============================================================================

static void BM_WsCpu_FlagsPackUnpack(benchmark::State& state) {
	WsCpuFlags flags = {};
	flags.Carry = true;
	flags.Zero = true;
	flags.Sign = true;
	flags.Irq = true;

	for (auto _ : state) {
		uint16_t packed = flags.Get();
		benchmark::DoNotOptimize(packed);
		flags.Set(packed);
		benchmark::DoNotOptimize(flags.Carry);
	}
	state.SetItemsProcessed(state.iterations() * 2); // pack + unpack
}
BENCHMARK(BM_WsCpu_FlagsPackUnpack);
