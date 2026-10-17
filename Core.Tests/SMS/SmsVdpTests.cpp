#include "pch.h"
#include "SMS/SmsTypes.h"

// =============================================================================
// SMS VDP State Tests
// =============================================================================
// Tests for VDP state structures, mode bit decoding, address computation,
// sprite evaluation rules, and scroll behavior.

namespace SmsVdpTestHelpers {
	/// <summary>
	/// Decodes the VDP display mode from control register bits.
	/// Mode 4 (SMS native) requires UseMode4 = true.
	/// TMS9918 modes: 0 (Graphics I), 1 (Text), 2 (Graphics II), 3 (Multicolor).
	/// </summary>
	static int DecodeDisplayMode(const SmsVdpState& state) {
		if (state.UseMode4) return 4;
		// TMS modes determined by M1/M2/M3 bits
		int mode = 0;
		if (state.M1_Use224LineMode) mode |= 1;	// M1 in TMS context
		if (state.M2_AllowHeightChange) mode |= 2;	// M2
		return mode;
	}

	/// <summary>
	/// Calculates the effective nametable address from the configured address and mask.
	/// </summary>
	static uint16_t ComputeEffectiveNametable(uint16_t address, uint16_t mask) {
		return address & mask;
	}

	/// <summary>
	/// Computes sprite height based on control register bits.
	/// 8x8 normal, 8x16 with UseLargeSprites, doubled if EnableDoubleSpriteSize.
	/// </summary>
	static int GetSpriteHeight(bool useLargeSprites, bool enableDoubleSize) {
		int base = useLargeSprites ? 16 : 8;
		return enableDoubleSize ? base * 2 : base;
	}

	/// <summary>
	/// Checks if a scanline is in the active display area for SMS (192/224/240 modes).
	/// </summary>
	static bool IsActiveScanline(uint16_t scanline, uint8_t visibleCount) {
		return scanline < visibleCount;
	}

	/// <summary>
	/// Computes horizontal scroll for a given column, respecting HorizontalScrollLock.
	/// Columns 24-31 (rightmost 8 columns) ignore scroll when lock is enabled.
	/// </summary>
	static uint8_t GetEffectiveHScroll(uint8_t scroll, int column, bool lockRight) {
		if (lockRight && column >= 24) return 0;
		return scroll;
	}

	/// <summary>
	/// Computes vertical scroll for a given column, respecting VerticalScrollLock.
	/// Columns 0-23 use 0 scroll when lock is enabled (top 2 rows stay fixed).
	/// Actually: VerticalScrollLock locks the RIGHT 8 columns (24-31).
	/// Wait — SMS docs: VerticalScrollLock disables V-scroll for columns 24-31.
	/// Let me check: bit 7 of reg 0 = "disable vertical scrolling for columns 24-31"
	/// </summary>
	static uint8_t GetEffectiveVScroll(uint8_t scroll, int column, bool lockRight) {
		if (lockRight && column >= 24) return 0;
		return scroll;
	}

	/// <summary>
	/// Checks if a sprite Y position means "end of sprite list" in Mode 4.
	/// Y = 0xD0 (208) terminates the list for 192-line mode.
	/// Y = 0xD0 does NOT terminate in 224/240-line modes.
	/// </summary>
	static bool IsSpriteTerminator(uint8_t y, uint8_t visibleLines) {
		return (visibleLines == 192) && (y == 0xd0);
	}

	/// <summary>
	/// SMS CRAM (Color RAM) size depends on model.
	/// SMS: 32 bytes (6-bit color, 64 entries in a palette of 64 colors)
	/// Game Gear: 64 bytes (12-bit color, 4096 palette)
	/// </summary>
	static int GetCramSize(SmsModel model) {
		return (model == SmsModel::GameGear) ? 64 : 32;
	}

	/// <summary>
	/// Converts SMS 6-bit CRAM entry to RGB components.
	/// Each component is 2 bits (0-3), mapping: 0→0, 1→85, 2→170, 3→255.
	/// Format: --BBGGRR
	/// </summary>
	static void SmsCramToRgb(uint8_t cram, uint8_t& r, uint8_t& g, uint8_t& b) {
		r = (cram & 0x03) * 85;
		g = ((cram >> 2) & 0x03) * 85;
		b = ((cram >> 4) & 0x03) * 85;
	}

	/// <summary>
	/// Converts Game Gear 12-bit CRAM entry to RGB components.
	/// Each component is 4 bits (0-15), mapping: value * 17.
	/// Format: ----BBBBGGGGRRRR (little-endian word)
	/// </summary>
	static void GgCramToRgb(uint16_t cram, uint8_t& r, uint8_t& g, uint8_t& b) {
		r = (cram & 0x0f) * 17;
		g = ((cram >> 4) & 0x0f) * 17;
		b = ((cram >> 8) & 0x0f) * 17;
	}
}

// =============================================================================
// VDP State Initialization
// =============================================================================

TEST(SmsVdpStateTest, DefaultState_ZeroInitialized) {
	SmsVdpState state = {};
	EXPECT_EQ(state.FrameCount, 0u);
	EXPECT_EQ(state.Cycle, 0);
	EXPECT_EQ(state.Scanline, 0);
	EXPECT_EQ(state.VCounter, 0);
	EXPECT_EQ(state.AddressReg, 0);
	EXPECT_EQ(state.CodeReg, 0);
	EXPECT_FALSE(state.ControlPortMsbToggle);
	EXPECT_EQ(state.VramBuffer, 0);
}

TEST(SmsVdpStateTest, DefaultState_AllControlBitsFalse) {
	SmsVdpState state = {};
	EXPECT_FALSE(state.SyncDisabled);
	EXPECT_FALSE(state.UseMode4);
	EXPECT_FALSE(state.ShiftSpritesLeft);
	EXPECT_FALSE(state.EnableScanlineIrq);
	EXPECT_FALSE(state.MaskFirstColumn);
	EXPECT_FALSE(state.HorizontalScrollLock);
	EXPECT_FALSE(state.VerticalScrollLock);
	EXPECT_FALSE(state.RenderingEnabled);
	EXPECT_FALSE(state.EnableVerticalBlankIrq);
	EXPECT_FALSE(state.UseLargeSprites);
	EXPECT_FALSE(state.EnableDoubleSpriteSize);
}

TEST(SmsVdpStateTest, DefaultState_StatusFlagsClear) {
	SmsVdpState state = {};
	EXPECT_FALSE(state.VerticalBlankIrqPending);
	EXPECT_FALSE(state.ScanlineIrqPending);
	EXPECT_FALSE(state.SpriteOverflow);
	EXPECT_FALSE(state.SpriteCollision);
	EXPECT_EQ(state.SpriteOverflowIndex, 0);
}

// =============================================================================
// Display Mode Decoding
// =============================================================================

TEST(SmsVdpModeTest, Mode4_WhenUseMode4Set) {
	SmsVdpState state = {};
	state.UseMode4 = true;
	EXPECT_EQ(SmsVdpTestHelpers::DecodeDisplayMode(state), 4);
}

TEST(SmsVdpModeTest, Mode0_GraphicsI_Default) {
	SmsVdpState state = {};
	state.UseMode4 = false;
	state.M1_Use224LineMode = false;
	state.M2_AllowHeightChange = false;
	EXPECT_EQ(SmsVdpTestHelpers::DecodeDisplayMode(state), 0);
}

TEST(SmsVdpModeTest, Mode1_TextMode) {
	SmsVdpState state = {};
	state.UseMode4 = false;
	state.M1_Use224LineMode = true;
	EXPECT_EQ(SmsVdpTestHelpers::DecodeDisplayMode(state), 1);
}

TEST(SmsVdpModeTest, Mode2_GraphicsII) {
	SmsVdpState state = {};
	state.UseMode4 = false;
	state.M2_AllowHeightChange = true;
	EXPECT_EQ(SmsVdpTestHelpers::DecodeDisplayMode(state), 2);
}

// =============================================================================
// Sprite Height Calculation
// =============================================================================

TEST(SmsVdpSpriteTest, SpriteHeight_8x8_Default) {
	EXPECT_EQ(SmsVdpTestHelpers::GetSpriteHeight(false, false), 8);
}

TEST(SmsVdpSpriteTest, SpriteHeight_8x16_LargeSprites) {
	EXPECT_EQ(SmsVdpTestHelpers::GetSpriteHeight(true, false), 16);
}

TEST(SmsVdpSpriteTest, SpriteHeight_Doubled_8x8) {
	EXPECT_EQ(SmsVdpTestHelpers::GetSpriteHeight(false, true), 16);
}

TEST(SmsVdpSpriteTest, SpriteHeight_Doubled_8x16) {
	EXPECT_EQ(SmsVdpTestHelpers::GetSpriteHeight(true, true), 32);
}

TEST(SmsVdpSpriteTest, SpriteTerminator_192LineMode) {
	EXPECT_TRUE(SmsVdpTestHelpers::IsSpriteTerminator(0xd0, 192));
}

TEST(SmsVdpSpriteTest, SpriteTerminator_NotIn224LineMode) {
	EXPECT_FALSE(SmsVdpTestHelpers::IsSpriteTerminator(0xd0, 224));
}

TEST(SmsVdpSpriteTest, SpriteTerminator_NotIn240LineMode) {
	EXPECT_FALSE(SmsVdpTestHelpers::IsSpriteTerminator(0xd0, 240));
}

TEST(SmsVdpSpriteTest, SpriteTerminator_OtherY_NotTerminator) {
	EXPECT_FALSE(SmsVdpTestHelpers::IsSpriteTerminator(0xcf, 192));
	EXPECT_FALSE(SmsVdpTestHelpers::IsSpriteTerminator(0xd1, 192));
	EXPECT_FALSE(SmsVdpTestHelpers::IsSpriteTerminator(0x00, 192));
	EXPECT_FALSE(SmsVdpTestHelpers::IsSpriteTerminator(0xff, 192));
}

// =============================================================================
// Nametable Address Computation
// =============================================================================

TEST(SmsVdpNametableTest, EffectiveAddress_MaskedCorrectly) {
	EXPECT_EQ(SmsVdpTestHelpers::ComputeEffectiveNametable(0x3800, 0x3fff), 0x3800);
	EXPECT_EQ(SmsVdpTestHelpers::ComputeEffectiveNametable(0x3c00, 0x3fff), 0x3c00);
}

TEST(SmsVdpNametableTest, EffectiveAddress_MaskClearsHighBits) {
	EXPECT_EQ(SmsVdpTestHelpers::ComputeEffectiveNametable(0x7800, 0x3fff), 0x3800);
}

TEST(SmsVdpNametableTest, EffectiveAddress_FullMask) {
	EXPECT_EQ(SmsVdpTestHelpers::ComputeEffectiveNametable(0x3800, 0xffff), 0x3800);
}

// =============================================================================
// Scroll Behavior
// =============================================================================

TEST(SmsVdpScrollTest, HScroll_NormalColumn_UsesScroll) {
	EXPECT_EQ(SmsVdpTestHelpers::GetEffectiveHScroll(0x40, 0, false), 0x40);
	EXPECT_EQ(SmsVdpTestHelpers::GetEffectiveHScroll(0x40, 16, false), 0x40);
	EXPECT_EQ(SmsVdpTestHelpers::GetEffectiveHScroll(0x40, 23, false), 0x40);
}

TEST(SmsVdpScrollTest, HScroll_LockedColumn_IgnoresScroll) {
	// Columns 24-31 ignore H-scroll when lock enabled
	EXPECT_EQ(SmsVdpTestHelpers::GetEffectiveHScroll(0x40, 24, true), 0);
	EXPECT_EQ(SmsVdpTestHelpers::GetEffectiveHScroll(0x40, 31, true), 0);
}

TEST(SmsVdpScrollTest, HScroll_LockedColumn_StillScrollsWhenUnlocked) {
	EXPECT_EQ(SmsVdpTestHelpers::GetEffectiveHScroll(0x40, 24, false), 0x40);
	EXPECT_EQ(SmsVdpTestHelpers::GetEffectiveHScroll(0x40, 31, false), 0x40);
}

TEST(SmsVdpScrollTest, VScroll_NormalColumn_UsesScroll) {
	EXPECT_EQ(SmsVdpTestHelpers::GetEffectiveVScroll(0x20, 0, false), 0x20);
	EXPECT_EQ(SmsVdpTestHelpers::GetEffectiveVScroll(0x20, 16, false), 0x20);
}

TEST(SmsVdpScrollTest, VScroll_LockedColumn_IgnoresScroll) {
	EXPECT_EQ(SmsVdpTestHelpers::GetEffectiveVScroll(0x20, 24, true), 0);
	EXPECT_EQ(SmsVdpTestHelpers::GetEffectiveVScroll(0x20, 31, true), 0);
}

// =============================================================================
// Active Scanline Detection
// =============================================================================

TEST(SmsVdpScanlineTest, ActiveScanline_192Mode) {
	EXPECT_TRUE(SmsVdpTestHelpers::IsActiveScanline(0, 192));
	EXPECT_TRUE(SmsVdpTestHelpers::IsActiveScanline(191, 192));
	EXPECT_FALSE(SmsVdpTestHelpers::IsActiveScanline(192, 192));
	EXPECT_FALSE(SmsVdpTestHelpers::IsActiveScanline(261, 192));
}

TEST(SmsVdpScanlineTest, ActiveScanline_224Mode) {
	EXPECT_TRUE(SmsVdpTestHelpers::IsActiveScanline(0, 224));
	EXPECT_TRUE(SmsVdpTestHelpers::IsActiveScanline(223, 224));
	EXPECT_FALSE(SmsVdpTestHelpers::IsActiveScanline(224, 224));
}

TEST(SmsVdpScanlineTest, ActiveScanline_240Mode) {
	EXPECT_TRUE(SmsVdpTestHelpers::IsActiveScanline(0, 240));
	EXPECT_TRUE(SmsVdpTestHelpers::IsActiveScanline(239, 240));
	EXPECT_FALSE(SmsVdpTestHelpers::IsActiveScanline(240, 240));
}

// =============================================================================
// CRAM (Color RAM) Model Differences
// =============================================================================

TEST(SmsVdpCramTest, CramSize_Sms_32Bytes) {
	EXPECT_EQ(SmsVdpTestHelpers::GetCramSize(SmsModel::Sms), 32);
}

TEST(SmsVdpCramTest, CramSize_GameGear_64Bytes) {
	EXPECT_EQ(SmsVdpTestHelpers::GetCramSize(SmsModel::GameGear), 64);
}

TEST(SmsVdpCramTest, CramSize_Sg_32Bytes) {
	EXPECT_EQ(SmsVdpTestHelpers::GetCramSize(SmsModel::Sg), 32);
}

TEST(SmsVdpCramTest, CramSize_ColecoVision_32Bytes) {
	EXPECT_EQ(SmsVdpTestHelpers::GetCramSize(SmsModel::ColecoVision), 32);
}

// =============================================================================
// SMS CRAM Color Conversion
// =============================================================================

TEST(SmsVdpColorTest, SmsCram_Black) {
	uint8_t r, g, b;
	SmsVdpTestHelpers::SmsCramToRgb(0x00, r, g, b);
	EXPECT_EQ(r, 0);
	EXPECT_EQ(g, 0);
	EXPECT_EQ(b, 0);
}

TEST(SmsVdpColorTest, SmsCram_White) {
	uint8_t r, g, b;
	SmsVdpTestHelpers::SmsCramToRgb(0x3f, r, g, b);
	EXPECT_EQ(r, 255);
	EXPECT_EQ(g, 255);
	EXPECT_EQ(b, 255);
}

TEST(SmsVdpColorTest, SmsCram_PureRed) {
	uint8_t r, g, b;
	SmsVdpTestHelpers::SmsCramToRgb(0x03, r, g, b);
	EXPECT_EQ(r, 255);
	EXPECT_EQ(g, 0);
	EXPECT_EQ(b, 0);
}

TEST(SmsVdpColorTest, SmsCram_PureGreen) {
	uint8_t r, g, b;
	SmsVdpTestHelpers::SmsCramToRgb(0x0c, r, g, b);
	EXPECT_EQ(r, 0);
	EXPECT_EQ(g, 255);
	EXPECT_EQ(b, 0);
}

TEST(SmsVdpColorTest, SmsCram_PureBlue) {
	uint8_t r, g, b;
	SmsVdpTestHelpers::SmsCramToRgb(0x30, r, g, b);
	EXPECT_EQ(r, 0);
	EXPECT_EQ(g, 0);
	EXPECT_EQ(b, 255);
}

TEST(SmsVdpColorTest, SmsCram_MidGray) {
	uint8_t r, g, b;
	// RR=01 GG=01 BB=01 → 0b_01_01_01 = 0x15
	SmsVdpTestHelpers::SmsCramToRgb(0x15, r, g, b);
	EXPECT_EQ(r, 85);
	EXPECT_EQ(g, 85);
	EXPECT_EQ(b, 85);
}

// =============================================================================
// Game Gear CRAM Color Conversion (12-bit)
// =============================================================================

TEST(SmsVdpGgColorTest, GgCram_Black) {
	uint8_t r, g, b;
	SmsVdpTestHelpers::GgCramToRgb(0x0000, r, g, b);
	EXPECT_EQ(r, 0);
	EXPECT_EQ(g, 0);
	EXPECT_EQ(b, 0);
}

TEST(SmsVdpGgColorTest, GgCram_White) {
	uint8_t r, g, b;
	SmsVdpTestHelpers::GgCramToRgb(0x0fff, r, g, b);
	EXPECT_EQ(r, 255);
	EXPECT_EQ(g, 255);
	EXPECT_EQ(b, 255);
}

TEST(SmsVdpGgColorTest, GgCram_PureRed) {
	uint8_t r, g, b;
	SmsVdpTestHelpers::GgCramToRgb(0x000f, r, g, b);
	EXPECT_EQ(r, 255);
	EXPECT_EQ(g, 0);
	EXPECT_EQ(b, 0);
}

TEST(SmsVdpGgColorTest, GgCram_PureGreen) {
	uint8_t r, g, b;
	SmsVdpTestHelpers::GgCramToRgb(0x00f0, r, g, b);
	EXPECT_EQ(r, 0);
	EXPECT_EQ(g, 255);
	EXPECT_EQ(b, 0);
}

TEST(SmsVdpGgColorTest, GgCram_PureBlue) {
	uint8_t r, g, b;
	SmsVdpTestHelpers::GgCramToRgb(0x0f00, r, g, b);
	EXPECT_EQ(r, 0);
	EXPECT_EQ(g, 0);
	EXPECT_EQ(b, 255);
}

TEST(SmsVdpGgColorTest, GgCram_MidValues) {
	uint8_t r, g, b;
	// R=8, G=8, B=8 → 0x0888
	SmsVdpTestHelpers::GgCramToRgb(0x0888, r, g, b);
	EXPECT_EQ(r, 136);	// 8 * 17
	EXPECT_EQ(g, 136);
	EXPECT_EQ(b, 136);
}

// =============================================================================
// VDP Control Port Toggle Behavior
// =============================================================================

TEST(SmsVdpControlTest, ControlPortToggle_InitiallyFalse) {
	SmsVdpState state = {};
	EXPECT_FALSE(state.ControlPortMsbToggle);
}

TEST(SmsVdpControlTest, VramBuffer_InitiallyZero) {
	SmsVdpState state = {};
	EXPECT_EQ(state.VramBuffer, 0);
}

// =============================================================================
// VDP Scroll Latching
// =============================================================================

TEST(SmsVdpLatchTest, ScrollLatch_IndependentFromCurrent) {
	SmsVdpState state = {};
	state.HorizontalScroll = 0x30;
	state.HorizontalScrollLatch = 0x20;
	state.VerticalScroll = 0x10;
	state.VerticalScrollLatch = 0x08;

	// Latch values are independent from current values
	EXPECT_NE(state.HorizontalScroll, state.HorizontalScrollLatch);
	EXPECT_NE(state.VerticalScroll, state.VerticalScrollLatch);
}

TEST(SmsVdpLatchTest, ScanlineCounter_IndependentFromLatch) {
	SmsVdpState state = {};
	state.ScanlineCounter = 0x0a;
	state.ScanlineCounterLatch = 0xff;
	EXPECT_NE(state.ScanlineCounter, state.ScanlineCounterLatch);
}
