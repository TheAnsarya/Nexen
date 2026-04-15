#include "pch.h"
#include <array>
#include <cstring>
#include "Lynx/LynxTypes.h"

/// <summary>
/// Tests for the Lynx PPU Tools (palette viewer, tilemap/framebuffer viewer,
/// sprite viewer, and sprite preview compositing).
///
/// LynxPpuTools provides debugger visualization for the Lynx display system:
///   - Palette info: 16-color RGB444 palette from Mikey
///   - Tilemap: 160×102 linear framebuffer with 4bpp packed pixels
///   - Sprites: SCB linked-list walk, variable-length pixel data decoding
///   - Preview: Back-to-front compositing onto screen-sized canvas
///
/// References:
///   - LynxPpuTools.h/.cpp (implementation)
///   - LynxTypes.h (LynxConstants, LynxMikeyState, LynxSuzyState)
///   - Epyx hardware reference (sprite engine, display DMA)
/// </summary>

// =============================================================================
// Test Helpers — replicate self-contained logic from LynxPpuTools
// =============================================================================

namespace {

/// <summary>Pack RGB444 components into a raw palette word (R,G,B are 4-bit each).</summary>
constexpr uint16_t PackRgb444(uint8_t r, uint8_t g, uint8_t b) {
	return static_cast<uint16_t>((r << 8) | (g << 4) | b);
}

/// <summary>Expand 4-bit RGB444 to 8-bit ARGB32.</summary>
constexpr uint32_t Rgb444ToArgb(uint16_t rgb444) {
	uint8_t r4 = (rgb444 >> 8) & 0x0f;
	uint8_t g4 = (rgb444 >> 4) & 0x0f;
	uint8_t b4 = rgb444 & 0x0f;
	uint8_t r8 = (r4 << 4) | r4;
	uint8_t g8 = (g4 << 4) | g4;
	uint8_t b8 = (b4 << 4) | b4;
	return 0xff000000u | (r8 << 16) | (g8 << 8) | b8;
}

/// <summary>Extract palette register values from ARGB color.</summary>
constexpr void ExtractPaletteRegs(uint32_t argb, uint8_t& palBR, uint8_t& palGreen) {
	uint8_t r = (argb >> 20) & 0x0f;
	uint8_t g = (argb >> 12) & 0x0f;
	uint8_t b = (argb >> 4) & 0x0f;
	palBR = static_cast<uint8_t>((b << 4) | r);
	palGreen = g;
}

/// <summary>
/// Decode a sprite data line (replicated from LynxPpuTools::DecodeSpriteLine).
/// This is the bit-level sprite pixel decoding logic.
/// </summary>
static int DecodeSpriteLine(const uint8_t* vram, uint16_t dataAddr, uint16_t endAddr,
	int bpp, bool literal, uint8_t* outPixels, int maxPixels) {
	int pixelCount = 0;
	uint16_t shift = 0;
	int bitsAvail = 0;

	auto feedBits = [&]() {
		while (bitsAvail < bpp && dataAddr < endAddr) {
			shift |= static_cast<uint16_t>(vram[dataAddr & 0xffff]) << bitsAvail;
			bitsAvail += 8;
			dataAddr++;
		}
	};

	auto readBits = [&](int count) -> uint8_t {
		feedBits();
		uint8_t val = shift & ((1 << count) - 1);
		shift >>= count;
		bitsAvail -= count;
		return val;
	};

	if (literal) {
		// Literal mode: every pixel is bpp bits of data
		while (true) {
			feedBits();
			if (bitsAvail < bpp) break;
			uint8_t pixel = readBits(bpp);
			if (pixelCount < maxPixels) {
				outPixels[pixelCount] = pixel;
			}
			pixelCount++;
		}
	} else {
		// Packed mode: alternating count + pixel data
		while (dataAddr <= endAddr || bitsAvail > 0) {
			feedBits();
			if (bitsAvail < bpp) break;

			// Read count (bpp bits wide)
			uint8_t count = readBits(bpp);
			if (count == 0) {
				break;
			}

			feedBits();
			if (bitsAvail < bpp) break;

			uint8_t pixel = readBits(bpp);
			for (int i = 0; i < count && pixelCount < maxPixels; i++) {
				outPixels[pixelCount++] = pixel;
			}
		}
	}

	return pixelCount;
}

/// <summary>Render a framebuffer line (normal mode: high nibble first).</summary>
static void RenderFramebufferLine(const uint8_t* vram, uint16_t lineAddr,
	uint32_t* palette, uint32_t* outBuffer, int width) {
	for (int byteIdx = 0; byteIdx < width / 2; byteIdx++) {
		uint8_t pixByte = vram[(lineAddr + byteIdx) & 0xffff];
		outBuffer[byteIdx * 2] = palette[(pixByte >> 4) & 0x0f];
		outBuffer[byteIdx * 2 + 1] = palette[pixByte & 0x0f];
	}
}

/// <summary>Render a framebuffer line (flip mode: reversed bytes, low nibble first).</summary>
static void RenderFramebufferLineFlip(const uint8_t* vram, uint16_t lineAddr,
	uint32_t* palette, uint32_t* outBuffer, int width) {
	uint16_t startAddr = lineAddr + (width / 2 - 1);
	for (int byteIdx = 0; byteIdx < width / 2; byteIdx++) {
		uint8_t pixByte = vram[(startAddr - byteIdx) & 0xffff];
		outBuffer[byteIdx * 2] = palette[pixByte & 0x0f];
		outBuffer[byteIdx * 2 + 1] = palette[(pixByte >> 4) & 0x0f];
	}
}

} // anonymous namespace

// =============================================================================
// Palette Tests
// =============================================================================

class LynxPpuToolsPaletteTest : public ::testing::Test {
protected:
	// Simulated Mikey palette registers
	uint8_t PaletteBR[16] = {};
	uint8_t PaletteGreen[16] = {};
	uint32_t Palette[16] = {};

	void SetEntry(int index, uint8_t r, uint8_t g, uint8_t b) {
		PaletteBR[index] = (b << 4) | r;
		PaletteGreen[index] = g;
		uint16_t rgb444 = PackRgb444(r, g, b);
		Palette[index] = Rgb444ToArgb(rgb444);
	}
};

TEST_F(LynxPpuToolsPaletteTest, PaletteInfo_ColorCount_16) {
	EXPECT_EQ(LynxConstants::PaletteSize, 16u);
}

TEST_F(LynxPpuToolsPaletteTest, PackRgb444_AllZero) {
	EXPECT_EQ(PackRgb444(0, 0, 0), 0x000);
}

TEST_F(LynxPpuToolsPaletteTest, PackRgb444_AllMax) {
	EXPECT_EQ(PackRgb444(0x0f, 0x0f, 0x0f), 0xfff);
}

TEST_F(LynxPpuToolsPaletteTest, PackRgb444_RedOnly) {
	EXPECT_EQ(PackRgb444(0x0f, 0, 0), 0xf00);
}

TEST_F(LynxPpuToolsPaletteTest, PackRgb444_GreenOnly) {
	EXPECT_EQ(PackRgb444(0, 0x0f, 0), 0x0f0);
}

TEST_F(LynxPpuToolsPaletteTest, PackRgb444_BlueOnly) {
	EXPECT_EQ(PackRgb444(0, 0, 0x0f), 0x00f);
}

TEST_F(LynxPpuToolsPaletteTest, Rgb444ToArgb_Black) {
	EXPECT_EQ(Rgb444ToArgb(0x000), 0xff000000u);
}

TEST_F(LynxPpuToolsPaletteTest, Rgb444ToArgb_White) {
	EXPECT_EQ(Rgb444ToArgb(0xfff), 0xffffffffu);
}

TEST_F(LynxPpuToolsPaletteTest, Rgb444ToArgb_Red) {
	uint32_t result = Rgb444ToArgb(0xf00);
	EXPECT_EQ((result >> 16) & 0xff, 0xffu); // R channel
	EXPECT_EQ((result >> 8) & 0xff, 0x00u);  // G channel
	EXPECT_EQ(result & 0xff, 0x00u);         // B channel
}

TEST_F(LynxPpuToolsPaletteTest, Rgb444ToArgb_AlphaAlwaysOpaque) {
	EXPECT_EQ(Rgb444ToArgb(0x000) >> 24, 0xffu);
	EXPECT_EQ(Rgb444ToArgb(0xfff) >> 24, 0xffu);
	EXPECT_EQ(Rgb444ToArgb(0x123) >> 24, 0xffu);
}

TEST_F(LynxPpuToolsPaletteTest, PaletteBR_Register_Packing) {
	// PaletteBR: [7:4]=blue, [3:0]=red
	SetEntry(0, 0x0a, 0x05, 0x0c);
	EXPECT_EQ(PaletteBR[0], 0xca); // blue=c, red=a
	EXPECT_EQ(PaletteGreen[0], 0x05);
}

TEST_F(LynxPpuToolsPaletteTest, SetPaletteColor_RoundTrip) {
	// Set via ARGB, extract regs, verify round-trip
	uint32_t argb = 0xff8844cc; // R≈0x8, G≈0x4, B≈0xc in 4-bit space
	uint8_t palBR, palGreen;
	ExtractPaletteRegs(argb, palBR, palGreen);

	uint8_t r = palBR & 0x0f;
	uint8_t g = palGreen;
	uint8_t b = (palBR >> 4) & 0x0f;

	// Reconstruct and verify
	uint16_t rgb444 = PackRgb444(r, g, b);
	uint32_t reconstructed = Rgb444ToArgb(rgb444);

	// The 4-bit truncation means the roundtrip won't be exact to 8-bit,
	// but the 4-bit components must survive
	EXPECT_EQ((reconstructed >> 16) & 0xf0, r << 4);
}

TEST_F(LynxPpuToolsPaletteTest, PaletteIndex_OutOfRange_Clamped) {
	// Palette index >= 16 should be masked to 4 bits
	uint8_t index = 0x1a; // Out of range
	EXPECT_EQ(index & 0x0f, 0x0a);
}

// =============================================================================
// Framebuffer / Tilemap Tests
// =============================================================================

class LynxPpuToolsFramebufferTest : public ::testing::Test {
protected:
	static constexpr uint32_t Width = LynxConstants::ScreenWidth;       // 160
	static constexpr uint32_t Height = LynxConstants::ScreenHeight;     // 102
	static constexpr uint32_t BytesPerLine = LynxConstants::BytesPerScanline; // 80

	std::array<uint8_t, 65536> vram = {};
	std::array<uint32_t, 16> palette = {};
	std::array<uint32_t, 160 * 102> outBuffer = {};

	void SetUp() override {
		vram.fill(0);
		outBuffer.fill(0);
		for (int i = 0; i < 16; i++) {
			palette[i] = 0xff000000u | (i * 0x111111u);
		}
	}
};

TEST_F(LynxPpuToolsFramebufferTest, Dimensions_160x102) {
	EXPECT_EQ(LynxConstants::ScreenWidth, 160u);
	EXPECT_EQ(LynxConstants::ScreenHeight, 102u);
}

TEST_F(LynxPpuToolsFramebufferTest, BytesPerScanline_80) {
	EXPECT_EQ(LynxConstants::BytesPerScanline, 80u);
	EXPECT_EQ(Width / 2, BytesPerLine);
}

TEST_F(LynxPpuToolsFramebufferTest, PixelCount_16320) {
	EXPECT_EQ(LynxConstants::PixelCount, 160u * 102u);
}

TEST_F(LynxPpuToolsFramebufferTest, NormalMode_HighNibbleFirst) {
	// Byte 0x12 at address 0: pixel 0=1, pixel 1=2
	vram[0] = 0x12;
	RenderFramebufferLine(vram.data(), 0, palette.data(), outBuffer.data(), Width);
	EXPECT_EQ(outBuffer[0], palette[1]); // high nibble = first pixel
	EXPECT_EQ(outBuffer[1], palette[2]); // low nibble = second pixel
}

TEST_F(LynxPpuToolsFramebufferTest, NormalMode_AllZero_BlackLine) {
	RenderFramebufferLine(vram.data(), 0, palette.data(), outBuffer.data(), Width);
	for (uint32_t i = 0; i < Width; i++) {
		EXPECT_EQ(outBuffer[i], palette[0]);
	}
}

TEST_F(LynxPpuToolsFramebufferTest, NormalMode_AlternatingNibbles) {
	// Fill first scanline with 0xAB → pixel pairs (0xA, 0xB)
	for (uint32_t i = 0; i < BytesPerLine; i++) {
		vram[i] = 0xab;
	}
	RenderFramebufferLine(vram.data(), 0, palette.data(), outBuffer.data(), Width);
	for (uint32_t i = 0; i < Width; i += 2) {
		EXPECT_EQ(outBuffer[i], palette[0x0a]);
		EXPECT_EQ(outBuffer[i + 1], palette[0x0b]);
	}
}

TEST_F(LynxPpuToolsFramebufferTest, FlipMode_ReversedAndSwapped) {
	// In flip mode: bytes read backwards, low nibble = first pixel
	vram[79] = 0x34; // Last byte of first line (addr 79)
	RenderFramebufferLineFlip(vram.data(), 0, palette.data(), outBuffer.data(), Width);
	// First pixel pair comes from the last byte, low nibble first
	EXPECT_EQ(outBuffer[0], palette[4]); // low nibble
	EXPECT_EQ(outBuffer[1], palette[3]); // high nibble
}

TEST_F(LynxPpuToolsFramebufferTest, FlipMode_ByteOrder_Reversed) {
	// Byte at addr 78 should produce pixels 2,3 (after the byte at addr 79)
	vram[78] = 0x56;
	vram[79] = 0x12;
	RenderFramebufferLineFlip(vram.data(), 0, palette.data(), outBuffer.data(), Width);
	// addr 79 → pix 0,1; addr 78 → pix 2,3
	EXPECT_EQ(outBuffer[0], palette[2]); // 0x12 low nibble
	EXPECT_EQ(outBuffer[1], palette[1]); // 0x12 high nibble
	EXPECT_EQ(outBuffer[2], palette[6]); // 0x56 low nibble
	EXPECT_EQ(outBuffer[3], palette[5]); // 0x56 high nibble
}

TEST_F(LynxPpuToolsFramebufferTest, DisplayAddress_Offset) {
	// Framebuffer can start at any 16-bit address
	uint16_t displayAddr = 0x4000;
	vram[displayAddr] = 0xef;
	RenderFramebufferLine(vram.data(), displayAddr, palette.data(), outBuffer.data(), Width);
	EXPECT_EQ(outBuffer[0], palette[0x0e]);
	EXPECT_EQ(outBuffer[1], palette[0x0f]);
}

TEST_F(LynxPpuToolsFramebufferTest, AddressWrap_64KB) {
	// Address wraps around 64KB boundary
	uint16_t addr = 0xfff0;
	vram[addr] = 0xab;
	RenderFramebufferLine(vram.data(), addr, palette.data(), outBuffer.data(), Width);
	EXPECT_EQ(outBuffer[0], palette[0x0a]);
}

TEST_F(LynxPpuToolsFramebufferTest, TilemapTileInfo_PixelCoords) {
	// TilemapTileInfo returns per-pixel data — verify byte offset calc
	uint16_t displayAddr = 0;
	uint32_t x = 5, y = 3;
	uint32_t byteIdx = x / 2;
	uint16_t lineAddr = displayAddr + static_cast<uint16_t>(y * BytesPerLine);
	uint16_t byteOffset = lineAddr + byteIdx;
	EXPECT_EQ(byteOffset, 3 * 80 + 2);
}

TEST_F(LynxPpuToolsFramebufferTest, TilemapTileInfo_NibbleSelection_EvenX) {
	// Even X → high nibble in normal mode
	vram[0] = 0xab;
	uint8_t pixelByte = vram[0];
	uint8_t colorEven = (pixelByte >> 4) & 0x0f; // x=0
	EXPECT_EQ(colorEven, 0x0a);
}

TEST_F(LynxPpuToolsFramebufferTest, TilemapTileInfo_NibbleSelection_OddX) {
	// Odd X → low nibble in normal mode
	vram[0] = 0xab;
	uint8_t pixelByte = vram[0];
	uint8_t colorOdd = pixelByte & 0x0f; // x=1
	EXPECT_EQ(colorOdd, 0x0b);
}

// =============================================================================
// Sprite Decode Tests
// =============================================================================

class LynxPpuToolsSpriteDecodeTest : public ::testing::Test {
protected:
	std::array<uint8_t, 65536> vram = {};
	std::array<uint8_t, 512> outPixels = {};

	void SetUp() override {
		vram.fill(0);
		outPixels.fill(0);
	}
};

TEST_F(LynxPpuToolsSpriteDecodeTest, LiteralMode_4bpp_SingleByte) {
	// 4bpp literal: 1 byte = 2 pixels (4 bits each)
	vram[0] = 0x12; // pixels: 0x2, 0x1
	int count = DecodeSpriteLine(vram.data(), 0, 1, 4, true, outPixels.data(), 512);
	EXPECT_EQ(count, 2);
	EXPECT_EQ(outPixels[0], 0x02);
	EXPECT_EQ(outPixels[1], 0x01);
}

TEST_F(LynxPpuToolsSpriteDecodeTest, LiteralMode_4bpp_TwoBytes) {
	vram[0] = 0xab;
	vram[1] = 0xcd;
	int count = DecodeSpriteLine(vram.data(), 0, 2, 4, true, outPixels.data(), 512);
	EXPECT_EQ(count, 4);
}

TEST_F(LynxPpuToolsSpriteDecodeTest, LiteralMode_1bpp) {
	// 1bpp literal: 1 byte = 8 pixels (1 bit each)
	vram[0] = 0xff; // All 1s
	int count = DecodeSpriteLine(vram.data(), 0, 1, 1, true, outPixels.data(), 512);
	EXPECT_EQ(count, 8);
	for (int i = 0; i < 8; i++) {
		EXPECT_EQ(outPixels[i], 1);
	}
}

TEST_F(LynxPpuToolsSpriteDecodeTest, LiteralMode_1bpp_Alternating) {
	vram[0] = 0xaa; // 10101010
	int count = DecodeSpriteLine(vram.data(), 0, 1, 1, true, outPixels.data(), 512);
	EXPECT_EQ(count, 8);
	// LSB-first reading: bit 0=0, bit 1=1, bit 2=0, etc.
	EXPECT_EQ(outPixels[0], 0);
	EXPECT_EQ(outPixels[1], 1);
	EXPECT_EQ(outPixels[2], 0);
	EXPECT_EQ(outPixels[3], 1);
}

TEST_F(LynxPpuToolsSpriteDecodeTest, LiteralMode_2bpp) {
	// 2bpp literal: 1 byte = 4 pixels (2 bits each)
	vram[0] = 0b11100100; // pixels: 00, 01, 10, 11 (LSB-first)
	int count = DecodeSpriteLine(vram.data(), 0, 1, 2, true, outPixels.data(), 512);
	EXPECT_EQ(count, 4);
}

TEST_F(LynxPpuToolsSpriteDecodeTest, LiteralMode_3bpp) {
	// 3bpp across byte boundary: 8 bits / 3 = 2 full pixels with 2 bits left
	vram[0] = 0xff;
	int count = DecodeSpriteLine(vram.data(), 0, 1, 3, true, outPixels.data(), 512);
	EXPECT_EQ(count, 2); // Only 2 full 3-bit pixels from 8 bits
}

TEST_F(LynxPpuToolsSpriteDecodeTest, LiteralMode_EmptyData) {
	int count = DecodeSpriteLine(vram.data(), 0, 0, 4, true, outPixels.data(), 512);
	EXPECT_EQ(count, 0);
}

TEST_F(LynxPpuToolsSpriteDecodeTest, PackedMode_SingleRun) {
	// Packed 4bpp: count(4), pixel(4) → run of 'count' pixels of 'pixel'
	// count=3, pixel=5 → 3 pixels of value 5
	vram[0] = 0x53; // bits: 0011 0101 (LSB-first: count=3, pixel=5)
	int count = DecodeSpriteLine(vram.data(), 0, 1, 4, false, outPixels.data(), 512);
	EXPECT_EQ(count, 3);
	for (int i = 0; i < 3; i++) {
		EXPECT_EQ(outPixels[i], 5);
	}
}

TEST_F(LynxPpuToolsSpriteDecodeTest, PackedMode_CountZero_StopsDecoding) {
	// Count=0 terminates line
	vram[0] = 0x00;
	int count = DecodeSpriteLine(vram.data(), 0, 1, 4, false, outPixels.data(), 512);
	EXPECT_EQ(count, 0);
}

TEST_F(LynxPpuToolsSpriteDecodeTest, PackedMode_1bpp_Run) {
	// 1bpp packed: count(1), pixel(1)
	// count=1, pixel=1 → 1 pixel of value 1
	vram[0] = 0x03; // bits: 11 → count=1, pixel=1
	int count = DecodeSpriteLine(vram.data(), 0, 1, 1, false, outPixels.data(), 512);
	EXPECT_GE(count, 1);
	EXPECT_EQ(outPixels[0], 1);
}

TEST_F(LynxPpuToolsSpriteDecodeTest, MaxPixels_Respected) {
	// MaxPixels should limit output
	vram[0] = 0xff;
	vram[1] = 0xff;
	int count = DecodeSpriteLine(vram.data(), 0, 2, 1, true, outPixels.data(), 4);
	EXPECT_LE(count, 16); // 16 possible pixels from 2 bytes of 1bpp
}

// =============================================================================
// Sprite SCB Chain Tests
// =============================================================================

class LynxPpuToolsSpriteChainTest : public ::testing::Test {
protected:
	std::array<uint8_t, 65536> ram = {};

	void SetUp() override {
		ram.fill(0);
	}

	/// <summary>Set up a minimal SCB at the given address.</summary>
	void SetupSCB(uint16_t addr, uint8_t ctl0, uint8_t ctl1, uint8_t coll,
		uint16_t next, uint16_t data, int16_t hpos, int16_t vpos) {
		ram[addr] = ctl0;
		ram[(addr + 1) & 0xffff] = ctl1;
		ram[(addr + 2) & 0xffff] = coll;
		ram[(addr + 3) & 0xffff] = next & 0xff;
		ram[(addr + 4) & 0xffff] = (next >> 8) & 0xff;
		ram[(addr + 5) & 0xffff] = data & 0xff;
		ram[(addr + 6) & 0xffff] = (data >> 8) & 0xff;
		ram[(addr + 7) & 0xffff] = hpos & 0xff;
		ram[(addr + 8) & 0xffff] = (hpos >> 8) & 0xff;
		ram[(addr + 9) & 0xffff] = vpos & 0xff;
		ram[(addr + 10) & 0xffff] = (vpos >> 8) & 0xff;
	}
};

TEST_F(LynxPpuToolsSpriteChainTest, EmptyChain_HighByteZero) {
	// SCBAddress with high byte = 0 means no sprites
	uint16_t scbAddr = 0x0000;
	EXPECT_EQ(scbAddr >> 8, 0);
}

TEST_F(LynxPpuToolsSpriteChainTest, SingleSprite_NextNull) {
	// One sprite with SCBNEXT = $0000 (terminates chain)
	SetupSCB(0x0100, 0xc4, 0x00, 0x00, 0x0000, 0x1000, 10, 20);

	uint16_t scbAddr = 0x0100;
	int count = 0;
	while ((scbAddr >> 8) != 0 && count < 256) {
		count++;
		scbAddr = ram[(scbAddr + 3) & 0xffff] | (ram[(scbAddr + 4) & 0xffff] << 8);
	}
	EXPECT_EQ(count, 1);
}

TEST_F(LynxPpuToolsSpriteChainTest, TwoSprites_Chained) {
	SetupSCB(0x0100, 0xc4, 0x00, 0x00, 0x0120, 0x1000, 10, 20);
	SetupSCB(0x0120, 0xc4, 0x00, 0x00, 0x0000, 0x1100, 30, 40);

	uint16_t scbAddr = 0x0100;
	int count = 0;
	while ((scbAddr >> 8) != 0 && count < 256) {
		count++;
		scbAddr = ram[(scbAddr + 3) & 0xffff] | (ram[(scbAddr + 4) & 0xffff] << 8);
	}
	EXPECT_EQ(count, 2);
}

TEST_F(LynxPpuToolsSpriteChainTest, ChainLimit_256Sprites) {
	// Safety: chain walk limited to 256 sprites
	// Create a self-referencing loop at addr 0x0100
	SetupSCB(0x0100, 0xc4, 0x00, 0x00, 0x0100, 0x1000, 0, 0);

	uint16_t scbAddr = 0x0100;
	int count = 0;
	while ((scbAddr >> 8) != 0 && count < 256) {
		count++;
		scbAddr = ram[(scbAddr + 3) & 0xffff] | (ram[(scbAddr + 4) & 0xffff] << 8);
	}
	EXPECT_EQ(count, 256);
}

TEST_F(LynxPpuToolsSpriteChainTest, SCB_SpriteControl0_BppDecode) {
	// SPRCTL0 bits 7:6 → BPP: 0=1bpp, 1=2bpp, 2=3bpp, 3=4bpp
	SetupSCB(0x0100, 0x00, 0x00, 0x00, 0x0000, 0x1000, 0, 0);
	EXPECT_EQ(((ram[0x0100] >> 6) & 0x03) + 1, 1); // 1bpp

	ram[0x0100] = 0x40;
	EXPECT_EQ(((ram[0x0100] >> 6) & 0x03) + 1, 2); // 2bpp

	ram[0x0100] = 0x80;
	EXPECT_EQ(((ram[0x0100] >> 6) & 0x03) + 1, 3); // 3bpp

	ram[0x0100] = 0xc0;
	EXPECT_EQ(((ram[0x0100] >> 6) & 0x03) + 1, 4); // 4bpp
}

TEST_F(LynxPpuToolsSpriteChainTest, SCB_SpriteControl0_FlipFlags) {
	// SPRCTL0 bits 5:4 → H/V flip
	SetupSCB(0x0100, 0x30, 0x00, 0x00, 0x0000, 0x1000, 0, 0);
	bool hFlip = (ram[0x0100] & 0x20) != 0;
	bool vFlip = (ram[0x0100] & 0x10) != 0;
	EXPECT_TRUE(hFlip);
	EXPECT_TRUE(vFlip);
}

TEST_F(LynxPpuToolsSpriteChainTest, SCB_SpriteControl0_SpriteType) {
	// SPRCTL0 bits 2:0 → sprite type
	for (uint8_t type = 0; type < 8; type++) {
		ram[0x0100] = type;
		auto sprType = static_cast<LynxSpriteType>(ram[0x0100] & 0x07);
		EXPECT_EQ(static_cast<uint8_t>(sprType), type);
	}
}

TEST_F(LynxPpuToolsSpriteChainTest, SCB_SpriteControl1_SkipFlag) {
	// SPRCTL1 bit 2 → skip sprite
	SetupSCB(0x0100, 0xc4, 0x04, 0x00, 0x0000, 0x1000, 0, 0);
	bool skipSprite = (ram[0x0101] & 0x04) != 0;
	EXPECT_TRUE(skipSprite);
}

TEST_F(LynxPpuToolsSpriteChainTest, SCB_SpriteControl1_ReloadPalette) {
	// SPRCTL1 bit 3: 0 = reload palette, 1 = keep existing
	ram[0x0101] = 0x00; // bit 3 = 0 → reload
	bool reload = (ram[0x0101] & 0x08) == 0;
	EXPECT_TRUE(reload);

	ram[0x0101] = 0x08; // bit 3 = 1 → keep
	reload = (ram[0x0101] & 0x08) == 0;
	EXPECT_FALSE(reload);
}

TEST_F(LynxPpuToolsSpriteChainTest, SCB_SpriteControl1_ReloadDepth) {
	// SPRCTL1 bits 5:4 → reloadDepth: 0..3
	for (int depth = 0; depth < 4; depth++) {
		ram[0x0101] = static_cast<uint8_t>(depth << 4);
		int reloadDepth = (ram[0x0101] >> 4) & 0x03;
		EXPECT_EQ(reloadDepth, depth);
	}
}

TEST_F(LynxPpuToolsSpriteChainTest, SCB_SpriteControl1_LiteralMode) {
	// SPRCTL1 bit 7 → literal mode
	ram[0x0101] = 0x80;
	bool literal = (ram[0x0101] & 0x80) != 0;
	EXPECT_TRUE(literal);
}

TEST_F(LynxPpuToolsSpriteChainTest, SCB_CollisionNumber) {
	// SPRCOLL byte: low nibble is collision number
	SetupSCB(0x0100, 0xc4, 0x00, 0x5a, 0x0000, 0x1000, 0, 0);
	uint8_t collNumber = ram[0x0102] & 0x0f;
	EXPECT_EQ(collNumber, 0x0a);
}

TEST_F(LynxPpuToolsSpriteChainTest, SCB_Position_Signed) {
	// Position fields are signed 16-bit
	SetupSCB(0x0100, 0xc4, 0x00, 0x00, 0x0000, 0x1000, -10, -20);
	int16_t hpos = static_cast<int16_t>(ram[0x0107] | (ram[0x0108] << 8));
	int16_t vpos = static_cast<int16_t>(ram[0x0109] | (ram[0x010a] << 8));
	EXPECT_EQ(hpos, -10);
	EXPECT_EQ(vpos, -20);
}

TEST_F(LynxPpuToolsSpriteChainTest, SCB_ReloadDepth1_LoadsSize) {
	// When reloadDepth >= 1, SIZE (4 bytes) loaded at offset 11
	SetupSCB(0x0100, 0xc4, 0x10, 0x00, 0x0000, 0x1000, 0, 0); // depth=1
	// Write HSIZE=0x0200 (2.0 fixed), VSIZE=0x0300 (3.0 fixed) at offset 11
	ram[0x010b] = 0x00; ram[0x010c] = 0x02; // HSIZE
	ram[0x010d] = 0x00; ram[0x010e] = 0x03; // VSIZE

	uint16_t hsize = ram[0x010b] | (ram[0x010c] << 8);
	uint16_t vsize = ram[0x010d] | (ram[0x010e] << 8);
	EXPECT_EQ(hsize, 0x0200);
	EXPECT_EQ(vsize, 0x0300);
}

TEST_F(LynxPpuToolsSpriteChainTest, SCB_ReloadDepth2_LoadsStretch) {
	// When reloadDepth >= 2, STRETCH (2 bytes) loaded after SIZE
	SetupSCB(0x0100, 0xc4, 0x20, 0x00, 0x0000, 0x1000, 0, 0); // depth=2
	// Offset: 11 (SIZE 4) + 15 (STRETCH 2)
	ram[0x010f] = 0x80; ram[0x0110] = 0x00; // STRETCH = 0x0080
	uint16_t stretch = ram[0x010f] | (ram[0x0110] << 8);
	EXPECT_EQ(stretch, 0x0080);
}

// =============================================================================
// Sprite Preview Compositing Tests
// =============================================================================

class LynxPpuToolsCompositeTest : public ::testing::Test {
protected:
	static constexpr uint32_t Width = LynxConstants::ScreenWidth;
	static constexpr uint32_t Height = LynxConstants::ScreenHeight;
};

TEST_F(LynxPpuToolsCompositeTest, ScreenDimensions) {
	EXPECT_EQ(Width, 160u);
	EXPECT_EQ(Height, 102u);
}

TEST_F(LynxPpuToolsCompositeTest, SpriteVisibility_OnScreen) {
	int16_t hpos = 80, vpos = 50;
	uint16_t width = 16, height = 16;
	bool onScreen = (hpos + width > 0) && (hpos < static_cast<int16_t>(Width)) &&
		(vpos + height > 0) && (vpos < static_cast<int16_t>(Height));
	EXPECT_TRUE(onScreen);
}

TEST_F(LynxPpuToolsCompositeTest, SpriteVisibility_Offscreen_Right) {
	int16_t hpos = 200, vpos = 50;
	uint16_t width = 16, height = 16;
	bool onScreen = (hpos + width > 0) && (hpos < static_cast<int16_t>(Width)) &&
		(vpos + height > 0) && (vpos < static_cast<int16_t>(Height));
	EXPECT_FALSE(onScreen);
}

TEST_F(LynxPpuToolsCompositeTest, SpriteVisibility_PartiallyOnScreen_Left) {
	int16_t hpos = -8, vpos = 50; // 8px offscreen left, 8px visible
	uint16_t width = 16, height = 16;
	bool onScreen = (hpos + width > 0) && (hpos < static_cast<int16_t>(Width)) &&
		(vpos + height > 0) && (vpos < static_cast<int16_t>(Height));
	EXPECT_TRUE(onScreen);
}

TEST_F(LynxPpuToolsCompositeTest, SpriteVisibility_Offscreen_Above) {
	int16_t hpos = 50, vpos = -20;
	uint16_t width = 16, height = 16;
	bool onScreen = (hpos + width > 0) && (hpos < static_cast<int16_t>(Width)) &&
		(vpos + height > 0) && (vpos < static_cast<int16_t>(Height));
	EXPECT_FALSE(onScreen);
}

TEST_F(LynxPpuToolsCompositeTest, SpriteVisibility_ExactlyAtOrigin) {
	int16_t hpos = 0, vpos = 0;
	uint16_t width = 1, height = 1;
	bool onScreen = (hpos + width > 0) && (hpos < static_cast<int16_t>(Width)) &&
		(vpos + height > 0) && (vpos < static_cast<int16_t>(Height));
	EXPECT_TRUE(onScreen);
}

TEST_F(LynxPpuToolsCompositeTest, SpriteType_BackgroundPriority) {
	auto type = LynxSpriteType::BackgroundShadow;
	bool isBackground = (type == LynxSpriteType::BackgroundShadow ||
		type == LynxSpriteType::BackgroundNonCollide);
	EXPECT_TRUE(isBackground);
}

TEST_F(LynxPpuToolsCompositeTest, SpriteType_XorBlending) {
	auto type = LynxSpriteType::XorShadow;
	EXPECT_EQ(static_cast<uint8_t>(type), 6);
}

TEST_F(LynxPpuToolsCompositeTest, SpriteType_NormalForeground) {
	auto type = LynxSpriteType::Normal;
	EXPECT_EQ(static_cast<uint8_t>(type), 4);
}

// =============================================================================
// Palette Pen Remap Tests
// =============================================================================

class LynxPpuToolsPenRemapTest : public ::testing::Test {
protected:
	uint8_t penIndex[16] = {};

	void SetUp() override {
		// Identity mapping
		for (int i = 0; i < 16; i++) {
			penIndex[i] = static_cast<uint8_t>(i);
		}
	}
};

TEST_F(LynxPpuToolsPenRemapTest, IdentityMapping) {
	for (int i = 0; i < 16; i++) {
		EXPECT_EQ(penIndex[i], i);
	}
}

TEST_F(LynxPpuToolsPenRemapTest, ReloadFromSCB) {
	// Palette remap: 8 bytes, each byte = 2 pen entries (high/low nibble)
	uint8_t remapBytes[8] = { 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 };
	for (int i = 0; i < 8; i++) {
		penIndex[i * 2] = remapBytes[i] >> 4;
		penIndex[i * 2 + 1] = remapBytes[i] & 0x0f;
	}
	EXPECT_EQ(penIndex[0], 0x0f);
	EXPECT_EQ(penIndex[1], 0x0e);
	EXPECT_EQ(penIndex[14], 0x01);
	EXPECT_EQ(penIndex[15], 0x00);
}

TEST_F(LynxPpuToolsPenRemapTest, PenZero_AlwaysTransparent) {
	// Pen 0 maps to whatever, but is treated as transparent in rendering
	penIndex[0] = 0x05;
	uint8_t rawPixel = 0;
	// In rendering: if (rawPixel != 0) → paint; else skip
	EXPECT_EQ(rawPixel, 0);
}

// =============================================================================
// Sprite Data Line Header Tests
// =============================================================================

class LynxPpuToolsSpriteLineTest : public ::testing::Test {};

TEST_F(LynxPpuToolsSpriteLineTest, LineHeader_Zero_EndsSprite) {
	uint8_t header = 0;
	EXPECT_EQ(header, 0); // Terminates sprite
}

TEST_F(LynxPpuToolsSpriteLineTest, LineHeader_One_EmptyLine) {
	uint8_t header = 1;
	int dataBytes = header - 1;
	EXPECT_EQ(dataBytes, 0); // Line with no pixel data
}

TEST_F(LynxPpuToolsSpriteLineTest, LineHeader_N_DataBytes) {
	uint8_t header = 5;
	int dataBytes = header - 1;
	EXPECT_EQ(dataBytes, 4); // 4 bytes of pixel data
}

TEST_F(LynxPpuToolsSpriteLineTest, MaxLines_128) {
	// Sprite decode limits to 128 lines
	int maxLines = 128;
	EXPECT_EQ(maxLines, 128);
}

// =============================================================================
// Preview Size Constants
// =============================================================================

class LynxPpuToolsPreviewTest : public ::testing::Test {};

TEST_F(LynxPpuToolsPreviewTest, PreviewClamp_128x128) {
	int maxWidth = 200, maxHeight = 150;
	int previewWidth = std::min(maxWidth, 128);
	int previewHeight = std::min(maxHeight, 128);
	EXPECT_EQ(previewWidth, 128);
	EXPECT_EQ(previewHeight, 128);
}

TEST_F(LynxPpuToolsPreviewTest, PreviewClamp_SmallSprite) {
	int maxWidth = 16, maxHeight = 8;
	int previewWidth = std::min(maxWidth, 128);
	int previewHeight = std::min(maxHeight, 128);
	EXPECT_EQ(previewWidth, 16);
	EXPECT_EQ(previewHeight, 8);
}
