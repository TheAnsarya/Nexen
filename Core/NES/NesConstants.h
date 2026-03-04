#pragma once
#include "pch.h"

#include "Shared/SettingTypes.h"

/// <summary>
/// NES hardware timing and display constants.
/// </summary>
/// <remarks>
/// **CPU Clock Rates:**
/// The NES CPU (Ricoh 2A03/2A07) runs at different speeds by region:
/// - NTSC: 1.7897727 MHz (21.47727 MHz master ÷ 12)
/// - PAL: 1.662607 MHz (26.601712 MHz master ÷ 16)
/// - Dendy: 1.773448 MHz (Soviet clone, PAL-like)
///
/// **PPU Timing:**
/// - 341 PPU cycles per scanline (3 PPU cycles per CPU cycle)
/// - NTSC: 262 scanlines (240 visible + 22 vblank)
/// - PAL: 312 scanlines (240 visible + 72 vblank)
///
/// **Display:**
/// - Resolution: 256×240 pixels
/// - 8×8 pixel tiles in 32×30 tile nametable
/// - Overscan typically hides top/bottom 8 pixels
///
/// **Dendy Notes:**
/// Dendy was a Soviet NES clone with PAL-like timing but
/// 50 Hz NTSC-like output. Used in Russia and former USSR.
/// </remarks>
class NesConstants {
public:
	/// <summary>NTSC CPU clock rate (Hz). ~1.79 MHz</summary>
	static constexpr uint32_t ClockRateNtsc = 1789773;

	/// <summary>PAL CPU clock rate (Hz). ~1.66 MHz</summary>
	static constexpr uint32_t ClockRatePal = 1662607;

	/// <summary>Dendy CPU clock rate (Hz). ~1.77 MHz</summary>
	static constexpr uint32_t ClockRateDendy = 1773448;

	/// <summary>PPU cycles per scanline (341 dots/line).</summary>
	static constexpr uint32_t CyclesPerLine = 341;

	/// <summary>Horizontal resolution in pixels.</summary>
	static constexpr uint32_t ScreenWidth = 256;

	/// <summary>Vertical resolution in pixels.</summary>
	static constexpr uint32_t ScreenHeight = 240;

	/// <summary>Total pixels per frame.</summary>
	static constexpr uint32_t ScreenPixelCount = ScreenWidth * ScreenHeight;

	/// <summary>
	/// Gets CPU clock rate for a given region.
	/// </summary>
	/// <param name="region">Console region (NTSC/PAL/Dendy).</param>
	/// <returns>Clock rate in Hz.</returns>
	static constexpr uint32_t GetClockRate(ConsoleRegion region) {
		switch (region) {
			default:
			case ConsoleRegion::Ntsc:
				return NesConstants::ClockRateNtsc;
				break;
			case ConsoleRegion::Pal:
				return NesConstants::ClockRatePal;
				break;
			case ConsoleRegion::Dendy:
				return NesConstants::ClockRateDendy;
				break;
		}
	}
};
