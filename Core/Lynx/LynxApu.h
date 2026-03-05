#pragma once
#include "pch.h"
#include "Lynx/LynxTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;
class LynxConsole;
class SoundMixer;

/// <summary>
/// Atari Lynx Audio Processing Unit.
///
/// 4 LFSR-based audio channels in Mikey at $FD20-$FD3F.
/// Each channel has an 8-byte register block:
///   +0 Volume     (signed 8-bit output magnitude)
///   +1 Feedback    (LFSR feedback tap select)
///   +2 Output      (current audio output, signed 8-bit)
///   +3 ShiftLo     (LFSR shift register low byte)
///   +4 ShiftHi     (LFSR shift register high nybble)
///   +5 BackupValue (timer reload for frequency)
///   +6 Control     (enable, integration, clock select)
///   +7 Counter     (current timer countdown)
///
/// Channel 3 ($FD38-$FD3F) supports DAC mode for PCM playback.
/// Per-channel stereo attenuation at $FD40-$FD43 (ATTEN_A-D, nibble-packed L/R).
/// Panning enable at $FD48 (MPAN).
/// Stereo channel enable at $FD50 (MSTEREO).
/// </summary>
class LynxApu final : public ISerializable {
public:
	/// <summary>Audio sample rate — Lynx master clock / 4 / timer period</summary>
	static constexpr uint32_t SampleRate = 22050;

	/// <summary>CPU cycles per audio sample: 4MHz / 22050 ≈ 181</summary>
	static constexpr uint32_t CpuCyclesPerSample = LynxConstants::CpuClockRate / SampleRate;

	/// <summary>Max samples buffered before flush</summary>
	static constexpr uint32_t MaxSamples = 2048;

private:
	Emulator* _emu = nullptr;
	LynxConsole* _console = nullptr;
	SoundMixer* _soundMixer = nullptr;

	LynxApuState _state = {};

	unique_ptr<int16_t[]> _soundBuffer;
	uint32_t _sampleCount = 0;
	uint64_t _lastSampleCycle = 0;

	/// <summary>Audio channel prescaler periods (CPU cycles per tick).</summary>
	/// <summary>Same prescaler values as system timers: {4,8,16,32,64,128,256,0}</summary>
	static constexpr uint32_t _prescalerPeriods[8] = { 4, 8, 16, 32, 64, 128, 256, 0 };

	/// <summary>Clock a single audio channel's LFSR (called on timer underflow)</summary>
	void ClockChannel(int ch);

	/// <summary>Tick a channel's timer based on its prescaler and elapsed CPU cycles</summary>
	void TickChannelTimer(int ch, uint64_t currentCycle);

	/// <summary>Cascade audio channel underflow to next linked channel</summary>
	void CascadeAudioChannel(int sourceChannel);

	/// <summary>Mix all channel outputs and write one sample to buffer</summary>
	void MixOutput();

	/// <summary>Flush audio buffer to sound mixer</summary>
	void PlayQueuedAudio();

public:
	LynxApu(Emulator* emu, LynxConsole* console);
	~LynxApu() = default;

	void Init();

	/// <summary>Called with current CPU cycle count — ticks audio timers and generates samples</summary>
	void Tick(uint64_t currentCycle);

	/// <summary>Cascade from system Timer 7 into Audio Channel 0.
	/// On real Lynx hardware, the cascade chain is:
	///   Timer 1 → 3 → 5 → 7 → Audio 0 → Audio 1 → Audio 2 → Audio 3
	/// This is called by LynxMikey::CascadeTimer when Timer 7 underflows.</summary>
	void CascadeFromSystemTimer();

	/// <summary>Read an audio register ($FD20-$FD4F range, relative offset)</summary>
	[[nodiscard]] uint8_t ReadRegister(uint8_t addr);

	/// <summary>Write an audio register ($FD20-$FD4F range, relative offset)</summary>
	void WriteRegister(uint8_t addr, uint8_t value);

	/// <summary>Get audio state for debugger</summary>
	[[nodiscard]] LynxApuState& GetState() { return _state; }

	/// <summary>End-of-frame: flush remaining audio</summary>
	void EndFrame();

	void Serialize(Serializer& s) override;
};
