#pragma once

#include "pch.h"
#include "Utilities/ISerializable.h"
#include "NES/INesMemoryHandler.h"
#include "NES/NesTypes.h"

class NesConsole;
class SquareChannel;
class TriangleChannel;
class NoiseChannel;
class DeltaModulationChannel;
class ApuFrameCounter;
class NesSoundMixer;
class EmuSettings;

enum class FrameType;
enum class ConsoleRegion;

/// <summary>
/// NES Audio Processing Unit (APU) - Ricoh 2A03/2A07 sound generator.
/// Generates audio through 5 channels: 2 pulse wave, 1 triangle wave, 1 noise, and 1 DMC.
/// Implements the memory-mapped I/O interface for APU registers ($4000-$4017).
/// </summary>
/// <remarks>
/// The APU runs at CPU clock speed but generates samples at a fixed rate.
/// Frame counter provides timing for length counters and envelope generators.
/// Supports expansion audio from mappers (VRC6, VRC7, MMC5, Namco 163, Sunsoft 5B, FDS).
/// </remarks>
class NesApu : public ISerializable, public INesMemoryHandler {
	/// <summary>Allows frame counter direct access for timing callbacks.</summary>
	friend ApuFrameCounter;

private:
	/// <summary>Whether the APU is enabled (controlled by $4015).</summary>
	bool _apuEnabled;

	/// <summary>Flag indicating pending audio processing work.</summary>
	bool _needToRun;

	/// <summary>CPU cycle count at last APU update.</summary>
	uint32_t _previousCycle;

	/// <summary>Current CPU cycle count for timing calculations.</summary>
	uint32_t _currentCycle;

	/// <summary>Pulse wave channel 1 ($4000-$4003) - variable duty cycle square wave.</summary>
	unique_ptr<SquareChannel> _square1;

	/// <summary>Pulse wave channel 2 ($4004-$4007) - variable duty cycle square wave.</summary>
	unique_ptr<SquareChannel> _square2;

	/// <summary>Triangle wave channel ($4008-$400B) - pseudo-triangle waveform.</summary>
	unique_ptr<TriangleChannel> _triangle;

	/// <summary>Noise channel ($400C-$400F) - pseudo-random noise generator.</summary>
	unique_ptr<NoiseChannel> _noise;

	/// <summary>Delta Modulation Channel ($4010-$4013) - 1-bit DPCM sample playback.</summary>
	unique_ptr<DeltaModulationChannel> _dmc;

	/// <summary>Frame counter for APU timing (length counters, envelopes, sweeps).</summary>
	unique_ptr<ApuFrameCounter> _frameCounter;

	/// <summary>Reference to parent NES console.</summary>
	NesConsole* _console;

	/// <summary>Sound mixer for combining channel outputs.</summary>
	NesSoundMixer* _mixer;

	/// <summary>Emulator settings reference.</summary>
	EmuSettings* _settings;

	/// <summary>Console region (NTSC/PAL/Dendy) affects timing and pitch tables.</summary>
	ConsoleRegion _region;

private:
	/// <summary>Checks if APU processing is needed at the given cycle.</summary>
	/// <param name="currentCycle">Current CPU cycle count.</param>
	/// <returns>True if APU needs to process audio at this cycle.</returns>
	__forceinline bool NeedToRun(uint32_t currentCycle);

	/// <summary>Handles frame counter tick events for length/envelope/sweep clocking.</summary>
	/// <param name="type">Type of frame counter tick (quarter/half frame).</param>
	void FrameCounterTick(FrameType type);

	/// <summary>Gets the APU status register ($4015) value.</summary>
	/// <returns>Bit flags indicating channel enable status and DMC/frame IRQ flags.</returns>
	uint8_t GetStatus();

public:
	/// <summary>Constructs the APU and all audio channels.</summary>
	/// <param name="console">Parent NES console instance.</param>
	NesApu(NesConsole* console);

	/// <summary>Destroys the APU and releases channel resources.</summary>
	~NesApu();

	/// <summary>Serializes APU state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override;

	/// <summary>Resets the APU to initial state.</summary>
	/// <param name="softReset">True for soft reset, false for power-on reset.</param>
	void Reset(bool softReset);

	/// <summary>Sets the console region for timing adjustments.</summary>
	/// <param name="region">Console region (NTSC/PAL/Dendy).</param>
	/// <param name="forceInit">Force re-initialization even if region unchanged.</param>
	void SetRegion(ConsoleRegion region, bool forceInit = false);

	/// <summary>Reads APU register with side effects.</summary>
	/// <param name="addr">Address in $4000-$4017 range.</param>
	/// <returns>Register value with appropriate side effects applied.</returns>
	uint8_t ReadRam(uint16_t addr) override;

	/// <summary>Reads APU register without side effects (for debugging).</summary>
	/// <param name="addr">Address in $4000-$4017 range.</param>
	/// <returns>Register value without triggering side effects.</returns>
	uint8_t PeekRam(uint16_t addr) override;

	/// <summary>Writes to APU register.</summary>
	/// <param name="addr">Address in $4000-$4017 range.</param>
	/// <param name="value">Value to write to the register.</param>
	void WriteRam(uint16_t addr, uint8_t value) override;

	/// <summary>Reports the memory ranges handled by the APU.</summary>
	/// <param name="ranges">Memory ranges structure to populate.</param>
	void GetMemoryRanges(MemoryRanges& ranges) override;

	/// <summary>Gets complete APU state for debugging.</summary>
	/// <returns>Snapshot of all APU channel states.</returns>
	ApuState GetState();

	/// <summary>Executes APU for one CPU instruction's worth of cycles.</summary>
	void Exec();

	/// <summary>Processes a single CPU clock cycle.</summary>
	void ProcessCpuClock();

	/// <summary>Runs APU to catch up with current CPU cycle.</summary>
	void Run();

	/// <summary>Finalizes audio for the current frame.</summary>
	void EndFrame();

	/// <summary>Adds expansion audio delta for mapper-provided audio channels.</summary>
	/// <param name="channel">Expansion audio channel identifier.</param>
	/// <param name="delta">Audio sample delta value.</param>
	void AddExpansionAudioDelta(AudioChannel channel, int16_t delta);

	/// <summary>Enables or disables the APU.</summary>
	/// <param name="enabled">True to enable, false to disable.</param>
	void SetApuStatus(bool enabled);

	/// <summary>Checks if the APU is currently enabled.</summary>
	/// <returns>True if APU is enabled.</returns>
	bool IsApuEnabled();

	/// <summary>Gets the APU region for a console (static helper).</summary>
	/// <param name="console">NES console to query.</param>
	/// <returns>Console region affecting APU timing.</returns>
	static ConsoleRegion GetApuRegion(NesConsole* console);

	/// <summary>Gets the current DMC sample read address.</summary>
	/// <returns>Address where DMC is reading sample data.</returns>
	uint16_t GetDmcReadAddress();
	void SetDmcReadBuffer(uint8_t value);
	void SetNeedToRun();
};