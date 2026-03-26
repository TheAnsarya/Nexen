#include "pch.h"
#include <array>
#include <cstring>
#include "Lynx/LynxTypes.h"

/// <summary>
/// Benchmarks for Lynx PPU Tools — palette, framebuffer, sprite decode, and compositing.
///
/// These measure the hot paths that the debugger visualization hits:
///   - Palette conversion (RGB444 → ARGB32)
///   - Framebuffer rendering (4bpp packed → pixel buffer)
///   - Sprite line decoding (both literal and packed modes)
///   - Sprite preview compositing (back-to-front)
/// </summary>

// =============================================================================
// Helper functions — replicating core logic for benchmarking
// =============================================================================

namespace {

static constexpr uint32_t ScreenWidth = LynxConstants::ScreenWidth;
static constexpr uint32_t ScreenHeight = LynxConstants::ScreenHeight;
static constexpr uint32_t BytesPerLine = LynxConstants::BytesPerScanline;

constexpr uint32_t Rgb444ToArgb(uint16_t rgb444) {
	uint8_t r4 = (rgb444 >> 8) & 0x0f;
	uint8_t g4 = (rgb444 >> 4) & 0x0f;
	uint8_t b4 = rgb444 & 0x0f;
	uint8_t r8 = (r4 << 4) | r4;
	uint8_t g8 = (g4 << 4) | g4;
	uint8_t b8 = (b4 << 4) | b4;
	return 0xff000000u | (r8 << 16) | (g8 << 8) | b8;
}

} // anonymous namespace

// =============================================================================
// Palette Conversion Benchmarks
// =============================================================================

static void BM_LynxPpuTools_PaletteConvert_16Colors(benchmark::State& state) {
	// Measure converting all 16 palette entries from RGB444 to ARGB32
	uint16_t rgb444[16];
	uint32_t argb[16];
	for (int i = 0; i < 16; i++) {
		rgb444[i] = static_cast<uint16_t>((i << 8) | (i << 4) | i);
	}

	for (auto _ : state) {
		for (int i = 0; i < 16; i++) {
			argb[i] = Rgb444ToArgb(rgb444[i]);
		}
		benchmark::DoNotOptimize(argb);
	}
	state.SetItemsProcessed(state.iterations() * 16);
}
BENCHMARK(BM_LynxPpuTools_PaletteConvert_16Colors);

// =============================================================================
// Framebuffer Rendering Benchmarks
// =============================================================================

static void BM_LynxPpuTools_Framebuffer_SingleLine(benchmark::State& state) {
	// Render one 160-pixel scanline from 4bpp packed framebuffer
	std::array<uint8_t, 80> lineData;
	std::array<uint32_t, 160> outBuffer;
	uint32_t palette[16];

	for (int i = 0; i < 80; i++) lineData[i] = static_cast<uint8_t>(i & 0xff);
	for (int i = 0; i < 16; i++) palette[i] = 0xff000000u | (i * 0x111111u);

	for (auto _ : state) {
		for (int byteIdx = 0; byteIdx < 80; byteIdx++) {
			uint8_t pixByte = lineData[byteIdx];
			outBuffer[byteIdx * 2] = palette[(pixByte >> 4) & 0x0f];
			outBuffer[byteIdx * 2 + 1] = palette[pixByte & 0x0f];
		}
		benchmark::DoNotOptimize(outBuffer.data());
	}
	state.SetBytesProcessed(state.iterations() * 80);
}
BENCHMARK(BM_LynxPpuTools_Framebuffer_SingleLine);

static void BM_LynxPpuTools_Framebuffer_FullScreen(benchmark::State& state) {
	// Render full 160×102 framebuffer
	std::array<uint8_t, 65536> vram;
	std::array<uint32_t, 160 * 102> outBuffer;
	uint32_t palette[16];

	vram.fill(0xab);
	for (int i = 0; i < 16; i++) palette[i] = 0xff000000u | (i * 0x111111u);

	for (auto _ : state) {
		for (uint32_t y = 0; y < ScreenHeight; y++) {
			uint32_t lineAddr = y * BytesPerLine;
			uint32_t* dest = outBuffer.data() + y * ScreenWidth;
			for (uint32_t byteIdx = 0; byteIdx < BytesPerLine; byteIdx++) {
				uint8_t pixByte = vram[lineAddr + byteIdx];
				dest[byteIdx * 2] = palette[(pixByte >> 4) & 0x0f];
				dest[byteIdx * 2 + 1] = palette[pixByte & 0x0f];
			}
		}
		benchmark::DoNotOptimize(outBuffer.data());
	}
	state.SetBytesProcessed(state.iterations() * BytesPerLine * ScreenHeight);
}
BENCHMARK(BM_LynxPpuTools_Framebuffer_FullScreen);

static void BM_LynxPpuTools_Framebuffer_FlipMode(benchmark::State& state) {
	// Render full screen in flip mode (reversed byte order, swapped nibbles)
	std::array<uint8_t, 65536> vram;
	std::array<uint32_t, 160 * 102> outBuffer;
	uint32_t palette[16];

	vram.fill(0xab);
	for (int i = 0; i < 16; i++) palette[i] = 0xff000000u | (i * 0x111111u);

	for (auto _ : state) {
		for (uint32_t y = 0; y < ScreenHeight; y++) {
			uint32_t startAddr = y * BytesPerLine + (BytesPerLine - 1);
			uint32_t* dest = outBuffer.data() + y * ScreenWidth;
			for (uint32_t byteIdx = 0; byteIdx < BytesPerLine; byteIdx++) {
				uint8_t pixByte = vram[startAddr - byteIdx];
				dest[byteIdx * 2] = palette[pixByte & 0x0f];
				dest[byteIdx * 2 + 1] = palette[(pixByte >> 4) & 0x0f];
			}
		}
		benchmark::DoNotOptimize(outBuffer.data());
	}
	state.SetBytesProcessed(state.iterations() * BytesPerLine * ScreenHeight);
}
BENCHMARK(BM_LynxPpuTools_Framebuffer_FlipMode);

// =============================================================================
// Sprite Decode Benchmarks
// =============================================================================

static void BM_LynxPpuTools_SpriteDecode_Literal_4bpp(benchmark::State& state) {
	// Decode 40 bytes of literal 4bpp sprite data → 80 pixels
	std::array<uint8_t, 40> spriteData;
	std::array<uint8_t, 256> outPixels;

	for (int i = 0; i < 40; i++) spriteData[i] = static_cast<uint8_t>((i * 17) & 0xff);

	for (auto _ : state) {
		int pixelCount = 0;
		uint16_t shift = 0;
		int bitsAvail = 0;
		int dataIdx = 0;
		while (dataIdx < 40 || bitsAvail >= 4) {
			while (bitsAvail < 4 && dataIdx < 40) {
				shift |= static_cast<uint16_t>(spriteData[dataIdx]) << bitsAvail;
				bitsAvail += 8;
				dataIdx++;
			}
			if (bitsAvail < 4) break;
			outPixels[pixelCount++] = shift & 0x0f;
			shift >>= 4;
			bitsAvail -= 4;
		}
		benchmark::DoNotOptimize(outPixels.data());
		benchmark::DoNotOptimize(pixelCount);
	}
	state.SetBytesProcessed(state.iterations() * 40);
}
BENCHMARK(BM_LynxPpuTools_SpriteDecode_Literal_4bpp);

static void BM_LynxPpuTools_SpriteDecode_Literal_1bpp(benchmark::State& state) {
	// Decode 20 bytes of 1bpp literal → 160 pixels
	std::array<uint8_t, 20> spriteData;
	std::array<uint8_t, 256> outPixels;

	for (int i = 0; i < 20; i++) spriteData[i] = static_cast<uint8_t>((i * 37) & 0xff);

	for (auto _ : state) {
		int pixelCount = 0;
		uint16_t shift = 0;
		int bitsAvail = 0;
		int dataIdx = 0;
		while (dataIdx < 20 || bitsAvail >= 1) {
			while (bitsAvail < 1 && dataIdx < 20) {
				shift |= static_cast<uint16_t>(spriteData[dataIdx]) << bitsAvail;
				bitsAvail += 8;
				dataIdx++;
			}
			if (bitsAvail < 1) break;
			outPixels[pixelCount++] = shift & 0x01;
			shift >>= 1;
			bitsAvail -= 1;
		}
		benchmark::DoNotOptimize(outPixels.data());
		benchmark::DoNotOptimize(pixelCount);
	}
	state.SetBytesProcessed(state.iterations() * 20);
}
BENCHMARK(BM_LynxPpuTools_SpriteDecode_Literal_1bpp);

static void BM_LynxPpuTools_SpriteDecode_Packed_4bpp(benchmark::State& state) {
	// Decode packed sprite data: count+pixel pairs
	// Example: 5 runs of (count=4, pixel=varying) = 20 total pixels
	std::array<uint8_t, 10> spriteData; // 5 runs × 2 nibble-pairs
	for (int i = 0; i < 5; i++) {
		// count=4, pixel=i → packed into bytes
		spriteData[i * 2] = 0x04 | ((i & 0x0f) << 4);
		spriteData[i * 2 + 1] = 0;
	}

	std::array<uint8_t, 256> outPixels;

	for (auto _ : state) {
		int pixelCount = 0;
		uint16_t shift = 0;
		int bitsAvail = 0;
		int dataIdx = 0;
		while (dataIdx < 10 || bitsAvail >= 8) {
			while (bitsAvail < 8 && dataIdx < 10) {
				shift |= static_cast<uint16_t>(spriteData[dataIdx]) << bitsAvail;
				bitsAvail += 8;
				dataIdx++;
			}
			if (bitsAvail < 8) break;
			uint8_t count = shift & 0x0f;
			shift >>= 4;
			bitsAvail -= 4;
			if (count == 0) break;
			uint8_t pixel = shift & 0x0f;
			shift >>= 4;
			bitsAvail -= 4;
			for (int j = 0; j < count && pixelCount < 256; j++) {
				outPixels[pixelCount++] = pixel;
			}
		}
		benchmark::DoNotOptimize(outPixels.data());
		benchmark::DoNotOptimize(pixelCount);
	}
	state.SetBytesProcessed(state.iterations() * 10);
}
BENCHMARK(BM_LynxPpuTools_SpriteDecode_Packed_4bpp);

// =============================================================================
// SCB Chain Walk Benchmark
// =============================================================================

static void BM_LynxPpuTools_SCBChainWalk_64Sprites(benchmark::State& state) {
	// Measure the cost of walking a 64-sprite SCB chain
	alignas(16) std::array<uint8_t, 65536> ram;
	ram.fill(0);

	uint16_t baseAddr = 0x0100;
	for (int i = 0; i < 64; i++) {
		uint16_t addr = baseAddr + static_cast<uint16_t>(i * 32);
		uint16_t next = (i < 63) ? (baseAddr + static_cast<uint16_t>((i + 1) * 32)) : 0;
		ram[addr] = 0xc4;                               // SPRCTL0
		ram[(addr + 1) & 0xffff] = 0x00;                // SPRCTL1
		ram[(addr + 2) & 0xffff] = 0x00;                // SPRCOLL
		ram[(addr + 3) & 0xffff] = next & 0xff;         // SCBNEXT low
		ram[(addr + 4) & 0xffff] = (next >> 8) & 0xff;  // SCBNEXT high
		ram[(addr + 5) & 0xffff] = 0x00;                // SPRDATA low
		ram[(addr + 6) & 0xffff] = 0x10;                // SPRDATA high
		ram[(addr + 7) & 0xffff] = static_cast<uint8_t>(i * 2);  // HPOS
		ram[(addr + 9) & 0xffff] = static_cast<uint8_t>(i);      // VPOS
	}

	for (auto _ : state) {
		uint16_t scbAddr = baseAddr;
		int count = 0;
		while ((scbAddr >> 8) != 0 && count < 256) {
			// Read SCB fields (simulated)
			uint8_t ctl0 = ram[scbAddr];
			uint8_t ctl1 = ram[(scbAddr + 1) & 0xffff];
			int16_t hpos = static_cast<int16_t>(ram[(scbAddr + 7) & 0xffff] |
				(ram[(scbAddr + 8) & 0xffff] << 8));
			int16_t vpos = static_cast<int16_t>(ram[(scbAddr + 9) & 0xffff] |
				(ram[(scbAddr + 10) & 0xffff] << 8));

			benchmark::DoNotOptimize(ctl0);
			benchmark::DoNotOptimize(ctl1);
			benchmark::DoNotOptimize(hpos);
			benchmark::DoNotOptimize(vpos);

			scbAddr = ram[(scbAddr + 3) & 0xffff] |
				(ram[(scbAddr + 4) & 0xffff] << 8);
			count++;
		}
		benchmark::DoNotOptimize(count);
	}
	state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_LynxPpuTools_SCBChainWalk_64Sprites);

// =============================================================================
// Pen Remap Benchmark
// =============================================================================

static void BM_LynxPpuTools_PenRemap_8Bytes(benchmark::State& state) {
	// Measure palette pen remapping from 8 SCB bytes → 16 pen indices
	std::array<uint8_t, 8> remapBytes = { 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 };
	uint8_t penIndex[16];

	for (auto _ : state) {
		for (int i = 0; i < 8; i++) {
			penIndex[i * 2] = remapBytes[i] >> 4;
			penIndex[i * 2 + 1] = remapBytes[i] & 0x0f;
		}
		benchmark::DoNotOptimize(penIndex);
	}
	state.SetItemsProcessed(state.iterations() * 16);
}
BENCHMARK(BM_LynxPpuTools_PenRemap_8Bytes);
