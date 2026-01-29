#pragma once

#include "pch.h"

/// <summary>
/// Audio playback statistics for latency monitoring and debugging.
/// </summary>
struct AudioStatistics {
	double AverageLatency = 0;           ///< Average audio latency in milliseconds
	uint32_t BufferUnderrunEventCount = 0; ///< Number of buffer underruns (audio starvation)
	uint32_t BufferSize = 0;             ///< Current buffer size in bytes
};

/// <summary>
/// Interface for platform-specific audio playback backends.
/// Implemented by DirectSound, XAudio2, ALSA, PulseAudio, CoreAudio, and SDL.
/// </summary>
/// <remarks>
/// Implementations:
/// - Windows: DirectSound (legacy), XAudio2 (modern), WASAPI (low-latency)
/// - Linux: ALSA (direct hardware), PulseAudio (system mixer)
/// - macOS: CoreAudio (native)
/// - SDL: Cross-platform fallback
/// 
/// Audio flow:
/// 1. SoundMixer generates audio samples (16-bit PCM)
/// 2. PlayBuffer() submits samples to audio device
/// 3. Audio device queues samples in ring buffer
/// 4. Hardware/driver pulls samples at sample rate
/// 5. ProcessEndOfFrame() called every video frame for sync
/// 
/// Latency management:
/// - Lower buffer size = lower latency, higher CPU usage
/// - Higher buffer size = higher latency, fewer underruns
/// - Typical latency: 20-50ms (optimal for emulation)
/// 
/// Thread model:
/// - PlayBuffer() called from emulation thread
/// - Audio device may use callback thread (implementation-specific)
/// - ProcessEndOfFrame() for frame-rate synchronization
/// </remarks>
class IAudioDevice {
public:
	virtual ~IAudioDevice() {}
	
	/// <summary>
	/// Submit audio samples to playback device.
	/// </summary>
	/// <param name="soundBuffer">PCM audio samples (16-bit signed)</param>
	/// <param name="bufferSize">Number of samples (mono) or sample pairs (stereo)</param>
	/// <param name="sampleRate">Sample rate in Hz (typically 48000)</param>
	/// <param name="isStereo">True for stereo (L/R pairs), false for mono</param>
	/// <remarks>
	/// Buffer format:
	/// - Mono: [sample0, sample1, sample2, ...]
	/// - Stereo: [L0, R0, L1, R1, L2, R2, ...]
	/// 
	/// Blocking behavior:
	/// - May block if audio buffer is full
	/// - Should return quickly to avoid frame drops
	/// </remarks>
	virtual void PlayBuffer(int16_t* soundBuffer, uint32_t bufferSize, uint32_t sampleRate, bool isStereo) = 0;
	
	/// <summary>
	/// Stop audio playback and clear buffers.
	/// </summary>
	virtual void Stop() = 0;
	
	/// <summary>
	/// Pause audio playback (preserves buffer contents).
	/// </summary>
	virtual void Pause() = 0;
	
	/// <summary>
	/// End-of-frame synchronization callback.
	/// </summary>
	/// <remarks>
	/// Called once per emulated video frame (60 FPS for NTSC, 50 FPS for PAL).
	/// Used for:
	/// - Audio/video sync adjustments
	/// - Buffer underrun detection
	/// - Dynamic latency compensation
	/// </remarks>
	virtual void ProcessEndOfFrame() = 0;

	/// <summary>
	/// Get list of available audio devices (newline-separated).
	/// </summary>
	/// <returns>Device names, one per line</returns>
	virtual string GetAvailableDevices() = 0;
	
	/// <summary>
	/// Select audio output device.
	/// </summary>
	/// <param name="deviceName">Device name from GetAvailableDevices()</param>
	virtual void SetAudioDevice(string deviceName) = 0;

	/// <summary>
	/// Get audio playback statistics.
	/// </summary>
	/// <returns>Current latency and buffer underrun counts</returns>
	virtual AudioStatistics GetStatistics() = 0;
};