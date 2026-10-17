// =================================================================================================
// PPU Timing Tests
// Tests for PPU timing accuracy across NES, SNES, and Game Boy
// =================================================================================================

#include "pch.h"
#include <gtest/gtest.h>
#include <cstdint>
#include <algorithm>

// =================================================================================================
// NES PPU Timing Constants
// =================================================================================================
// NES PPU: 262 scanlines (NTSC), 341 cycles per scanline
// - Scanlines 0-239: Visible
// - Scanline 240: Post-render
// - Scanline 241: VBlank begins (NMI triggered)
// - Scanlines 242-260: VBlank
// - Scanline 261: Pre-render

namespace NesPpuTiming {
	constexpr int32_t CYCLES_PER_SCANLINE = 341;
	constexpr int32_t VISIBLE_SCANLINES = 240;
	constexpr int32_t POST_RENDER_SCANLINE = 240;
	constexpr int32_t VBLANK_START_SCANLINE = 241;
	constexpr int32_t PRE_RENDER_SCANLINE = 261;
	constexpr int32_t TOTAL_SCANLINES_NTSC = 262;
	constexpr int32_t TOTAL_SCANLINES_PAL = 312;

	// PPU register timing
	constexpr int32_t PPUSTATUS_READ_CYCLE = 1;  // Cycle when VBlank flag can be read
	constexpr int32_t NMI_TRIGGER_CYCLE = 1;      // Cycle when NMI is triggered

	// Sprite evaluation timing
	constexpr int32_t SPRITE_EVAL_START_CYCLE = 65;
	constexpr int32_t SPRITE_EVAL_END_CYCLE = 256;
	constexpr int32_t SPRITE_FETCH_START_CYCLE = 257;
	constexpr int32_t SPRITE_FETCH_END_CYCLE = 320;

	// Background fetch timing
	constexpr int32_t BG_FETCH_START_CYCLE = 1;
	constexpr int32_t BG_FETCH_END_CYCLE = 256;

	// Helper functions
	constexpr int32_t CycleToPixel(int32_t cycle) {
		// Cycles 1-256 correspond to pixels 0-255
		return (cycle > 0 && cycle <= 256) ? cycle - 1 : -1;
	}

	constexpr bool IsVisibleScanline(int32_t scanline) {
		return scanline >= 0 && scanline < VISIBLE_SCANLINES;
	}

	constexpr bool IsVBlank(int32_t scanline) {
		return scanline >= VBLANK_START_SCANLINE && scanline <= PRE_RENDER_SCANLINE - 1;
	}

	constexpr bool IsPreRenderScanline(int32_t scanline) {
		return scanline == PRE_RENDER_SCANLINE;
	}

	constexpr bool IsSpriteEvalActive(int32_t cycle) {
		return cycle >= SPRITE_EVAL_START_CYCLE && cycle <= SPRITE_EVAL_END_CYCLE;
	}

	// Odd frame skip: On odd frames, the pre-render scanline is 1 cycle shorter
	constexpr int32_t GetPreRenderScanlineCycles(bool oddFrame, bool renderingEnabled) {
		if (oddFrame && renderingEnabled) {
			return CYCLES_PER_SCANLINE - 1;  // 340 cycles on odd frames
		}
		return CYCLES_PER_SCANLINE;
	}

	// Calculate total frame cycles
	constexpr int32_t GetFrameCycles(bool oddFrame, bool renderingEnabled) {
		int32_t cycles = (TOTAL_SCANLINES_NTSC - 1) * CYCLES_PER_SCANLINE;
		cycles += GetPreRenderScanlineCycles(oddFrame, renderingEnabled);
		return cycles;
	}
} // namespace NesPpuTiming

// =================================================================================================
// SNES PPU Timing Constants
// =================================================================================================
// SNES PPU: 262 scanlines (NTSC), 340 H-clocks per scanline (1364 master clocks / 4)
// - Scanlines 1-224: Visible (or 1-239 in overscan mode)
// - Scanline 225/240: Post-render
// - Scanlines 225-261: VBlank (or 240-261 in overscan mode)
// - Scanline 0: Pre-render

namespace SnesPpuTiming {
	constexpr int32_t MASTER_CYCLES_PER_DOT = 4;
	constexpr int32_t DOTS_PER_SCANLINE = 340;
	constexpr int32_t H_CLOCKS_PER_SCANLINE = 1364;

	constexpr int32_t VISIBLE_SCANLINES_NORMAL = 224;
	constexpr int32_t VISIBLE_SCANLINES_OVERSCAN = 239;

	constexpr int32_t VBLANK_START_NORMAL = 225;
	constexpr int32_t VBLANK_START_OVERSCAN = 240;
	constexpr int32_t VBLANK_END_NTSC = 261;
	constexpr int32_t VBLANK_END_PAL = 311;

	constexpr int32_t TOTAL_SCANLINES_NTSC = 262;
	constexpr int32_t TOTAL_SCANLINES_PAL = 312;

	// HDMA timing
	constexpr int32_t HDMA_INIT_SCANLINE = 0;
	constexpr int32_t HDMA_TRIGGER_H_POS = 278;  // HDMA triggers at H=278

	// H-blank timing
	constexpr int32_t HBLANK_START_H = 274;  // Approximately
	constexpr int32_t HBLANK_END_H = 1;

	// Helper functions
	constexpr bool IsVisibleScanline(int32_t scanline, bool overscanMode) {
		int32_t maxVisible = overscanMode ? VISIBLE_SCANLINES_OVERSCAN : VISIBLE_SCANLINES_NORMAL;
		return scanline >= 1 && scanline <= maxVisible;
	}

	constexpr bool IsVBlank(int32_t scanline, bool overscanMode, bool isPal) {
		int32_t vblankStart = overscanMode ? VBLANK_START_OVERSCAN : VBLANK_START_NORMAL;
		int32_t vblankEnd = isPal ? VBLANK_END_PAL : VBLANK_END_NTSC;
		return scanline >= vblankStart && scanline <= vblankEnd;
	}

	constexpr int32_t GetVBlankStartScanline(bool overscanMode) {
		return overscanMode ? VBLANK_START_OVERSCAN : VBLANK_START_NORMAL;
	}

	// Calculate ROM offset (dots to master cycles)
	constexpr int32_t DotsToMasterCycles(int32_t dots) {
		return dots * MASTER_CYCLES_PER_DOT;
	}
} // namespace SnesPpuTiming

// =================================================================================================
// Game Boy PPU Timing Constants
// =================================================================================================
// GB PPU: 154 scanlines, 456 dots per scanline
// - Scanlines 0-143: Visible
// - Scanlines 144-153: VBlank
// Each visible scanline: Mode 2 (OAM) -> Mode 3 (Drawing) -> Mode 0 (HBlank)

namespace GbPpuTiming {
	constexpr int32_t DOTS_PER_SCANLINE = 456;
	constexpr int32_t VISIBLE_SCANLINES = 144;
	constexpr int32_t VBLANK_SCANLINES = 10;
	constexpr int32_t TOTAL_SCANLINES = 154;

	// Mode durations (in dots)
	constexpr int32_t MODE_2_DURATION = 80;    // OAM search
	constexpr int32_t MODE_3_MIN_DURATION = 172;  // Minimum drawing time
	constexpr int32_t MODE_3_MAX_DURATION = 289;  // Maximum drawing time (with sprites)

	// Mode timing within a scanline
	constexpr int32_t MODE_2_START = 0;
	constexpr int32_t MODE_2_END = 79;
	constexpr int32_t MODE_3_START = 80;
	// MODE_3_END varies based on sprites
	// MODE_0 (HBlank) fills remainder of scanline

	// VBlank timing
	constexpr int32_t VBLANK_START_SCANLINE = 144;
	constexpr int32_t VBLANK_END_SCANLINE = 153;

	// LY register timing
	constexpr int32_t LY_INCREMENT_DOT = 0;  // LY increments at dot 0 of each scanline

	// STAT register modes
	constexpr uint8_t MODE_HBLANK = 0;
	constexpr uint8_t MODE_VBLANK = 1;
	constexpr uint8_t MODE_OAM = 2;
	constexpr uint8_t MODE_DRAWING = 3;

	// Frame timing
	constexpr int32_t DOTS_PER_FRAME = DOTS_PER_SCANLINE * TOTAL_SCANLINES;  // 70224

	// Helper functions
	constexpr uint8_t GetModeForDot(int32_t dot, int32_t scanline) {
		if (scanline >= VBLANK_START_SCANLINE) {
			return MODE_VBLANK;
		}
		if (dot < MODE_2_DURATION) {
			return MODE_OAM;
		}
		if (dot < MODE_2_DURATION + MODE_3_MIN_DURATION) {
			return MODE_DRAWING;  // Approximate - actual duration varies
		}
		return MODE_HBLANK;
	}

	constexpr bool IsVisibleScanline(int32_t scanline) {
		return scanline >= 0 && scanline < VISIBLE_SCANLINES;
	}

	constexpr bool IsVBlank(int32_t scanline) {
		return scanline >= VBLANK_START_SCANLINE && scanline <= VBLANK_END_SCANLINE;
	}

	// Mode 3 duration calculation based on sprites
	// Each sprite adds approximately 6-11 dots depending on position
	constexpr int32_t EstimateMode3Duration(int32_t spriteCount) {
		// Base + sprite penalty (approximately 6 dots per sprite on average)
		return MODE_3_MIN_DURATION + (spriteCount * 6);
	}
} // namespace GbPpuTiming

// =================================================================================================
// NES PPU Timing Tests
// =================================================================================================

class NesPpuTimingTest : public ::testing::Test {};

TEST_F(NesPpuTimingTest, ScanlineCount_NTSC) {
	EXPECT_EQ(NesPpuTiming::TOTAL_SCANLINES_NTSC, 262);
}

TEST_F(NesPpuTimingTest, ScanlineCount_PAL) {
	EXPECT_EQ(NesPpuTiming::TOTAL_SCANLINES_PAL, 312);
}

TEST_F(NesPpuTimingTest, CyclesPerScanline) {
	EXPECT_EQ(NesPpuTiming::CYCLES_PER_SCANLINE, 341);
}

TEST_F(NesPpuTimingTest, VisibleScanlines) {
	// Scanlines 0-239 are visible
	for (int32_t scanline = 0; scanline < 240; ++scanline) {
		EXPECT_TRUE(NesPpuTiming::IsVisibleScanline(scanline)) << "Scanline " << scanline;
	}
	EXPECT_FALSE(NesPpuTiming::IsVisibleScanline(240));  // Post-render
	EXPECT_FALSE(NesPpuTiming::IsVisibleScanline(241));  // VBlank
}

TEST_F(NesPpuTimingTest, VBlankRange) {
	// VBlank is scanlines 241-260
	EXPECT_FALSE(NesPpuTiming::IsVBlank(240));  // Post-render, not VBlank
	for (int32_t scanline = 241; scanline < 261; ++scanline) {
		EXPECT_TRUE(NesPpuTiming::IsVBlank(scanline)) << "Scanline " << scanline;
	}
	EXPECT_FALSE(NesPpuTiming::IsVBlank(261));  // Pre-render
}

TEST_F(NesPpuTimingTest, PreRenderScanline) {
	EXPECT_FALSE(NesPpuTiming::IsPreRenderScanline(260));
	EXPECT_TRUE(NesPpuTiming::IsPreRenderScanline(261));
	EXPECT_FALSE(NesPpuTiming::IsPreRenderScanline(0));
}

TEST_F(NesPpuTimingTest, CycleToPixel_ValidRange) {
	// Cycles 1-256 map to pixels 0-255
	EXPECT_EQ(NesPpuTiming::CycleToPixel(1), 0);
	EXPECT_EQ(NesPpuTiming::CycleToPixel(128), 127);
	EXPECT_EQ(NesPpuTiming::CycleToPixel(256), 255);
}

TEST_F(NesPpuTimingTest, CycleToPixel_InvalidRange) {
	EXPECT_EQ(NesPpuTiming::CycleToPixel(0), -1);
	EXPECT_EQ(NesPpuTiming::CycleToPixel(257), -1);
	EXPECT_EQ(NesPpuTiming::CycleToPixel(340), -1);
}

TEST_F(NesPpuTimingTest, SpriteEvalTiming) {
	EXPECT_FALSE(NesPpuTiming::IsSpriteEvalActive(64));
	EXPECT_TRUE(NesPpuTiming::IsSpriteEvalActive(65));
	EXPECT_TRUE(NesPpuTiming::IsSpriteEvalActive(256));
	EXPECT_FALSE(NesPpuTiming::IsSpriteEvalActive(257));
}

TEST_F(NesPpuTimingTest, OddFrameSkip_RenderingEnabled) {
	// Odd frames with rendering enabled are 1 cycle shorter
	EXPECT_EQ(NesPpuTiming::GetPreRenderScanlineCycles(true, true), 340);
	EXPECT_EQ(NesPpuTiming::GetPreRenderScanlineCycles(false, true), 341);
}

TEST_F(NesPpuTimingTest, OddFrameSkip_RenderingDisabled) {
	// With rendering disabled, no skip occurs
	EXPECT_EQ(NesPpuTiming::GetPreRenderScanlineCycles(true, false), 341);
	EXPECT_EQ(NesPpuTiming::GetPreRenderScanlineCycles(false, false), 341);
}

TEST_F(NesPpuTimingTest, FrameCycles_EvenFrame) {
	// Even frame: 262 * 341 = 89342 cycles
	int32_t expected = 262 * 341;
	EXPECT_EQ(NesPpuTiming::GetFrameCycles(false, true), expected);
}

TEST_F(NesPpuTimingTest, FrameCycles_OddFrame) {
	// Odd frame with rendering: 89341 cycles (1 less)
	int32_t expected = 262 * 341 - 1;
	EXPECT_EQ(NesPpuTiming::GetFrameCycles(true, true), expected);
}

// =================================================================================================
// SNES PPU Timing Tests
// =================================================================================================

class SnesPpuTimingTest : public ::testing::Test {};

TEST_F(SnesPpuTimingTest, ScanlineCount_NTSC) {
	EXPECT_EQ(SnesPpuTiming::TOTAL_SCANLINES_NTSC, 262);
}

TEST_F(SnesPpuTimingTest, ScanlineCount_PAL) {
	EXPECT_EQ(SnesPpuTiming::TOTAL_SCANLINES_PAL, 312);
}

TEST_F(SnesPpuTimingTest, DotsPerScanline) {
	EXPECT_EQ(SnesPpuTiming::DOTS_PER_SCANLINE, 340);
}

TEST_F(SnesPpuTimingTest, HClocksPerScanline) {
	EXPECT_EQ(SnesPpuTiming::H_CLOCKS_PER_SCANLINE, 1364);
}

TEST_F(SnesPpuTimingTest, VisibleScanlines_NormalMode) {
	// Scanlines 1-224 are visible in normal mode
	EXPECT_FALSE(SnesPpuTiming::IsVisibleScanline(0, false));  // Pre-render
	for (int32_t scanline = 1; scanline <= 224; ++scanline) {
		EXPECT_TRUE(SnesPpuTiming::IsVisibleScanline(scanline, false)) << "Scanline " << scanline;
	}
	EXPECT_FALSE(SnesPpuTiming::IsVisibleScanline(225, false));  // VBlank
}

TEST_F(SnesPpuTimingTest, VisibleScanlines_OverscanMode) {
	// Scanlines 1-239 are visible in overscan mode
	for (int32_t scanline = 1; scanline <= 239; ++scanline) {
		EXPECT_TRUE(SnesPpuTiming::IsVisibleScanline(scanline, true)) << "Scanline " << scanline;
	}
	EXPECT_FALSE(SnesPpuTiming::IsVisibleScanline(240, true));  // VBlank
}

TEST_F(SnesPpuTimingTest, VBlankStart_NormalMode) {
	EXPECT_EQ(SnesPpuTiming::GetVBlankStartScanline(false), 225);
}

TEST_F(SnesPpuTimingTest, VBlankStart_OverscanMode) {
	EXPECT_EQ(SnesPpuTiming::GetVBlankStartScanline(true), 240);
}

TEST_F(SnesPpuTimingTest, VBlank_NormalMode_NTSC) {
	EXPECT_FALSE(SnesPpuTiming::IsVBlank(224, false, false));
	for (int32_t scanline = 225; scanline <= 261; ++scanline) {
		EXPECT_TRUE(SnesPpuTiming::IsVBlank(scanline, false, false)) << "Scanline " << scanline;
	}
}

TEST_F(SnesPpuTimingTest, DotsToMasterCycles) {
	EXPECT_EQ(SnesPpuTiming::DotsToMasterCycles(1), 4);
	EXPECT_EQ(SnesPpuTiming::DotsToMasterCycles(340), 1360);
}

// =================================================================================================
// Game Boy PPU Timing Tests
// =================================================================================================

class GbPpuTimingTest : public ::testing::Test {};

TEST_F(GbPpuTimingTest, ScanlineCount) {
	EXPECT_EQ(GbPpuTiming::TOTAL_SCANLINES, 154);
}

TEST_F(GbPpuTimingTest, VisibleScanlineCount) {
	EXPECT_EQ(GbPpuTiming::VISIBLE_SCANLINES, 144);
}

TEST_F(GbPpuTimingTest, VBlankScanlineCount) {
	EXPECT_EQ(GbPpuTiming::VBLANK_SCANLINES, 10);
}

TEST_F(GbPpuTimingTest, DotsPerScanline) {
	EXPECT_EQ(GbPpuTiming::DOTS_PER_SCANLINE, 456);
}

TEST_F(GbPpuTimingTest, DotsPerFrame) {
	EXPECT_EQ(GbPpuTiming::DOTS_PER_FRAME, 70224);
}

TEST_F(GbPpuTimingTest, VisibleScanlines) {
	for (int32_t scanline = 0; scanline < 144; ++scanline) {
		EXPECT_TRUE(GbPpuTiming::IsVisibleScanline(scanline)) << "Scanline " << scanline;
	}
	EXPECT_FALSE(GbPpuTiming::IsVisibleScanline(144));
	EXPECT_FALSE(GbPpuTiming::IsVisibleScanline(153));
}

TEST_F(GbPpuTimingTest, VBlankRange) {
	EXPECT_FALSE(GbPpuTiming::IsVBlank(143));
	for (int32_t scanline = 144; scanline <= 153; ++scanline) {
		EXPECT_TRUE(GbPpuTiming::IsVBlank(scanline)) << "Scanline " << scanline;
	}
}

TEST_F(GbPpuTimingTest, Mode2_Duration) {
	EXPECT_EQ(GbPpuTiming::MODE_2_DURATION, 80);
}

TEST_F(GbPpuTimingTest, Mode3_MinDuration) {
	EXPECT_EQ(GbPpuTiming::MODE_3_MIN_DURATION, 172);
}

TEST_F(GbPpuTimingTest, Mode3_MaxDuration) {
	EXPECT_EQ(GbPpuTiming::MODE_3_MAX_DURATION, 289);
}

TEST_F(GbPpuTimingTest, GetModeForDot_OamSearch) {
	// Mode 2 (OAM) is dots 0-79
	EXPECT_EQ(GbPpuTiming::GetModeForDot(0, 0), GbPpuTiming::MODE_OAM);
	EXPECT_EQ(GbPpuTiming::GetModeForDot(79, 0), GbPpuTiming::MODE_OAM);
}

TEST_F(GbPpuTimingTest, GetModeForDot_Drawing) {
	// Mode 3 (Drawing) starts at dot 80
	EXPECT_EQ(GbPpuTiming::GetModeForDot(80, 0), GbPpuTiming::MODE_DRAWING);
	EXPECT_EQ(GbPpuTiming::GetModeForDot(200, 0), GbPpuTiming::MODE_DRAWING);
}

TEST_F(GbPpuTimingTest, GetModeForDot_HBlank) {
	// Mode 0 (HBlank) after Mode 3 ends (approximately dot 252+)
	EXPECT_EQ(GbPpuTiming::GetModeForDot(300, 0), GbPpuTiming::MODE_HBLANK);
	EXPECT_EQ(GbPpuTiming::GetModeForDot(455, 0), GbPpuTiming::MODE_HBLANK);
}

TEST_F(GbPpuTimingTest, GetModeForDot_VBlank) {
	// Mode 1 (VBlank) for all dots on scanlines 144+
	EXPECT_EQ(GbPpuTiming::GetModeForDot(0, 144), GbPpuTiming::MODE_VBLANK);
	EXPECT_EQ(GbPpuTiming::GetModeForDot(200, 144), GbPpuTiming::MODE_VBLANK);
	EXPECT_EQ(GbPpuTiming::GetModeForDot(0, 153), GbPpuTiming::MODE_VBLANK);
}

TEST_F(GbPpuTimingTest, Mode3_Duration_NoSprites) {
	EXPECT_EQ(GbPpuTiming::EstimateMode3Duration(0), 172);
}

TEST_F(GbPpuTimingTest, Mode3_Duration_WithSprites) {
	// 10 sprites adds approximately 60 dots
	EXPECT_EQ(GbPpuTiming::EstimateMode3Duration(10), 232);
}

TEST_F(GbPpuTimingTest, Mode3_Duration_MaxSprites) {
	// Max 10 sprites per line (but can overlap)
	int32_t duration = GbPpuTiming::EstimateMode3Duration(10);
	EXPECT_GE(duration, GbPpuTiming::MODE_3_MIN_DURATION);
	EXPECT_LE(duration, GbPpuTiming::MODE_3_MAX_DURATION);
}

// =================================================================================================
// Cross-Platform Timing Comparison Tests
// =================================================================================================

class CrossPlatformPpuTimingTest : public ::testing::Test {};

TEST_F(CrossPlatformPpuTimingTest, NTSC_ScanlineCount_SameForNesAndSnes) {
	EXPECT_EQ(NesPpuTiming::TOTAL_SCANLINES_NTSC, SnesPpuTiming::TOTAL_SCANLINES_NTSC);
}

TEST_F(CrossPlatformPpuTimingTest, PAL_ScanlineCount_SameForNesAndSnes) {
	EXPECT_EQ(NesPpuTiming::TOTAL_SCANLINES_PAL, SnesPpuTiming::TOTAL_SCANLINES_PAL);
}

TEST_F(CrossPlatformPpuTimingTest, GB_HasFewerScanlines) {
	EXPECT_LT(GbPpuTiming::TOTAL_SCANLINES, NesPpuTiming::TOTAL_SCANLINES_NTSC);
	EXPECT_LT(GbPpuTiming::TOTAL_SCANLINES, SnesPpuTiming::TOTAL_SCANLINES_NTSC);
}

TEST_F(CrossPlatformPpuTimingTest, GB_HasFewerVisibleLines) {
	EXPECT_LT(GbPpuTiming::VISIBLE_SCANLINES, NesPpuTiming::VISIBLE_SCANLINES);
	EXPECT_LT(GbPpuTiming::VISIBLE_SCANLINES, SnesPpuTiming::VISIBLE_SCANLINES_NORMAL);
}

// =================================================================================================
// Parameterized Tests for NES Scanline Types
// =================================================================================================

struct NesScanlineTypeParams {
	int32_t scanline;
	bool isVisible;
	bool isVBlank;
	bool isPreRender;
	const char* description;
};

class NesScanlineTypeTest : public ::testing::TestWithParam<NesScanlineTypeParams> {};

TEST_P(NesScanlineTypeTest, ScanlineType) {
	const auto& params = GetParam();
	EXPECT_EQ(NesPpuTiming::IsVisibleScanline(params.scanline), params.isVisible) 
		<< params.description;
	EXPECT_EQ(NesPpuTiming::IsVBlank(params.scanline), params.isVBlank) 
		<< params.description;
	EXPECT_EQ(NesPpuTiming::IsPreRenderScanline(params.scanline), params.isPreRender) 
		<< params.description;
}

INSTANTIATE_TEST_SUITE_P(
	NesScanlineTypes,
	NesScanlineTypeTest,
	::testing::Values(
		NesScanlineTypeParams{0, true, false, false, "First_visible_scanline"},
		NesScanlineTypeParams{239, true, false, false, "Last_visible_scanline"},
		NesScanlineTypeParams{240, false, false, false, "Post_render_scanline"},
		NesScanlineTypeParams{241, false, true, false, "First_VBlank_scanline"},
		NesScanlineTypeParams{260, false, true, false, "Last_VBlank_scanline"},
		NesScanlineTypeParams{261, false, false, true, "Pre_render_scanline"}
	),
	[](const ::testing::TestParamInfo<NesScanlineTypeParams>& info) {
		return std::string(info.param.description);
	}
);

// =================================================================================================
// Parameterized Tests for GB PPU Modes
// =================================================================================================

struct GbPpuModeParams {
	int32_t dot;
	int32_t scanline;
	uint8_t expectedMode;
	const char* description;
};

class GbPpuModeTest : public ::testing::TestWithParam<GbPpuModeParams> {};

TEST_P(GbPpuModeTest, PpuModeAtDotAndScanline) {
	const auto& params = GetParam();
	EXPECT_EQ(GbPpuTiming::GetModeForDot(params.dot, params.scanline), params.expectedMode)
		<< params.description;
}

INSTANTIATE_TEST_SUITE_P(
	GbPpuModes,
	GbPpuModeTest,
	::testing::Values(
		GbPpuModeParams{0, 0, GbPpuTiming::MODE_OAM, "OAM_start_on_visible_line"},
		GbPpuModeParams{79, 100, GbPpuTiming::MODE_OAM, "OAM_end_on_visible_line"},
		GbPpuModeParams{80, 50, GbPpuTiming::MODE_DRAWING, "Drawing_start"},
		GbPpuModeParams{251, 50, GbPpuTiming::MODE_DRAWING, "Drawing_middle"},
		GbPpuModeParams{252, 50, GbPpuTiming::MODE_HBLANK, "HBlank_start"},
		GbPpuModeParams{455, 50, GbPpuTiming::MODE_HBLANK, "HBlank_end"},
		GbPpuModeParams{0, 144, GbPpuTiming::MODE_VBLANK, "VBlank_start"},
		GbPpuModeParams{455, 153, GbPpuTiming::MODE_VBLANK, "VBlank_end"}
	),
	[](const ::testing::TestParamInfo<GbPpuModeParams>& info) {
		return std::string(info.param.description);
	}
);
