#pragma once
#include "pch.h"

/// <summary>
/// GBS (Game Boy Sound) file header structure.
/// </summary>
/// <remarks>
/// **GBS Format Overview:**
/// GBS files contain ripped Game Boy music that can be played
/// back on real hardware or emulators. The format was designed
/// to be similar to NSF for NES music.
///
/// **File Structure:**
/// - $00-$02: Signature "GBS"
/// - $03: Version (usually $01)
/// - $04: Number of tracks
/// - $05: First track to play (1-based)
/// - $06-$07: Load address (little-endian)
/// - $08-$09: Init address (little-endian)
/// - $0A-$0B: Play address (little-endian)
/// - $0C-$0D: Stack pointer initial value
/// - $0E: Timer Modulo (TMA) for timing
/// - $0F: Timer Control (TAC) for timing
/// - $10-$2F: Title (32 bytes, null-padded)
/// - $30-$4F: Author (32 bytes, null-padded)
/// - $50-$6F: Copyright (32 bytes, null-padded)
/// - $70+: Music data (loaded at LoadAddress)
///
/// **Playback Process:**
/// 1. Load music data to LoadAddress
/// 2. Set SP to StackPointer value
/// 3. Call Init with track# in A register
/// 4. Call Play on every frame/timer interrupt
///
/// **Timing Options:**
/// - Timer-based: Uses TMA/TAC for precise BPM
/// - VBlank-based: 59.7275 Hz interrupt rate
/// - TAC bit 7: Enable CGB double-speed mode
/// </remarks>
struct GbsHeader {
	/// <summary>File signature ("GBS").</summary>
	char Header[3];

	/// <summary>GBS format version (usually $01).</summary>
	uint8_t Version;

	/// <summary>Number of music tracks in file.</summary>
	uint8_t TrackCount;

	/// <summary>First track to play (1-based index).</summary>
	uint8_t FirstTrack;

	/// <summary>Where to load music data (little-endian).</summary>
	uint8_t LoadAddress[2];

	/// <summary>Init routine address (little-endian).</summary>
	/// <remarks>Called once per track with track# in A register.</remarks>
	uint8_t InitAddress[2];

	/// <summary>Play routine address (little-endian).</summary>
	/// <remarks>Called on every frame/timer tick.</remarks>
	uint8_t PlayAddress[2];

	/// <summary>Initial stack pointer value (little-endian).</summary>
	uint8_t StackPointer[2];

	/// <summary>Timer Modulo (TMA, $FF06) for timing.</summary>
	/// <remarks>Determines tempo when using timer-based playback.</remarks>
	uint8_t TimerModulo;

	/// <summary>Timer Control (TAC, $FF07) for timing.</summary>
	/// <remarks>
	/// - Bit 2: Enable timer (0 = use VBlank)
	/// - Bits 0-1: Timer speed select
	/// - Bit 7: Enable CGB double-speed mode
	/// </remarks>
	uint8_t TimerControl;

	/// <summary>Game/music title (32 chars, null-padded).</summary>
	char Title[32];

	/// <summary>Composer/artist name (32 chars, null-padded).</summary>
	char Author[32];

	/// <summary>Copyright notice (32 chars, null-padded).</summary>
	char Copyright[32];
};