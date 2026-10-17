#pragma once
#include "pch.h"
#include <memory>
#include "WS/WsTypes.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Audio/OnePoleLowPassFilter.h"

class Emulator;
class WsConsole;
class WsMemoryManager;
class WsDmaController;
class WsApuCh1;
class WsApuCh2;
class WsApuCh3;
class WsApuCh4;
class WsHyperVoice;
class SoundMixer;

/// <summary>
/// WonderSwan Audio Processing Unit.
/// Implements the 4-channel wavetable synthesizer plus HyperVoice digital audio.
/// </summary>
/// <remarks>
/// The WonderSwan APU provides:
/// - 4 wavetable channels with 32 4-bit samples each
/// - Channel 2 can optionally output PCM voice samples
/// - Channel 3 can generate sweep effects
/// - Channel 4 can produce noise via LFSR
/// - HyperVoice: DMA-driven 8-bit PCM playback (WonderSwan Color only)
/// </remarks>
class WsApu final : public ISerializable {
private:
	/// <summary>APU output sample rate (24 KHz).</summary>
	static constexpr int ApuFrequency = 24000;

	/// <summary>Maximum samples per buffer.</summary>
	static constexpr int MaxSamples = 256;

private:
	/// <summary>APU register state.</summary>
	WsApuState _state = {};

	/// <summary>Emulator instance reference.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Parent WonderSwan console reference.</summary>
	WsConsole* _console = nullptr;

	/// <summary>Memory manager for wave RAM access.</summary>
	WsMemoryManager* _memoryManager = nullptr;

	/// <summary>DMA controller for HyperVoice transfers.</summary>
	WsDmaController* _dmaController = nullptr;

	/// <summary>Sound mixer for final output.</summary>
	SoundMixer* _soundMixer = nullptr;

	/// <summary>Wavetable channel 1 - basic wavetable playback.</summary>
	unique_ptr<WsApuCh1> _ch1;

	/// <summary>Wavetable channel 2 - wavetable with optional PCM voice mode.</summary>
	unique_ptr<WsApuCh2> _ch2;

	/// <summary>Wavetable channel 3 - wavetable with optional sweep.</summary>
	unique_ptr<WsApuCh3> _ch3;

	/// <summary>Wavetable channel 4 - wavetable with optional noise mode.</summary>
	unique_ptr<WsApuCh4> _ch4;

	/// <summary>HyperVoice DMA-driven PCM channel (WSC only).</summary>
	unique_ptr<WsHyperVoice> _hyperVoice;

	/// <summary>Low-pass filter for left channel.</summary>
	OnePoleLowPassFilter _filterL;

	/// <summary>Low-pass filter for right channel.</summary>
	OnePoleLowPassFilter _filterR;

	/// <summary>Output sample buffer.</summary>
	std::unique_ptr<int16_t[]> _soundBuffer;

	/// <summary>Clock counter for timing.</summary>
	uint32_t _clockCounter = 0;

	/// <summary>Number of samples in current buffer.</summary>
	uint16_t _sampleCount = 0;

	/// <summary>Updates mixed audio output from all channels.</summary>
	void UpdateOutput();

	/// <summary>Gets combined APU output for one stereo channel.</summary>
	/// <param name="forRight">True for right channel, false for left.</param>
	/// <returns>Mixed sample value.</returns>
	uint16_t GetApuOutput(bool forRight);

public:
	/// <summary>Constructs the WonderSwan APU.</summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="console">Parent WonderSwan console.</param>
	/// <param name="memoryManager">Memory manager for wave RAM.</param>
	/// <param name="dmaController">DMA controller for HyperVoice.</param>
	WsApu(Emulator* emu, WsConsole* console, WsMemoryManager* memoryManager, WsDmaController* dmaController);

	/// <summary>Destroys the APU and releases resources.</summary>
	~WsApu();

	/// <summary>Updates output when master volume setting changes.</summary>
	void ChangeMasterVolume();

	/// <summary>Outputs queued audio samples to the sound mixer.</summary>
	void PlayQueuedAudio();

	/// <summary>Writes a DMA sample to audio channel or HyperVoice.</summary>
	/// <param name="forHyperVoice">True for HyperVoice, false for channel 2 PCM.</param>
	/// <param name="sampleValue">8-bit sample value to write.</param>
	void WriteDma(bool forHyperVoice, uint8_t sampleValue);

	/// <summary>Gets mutable reference to APU state.</summary>
	/// <returns>Reference to APU register state.</returns>
	WsApuState& GetState() { return _state; }

	/// <summary>Gets the current internal master volume level.</summary>
	/// <returns>Master volume (0-15).</returns>
	uint8_t GetMasterVolume() { return _state.InternalMasterVolume; }

	/// <summary>Reads a sample from wave RAM.</summary>
	/// <param name="ch">Channel number (0-3).</param>
	/// <param name="pos">Sample position within wave table (0-31).</param>
	/// <returns>4-bit sample value.</returns>
	uint8_t ReadSample(uint8_t ch, uint8_t pos);

	/// <summary>Runs APU to catch up with current cycle count.</summary>
	void Run();

	/// <summary>Reads an APU I/O port.</summary>
	/// <param name="port">Port address ($80-$9F).</param>
	/// <returns>Port value.</returns>
	uint8_t Read(uint16_t port);

	/// <summary>Writes to an APU I/O port.</summary>
	/// <param name="port">Port address ($80-$9F).</param>
	/// <param name="value">Value to write.</param>
	void Write(uint16_t port, uint8_t value);

	/// <summary>Serializes APU state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override;
};