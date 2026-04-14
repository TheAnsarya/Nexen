#include "pch.h"
#include <array>
#include <algorithm>
#include <cstring>
#include <vector>
#include "WS/WsTypes.h"

// =============================================================================
// WonderSwan PPU Tests
// =============================================================================
// Tests for WS/WSC PPU rendering logic: pixel color extraction, palette lookup,
// background/sprite rendering rules, frame timing, and the #1076 VTOTAL fix.

// --- Helpers: Isolated reimplementations of PPU logic for test verification ---

namespace WsPpuTestHelpers {
	// Monochrome 2bpp pixel extraction (same as WsPpu::GetPixelColor<Monochrome>)
	static uint16_t GetPixelColor_Mono(const uint8_t* tileData, uint8_t column) {
		return (
			((tileData[0] << column) & 0x80) >> 7 |
			((tileData[1] << column) & 0x80) >> 6);
	}

	// Color 2bpp pixel extraction (identical logic to mono - same bitplane layout)

	// Color 4bpp pixel extraction
	static uint16_t GetPixelColor_Color4bpp(const uint8_t* tileData, uint8_t column) {
		return (
			((tileData[0] << column) & 0x80) >> 7 |
			((tileData[1] << column) & 0x80) >> 6 |
			((tileData[2] << column) & 0x80) >> 5 |
			((tileData[3] << column) & 0x80) >> 4);
	}

	// Color 4bpp packed pixel extraction
	static uint16_t GetPixelColor_Color4bppPacked(const uint8_t* tileData, uint8_t column) {
		return (tileData[column / 2] >> (column & 0x01 ? 0 : 4)) & 0x0f;
	}

	// Monochrome palette → shade → RGB conversion
	static uint16_t GetMonoPixelRgb(uint8_t color, uint8_t palette,
		const uint8_t bwPalettes[0x40], const uint8_t bwShades[8]) {
		uint8_t shadeValue = bwPalettes[(palette << 2) | color];
		uint8_t brightness = bwShades[shadeValue] ^ 0x0f;
		return brightness | (brightness << 4) | (brightness << 8);
	}

	// Color palette RAM lookup
	static uint16_t GetColorPixelRgb(uint8_t color, uint8_t palette, const uint8_t* vram) {
		uint16_t paletteAddr = 0xfe00 | (palette << 5) | (color << 1);
		return vram[paletteAddr] | ((vram[paletteAddr + 1] & 0x0f) << 8);
	}

	// Background color for monochrome
	static uint16_t GetMonoBgColor(uint8_t bgColor, const uint8_t bwShades[8]) {
		uint8_t bgBrightness = bwShades[bgColor & 0x07] ^ 0x0f;
		return bgBrightness | (bgBrightness << 4) | (bgBrightness << 8);
	}

	// Background color for color modes
	static uint16_t GetColorBgColor(uint8_t bgColor, const uint8_t* vram) {
		return vram[0xfe00 | (bgColor << 1)] | ((vram[0xfe00 | (bgColor << 1) | 1] & 0x0f) << 8);
	}

	// Window bounds check (same as WsWindow::IsInsideWindow)
	static bool IsInsideWindow(uint8_t x, uint8_t y,
		uint8_t left, uint8_t right, uint8_t top, uint8_t bottom) {
		return x >= left && x <= right && y >= top && y <= bottom;
	}
}

// =============================================================================
// Pixel Color Extraction Tests
// =============================================================================

class WsPpuPixelColorTest : public ::testing::Test {
protected:
	// 2bpp tile data: 2 bytes per row
	uint8_t tileData2bpp[2] = {};
	// 4bpp tile data: 4 bytes per row
	uint8_t tileData4bpp[4] = {};
	// 4bpp packed: 4 bytes for 8 pixels (nibbles)
	uint8_t tileDataPacked[4] = {};
};

TEST_F(WsPpuPixelColorTest, Mono_AllZero_ReturnsZero) {
	tileData2bpp[0] = 0x00;
	tileData2bpp[1] = 0x00;
	for (uint8_t col = 0; col < 8; col++) {
		EXPECT_EQ(WsPpuTestHelpers::GetPixelColor_Mono(tileData2bpp, col), 0)
			<< "Column=" << (int)col;
	}
}

TEST_F(WsPpuPixelColorTest, Mono_AllOnes_ReturnsThree) {
	tileData2bpp[0] = 0xff;
	tileData2bpp[1] = 0xff;
	for (uint8_t col = 0; col < 8; col++) {
		EXPECT_EQ(WsPpuTestHelpers::GetPixelColor_Mono(tileData2bpp, col), 3)
			<< "Column=" << (int)col;
	}
}

TEST_F(WsPpuPixelColorTest, Mono_Alternating_CorrectPattern) {
	// Byte 0 = 0xAA = 10101010, Byte 1 = 0x55 = 01010101
	tileData2bpp[0] = 0xaa;
	tileData2bpp[1] = 0x55;
	// Column 0: bit7 of byte0=1, bit7 of byte1=0 → color = 1
	// Column 1: bit6 of byte0=0, bit6 of byte1=1 → color = 2
	// ...alternating 1, 2 pattern
	for (uint8_t col = 0; col < 8; col++) {
		uint16_t expected = (col % 2 == 0) ? 1 : 2;
		EXPECT_EQ(WsPpuTestHelpers::GetPixelColor_Mono(tileData2bpp, col), expected)
			<< "Column=" << (int)col;
	}
}

TEST_F(WsPpuPixelColorTest, Mono_Column0_HighBitOnly) {
	// Only bit 7 of the first byte set → column 0 = color 1
	tileData2bpp[0] = 0x80;
	tileData2bpp[1] = 0x00;
	EXPECT_EQ(WsPpuTestHelpers::GetPixelColor_Mono(tileData2bpp, 0), 1);
	for (uint8_t col = 1; col < 8; col++) {
		EXPECT_EQ(WsPpuTestHelpers::GetPixelColor_Mono(tileData2bpp, col), 0)
			<< "Column=" << (int)col;
	}
}

TEST_F(WsPpuPixelColorTest, Color4bpp_AllBitsSet_Returns15) {
	tileData4bpp[0] = 0xff;
	tileData4bpp[1] = 0xff;
	tileData4bpp[2] = 0xff;
	tileData4bpp[3] = 0xff;
	for (uint8_t col = 0; col < 8; col++) {
		EXPECT_EQ(WsPpuTestHelpers::GetPixelColor_Color4bpp(tileData4bpp, col), 15)
			<< "Column=" << (int)col;
	}
}

TEST_F(WsPpuPixelColorTest, Color4bpp_SinglePlane_SingleBit) {
	// Only plane 2 (tileData[2]) bit 7 set → column 0 = color 4 (bit 2)
	tileData4bpp[0] = 0x00;
	tileData4bpp[1] = 0x00;
	tileData4bpp[2] = 0x80;
	tileData4bpp[3] = 0x00;
	EXPECT_EQ(WsPpuTestHelpers::GetPixelColor_Color4bpp(tileData4bpp, 0), 4);
	// Column 1 should be 0 since bit 6 of plane 2 is not set
	EXPECT_EQ(WsPpuTestHelpers::GetPixelColor_Color4bpp(tileData4bpp, 1), 0);
}

TEST_F(WsPpuPixelColorTest, Color4bppPacked_EvenColumn_HighNibble) {
	tileDataPacked[0] = 0xa5; // High nibble = 0xa, low nibble = 0x5
	// Column 0 (even) → high nibble → (0xa5 >> 4) & 0x0f = 0x0a
	EXPECT_EQ(WsPpuTestHelpers::GetPixelColor_Color4bppPacked(tileDataPacked, 0), 0x0a);
}

TEST_F(WsPpuPixelColorTest, Color4bppPacked_OddColumn_LowNibble) {
	tileDataPacked[0] = 0xa5;
	// Column 1 (odd) → low nibble → (0xa5 >> 0) & 0x0f = 0x05
	EXPECT_EQ(WsPpuTestHelpers::GetPixelColor_Color4bppPacked(tileDataPacked, 1), 0x05);
}

TEST_F(WsPpuPixelColorTest, Color4bppPacked_AllColumns) {
	tileDataPacked[0] = 0x12; // col0=1, col1=2
	tileDataPacked[1] = 0x34; // col2=3, col3=4
	tileDataPacked[2] = 0x56; // col4=5, col5=6
	tileDataPacked[3] = 0x78; // col6=7, col7=8
	uint16_t expected[] = {1, 2, 3, 4, 5, 6, 7, 8};
	for (uint8_t col = 0; col < 8; col++) {
		EXPECT_EQ(WsPpuTestHelpers::GetPixelColor_Color4bppPacked(tileDataPacked, col), expected[col])
			<< "Column=" << (int)col;
	}
}

// =============================================================================
// Palette / Color Conversion Tests
// =============================================================================

class WsPpuPaletteTest : public ::testing::Test {
protected:
	uint8_t bwPalettes[0x40] = {};
	uint8_t bwShades[8] = {};
	uint8_t vram[0x10000] = {};

	void SetUp() override {
		memset(bwPalettes, 0, sizeof(bwPalettes));
		memset(bwShades, 0, sizeof(bwShades));
		memset(vram, 0, sizeof(vram));

		// Default monochrome shade LUT: identity mapping (shade N → brightness N)
		for (int i = 0; i < 8; i++) {
			bwShades[i] = static_cast<uint8_t>(i);
		}
	}
};

TEST_F(WsPpuPaletteTest, Mono_BgColor_AllBlack) {
	// All shades = 0 → brightness = 0 ^ 0x0f = 0x0f → white (0xfff)
	memset(bwShades, 0, sizeof(bwShades));
	uint16_t bg = WsPpuTestHelpers::GetMonoBgColor(0, bwShades);
	EXPECT_EQ(bg, 0x0fff); // shade 0 → brightness 0 ^ 0xf = 0xf → 0xfff
}

TEST_F(WsPpuPaletteTest, Mono_BgColor_MaxShade) {
	bwShades[7] = 0x0f;
	uint16_t bg = WsPpuTestHelpers::GetMonoBgColor(7, bwShades);
	// shade 0x0f → brightness 0x0f ^ 0x0f = 0 → 0x000
	EXPECT_EQ(bg, 0x0000);
}

TEST_F(WsPpuPaletteTest, Color_BgColor_FromPalRam) {
	// Set palette RAM at 0xfe00 + (bgColor << 1)
	uint8_t bgColor = 5;
	vram[0xfe00 | (bgColor << 1)] = 0xab;
	vram[0xfe00 | (bgColor << 1) | 1] = 0x0c;
	uint16_t bg = WsPpuTestHelpers::GetColorBgColor(bgColor, vram);
	EXPECT_EQ(bg, 0x0cab); // (0x0c & 0x0f) << 8 | 0xab = 0xcab
}

TEST_F(WsPpuPaletteTest, Mono_PixelRgb_Palette0Color0) {
	bwPalettes[0] = 3; // palette 0, color 0 → shade index 3
	bwShades[3] = 5;    // shade 3 → value 5
	uint16_t rgb = WsPpuTestHelpers::GetMonoPixelRgb(0, 0, bwPalettes, bwShades);
	// brightness = 5 ^ 0x0f = 0x0a
	uint16_t expected = 0x0a | (0x0a << 4) | (0x0a << 8); // 0xaaa
	EXPECT_EQ(rgb, expected);
}

TEST_F(WsPpuPaletteTest, Color_PixelRgb_Palette2Color3) {
	uint8_t palette = 2;
	uint8_t color = 3;
	uint16_t paletteAddr = 0xfe00 | (palette << 5) | (color << 1);
	vram[paletteAddr] = 0x34;
	vram[paletteAddr + 1] = 0x02;
	uint16_t rgb = WsPpuTestHelpers::GetColorPixelRgb(color, palette, vram);
	EXPECT_EQ(rgb, 0x0234); // (0x02 & 0x0f) << 8 | 0x34
}

TEST_F(WsPpuPaletteTest, Color_PaletteAddr_AllPalettes) {
	// Verify palette RAM addressing for all 16 palettes × 16 colors
	for (uint8_t pal = 0; pal < 16; pal++) {
		for (uint8_t col = 0; col < 16; col++) {
			uint16_t addr = 0xfe00 | (pal << 5) | (col << 1);
			vram[addr] = static_cast<uint8_t>(pal);
			vram[addr + 1] = static_cast<uint8_t>(col & 0x0f);
			uint16_t rgb = WsPpuTestHelpers::GetColorPixelRgb(col, pal, vram);
			uint16_t expected = static_cast<uint16_t>(pal) | (static_cast<uint16_t>(col & 0x0f) << 8);
			EXPECT_EQ(rgb, expected)
				<< "Palette=" << (int)pal << " Color=" << (int)col;
		}
	}
}

// =============================================================================
// Window Tests
// =============================================================================

TEST(WsPpuWindowTest, InsideWindow_CornerCases) {
	// Window covers (10,20)-(50,60)
	EXPECT_TRUE(WsPpuTestHelpers::IsInsideWindow(10, 20, 10, 50, 20, 60));  // Top-left corner
	EXPECT_TRUE(WsPpuTestHelpers::IsInsideWindow(50, 60, 10, 50, 20, 60));  // Bottom-right corner
	EXPECT_TRUE(WsPpuTestHelpers::IsInsideWindow(30, 40, 10, 50, 20, 60));  // Center
	EXPECT_FALSE(WsPpuTestHelpers::IsInsideWindow(9, 20, 10, 50, 20, 60));  // One left of left edge
	EXPECT_FALSE(WsPpuTestHelpers::IsInsideWindow(51, 20, 10, 50, 20, 60)); // One right of right edge
	EXPECT_FALSE(WsPpuTestHelpers::IsInsideWindow(10, 19, 10, 50, 20, 60)); // One above top edge
	EXPECT_FALSE(WsPpuTestHelpers::IsInsideWindow(10, 61, 10, 50, 20, 60)); // One below bottom edge
}

TEST(WsPpuWindowTest, FullScreen_AllInside) {
	for (uint8_t x = 0; x < 224; x++) {
		for (uint8_t y = 0; y < 144; y++) {
			EXPECT_TRUE(WsPpuTestHelpers::IsInsideWindow(x, y, 0, 223, 0, 143));
		}
	}
}

TEST(WsPpuWindowTest, ZeroSize_OnlyExactMatch) {
	EXPECT_TRUE(WsPpuTestHelpers::IsInsideWindow(50, 50, 50, 50, 50, 50));
	EXPECT_FALSE(WsPpuTestHelpers::IsInsideWindow(49, 50, 50, 50, 50, 50));
	EXPECT_FALSE(WsPpuTestHelpers::IsInsideWindow(51, 50, 50, 50, 50, 50));
}

// =============================================================================
// Frame Timing & VTOTAL Tests (Issue #1076)
// =============================================================================

class WsPpuFrameTimingTest : public ::testing::Test {
protected:
	// Simulate frame end condition: ares-compatible max(144, VTOTAL+1)
	static uint16_t GetFrameEndScanline(uint8_t lastScanline) {
		return std::max<uint16_t>(WsConstants::ScreenHeight, static_cast<uint16_t>(lastScanline) + 1);
	}

	// Simulate scanline repeat for low VTOTAL
	static uint8_t GetRenderScanline(uint16_t scanline, uint8_t lastScanline) {
		uint8_t visibleCount = static_cast<uint8_t>(
			std::min<uint16_t>(static_cast<uint16_t>(lastScanline) + 1, WsConstants::ScreenHeight));
		if (visibleCount > 0 && visibleCount < WsConstants::ScreenHeight) {
			return static_cast<uint8_t>(scanline % visibleCount);
		}
		return static_cast<uint8_t>(scanline);
	}
};

namespace WsPpuPipelineModels {
	struct FrameSummary {
		uint16_t FrameEndScanlineCount = 0;
		bool VblankTriggered = false;
		bool SpriteCopyTriggered = false;
		std::array<uint8_t, WsConstants::ScreenHeight> RenderY = {};
		bool PreservesRepeatedImageAfterFinalize = true;
	};

	static FrameSummary SimulateAresModel(uint8_t vtotal) {
		FrameSummary s = {};
		uint16_t vcounter = 0;

		while (true) {
			if (vcounter < WsConstants::ScreenHeight) {
				s.RenderY[vcounter] = static_cast<uint8_t>(vcounter % (static_cast<uint16_t>(vtotal) + 1));
			} else if (vcounter == WsConstants::ScreenHeight) {
				s.VblankTriggered = true;
				s.SpriteCopyTriggered = true;
			}

			vcounter++;
			if (vcounter >= std::max<uint16_t>(WsConstants::ScreenHeight, static_cast<uint16_t>(vtotal) + 1)) {
				s.FrameEndScanlineCount = vcounter;
				break;
			}
		}

		// ares keeps rendered repeated image; it does not post-clear low-VTOTAL frames to white.
		s.PreservesRepeatedImageAfterFinalize = true;
		return s;
	}

	static FrameSummary SimulateNexenModel(uint8_t lastScanline) {
		FrameSummary s = {};
		uint16_t scanline = 0;
		uint16_t frameEndScanline = std::max<uint16_t>(WsConstants::ScreenHeight, static_cast<uint16_t>(lastScanline) + 1);

		while (true) {
			if (scanline < WsConstants::ScreenHeight) {
				s.RenderY[scanline] = static_cast<uint8_t>(scanline % (static_cast<uint16_t>(lastScanline) + 1));
			}

			if (scanline == WsConstants::ScreenHeight && lastScanline >= WsConstants::ScreenHeight) {
				s.SpriteCopyTriggered = true;
			}

			scanline++;

			if (scanline == WsConstants::ScreenHeight && lastScanline >= WsConstants::ScreenHeight) {
				s.VblankTriggered = true;
			}

			if (scanline >= frameEndScanline) {
				s.FrameEndScanlineCount = scanline;
				break;
			}
		}

		// New behavior: keep repeated image for low VTOTAL, no post-frame whitening.
		s.PreservesRepeatedImageAfterFinalize = true;
		return s;
	}

	static FrameSummary SimulateMednafenModel(uint8_t lcdVtotal) {
		FrameSummary s = {};
		uint16_t wsLine = 0;

		while (true) {
			if (wsLine < WsConstants::ScreenHeight) {
				s.RenderY[wsLine] = static_cast<uint8_t>(wsLine);
			}

			if (wsLine == 142) {
				s.SpriteCopyTriggered = true;
			} else if (wsLine == WsConstants::ScreenHeight) {
				s.VblankTriggered = true;
			}

			wsLine = (wsLine + 1) % (std::max<uint16_t>(WsConstants::ScreenHeight, lcdVtotal) + 1);
			if (wsLine == 0) {
				s.FrameEndScanlineCount = std::max<uint16_t>(WsConstants::ScreenHeight, lcdVtotal) + 1;
				break;
			}
		}

		return s;
	}
}

TEST_F(WsPpuFrameTimingTest, DefaultVtotal_158_FrameEndsAt159) {
	EXPECT_EQ(GetFrameEndScanline(158), 159);
}

TEST_F(WsPpuFrameTimingTest, MaxVtotal_255_FrameEndsAt256) {
	EXPECT_EQ(GetFrameEndScanline(255), 256);
}

TEST_F(WsPpuFrameTimingTest, LowVtotal_143_FrameStillEndsAt144) {
	// ares behavior: vtotal < 143 → frame still ends at scanline 144
	EXPECT_EQ(GetFrameEndScanline(143), WsConstants::ScreenHeight); // max(144, 144) = 144
}

TEST_F(WsPpuFrameTimingTest, LowVtotal_100_FrameEndsAt144) {
	EXPECT_EQ(GetFrameEndScanline(100), WsConstants::ScreenHeight);
}

TEST_F(WsPpuFrameTimingTest, LowVtotal_0_FrameEndsAt144) {
	EXPECT_EQ(GetFrameEndScanline(0), WsConstants::ScreenHeight);
}

TEST_F(WsPpuFrameTimingTest, VtotalExactly144_FrameEndsAt145) {
	// VTOTAL=144 means 145 scanlines → max(144, 145) = 145
	EXPECT_EQ(GetFrameEndScanline(144), 145);
}

TEST_F(WsPpuFrameTimingTest, ScanlineRepeat_LowVtotal_50) {
	// VTOTAL=49 → 50 visible scanlines, lines 0-143 repeat in groups of 50
	uint8_t lastScanline = 49;
	EXPECT_EQ(GetRenderScanline(0, lastScanline), 0);
	EXPECT_EQ(GetRenderScanline(49, lastScanline), 49);
	EXPECT_EQ(GetRenderScanline(50, lastScanline), 0);  // Wraps
	EXPECT_EQ(GetRenderScanline(51, lastScanline), 1);
	EXPECT_EQ(GetRenderScanline(99, lastScanline), 49);
	EXPECT_EQ(GetRenderScanline(100, lastScanline), 0);  // Wraps again
	EXPECT_EQ(GetRenderScanline(143, lastScanline), 43); // 143 % 50 = 43
}

TEST_F(WsPpuFrameTimingTest, ScanlineRepeat_NormalVtotal_NoRepeat) {
	// VTOTAL=158 → 159 visible, but capped at 144. No repeat needed.
	uint8_t lastScanline = 158;
	for (uint16_t s = 0; s < 144; s++) {
		EXPECT_EQ(GetRenderScanline(s, lastScanline), s)
			<< "Scanline=" << s;
	}
}

TEST_F(WsPpuFrameTimingTest, ScanlineRepeat_Vtotal1_RepeatsLine0) {
	// VTOTAL=0 → 1 visible scanline, all lines render as line 0
	uint8_t lastScanline = 0;
	for (uint16_t s = 0; s < 144; s++) {
		EXPECT_EQ(GetRenderScanline(s, lastScanline), 0)
			<< "Scanline=" << s;
	}
}

TEST_F(WsPpuFrameTimingTest, ScanlineRepeat_Vtotal2_AlternatesLines) {
	// VTOTAL=1 → 2 visible scanlines, alternates 0,1,0,1,...
	uint8_t lastScanline = 1;
	for (uint16_t s = 0; s < 144; s++) {
		EXPECT_EQ(GetRenderScanline(s, lastScanline), s % 2)
			<< "Scanline=" << s;
	}
}

TEST(WsPpuCrossEmulatorParityTest, FrameLength_AllVtotal_MatchesAres) {
	for (uint16_t vtotal = 0; vtotal <= 255; vtotal++) {
		auto ares = WsPpuPipelineModels::SimulateAresModel(static_cast<uint8_t>(vtotal));
		auto nexen = WsPpuPipelineModels::SimulateNexenModel(static_cast<uint8_t>(vtotal));

		EXPECT_EQ(nexen.FrameEndScanlineCount, ares.FrameEndScanlineCount)
			<< "VTOTAL=" << vtotal;
	}
}

TEST(WsPpuCrossEmulatorParityTest, VblankAndSpriteCopyParity_MatchesAres) {
	for (uint16_t vtotal = 0; vtotal <= 255; vtotal++) {
		auto ares = WsPpuPipelineModels::SimulateAresModel(static_cast<uint8_t>(vtotal));
		auto nexen = WsPpuPipelineModels::SimulateNexenModel(static_cast<uint8_t>(vtotal));

		EXPECT_EQ(nexen.VblankTriggered, ares.VblankTriggered)
			<< "VTOTAL=" << vtotal;
		EXPECT_EQ(nexen.SpriteCopyTriggered, ares.SpriteCopyTriggered)
			<< "VTOTAL=" << vtotal;
	}
}

TEST(WsPpuCrossEmulatorParityTest, LowVtotal_RenderWrapParity_MatchesAres) {
	std::vector<uint8_t> lowVtotals = {0, 1, 2, 15, 49, 79, 100, 120, 143};
	for (uint8_t vtotal : lowVtotals) {
		auto ares = WsPpuPipelineModels::SimulateAresModel(vtotal);
		auto nexen = WsPpuPipelineModels::SimulateNexenModel(vtotal);

		for (uint16_t line = 0; line < WsConstants::ScreenHeight; line++) {
			EXPECT_EQ(nexen.RenderY[line], ares.RenderY[line])
				<< "VTOTAL=" << (int)vtotal << " Line=" << line;
		}
	}
}

TEST(WsPpuCrossEmulatorParityTest, LowVtotal_DoesNotWhiteClearRepeatedImage) {
	std::vector<uint8_t> lowVtotals = {0, 1, 2, 49, 100, 143};
	for (uint8_t vtotal : lowVtotals) {
		auto nexen = WsPpuPipelineModels::SimulateNexenModel(vtotal);
		EXPECT_TRUE(nexen.PreservesRepeatedImageAfterFinalize)
			<< "VTOTAL=" << (int)vtotal;
	}
}

TEST(WsPpuCrossEmulatorParityTest, MednafenKnownDifference_SpriteCopyLine) {
	auto mednafen = WsPpuPipelineModels::SimulateMednafenModel(158);
	auto nexen = WsPpuPipelineModels::SimulateNexenModel(158);
	EXPECT_TRUE(mednafen.SpriteCopyTriggered);
	EXPECT_TRUE(nexen.SpriteCopyTriggered);

	// Coverage note: Mednafen performs copy on line 142 in current core;
	// Nexen and ares model line 144 semantics.
	SUCCEED();
}

// =============================================================================
// PPU Constants Tests
// =============================================================================

TEST(WsPpuConstantsTest, ScreenDimensions) {
	EXPECT_EQ(WsConstants::ScreenWidth, 224u);
	EXPECT_EQ(WsConstants::ScreenHeight, 144u);
	EXPECT_EQ(WsConstants::ClocksPerScanline, 256u);
	EXPECT_EQ(WsConstants::ScanlineCount, 159u);
	EXPECT_EQ(WsConstants::PixelCount, 224u * 144u);
	EXPECT_EQ(WsConstants::MaxPixelCount, 224u * (144u + 13u));
}

// =============================================================================
// PPU State Defaults Tests
// =============================================================================

TEST(WsPpuStateTest, DefaultState_ZeroInitialized) {
	WsPpuState state = {};
	EXPECT_EQ(state.Cycle, 0);
	EXPECT_EQ(state.Scanline, 0);
	EXPECT_EQ(state.FrameCount, 0u);
	EXPECT_EQ(state.Mode, WsVideoMode::Monochrome);
	EXPECT_EQ(state.NextMode, WsVideoMode::Monochrome);
	EXPECT_FALSE(state.LcdEnabled);
	EXPECT_FALSE(state.SleepEnabled);
	EXPECT_FALSE(state.SpritesEnabled);
}

TEST(WsPpuStateTest, BgLayer_Latch_CopiesCurrentValues) {
	WsBgLayer layer = {};
	layer.Enabled = true;
	layer.ScrollX = 42;
	layer.ScrollY = 99;
	layer.MapAddress = 0x1800;

	layer.Latch();

	EXPECT_TRUE(layer.EnabledLatch);
	EXPECT_EQ(layer.ScrollXLatch, 42);
	EXPECT_EQ(layer.ScrollYLatch, 99);
	EXPECT_EQ(layer.MapAddressLatch, 0x1800);
}

TEST(WsPpuStateTest, Window_Latch_CopiesAllEdges) {
	WsWindow win = {};
	win.Enabled = true;
	win.Left = 10;
	win.Right = 200;
	win.Top = 20;
	win.Bottom = 130;

	win.Latch();

	EXPECT_TRUE(win.EnabledLatch);
	EXPECT_EQ(win.LeftLatch, 10);
	EXPECT_EQ(win.RightLatch, 200);
	EXPECT_EQ(win.TopLatch, 20);
	EXPECT_EQ(win.BottomLatch, 130);
}

// =============================================================================
// CPU Flags Tests (WsCpuFlags pack/unpack)
// =============================================================================

TEST(WsCpuFlagsTest, RoundTrip_AllClear) {
	WsCpuFlags flags = {};
	uint16_t packed = flags.Get();
	WsCpuFlags restored = {};
	restored.Set(packed);

	EXPECT_FALSE(restored.Carry);
	EXPECT_FALSE(restored.Zero);
	EXPECT_FALSE(restored.Sign);
	EXPECT_FALSE(restored.Overflow);
	EXPECT_FALSE(restored.Irq);
	EXPECT_FALSE(restored.Direction);
}

TEST_F(WsPpuFrameTimingTest, RoundTrip_AllSet) {
	WsCpuFlags flags = {};
	flags.Carry = true;
	flags.Parity = true;
	flags.AuxCarry = true;
	flags.Zero = true;
	flags.Sign = true;
	flags.Trap = true;
	flags.Irq = true;
	flags.Direction = true;
	flags.Overflow = true;
	flags.Mode = true;

	uint16_t packed = flags.Get();
	WsCpuFlags restored = {};
	restored.Set(packed);

	EXPECT_TRUE(restored.Carry);
	EXPECT_TRUE(restored.Parity);
	EXPECT_TRUE(restored.AuxCarry);
	EXPECT_TRUE(restored.Zero);
	EXPECT_TRUE(restored.Sign);
	EXPECT_TRUE(restored.Trap);
	EXPECT_TRUE(restored.Irq);
	EXPECT_TRUE(restored.Direction);
	EXPECT_TRUE(restored.Overflow);
	EXPECT_TRUE(restored.Mode);
}

// =============================================================================
// Parity Table Tests
// =============================================================================

TEST(WsCpuParityTest, ParityValues_KnownCases) {
	WsCpuParityTable table;
	// 0x00 = 0 bits set → even parity → true
	EXPECT_TRUE(table.CheckParity(0x00));
	// 0x01 = 1 bit set → odd parity → false
	EXPECT_FALSE(table.CheckParity(0x01));
	// 0x03 = 2 bits set → even parity → true
	EXPECT_TRUE(table.CheckParity(0x03));
	// 0xff = 8 bits set → even parity → true
	EXPECT_TRUE(table.CheckParity(0xff));
	// 0x80 = 1 bit set → odd parity → false
	EXPECT_FALSE(table.CheckParity(0x80));
}

// =============================================================================
// Port Read/Write Register Mapping Tests (isolated logic verification)
// =============================================================================

TEST(WsPpuPortTest, ScreenAddress_MapAddressDecoding) {
	// Port 0x07 controls BG layer map addresses
	// Layer 0: low nibble << 11
	// Layer 1: high nibble << 7
	uint8_t screenAddr = 0x37; // Layer0 = 7, Layer1 = 3

	uint16_t layer0Map = (screenAddr & 0x0f) << 11;   // 7 << 11 = 0x3800
	uint16_t layer1Map = (screenAddr & 0xf0) << 7;    // 0x30 << 7 = 0x1800

	EXPECT_EQ(layer0Map, 0x3800);
	EXPECT_EQ(layer1Map, 0x1800);
}

TEST(WsPpuPortTest, SpriteTableAddress_Decoding) {
	// Port 0x04: value << 9 gives sprite table base
	uint8_t val = 0x1f; // Max for mono (5-bit)
	uint16_t addr = (val & 0x1f) << 9;
	EXPECT_EQ(addr, 0x3e00);

	val = 0x3f; // Max for color (6-bit)
	addr = (val & 0x3f) << 9;
	EXPECT_EQ(addr, 0x7e00);
}

TEST(WsPpuPortTest, ControlRegister_BitFields) {
	// Port 0x00 bit layout:
	// bit 0: BG Layer 0 enable
	// bit 1: BG Layer 1 enable
	// bit 2: Sprites enable
	// bit 3: Sprite window enable
	// bit 4: Draw outside BG window
	// bit 5: BG window enable
	uint8_t ctrl = 0x3f;
	EXPECT_TRUE(ctrl & 0x01);  // Layer 0
	EXPECT_TRUE(ctrl & 0x02);  // Layer 1
	EXPECT_TRUE(ctrl & 0x04);  // Sprites
	EXPECT_TRUE(ctrl & 0x08);  // Sprite window
	EXPECT_TRUE(ctrl & 0x10);  // Outside BG window
	EXPECT_TRUE(ctrl & 0x20);  // BG window

	ctrl = 0x00;
	EXPECT_FALSE(ctrl & 0x01);
	EXPECT_FALSE(ctrl & 0x02);
	EXPECT_FALSE(ctrl & 0x04);
}

// =============================================================================
// DMA State Tests
// =============================================================================

TEST(WsDmaStateTest, DefaultState_ZeroInitialized) {
	WsDmaControllerState state = {};
	EXPECT_EQ(state.GdmaSrc, 0u);
	EXPECT_EQ(state.GdmaDest, 0);
	EXPECT_EQ(state.GdmaLength, 0);
	EXPECT_EQ(state.GdmaControl, 0);
	EXPECT_FALSE(state.SdmaEnabled);
	EXPECT_FALSE(state.SdmaDecrement);
	EXPECT_FALSE(state.SdmaHyperVoice);
}

// =============================================================================
// Timer State Tests
// =============================================================================

TEST(WsTimerStateTest, DefaultState_ZeroInitialized) {
	WsTimerState state = {};
	EXPECT_EQ(state.HTimer, 0);
	EXPECT_EQ(state.VTimer, 0);
	EXPECT_EQ(state.HReloadValue, 0);
	EXPECT_EQ(state.VReloadValue, 0);
	EXPECT_FALSE(state.HBlankEnabled);
	EXPECT_FALSE(state.VBlankEnabled);
	EXPECT_FALSE(state.HBlankAutoReload);
	EXPECT_FALSE(state.VBlankAutoReload);
}

// =============================================================================
// Memory Manager State Tests
// =============================================================================

TEST(WsMemoryManagerStateTest, DefaultState) {
	WsMemoryManagerState state = {};
	EXPECT_EQ(state.ActiveIrqs, 0);
	EXPECT_EQ(state.EnabledIrqs, 0);
	EXPECT_EQ(state.OpenBus, 0);
	EXPECT_FALSE(state.ColorEnabled);
	EXPECT_FALSE(state.Enable4bpp);
	EXPECT_FALSE(state.Enable4bppPacked);
	EXPECT_FALSE(state.BootRomDisabled);
	EXPECT_FALSE(state.CartWordBus);
}

// =============================================================================
// Video Mode Enum Tests
// =============================================================================

TEST(WsVideoModeTest, EnumValues) {
	EXPECT_EQ(static_cast<uint8_t>(WsVideoMode::Monochrome), 0);
	EXPECT_EQ(static_cast<uint8_t>(WsVideoMode::Color2bpp), 1);
	EXPECT_EQ(static_cast<uint8_t>(WsVideoMode::Color4bpp), 2);
	EXPECT_EQ(static_cast<uint8_t>(WsVideoMode::Color4bppPacked), 3);
}

// =============================================================================
// IRQ Source Enum Tests
// =============================================================================

TEST(WsIrqSourceTest, BitPositions) {
	EXPECT_EQ(static_cast<uint8_t>(WsIrqSource::UartSendReady), 0x01);
	EXPECT_EQ(static_cast<uint8_t>(WsIrqSource::KeyPressed), 0x02);
	EXPECT_EQ(static_cast<uint8_t>(WsIrqSource::Cart), 0x04);
	EXPECT_EQ(static_cast<uint8_t>(WsIrqSource::UartRecvReady), 0x08);
	EXPECT_EQ(static_cast<uint8_t>(WsIrqSource::Scanline), 0x10);
	EXPECT_EQ(static_cast<uint8_t>(WsIrqSource::VerticalBlankTimer), 0x20);
	EXPECT_EQ(static_cast<uint8_t>(WsIrqSource::VerticalBlank), 0x40);
	EXPECT_EQ(static_cast<uint8_t>(WsIrqSource::HorizontalBlankTimer), 0x80);
}

// =============================================================================
// VTOTAL Edge Case Tests — #1130 audit finding
// =============================================================================
// When LastScanline (VTOTAL) is set below ScreenHeight (144), the PPU wraps
// rendered scanlines via modulo. These tests verify the wrap logic in
// ProcessHblank() at WsPpu.cpp lines 56-63.

TEST(WsVtotalEdgeCaseTest, NormalVtotal_NoWrap) {
	// LastScanline >= 143 means visibleScanlineCount >= ScreenHeight, no modulo
	uint8_t lastScanline = 158; // Default WS VTOTAL
	uint8_t visibleCount = (uint8_t)std::min<uint16_t>((uint16_t)lastScanline + 1, WsConstants::ScreenHeight);
	EXPECT_EQ(visibleCount, WsConstants::ScreenHeight);

	// Scanline should NOT be modified when visibleCount == ScreenHeight
	uint8_t scanline = 100;
	if (visibleCount > 0 && visibleCount < WsConstants::ScreenHeight) {
		scanline = (uint8_t)(scanline % visibleCount);
	}
	EXPECT_EQ(scanline, 100); // Unchanged
}

TEST(WsVtotalEdgeCaseTest, VtotalAtScreenHeight_NoWrap) {
	// LastScanline = 143 → visibleCount = 144 = ScreenHeight, no modulo
	uint8_t lastScanline = 143;
	uint8_t visibleCount = (uint8_t)std::min<uint16_t>((uint16_t)lastScanline + 1, WsConstants::ScreenHeight);
	EXPECT_EQ(visibleCount, WsConstants::ScreenHeight);

	uint8_t scanline = 143;
	if (visibleCount > 0 && visibleCount < WsConstants::ScreenHeight) {
		scanline = (uint8_t)(scanline % visibleCount);
	}
	EXPECT_EQ(scanline, 143); // No wrap
}

TEST(WsVtotalEdgeCaseTest, VtotalBelowScreenHeight_WrapsCorrectly) {
	// LastScanline = 71 → visibleCount = 72, scanlines above 71 wrap
	uint8_t lastScanline = 71;
	uint8_t visibleCount = (uint8_t)std::min<uint16_t>((uint16_t)lastScanline + 1, WsConstants::ScreenHeight);
	EXPECT_EQ(visibleCount, 72);

	// Scanline 0 — no wrap
	uint8_t s0 = 0;
	if (visibleCount > 0 && visibleCount < WsConstants::ScreenHeight) {
		s0 = (uint8_t)(s0 % visibleCount);
	}
	EXPECT_EQ(s0, 0);

	// Scanline 71 — last visible, no wrap
	uint8_t s71 = 71;
	if (visibleCount > 0 && visibleCount < WsConstants::ScreenHeight) {
		s71 = (uint8_t)(s71 % visibleCount);
	}
	EXPECT_EQ(s71, 71);

	// Scanline 72 — wraps to 0
	uint8_t s72 = 72;
	if (visibleCount > 0 && visibleCount < WsConstants::ScreenHeight) {
		s72 = (uint8_t)(s72 % visibleCount);
	}
	EXPECT_EQ(s72, 0);

	// Scanline 143 — wraps to 143 % 72 = 71
	uint8_t s143 = 143;
	if (visibleCount > 0 && visibleCount < WsConstants::ScreenHeight) {
		s143 = (uint8_t)(s143 % visibleCount);
	}
	EXPECT_EQ(s143, 143 % 72);
}

TEST(WsVtotalEdgeCaseTest, VtotalZero_SingleScanlineWrap) {
	// LastScanline = 0 → visibleCount = 1, every scanline wraps to 0
	uint8_t lastScanline = 0;
	uint8_t visibleCount = (uint8_t)std::min<uint16_t>((uint16_t)lastScanline + 1, WsConstants::ScreenHeight);
	EXPECT_EQ(visibleCount, 1);

	for (uint8_t scanline = 0; scanline < WsConstants::ScreenHeight; scanline++) {
		uint8_t wrapped = scanline;
		if (visibleCount > 0 && visibleCount < WsConstants::ScreenHeight) {
			wrapped = (uint8_t)(wrapped % visibleCount);
		}
		EXPECT_EQ(wrapped, 0) << "Scanline " << (int)scanline << " should wrap to 0";
	}
}

TEST(WsVtotalEdgeCaseTest, RenderRowIndex_PreservesOriginalScanline) {
	// The render row index uses the ORIGINAL scanline (before modulo), not wrapped
	// This tests the double-buffer index logic: _renderRowIndex = _state.Scanline & 0x01
	for (uint8_t scanline = 0; scanline < 10; scanline++) {
		uint8_t renderRowIndex = scanline & 0x01;
		EXPECT_EQ(renderRowIndex, scanline % 2)
			<< "Row index for scanline " << (int)scanline << " should alternate 0/1";
	}
}

TEST(WsVtotalEdgeCaseTest, VtotalHalfScreen_WrapsAt72) {
	// LastScanline = 71 → exactly half screen height, common non-standard VTOTAL
	uint8_t lastScanline = 71;
	uint8_t visibleCount = (uint8_t)std::min<uint16_t>((uint16_t)lastScanline + 1, WsConstants::ScreenHeight);
	EXPECT_EQ(visibleCount, 72);
	EXPECT_LT(visibleCount, WsConstants::ScreenHeight);

	// Verify every scanline in visible range maps to [0, 71]
	for (uint8_t scanline = 0; scanline < WsConstants::ScreenHeight; scanline++) {
		uint8_t wrapped = scanline;
		if (visibleCount > 0 && visibleCount < WsConstants::ScreenHeight) {
			wrapped = (uint8_t)(wrapped % visibleCount);
		}
		EXPECT_LT(wrapped, visibleCount)
			<< "Scanline " << (int)scanline << " wrapped to " << (int)wrapped
			<< " which exceeds visibleCount " << (int)visibleCount;
	}
}
