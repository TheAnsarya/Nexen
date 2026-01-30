#pragma once
#include "pch.h"
#include "Core/Shared/Interfaces/IAudioDevice.h"
#include "Utilities/safe_ptr.h"
#include "Utilities/Audio/HermiteResampler.h"

class Emulator;
class Equalizer;
class SoundResampler;
class WaveRecorder;
class IAudioProvider;
class CrossFeedFilter;
class ReverbFilter;

/// <summary>
/// Audio mixing, resampling, and effects processing coordinator.
/// Combines audio from multiple sources and applies equalizer/reverb/crossfeed effects.
/// </summary>
/// <remarks>
/// Architecture:
/// - Multi-source mixing (combines NES/SNES/PCE/etc. audio channels)
/// - Hermite resampling for pitch adjustment (turbo mode, speed changes)
/// - Effect chain: Equalizer → CrossFeed → Reverb
///
/// Audio sources (IAudioProvider):
/// - APU (Audio Processing Unit) from each console
/// - External audio chips (FDS, VRC6/7, N163, etc.)
/// - CD-ROM audio (PCE, Sega CD)
/// - Sample playback (PCM, ADPCM)
///
/// Effects:
/// - Equalizer: Per-band volume adjustment (bass/mid/treble)
/// - CrossFeedFilter: Stereo separation reduction (simulates speaker bleed)
/// - ReverbFilter: Artificial room reverb
///
/// Resampling:
/// - Source rate varies by console (NES: ~44100 Hz, SNES: 32040 Hz, etc.)
/// - Target rate matches audio device (typically 48000 Hz)
/// - Hermite interpolation for high-quality resampling
///
/// Recording:
/// - safe_ptr<WaveRecorder> for async WAV recording
/// - Records post-mix, post-effects audio
///
/// Thread safety:
/// - safe_ptr guards recorder lifecycle
/// - Providers register/unregister from different threads
/// </remarks>
class SoundMixer {
private:
	IAudioDevice* _audioDevice;
	vector<IAudioProvider*> _audioProviders;
	Emulator* _emu;
	unique_ptr<Equalizer> _equalizer;
	unique_ptr<SoundResampler> _resampler;
	safe_ptr<WaveRecorder> _waveRecorder;
	std::unique_ptr<int16_t[]> _sampleBuffer;

	HermiteResampler _pitchAdjust;
	std::unique_ptr<int16_t[]> _pitchAdjustBuffer;

	int16_t _leftSample = 0;
	int16_t _rightSample = 0;

	unique_ptr<CrossFeedFilter> _crossFeedFilter;
	unique_ptr<ReverbFilter> _reverbFilter;

	void ProcessEqualizer(int16_t* samples, uint32_t sampleCount, uint32_t targetRate);

public:
	SoundMixer(Emulator* emu);
	~SoundMixer();

	void PlayAudioBuffer(int16_t* samples, uint32_t sampleCount, uint32_t sourceRate);
	void StopAudio(bool clearBuffer = false);

	void RegisterAudioDevice(IAudioDevice* audioDevice);

	void RegisterAudioProvider(IAudioProvider* provider);
	void UnregisterAudioProvider(IAudioProvider* provider);

	AudioStatistics GetStatistics();
	double GetRateAdjustment();

	void StartRecording(string filepath);
	void StopRecording();
	bool IsRecording();
	void GetLastSamples(int16_t& left, int16_t& right);
};
