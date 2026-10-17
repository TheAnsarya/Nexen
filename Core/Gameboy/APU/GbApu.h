#pragma once
#include "pch.h"
#include <memory>
#include "Gameboy/APU/GbSquareChannel.h"
#include "Gameboy/APU/GbWaveChannel.h"
#include "Gameboy/APU/GbNoiseChannel.h"
#include "Utilities/Audio/blip_buf.h"
#include "Utilities/ISerializable.h"

class Emulator;
class Gameboy;
class SoundMixer;
class EmuSettings;
struct GameboyConfig;

/// <summary>
/// Game Boy Audio Processing Unit (APU).
/// Implements the original DMG/CGB sound hardware with 4 channels.
/// </summary>
/// <remarks>
/// Channels:
/// - Channel 1: Square wave with sweep and envelope
/// - Channel 2: Square wave with envelope (no sweep)
/// - Channel 3: Programmable waveform (32 4-bit samples)
/// - Channel 4: Pseudo-random noise with envelope
/// Uses blip_buf for band-limited synthesis and high-quality resampling.
/// CGB adds stereo panning and speed mode affects timing.
/// </remarks>
class GbApu : public ISerializable {
public:
	/// <summary>Output sample rate (96 KHz for high quality).</summary>
	static constexpr int SampleRate = 96000;

private:
	/// <summary>APU internal frequency (4 MHz from master clock).</summary>
	static constexpr int ApuFrequency = 1024 * 1024 * 4; // 4mhz

	/// <summary>Maximum samples per audio buffer.</summary>
	static constexpr int MaxSamples = 4000;

	/// <summary>Emulator instance reference.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Parent Game Boy console reference.</summary>
	Gameboy* _gameboy = nullptr;

	/// <summary>Emulator settings reference.</summary>
	EmuSettings* _settings = nullptr;

	/// <summary>Sound mixer for final output.</summary>
	SoundMixer* _soundMixer = nullptr;

	/// <summary>Square channel 1 with frequency sweep and volume envelope.</summary>
	unique_ptr<GbSquareChannel> _square1;

	/// <summary>Square channel 2 with volume envelope (no sweep).</summary>
	unique_ptr<GbSquareChannel> _square2;

	/// <summary>Wave channel with 32 programmable 4-bit samples.</summary>
	unique_ptr<GbWaveChannel> _wave;

	/// <summary>Noise channel with LFSR-based pseudo-random generation.</summary>
	unique_ptr<GbNoiseChannel> _noise;

	/// <summary>Output sample buffer.</summary>
	std::unique_ptr<int16_t[]> _soundBuffer;

	/// <summary>Blip buffer for left channel band-limited synthesis.</summary>
	blip_t* _leftChannel = nullptr;

	/// <summary>Blip buffer for right channel band-limited synthesis.</summary>
	blip_t* _rightChannel = nullptr;

	/// <summary>Previous left channel output for delta calculation.</summary>
	int16_t _prevLeftOutput = 0;

	/// <summary>Previous right channel output for delta calculation.</summary>
	int16_t _prevRightOutput = 0;

	/// <summary>Clock counter for timing.</summary>
	uint32_t _clockCounter = 0;

	/// <summary>Previous clock count for delta calculations.</summary>
	uint64_t _prevClockCount = 0;

	/// <summary>Counter for skipping first events after power-on.</summary>
	uint32_t _skipFirstEventCounter = 0;

	/// <summary>Cycle count when APU was powered on.</summary>
	uint64_t _powerOnCycle = 0;

	/// <summary>APU register state.</summary>
	GbApuState _state = {};

	/// <summary>Internal read for APU registers with side effects.</summary>
	/// <param name="addr">Register address ($FF10-$FF3F).</param>
	/// <returns>Register value.</returns>
	uint8_t InternalRead(uint16_t addr);

	/// <summary>Internal read for CGB-specific APU registers.</summary>
	/// <param name="addr">CGB register address.</param>
	/// <returns>Register value.</returns>
	uint8_t InternalReadCgbRegister(uint16_t addr);

	/// <summary>Updates audio output based on current channel states.</summary>
	/// <param name="cfg">Game Boy configuration for volume/panning settings.</param>
	void UpdateOutput(GameboyConfig& cfg);

public:
	/// <summary>Constructs the Game Boy APU.</summary>
	GbApu();

	/// <summary>Destroys the APU and releases blip buffers.</summary>
	virtual ~GbApu();

	/// <summary>Initializes the APU with system references.</summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="gameboy">Parent Game Boy console.</param>
	void Init(Emulator* emu, Gameboy* gameboy);

	/// <summary>Gets complete APU state for debugging.</summary>
	/// <returns>Debug state snapshot of all APU channels and registers.</returns>
	GbApuDebugState GetState();

	/// <summary>Checks if current cycle is odd (for APU timing quirks).</summary>
	/// <returns>True if odd APU cycle.</returns>
	bool IsOddApuCycle();

	/// <summary>Gets elapsed APU cycles since power-on.</summary>
	/// <returns>Total APU cycle count.</returns>
	uint64_t GetElapsedApuCycles();

	/// <summary>Runs APU to catch up with current cycle count.</summary>
	void Run();

	/// <summary>Outputs queued audio samples to the sound mixer.</summary>
	void PlayQueuedAudio();

	/// <summary>Gets sound samples from blip buffers.</summary>
	/// <param name="samples">Output pointer to sample buffer.</param>
	/// <param name="sampleCount">Output sample count.</param>
	void GetSoundSamples(int16_t*& samples, uint32_t& sampleCount);

	/// <summary>Clocks the frame sequencer for length/envelope/sweep timing.</summary>
	/// <remarks>Called at 512 Hz from the DIV timer.</remarks>
	void ClockFrameSequencer();

	/// <summary>Reads APU register without side effects (for debugging).</summary>
	/// <param name="addr">Register address.</param>
	/// <returns>Register value.</returns>
	uint8_t Peek(uint16_t addr);

	/// <summary>Reads APU register with side effects.</summary>
	/// <param name="addr">Register address ($FF10-$FF3F).</param>
	/// <returns>Register value.</returns>
	uint8_t Read(uint16_t addr);

	/// <summary>Writes to APU register.</summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void Write(uint16_t addr, uint8_t value);

	/// <summary>Reads CGB APU register without side effects.</summary>
	/// <param name="addr">CGB register address.</param>
	/// <returns>Register value.</returns>
	uint8_t PeekCgbRegister(uint16_t addr);

	/// <summary>Reads CGB APU register with side effects.</summary>
	/// <param name="addr">CGB register address.</param>
	/// <returns>Register value.</returns>
	uint8_t ReadCgbRegister(uint16_t addr);

	/// <summary>Processes length enable flag writes with proper timing behavior.</summary>
	/// <typeparam name="T">Length counter type.</typeparam>
	/// <param name="value">Written value containing length enable bit.</param>
	/// <param name="length">Length counter to update.</param>
	/// <param name="lengthEnabled">Length enabled flag to update.</param>
	/// <param name="enabled">Channel enabled flag to potentially clear.</param>
	template <typename T>
	void ProcessLengthEnableFlag(uint8_t value, T& length, bool& lengthEnabled, bool& enabled);

	/// <summary>Serializes APU state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override;
};
