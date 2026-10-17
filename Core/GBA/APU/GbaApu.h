#pragma once
#include "pch.h"
#include <memory>
#include "GBA/GbaTypes.h"
#include "GBA/APU/GbaApuFifo.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Audio/OnePoleLowPassFilter.h"

class Emulator;
class GbaConsole;
class GbaDmaController;
class GbaMemoryManager;

class GbaSquareChannel;
class GbaNoiseChannel;
class GbaWaveChannel;

class EmuSettings;
class SoundMixer;

/// <summary>
/// Game Boy Advance Audio Processing Unit.
/// Combines Game Boy-compatible channels with GBA-specific Direct Sound (DMA audio) FIFOs.
/// </summary>
/// <remarks>
/// The GBA APU provides:
/// - 4 legacy Game Boy channels (2 square, wave, noise) with enhanced features
/// - 2 Direct Sound FIFO channels for 8-bit PCM playback via DMA
/// - Hardware mixing with per-channel volume and panning
/// - Bias level adjustment for DC offset correction
/// Sample rate is variable based on timer frequency for FIFO channels.
/// </remarks>
class GbaApu final : public ISerializable {
	/// <summary>Maximum internal sample rate (256 KHz) for high-quality resampling.</summary>
	static constexpr int MaxSampleRate = 256 * 1024;

	/// <summary>Maximum samples per buffer based on max rate and 60fps minimum.</summary>
	static constexpr int MaxSamples = MaxSampleRate * 8 / 60;

private:
	/// <summary>Emulator instance reference.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Parent GBA console reference.</summary>
	GbaConsole* _console = nullptr;

	/// <summary>DMA controller for FIFO audio transfers.</summary>
	GbaDmaController* _dmaController = nullptr;

	/// <summary>Memory manager for register access.</summary>
	GbaMemoryManager* _memoryManager = nullptr;

	/// <summary>Emulator settings reference.</summary>
	EmuSettings* _settings = nullptr;

	/// <summary>Sound mixer for final output.</summary>
	SoundMixer* _soundMixer = nullptr;

	/// <summary>Square wave channel 1 with sweep and envelope.</summary>
	unique_ptr<GbaSquareChannel> _square1;

	/// <summary>Square wave channel 2 with envelope (no sweep).</summary>
	unique_ptr<GbaSquareChannel> _square2;

	/// <summary>Programmable waveform channel with 32-sample wave RAM.</summary>
	unique_ptr<GbaWaveChannel> _wave;

	/// <summary>Pseudo-random noise channel with LFSR.</summary>
	unique_ptr<GbaNoiseChannel> _noise;

	/// <summary>Low-pass filter for left channel anti-aliasing.</summary>
	OnePoleLowPassFilter _filterL;

	/// <summary>Low-pass filter for right channel anti-aliasing.</summary>
	OnePoleLowPassFilter _filterR;

	/// <summary>APU register state.</summary>
	GbaApuState _state = {};

	/// <summary>Direct Sound FIFO channels A and B for DMA-driven PCM audio.</summary>
	GbaApuFifo _fifo[2] = {};

	/// <summary>Output sample buffer.</summary>
	std::unique_ptr<int16_t[]> _soundBuffer;

	/// <summary>Number of samples in current buffer.</summary>
	uint32_t _sampleCount = 0;

	/// <summary>Current right channel sample.</summary>
	int16_t _rightSample = 0;

	/// <summary>Current left channel sample.</summary>
	int16_t _leftSample = 0;

	/// <summary>Current sample rate (varies with timer settings).</summary>
	uint32_t _sampleRate = 32 * 1024;

	/// <summary>Cycle count when APU was powered on.</summary>
	uint64_t _powerOnCycle = 0;

	/// <summary>Previous clock count for delta calculations.</summary>
	uint64_t _prevClockCount = 0;

	/// <summary>Bitmask of currently enabled channels (for optimized run dispatch).</summary>
	uint8_t _enabledChannels = 0;

	/// <summary>Function pointer type for channel-specific run functions.</summary>
	typedef void (GbaApu::*Func)();

	/// <summary>Lookup table of run functions indexed by enabled channel bitmask.</summary>
	Func _runFunc[16] = {};

	/// <summary>Clocks the frame sequencer for length/envelope/sweep timing.</summary>
	void ClockFrameSequencer();

	/// <summary>Updates the sample rate based on current timer configuration.</summary>
	void UpdateSampleRate();

	/// <summary>Template-optimized run function for specific channel combinations.</summary>
	/// <typeparam name="sq1Enabled">Whether square 1 channel is active.</typeparam>
	/// <typeparam name="sq2Enabled">Whether square 2 channel is active.</typeparam>
	/// <typeparam name="waveEnabled">Whether wave channel is active.</typeparam>
	/// <typeparam name="noiseEnabled">Whether noise channel is active.</typeparam>
	template <bool sq1Enabled, bool sq2Enabled, bool waveEnabled, bool noiseEnabled>
	void InternalRun();

public:
	/// <summary>Constructs the GBA APU.</summary>
	GbaApu();

	/// <summary>Destroys the APU and releases resources.</summary>
	~GbaApu();

	/// <summary>Initializes the APU with system references.</summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="console">Parent GBA console.</param>
	/// <param name="dmaController">DMA controller for FIFO transfers.</param>
	/// <param name="memoryManager">Memory manager for register access.</param>
	void Init(Emulator* emu, GbaConsole* console, GbaDmaController* dmaController, GbaMemoryManager* memoryManager);

	/// <summary>Gets complete APU state for debugging.</summary>
	/// <returns>Debug state snapshot of all APU channels and registers.</returns>
	GbaApuDebugState GetState();

	/// <summary>Runs APU to catch up with current cycle count.</summary>
	/// <remarks>Uses function pointer dispatch for channel-optimized execution.</remarks>
	__forceinline void Run() {
		(this->*_runFunc[_enabledChannels])();
	}

	/// <summary>Outputs queued audio samples to the sound mixer.</summary>
	void PlayQueuedAudio();

	/// <summary>Reads an APU register.</summary>
	/// <param name="addr">Register address (0x04000060-0x040000A8).</param>
	/// <returns>Register value.</returns>
	uint8_t ReadRegister(uint32_t addr);

	/// <summary>Writes to an APU register.</summary>
	/// <param name="mode">Access mode (byte/halfword/word).</param>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteRegister(GbaAccessModeVal mode, uint32_t addr, uint8_t value);

	/// <summary>Clocks a FIFO channel when its associated timer overflows.</summary>
	/// <param name="timerIndex">Timer index (0 or 1) that triggered the clock.</param>
	void ClockFifo(uint8_t timerIndex);

	/// <summary>Updates the enabled channels bitmask and run function pointer.</summary>
	void UpdateEnabledChannels();

	/// <summary>Checks if current cycle is odd (for APU timing).</summary>
	/// <returns>True if odd APU cycle.</returns>
	bool IsOddApuCycle();

	/// <summary>Gets elapsed APU cycles since power-on.</summary>
	/// <returns>Total APU cycle count.</returns>
	uint64_t GetElapsedApuCycles();

	/// <summary>Serializes APU state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override;
};