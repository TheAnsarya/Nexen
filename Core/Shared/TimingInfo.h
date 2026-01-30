#pragma once
#include "pch.h"

/// <summary>
/// Platform-specific timing information for accurate emulation and synchronization.
/// Different platforms have different refresh rates, clock speeds, and scanline counts.
/// </summary>
/// <remarks>
/// Used for:
/// - Frame limiter synchronization
/// - Audio/video sample rate calculation
/// - Cycle-accurate emulation timing
/// - FPS display and performance metrics
///
/// Platform examples:
/// - NTSC NES: 60.1 Hz, ~21.477 MHz master clock, 262 scanlines
/// - PAL NES: 50.0 Hz, ~26.602 MHz master clock, 312 scanlines
/// - SNES: 60.0 Hz, ~21.477 MHz master clock, 262/312 scanlines (NTSC/PAL)
/// - Game Boy: 59.7 Hz, ~4.194 MHz master clock, 154 scanlines
/// </remarks>
struct TimingInfo {
	/// <summary>Frames per second (typically 60 Hz NTSC or 50 Hz PAL)</summary>
	/// <remarks>
	/// Precise values vary by platform:
	/// - NTSC: ~59.94 Hz (NES), 60.0 Hz (SNES/Game Boy)
	/// - PAL: 50.0 Hz
	/// Used by frame limiter to maintain correct playback speed.
	/// </remarks>
	double Fps;

	/// <summary>Current master clock cycle count (64-bit for overflow prevention)</summary>
	/// <remarks>
	/// Monotonic counter incremented every master clock cycle.
	/// Never resets during emulation - allows calculating elapsed time.
	/// 64-bit prevents overflow (would take centuries at typical clock rates).
	/// </remarks>
	uint64_t MasterClock;

	/// <summary>Master clock frequency in Hz (cycles per second)</summary>
	/// <remarks>
	/// Platform examples:
	/// - NES NTSC: 21,477,272 Hz (~21.5 MHz)
	/// - SNES: 21,477,272 Hz (~21.5 MHz)
	/// - Game Boy: 4,194,304 Hz (~4.2 MHz)
	/// - GBA: 16,777,216 Hz (16.78 MHz)
	/// CPU clock is typically derived by dividing master clock.
	/// </remarks>
	uint32_t MasterClockRate;

	/// <summary>Total frames emulated since power-on (monotonic counter)</summary>
	/// <remarks>
	/// Increments by 1 at the end of each frame.
	/// Used for movie sync, lag frame detection, speedrun timing.
	/// Does NOT reset on soft reset (only on power cycle).
	/// </remarks>
	uint32_t FrameCount;

	/// <summary>Number of scanlines per frame (platform-dependent)</summary>
	/// <remarks>
	/// Platform examples:
	/// - NTSC NES: 262 scanlines (240 visible + 22 VBlank)
	/// - PAL NES: 312 scanlines (240 visible + 72 VBlank)
	/// - SNES: 262/312 scanlines (NTSC/PAL)
	/// - Game Boy: 154 scanlines (144 visible + 10 VBlank)
	/// </remarks>
	uint32_t ScanlineCount;

	/// <summary>First visible scanline (typically 0, but platform-dependent)</summary>
	/// <remarks>
	/// Most platforms start at 0, but some have offset:
	/// - NES: -1 (pre-render scanline is numbered -1)
	/// - SNES: 0
	/// - Game Boy: 0
	/// Used for accurate PPU timing and raster effects.
	/// </remarks>
	int32_t FirstScanline;

	/// <summary>Number of master clock cycles per scanline</summary>
	/// <remarks>
	/// Platform examples:
	/// - NES NTSC: ~113.667 cycles/scanline (341 PPU dots * 4 master clocks / 12)
	/// - SNES: ~1364 cycles/scanline
	/// - Game Boy: ~456 cycles/scanline (114 T-states * 4)
	/// Used for scanline-accurate emulation and raster effects.
	/// </remarks>
	uint32_t CycleCount;
};