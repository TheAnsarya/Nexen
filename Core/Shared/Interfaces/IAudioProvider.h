#pragma once
#include "pch.h"

/// <summary>
/// Interface for components that generate audio samples.
/// Implemented by APUs, audio chips, and sound effects generators.
/// </summary>
/// <remarks>
/// Implementers:
/// - NES APU (2A03 - pulse/triangle/noise/DMC channels)
/// - SNES APU (SPC700 - 8 channels DSP)
/// - Game Boy APU (4 channels - pulse/wave/noise)
/// - PC Engine PSG (6 channels wave)
/// - External chips (VRC6, FDS, MMC5, N163, etc.)
/// - CD-ROM audio (PCE CD, Sega CD)
///
/// Audio mixing flow:
/// 1. SoundMixer calls MixAudio() on each registered provider
/// 2. Provider writes samples to output buffer (additive mixing)
/// 3. SoundMixer applies effects (equalizer, reverb, crossfeed)
/// 4. Final mix sent to audio device
///
/// Thread safety:
/// - Called from emulation thread (locked by EmulatorLock)
/// - Must not block or perform I/O
/// - Sample buffer is caller-owned (no deallocation)
/// </remarks>
class IAudioProvider {
public:
	/// <summary>
	/// Generate and mix audio samples into output buffer.
	/// </summary>
	/// <param name="out">Output buffer (16-bit signed stereo pairs, L/R/L/R/...)</param>
	/// <param name="sampleCount">Number of stereo sample pairs to generate</param>
	/// <param name="sampleRate">Target sample rate in Hz (typically 48000)</param>
	/// <remarks>
	/// Implementation notes:
	/// - Add samples to buffer (don't overwrite - multiple providers mix additively)
	/// - Clamp output to [-32768, 32767] range
	/// - Resample if internal rate differs from sampleRate
	/// - Called every frame or when SoundMixer requests audio
	/// </remarks>
	virtual void MixAudio(int16_t* out, uint32_t sampleCount, uint32_t sampleRate) = 0;
};