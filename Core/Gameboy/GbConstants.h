#pragma once
#include "pch.h"

/// <summary>
/// Game Boy hardware display constants.
/// </summary>
/// <remarks>
/// **Game Boy LCD Specifications:**
/// - Resolution: 160×144 pixels (20×18 tiles)
/// - Dot clock: 4.194304 MHz (÷4 for CPU clock)
/// - Refresh rate: 59.7275 Hz
/// - Scanlines: 154 total (144 visible + 10 VBlank)
///
/// **Display Area:**
/// - Horizontal: 160 pixels visible (456 dots per line total)
/// - Vertical: 144 lines visible (154 lines per frame total)
/// - Total pixels per frame: 160 × 144 = 23,040
///
/// **Comparison to other handhelds:**
/// - Game Boy: 160×144 (23,040 pixels)
/// - Game Gear: 160×144 (same)
/// - Lynx: 160×102 (16,320 pixels)
/// - GBA: 240×160 (38,400 pixels)
/// </remarks>
class GbConstants {
public:
	/// <summary>Horizontal display resolution in pixels.</summary>
	static constexpr uint32_t ScreenWidth = 160;

	/// <summary>Vertical display resolution in pixels.</summary>
	static constexpr uint32_t ScreenHeight = 144;

	/// <summary>Total visible pixels per frame.</summary>
	static constexpr uint32_t PixelCount = GbConstants::ScreenWidth * GbConstants::ScreenHeight;
};