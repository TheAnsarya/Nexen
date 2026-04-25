#include "pch.h"
#include <array>
#include <cstring>
#include "Genesis/GenesisVdp.h"

// =============================================================================
// Genesis VDP Rendering Hot-Path Benchmarks
// =============================================================================
// The Genesis VDP (Video Display Processor) is the hottest code path in
// Genesis emulation. It processes up to 320×224 pixels per frame, with
// 64 CRAM entries, 64KB VRAM, and complex sprite attribute table scanning.
//
// Key hot paths measured here:
//   1. 4bpp tile pixel extraction (packed nibble format, 8 pixels per 4 bytes)
//   2. CRAM color conversion (Genesis CRAM → RGB555)
//   3. DMA fill (VRAM byte fill, up to 64KB per frame)
//   4. DMA VRAM-to-VRAM copy (up to 64KB per operation)
//   5. Sprite attribute table scan (up to 80 sprites, link-list traversal)
//   6. Full background scanline render (256/320 pixels per plane)
//   7. Per-pixel CRAM lookup for a scanline
// =============================================================================

// -----------------------------------------------------------------------------
// 1. Genesis 4bpp Tile Pixel Extraction
// -----------------------------------------------------------------------------
// Genesis tiles are stored as 4bpp packed data: 4 bytes per row, 2 pixels per
// byte (high nibble = left pixel, low nibble = right pixel). Each tile row is
// 4 bytes = 8 pixels. This is the innermost pixel extraction loop.

namespace {
	// Extract one 4bpp pixel from packed tile row (Genesis format)
	// tileRow: 4 bytes for one row of a tile (big-endian nibbles)
	// col: pixel column 0-7 (0=leftmost)
	inline uint8_t GenesisExtract4bppPixel(const uint8_t* tileRow, uint8_t col) {
		uint8_t byteIdx = col / 2;
		uint8_t pixelData = tileRow[byteIdx];
		if (col & 1) {
			return pixelData & 0x0F;        // Right nibble (odd pixel)
		} else {
			return (pixelData >> 4) & 0x0F; // Left nibble (even pixel)
		}
	}

	// Extract all 8 pixels from a Genesis tile row into an output buffer
	inline void GenesisExtract4bppRow(const uint8_t* tileRow, uint8_t* out) {
		out[0] = (tileRow[0] >> 4) & 0x0F;
		out[1] = (tileRow[0])      & 0x0F;
		out[2] = (tileRow[1] >> 4) & 0x0F;
		out[3] = (tileRow[1])      & 0x0F;
		out[4] = (tileRow[2] >> 4) & 0x0F;
		out[5] = (tileRow[2])      & 0x0F;
		out[6] = (tileRow[3] >> 4) & 0x0F;
		out[7] = (tileRow[3])      & 0x0F;
	}

	// Extract all 8 pixels with horizontal flip
	inline void GenesisExtract4bppRow_HFlip(const uint8_t* tileRow, uint8_t* out) {
		out[7] = (tileRow[0] >> 4) & 0x0F;
		out[6] = (tileRow[0])      & 0x0F;
		out[5] = (tileRow[1] >> 4) & 0x0F;
		out[4] = (tileRow[1])      & 0x0F;
		out[3] = (tileRow[2] >> 4) & 0x0F;
		out[2] = (tileRow[2])      & 0x0F;
		out[1] = (tileRow[3] >> 4) & 0x0F;
		out[0] = (tileRow[3])      & 0x0F;
	}

	// Convert Genesis CRAM entry (0000BBB0GGG0RRR0) to RGB555
	// This is GenesisVdp::CramToRgb555()
	inline uint16_t CramToRgb555(uint16_t cramColor) {
		uint8_t r = (cramColor >> 1) & 7;
		uint8_t g = (cramColor >> 5) & 7;
		uint8_t b = (cramColor >> 9) & 7;
		// Scale 3-bit (0-7) to 5-bit (0-31)
		r = (r << 2) | (r >> 1);
		g = (g << 2) | (g >> 1);
		b = (b << 2) | (b >> 1);
		return static_cast<uint16_t>((r << 10) | (g << 5) | b);
	}
} // anonymous namespace

// Benchmark: Extract single 4bpp pixel (baseline nibble operation)
static void BM_GenesisVdp_TileExtract_4bpp_SinglePixel(benchmark::State& state) {
	uint8_t tileRow[4] = {0xAB, 0xCD, 0xEF, 0x01};
	uint8_t col = 0;
	for (auto _ : state) {
		uint8_t px = GenesisExtract4bppPixel(tileRow, col);
		benchmark::DoNotOptimize(px);
		col = (col + 1) & 7;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GenesisVdp_TileExtract_4bpp_SinglePixel);

// Benchmark: Extract all 8 pixels from one tile row (manual unroll)
static void BM_GenesisVdp_TileExtract_4bpp_FullRow_Unrolled(benchmark::State& state) {
	uint8_t tileRow[4] = {0xAB, 0xCD, 0xEF, 0x01};
	uint8_t pixels[8] = {};
	for (auto _ : state) {
		GenesisExtract4bppRow(tileRow, pixels);
		benchmark::DoNotOptimize(pixels[0]);
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_GenesisVdp_TileExtract_4bpp_FullRow_Unrolled);

// Benchmark: Extract all 8 pixels with horizontal flip
static void BM_GenesisVdp_TileExtract_4bpp_FullRow_HFlip(benchmark::State& state) {
	uint8_t tileRow[4] = {0xAB, 0xCD, 0xEF, 0x01};
	uint8_t pixels[8] = {};
	for (auto _ : state) {
		GenesisExtract4bppRow_HFlip(tileRow, pixels);
		benchmark::DoNotOptimize(pixels[0]);
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_GenesisVdp_TileExtract_4bpp_FullRow_HFlip);

// Benchmark: Extract 320-pixel scanline worth of 4bpp tiles (40 tiles)
static void BM_GenesisVdp_TileExtract_4bpp_Scanline_H40(benchmark::State& state) {
	// 40 tiles × 8 pixels = 320 pixels (H40 mode)
	alignas(64) uint8_t tileData[40 * 4] = {}; // 40 tile rows, 4 bytes each
	for (int i = 0; i < (int)sizeof(tileData); i++) {
		tileData[i] = static_cast<uint8_t>(i * 37 + 0xAB);
	}

	alignas(64) uint8_t lineBuffer[320] = {};

	for (auto _ : state) {
		for (int tile = 0; tile < 40; tile++) {
			GenesisExtract4bppRow(&tileData[tile * 4], &lineBuffer[tile * 8]);
		}
		benchmark::DoNotOptimize(lineBuffer[0]);
	}
	state.SetItemsProcessed(state.iterations() * 320);
}
BENCHMARK(BM_GenesisVdp_TileExtract_4bpp_Scanline_H40);

// Benchmark: Extract 256-pixel scanline (H32 mode, 32 tiles)
static void BM_GenesisVdp_TileExtract_4bpp_Scanline_H32(benchmark::State& state) {
	alignas(64) uint8_t tileData[32 * 4] = {};
	for (int i = 0; i < (int)sizeof(tileData); i++) {
		tileData[i] = static_cast<uint8_t>(i * 37 + 0xAB);
	}

	alignas(64) uint8_t lineBuffer[256] = {};

	for (auto _ : state) {
		for (int tile = 0; tile < 32; tile++) {
			GenesisExtract4bppRow(&tileData[tile * 4], &lineBuffer[tile * 8]);
		}
		benchmark::DoNotOptimize(lineBuffer[0]);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_GenesisVdp_TileExtract_4bpp_Scanline_H32);

// -----------------------------------------------------------------------------
// 2. CRAM Color Conversion
// -----------------------------------------------------------------------------
// GenesisVdp::CramToRgb555() converts Genesis 9-bit BGR to RGB555.
// Called for every non-transparent pixel. 64 CRAM entries total.

// Benchmark: Convert single CRAM entry to RGB555
static void BM_GenesisVdp_CramToRgb555_Single(benchmark::State& state) {
	uint16_t cramColor = 0x0EEE; // Maximum white (all channels max)
	for (auto _ : state) {
		uint16_t rgb = CramToRgb555(cramColor);
		benchmark::DoNotOptimize(rgb);
		cramColor = (cramColor + 0x0222) & 0x0EEE;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GenesisVdp_CramToRgb555_Single);

// Benchmark: Convert all 64 CRAM entries (one palette update)
static void BM_GenesisVdp_CramToRgb555_AllEntries(benchmark::State& state) {
	alignas(64) uint16_t cram[64] = {};
	for (int i = 0; i < 64; i++) {
		// Genesis CRAM: 0000BBB0GGG0RRR0 format
		uint8_t r = i & 7;
		uint8_t g = (i >> 3) & 7;
		uint8_t b = (i >> 4) & 7;  // Wrap intentionally for variety
		cram[i] = static_cast<uint16_t>((r << 1) | (g << 5) | (b << 9));
	}

	alignas(64) uint16_t rgb555[64] = {};

	for (auto _ : state) {
		for (int i = 0; i < 64; i++) {
			rgb555[i] = CramToRgb555(cram[i]);
		}
		benchmark::DoNotOptimize(rgb555[0]);
	}
	state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_GenesisVdp_CramToRgb555_AllEntries);

// Benchmark: Per-pixel CRAM lookup + conversion for a 320-pixel scanline
static void BM_GenesisVdp_CramLookup_Scanline_H40(benchmark::State& state) {
	alignas(64) uint16_t cram[64] = {};
	for (int i = 0; i < 64; i++) cram[i] = static_cast<uint16_t>(i * 0x124);

	// Simulate color indices: palette (0-3) × 16 entries
	alignas(64) uint8_t colorIndices[320] = {};
	for (int i = 0; i < 320; i++) {
		colorIndices[i] = static_cast<uint8_t>((i * 13 + 7) & 63);
	}

	alignas(64) uint16_t lineBuffer[320] = {};

	for (auto _ : state) {
		for (int x = 0; x < 320; x++) {
			if (colorIndices[x] != 0) { // Color 0 is transparent in each palette
				lineBuffer[x] = CramToRgb555(cram[colorIndices[x]]);
			}
		}
		benchmark::DoNotOptimize(lineBuffer[0]);
	}
	state.SetItemsProcessed(state.iterations() * 320);
}
BENCHMARK(BM_GenesisVdp_CramLookup_Scanline_H40);

// -----------------------------------------------------------------------------
// 3. DMA Operations
// -----------------------------------------------------------------------------
// The Genesis VDP supports three DMA modes:
//   - 68K→VRAM/CRAM/VSRAM (CPU-initiated block copy)
//   - VRAM fill (fill block with one byte value, up to 64KB)
//   - VRAM-to-VRAM copy (internal VDP memory copy)
// In practice, games use DMA for large bulk transfers every frame.

// Benchmark: VRAM fill (DMA mode 2) — byte fill of various sizes
static void BM_GenesisVdp_DmaFill_1KB(benchmark::State& state) {
	alignas(64) uint8_t vram[0x10000] = {};
	uint8_t fillByte = 0xFF;
	for (auto _ : state) {
		for (uint32_t i = 0; i < 1024; i++) {
			vram[i] = fillByte;
		}
		benchmark::DoNotOptimize(vram[0]);
		benchmark::ClobberMemory();
		fillByte++;
	}
	state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_GenesisVdp_DmaFill_1KB);

static void BM_GenesisVdp_DmaFill_16KB(benchmark::State& state) {
	alignas(64) uint8_t vram[0x10000] = {};
	uint8_t fillByte = 0xAA;
	for (auto _ : state) {
		for (uint32_t i = 0; i < 16384; i++) {
			vram[i] = fillByte;
		}
		benchmark::DoNotOptimize(vram[0]);
		benchmark::ClobberMemory();
		fillByte++;
	}
	state.SetBytesProcessed(state.iterations() * 16384);
}
BENCHMARK(BM_GenesisVdp_DmaFill_16KB);

// Benchmark: memset baseline for comparison with DMA fill
static void BM_GenesisVdp_DmaFill_Memset_16KB(benchmark::State& state) {
	alignas(64) uint8_t vram[0x10000] = {};
	for (auto _ : state) {
		memset(vram, 0xFF, 16384);
		benchmark::DoNotOptimize(vram[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 16384);
}
BENCHMARK(BM_GenesisVdp_DmaFill_Memset_16KB);

// Benchmark: VRAM-to-VRAM copy (DMA mode 3)
static void BM_GenesisVdp_DmaCopy_1KB(benchmark::State& state) {
	alignas(64) uint8_t vram[0x10000] = {};
	for (int i = 0; i < 0x10000; i++) vram[i] = static_cast<uint8_t>(i * 37);

	uint16_t srcAddr = 0x0000;
	uint16_t dstAddr = 0x4000;

	for (auto _ : state) {
		for (uint16_t i = 0; i < 1024; i++) {
			uint32_t src = (srcAddr + i) & 0xFFFF;
			uint32_t dst = (dstAddr + i) & 0xFFFF;
			vram[dst] = vram[src];
		}
		benchmark::DoNotOptimize(vram[dstAddr]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_GenesisVdp_DmaCopy_1KB);

static void BM_GenesisVdp_DmaCopy_16KB(benchmark::State& state) {
	alignas(64) uint8_t vram[0x10000] = {};
	for (int i = 0; i < 0x10000; i++) vram[i] = static_cast<uint8_t>(i * 37);

	uint16_t srcAddr = 0x0000;
	uint16_t dstAddr = 0x4000;

	for (auto _ : state) {
		for (uint16_t i = 0; i < 16384; i++) {
			uint32_t src = (srcAddr + i) & 0xFFFF;
			uint32_t dst = (dstAddr + i) & 0xFFFF;
			vram[dst] = vram[src];
		}
		benchmark::DoNotOptimize(vram[dstAddr]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 16384);
}
BENCHMARK(BM_GenesisVdp_DmaCopy_16KB);

// Benchmark: memcpy baseline for comparison
static void BM_GenesisVdp_DmaCopy_Memcpy_16KB(benchmark::State& state) {
	alignas(64) uint8_t src[16384] = {};
	alignas(64) uint8_t dst[16384] = {};
	for (int i = 0; i < 16384; i++) src[i] = static_cast<uint8_t>(i * 37);

	for (auto _ : state) {
		memcpy(dst, src, 16384);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 16384);
}
BENCHMARK(BM_GenesisVdp_DmaCopy_Memcpy_16KB);

// -----------------------------------------------------------------------------
// 4. Sprite Attribute Table (SAT) Scan
// -----------------------------------------------------------------------------
// The Genesis has up to 80 sprites (H40 mode) in a linked-list SAT.
// For each scanline, the VDP must walk the list to find visible sprites.
// Each SAT entry is 8 bytes: Y, size+link, attr, X.
// This is a cache-hostile linked-list traversal.

static void BM_GenesisVdp_SpriteScan_MaxSprites_H40(benchmark::State& state) {
	// Build a sprite SAT for 80 sprites in a linear linked list
	// Each entry: bytes [0..1]=Y, [2]=size+link, [3]=link, [4..5]=attr, [6..7]=X
	alignas(64) uint8_t vram[0x10000] = {};
	constexpr uint32_t satBase = 0xB800; // Typical SAT base address (H40)

	for (uint8_t i = 0; i < 80; i++) {
		uint32_t addr = satBase + i * 8;
		// Y position (offset by 128): scatter sprites across scanlines 0..223
		uint16_t yPos = 128 + (i * 3 % 224);
		vram[addr + 0] = (uint8_t)(yPos >> 8);
		vram[addr + 1] = (uint8_t)(yPos & 0xFF);
		// Size 1×1, link to next sprite (last links to 0 to terminate)
		vram[addr + 2] = 0x00; // 1×1 cells
		vram[addr + 3] = (uint8_t)((i + 1) % 80); // link
		// Tile index
		vram[addr + 4] = (uint8_t)(i >> 3);
		vram[addr + 5] = (uint8_t)(i & 7);
		// X position
		uint16_t xPos = 128 + (i * 4 % 320);
		vram[addr + 6] = (uint8_t)(xPos >> 8);
		vram[addr + 7] = (uint8_t)(xPos & 0xFF);
	}
	// Terminate the last sprite
	vram[satBase + 79 * 8 + 3] = 0; // link=0 terminates

	uint16_t testScanline = 100;

	for (auto _ : state) {
		uint16_t spritesOnLine = 0;
		uint16_t spriteIdx = 0;

		for (uint16_t i = 0; i < 80; i++) {
			uint32_t satAddr = satBase + spriteIdx * 8;
			if (satAddr + 7 >= 0x10000) break;

			uint16_t yPos = (((uint16_t)vram[satAddr] << 8) | vram[satAddr + 1]) & 0x3FF;
			uint8_t sizeV = ((vram[satAddr + 2] >> 2) & 3) + 1;
			uint8_t link  = vram[satAddr + 3] & 0x7F;

			int16_t screenY = (int16_t)yPos - 128;
			if (testScanline >= (uint16_t)screenY &&
			    testScanline < (uint16_t)(screenY + sizeV * 8)) {
				spritesOnLine++;
				if (spritesOnLine >= 20) break; // H40 max
			}

			if (link == 0) break;
			spriteIdx = link;
		}
		benchmark::DoNotOptimize(spritesOnLine);
		testScanline = (testScanline + 1) % 224;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GenesisVdp_SpriteScan_MaxSprites_H40);

// Benchmark: SAT scan with early termination (typical case: few sprites visible)
static void BM_GenesisVdp_SpriteScan_FewVisible(benchmark::State& state) {
	// Only sprites 0-3 are on-screen; rest are Y=0 (above screen)
	alignas(64) uint8_t vram[0x10000] = {};
	constexpr uint32_t satBase = 0xB800;

	for (uint8_t i = 0; i < 80; i++) {
		uint32_t addr = satBase + i * 8;
		uint16_t yPos = (i < 4) ? (uint16_t)(128 + 100) : (uint16_t)0; // Only 4 sprites visible
		vram[addr + 0] = (uint8_t)(yPos >> 8);
		vram[addr + 1] = (uint8_t)(yPos & 0xFF);
		vram[addr + 2] = 0x00;
		vram[addr + 3] = (uint8_t)((i + 1) % 80);
	}
	vram[satBase + 79 * 8 + 3] = 0;

	uint16_t testScanline = 100;

	for (auto _ : state) {
		uint16_t spritesOnLine = 0;
		uint16_t spriteIdx = 0;
		for (uint16_t i = 0; i < 80; i++) {
			uint32_t satAddr = satBase + spriteIdx * 8;
			uint16_t yPos  = (((uint16_t)vram[satAddr] << 8) | vram[satAddr + 1]) & 0x3FF;
			uint8_t sizeV  = ((vram[satAddr + 2] >> 2) & 3) + 1;
			uint8_t link   = vram[satAddr + 3] & 0x7F;
			int16_t screenY = (int16_t)yPos - 128;
			if (testScanline >= (uint16_t)screenY &&
			    testScanline < (uint16_t)(screenY + sizeV * 8)) {
				spritesOnLine++;
			}
			if (link == 0) break;
			spriteIdx = link;
		}
		benchmark::DoNotOptimize(spritesOnLine);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GenesisVdp_SpriteScan_FewVisible);

// -----------------------------------------------------------------------------
// 5. Full Background Scanline Render
// -----------------------------------------------------------------------------
// Simulate the innermost loop of GenesisVdp::RenderBackground():
// For each pixel: get tile, decode nibble, CRAM lookup, write to line buffer.

static void BM_GenesisVdp_RenderBackground_H40(benchmark::State& state) {
	// Simulate 40×28 tile background (H40, 320 wide, 224 tall)
	constexpr uint32_t VramSize = 0x10000;
	alignas(64) uint8_t vram[VramSize] = {};
	alignas(64) uint16_t cram[64] = {};

	// Fill VRAM with dummy nametable at 0xC000 and tile data at 0x0000
	constexpr uint32_t planeBase = 0xC000;
	constexpr uint16_t planeW = 64; // 64-tile wide plane (H40 common)

	for (int row = 0; row < 28; row++) {
		for (int col = 0; col < 64; col++) {
			uint32_t ntAddr = planeBase + (row * planeW + col) * 2;
			if (ntAddr + 1 < VramSize) {
				uint16_t tileIdx = static_cast<uint16_t>((row * 64 + col) % 512);
				vram[ntAddr]     = static_cast<uint8_t>(tileIdx >> 3);
				vram[ntAddr + 1] = static_cast<uint8_t>((tileIdx << 5) & 0xE0);
			}
		}
	}

	// Fill tile data at 0x0000 (512 tiles × 32 bytes each)
	for (int i = 0; i < 512 * 32 && i < (int)VramSize; i++) {
		vram[i] = static_cast<uint8_t>(i * 37 + 0xAB);
	}

	for (int i = 0; i < 64; i++) {
		uint8_t r = i & 7, g = (i >> 1) & 7, b = (i >> 2) & 7;
		cram[i] = static_cast<uint16_t>((r << 1) | (g << 5) | (b << 9));
	}

	alignas(64) uint16_t lineBuffer[320] = {};
	uint16_t scanline = 100; // Fixed scanline

	for (auto _ : state) {
		// Per-pixel loop (simplified from GenesisVdp::RenderBackground)
		for (uint16_t x = 0; x < 320; x++) {
			uint16_t px = x & ((planeW * 8) - 1);
			uint16_t py = (scanline + 0) & ((28 * 8) - 1); // no vscroll for bench

			uint16_t tileCol = px / 8;
			uint16_t tileRow = py / 8;
			uint32_t ntAddr  = planeBase + (tileRow * planeW + tileCol) * 2;

			if (ntAddr + 1 >= VramSize) continue;

			uint16_t ntEntry   = ((uint16_t)vram[ntAddr] << 8) | vram[ntAddr + 1];
			uint16_t tileIndex = ntEntry & 0x7FF;
			bool hFlip         = (ntEntry >> 11) & 1;
			uint8_t palette    = (ntEntry >> 13) & 3;

			uint8_t tileX = (uint8_t)(px & 7);
			uint8_t tileY = (uint8_t)(py & 7);
			if (hFlip) tileX = 7 - tileX;

			uint32_t tileAddr  = tileIndex * 32 + tileY * 4;
			if (tileAddr + 3 >= VramSize) continue;

			uint8_t byteIdx    = tileX / 2;
			uint8_t pixelData  = vram[tileAddr + byteIdx];
			uint8_t colorIdx   = (tileX & 1) ? (pixelData & 0x0F) : ((pixelData >> 4) & 0x0F);

			if (colorIdx != 0) {
				uint16_t cramIdx = palette * 16 + colorIdx;
				lineBuffer[x] = CramToRgb555(cram[cramIdx]);
			}
		}
		benchmark::DoNotOptimize(lineBuffer[0]);
		scanline = (scanline + 1) % 224;
	}
	state.SetItemsProcessed(state.iterations() * 320);
}
BENCHMARK(BM_GenesisVdp_RenderBackground_H40);

// Benchmark: GenesisVdp object construction + Run() call (tests real code path)
// Uses Run() which internally calls ProcessScanline/RenderScanline
static void BM_GenesisVdp_Object_Run_OneScanline(benchmark::State& state) {
	GenesisVdp vdp;
	vdp.Init(nullptr, nullptr, nullptr, nullptr);

	// Genesis NTSC: 3579545 master clock, 488 clocks per scanline
	constexpr uint64_t kClocksPerScanline = 488;
	uint64_t cycle = kClocksPerScanline;

	for (auto _ : state) {
		vdp.Run(cycle);
		benchmark::DoNotOptimize(vdp.GetState());
		cycle += kClocksPerScanline;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GenesisVdp_Object_Run_OneScanline);

// Benchmark: GenesisVdp DMA setup via WriteControlPort + Run (real DMA trigger path)
// Configures DMA fill via register writes and runs one scanline to process DMA
static void BM_GenesisVdp_Object_DmaFill_ViaRegisters(benchmark::State& state) {
	GenesisVdp vdp;
	vdp.Init(nullptr, nullptr, nullptr, nullptr);

	// Enable DMA (reg 1 bit 4)
	vdp.WriteControlPort(0x8110); // reg 1 = 0x10

	// DMA mode 2 (fill): reg 23 bit 7:6 = 10
	vdp.WriteControlPort(0x9780); // reg 23 = 0x80 (fill mode, fill byte = 0)

	GenesisVdpState setupState = vdp.GetState();
	if (((setupState.Registers[23] >> 6) & 0x03) != 0x02) {
		state.SkipWithError("VDP DMA mode misconfigured: expected fill mode via register 23");
		return;
	}

	constexpr uint64_t kClocksPerScanline = 488;
	uint64_t cycle = kClocksPerScanline;

	for (auto _ : state) {
		// Set DMA length (regs 19, 20): 512 words
		vdp.WriteControlPort(0x8D00); // reg 13 = 0 (clear)
		vdp.WriteControlPort(0x9300); // reg 19 = 0 (length low)
		vdp.WriteControlPort(0x9402); // reg 20 = 2 (length high: 512 words)
		// Trigger DMA with write to control port (CD5=1 second word)
		vdp.WriteControlPort(0x4000); // First word: code=VRAM write, addr=0
		vdp.WriteControlPort(0x0080); // Second word: CD5=1 = DMA enable

		vdp.Run(cycle);
		benchmark::DoNotOptimize(vdp.GetState());
		cycle += kClocksPerScanline;
	}
	state.SetItemsProcessed(state.iterations() * 512);
}
BENCHMARK(BM_GenesisVdp_Object_DmaFill_ViaRegisters);

// Benchmark: GenesisVdp DMA copy setup via WriteControlPort + Run (real DMA trigger path)
// Configures DMA copy via register writes and runs one scanline to process DMA.
static void BM_GenesisVdp_Object_DmaCopy_ViaRegisters(benchmark::State& state) {
	GenesisVdp vdp;
	vdp.Init(nullptr, nullptr, nullptr, nullptr);

	// Enable DMA (reg 1 bit 4)
	vdp.WriteControlPort(0x8110); // reg 1 = 0x10

	// DMA mode 3 (VRAM copy): reg 23 bit 7:6 = 11
	vdp.WriteControlPort(0x97c0); // reg 23 = 0xc0

	GenesisVdpState setupState = vdp.GetState();
	if ((setupState.Registers[1] & 0x10) == 0) {
		state.SkipWithError("VDP DMA misconfigured: DMA enable bit in register 1 is not set");
		return;
	}
	if (((setupState.Registers[23] >> 6) & 0x03) != 0x03) {
		state.SkipWithError("VDP DMA mode misconfigured: expected copy mode via register 23");
		return;
	}

	constexpr uint64_t kClocksPerScanline = 488;
	uint64_t cycle = kClocksPerScanline;

	for (auto _ : state) {
		// DMA source (regs 21, 22): 0x0100
		vdp.WriteControlPort(0x9500); // reg 21 = 0x00
		vdp.WriteControlPort(0x9601); // reg 22 = 0x01

		// DMA length (regs 19, 20): 256 units in this simplified model
		vdp.WriteControlPort(0x9300); // reg 19 = 0x00
		vdp.WriteControlPort(0x9401); // reg 20 = 0x01

		// Trigger DMA with destination address 0x2000 (VRAM write + CD5 set)
		vdp.WriteControlPort(0x6000); // First word: VRAM write, addr=0x2000
		vdp.WriteControlPort(0x0080); // Second word: CD5=1 = DMA enable

		GenesisVdpState preRunState = vdp.GetState();
		if (!preRunState.DmaActive) {
			state.SkipWithError("VDP DMA trigger failed: DmaActive was not asserted before Run()");
			return;
		}

		vdp.Run(cycle);
		benchmark::DoNotOptimize(vdp.GetState());
		cycle += kClocksPerScanline;
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_GenesisVdp_Object_DmaCopy_ViaRegisters);
