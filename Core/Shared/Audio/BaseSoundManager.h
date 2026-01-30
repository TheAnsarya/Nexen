#pragma once
#include "Core/Shared/Interfaces/IAudioDevice.h"

/// <summary>
/// Base class for platform-specific audio device implementations.
/// Provides latency tracking and buffer underrun detection.
/// </summary>
/// <remarks>
/// Concrete implementations exist for each platform:
/// - Windows: DirectSound, XAudio2, WASAPI
/// - Linux: ALSA, PulseAudio
/// - macOS: CoreAudio
/// - SDL: Cross-platform fallback
///
/// Latency tracking:
/// - Measures cursor gap (playback position vs write position)
/// - Averages over 60-sample window
/// - Reports buffer underruns (audio starvation)
///
/// Statistics provided:
/// - Average latency (milliseconds)
/// - Buffer size
/// - Underrun event count
///
/// Derived classes implement:
/// - PlayBuffer() - Submit samples to audio device
/// - Stop() - Stop playback and clear buffers
/// - SetSampleRate() - Configure sample rate
/// - ProcessEndOfFrame() - Per-frame synchronization
/// </remarks>
class BaseSoundManager : public IAudioDevice {
public:
	void ProcessLatency(uint32_t readPosition, uint32_t writePosition);
	AudioStatistics GetStatistics();

protected:
	bool _isStereo;
	uint32_t _sampleRate = 0;

	double _averageLatency = 0;
	uint32_t _bufferSize = 0x10000;
	uint32_t _bufferUnderrunEventCount = 0;

	int32_t _cursorGaps[60];
	int32_t _cursorGapIndex = 0;
	bool _cursorGapFilled = false;

	void ResetStats();
};
