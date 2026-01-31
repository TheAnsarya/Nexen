#pragma once
#include "pch.h"
#include "NES/APU/NesApu.h"
#include "NES/APU/ApuTimer.h"
#include "NES/APU/ApuEnvelope.h"
#include "NES/NesConstants.h"
#include "NES/NesConsole.h"
#include "NES/INesMemoryHandler.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

/// <summary>
/// NES APU noise channel ($400C-$400F).
/// </summary>
/// <remarks>
/// **Operation:**
/// The noise channel generates pseudo-random noise using a 15-bit Linear
/// Feedback Shift Register (LFSR). The output is either 0 or the current
/// envelope volume based on bit 0 of the shift register.
///
/// **Shift Register:**
/// The LFSR generates pseudo-random sequences:
/// - Feedback = bit 0 XOR bit N (N=1 for normal, N=6 for short mode)
/// - Register shifts right, feedback enters bit 14
/// - Output is muted when bit 0 = 1
///
/// **Modes:**
/// - Normal mode (bit 7 = 0): XOR bits 0 and 1, period ~32767
///   Creates white noise / "hiss" sound
/// - Short mode (bit 7 = 1): XOR bits 0 and 6, period 93
///   Creates more metallic / periodic sound
///
/// **Period Table (NTSC):**
/// ```
/// Index: 0   1   2   3   4   5   6    7    8    9   10   11   12    13    14    15
/// Value: 4   8  16  32  64  96 128  160  202  254  380  508  762  1016  2034  4068
/// ```
///
/// **Frequency Calculation:**
/// freq = CPU_clock / period_value
/// Range: ~440 Hz to ~447 kHz (NTSC)
///
/// **Sound Character:**
/// - White noise at high frequencies
/// - Pitched noise at lower frequencies
/// - Short mode creates "metallic" tone useful for snares
///
/// **Registers:**
/// - $400C: Envelope control (volume, constant/decay, loop)
/// - $400E: Timer period index (bits 0-3) + mode flag (bit 7)
/// - $400F: Length counter load (bits 3-7)
/// </remarks>
class NoiseChannel : public INesMemoryHandler, public ISerializable {
private:
	/// <summary>NTSC period lookup table (16 entries).</summary>
	static constexpr uint16_t _noisePeriodLookupTableNtsc[16] = {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};

	/// <summary>PAL period lookup table (16 entries).</summary>
	static constexpr uint16_t _noisePeriodLookupTablePal[16] = {4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708, 944, 1890, 3778};

	NesConsole* _console = nullptr;
	ApuEnvelope _envelope;       ///< Volume envelope generator
	ApuTimer _timer;             ///< Frequency timer

	uint16_t _shiftRegister = 1; ///< 15-bit LFSR (initialized to 1 on power-up)
	bool _modeFlag = false;      ///< False = normal (long), true = short (metallic)

	/// <summary>Checks if channel output is muted.</summary>
	/// <returns>True if shift register bit 0 is 1 (muted).</returns>
	/// <remarks>
	/// The mixer receives the current envelope volume except when
	/// bit 0 of the shift register is set, or the length counter is zero.
	/// </remarks>
	bool IsMuted() {
		return (_shiftRegister & 0x01) == 0x01;
	}

public:
	/// <summary>Constructs noise channel.</summary>
	/// <param name="console">Parent NES console.</param>
	NoiseChannel(NesConsole* console) : _envelope(AudioChannel::Noise, console), _timer(AudioChannel::Noise, console->GetSoundMixer()) {
		_console = console;
	}

	/// <summary>
	/// Runs noise channel to target CPU cycle.
	/// </summary>
	/// <param name="targetCycle">CPU cycle to run to.</param>
	/// <remarks>
	/// Each timer clock:
	/// 1. Calculate feedback (bit 0 XOR bit 1 or 6)
	/// 2. Shift register right
	/// 3. Insert feedback at bit 14
	/// 4. Output envelope volume if bit 0 = 0, else 0
	/// </remarks>
	void Run(uint32_t targetCycle) {
		while (_timer.Run(targetCycle)) {
			// Feedback is calculated as the exclusive-OR of bit 0 and one other bit: bit 6 if Mode flag is set, otherwise bit 1.
			bool mode = _console->GetNesConfig().DisableNoiseModeFlag ? false : _modeFlag;

			uint16_t feedback = (_shiftRegister & 0x01) ^ ((_shiftRegister >> (mode ? 6 : 1)) & 0x01);
			_shiftRegister >>= 1;
			_shiftRegister |= (feedback << 14);

			if (IsMuted()) {
				_timer.AddOutput(0);
			} else {
				_timer.AddOutput(_envelope.GetVolume());
			}
		}
	}

	/// <summary>Advances envelope by one quarter-frame clock.</summary>
	void TickEnvelope() {
		_envelope.TickEnvelope();
	}

	/// <summary>Advances length counter by one half-frame clock.</summary>
	void TickLengthCounter() {
		_envelope.LengthCounter.TickLengthCounter();
	}

	/// <summary>Reloads length counter from pending value.</summary>
	void ReloadLengthCounter() {
		_envelope.LengthCounter.ReloadCounter();
	}

	/// <summary>Resets cycle counter at frame boundary.</summary>
	void EndFrame() {
		_timer.EndFrame();
	}

	/// <summary>Sets channel enable state.</summary>
	/// <param name="enabled">True to enable, false to disable.</param>
	void SetEnabled(bool enabled) {
		_envelope.LengthCounter.SetEnabled(enabled);
	}

	/// <summary>Gets channel active status.</summary>
	/// <returns>True if length counter > 0.</returns>
	bool GetStatus() {
		return _envelope.LengthCounter.GetStatus();
	}

	/// <summary>Resets noise channel to initial state.</summary>
	/// <param name="softReset">True for soft reset, false for hard reset.</param>
	void Reset(bool softReset) {
		_envelope.Reset(softReset);
		_timer.Reset(softReset);

		// Set initial period from index 0
		_timer.SetPeriod((NesApu::GetApuRegion(_console) == ConsoleRegion::Ntsc ? _noisePeriodLookupTableNtsc : _noisePeriodLookupTablePal)[0] - 1);
		_shiftRegister = 1;  // Power-up value
		_modeFlag = false;
	}

	/// <summary>Serializes noise channel state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_shiftRegister);
		SV(_modeFlag);
		SV(_envelope);
		SV(_timer);
	}

	/// <summary>Gets memory ranges handled by this channel.</summary>
	/// <param name="ranges">Range collection to populate.</param>
	void GetMemoryRanges(MemoryRanges& ranges) override {
		ranges.AddHandler(MemoryOperation::Write, 0x400C, 0x400F);
	}

	/// <summary>Handles register writes ($400C-$400F).</summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteRam(uint16_t addr, uint8_t value) override {
		_console->GetApu()->Run();

		switch (addr & 0x03) {
			case 0: // $400C: Envelope control
				_envelope.InitializeEnvelope(value);
				break;

			case 2: // $400E: Period + mode flag
				_timer.SetPeriod((NesApu::GetApuRegion(_console) == ConsoleRegion::Ntsc ? _noisePeriodLookupTableNtsc : _noisePeriodLookupTablePal)[value & 0x0F] - 1);
				_modeFlag = (value & 0x80) == 0x80;
				break;

			case 3: // $400F: Length counter load
				_envelope.LengthCounter.LoadLengthCounter(value >> 3);
				// The envelope is also restarted
				_envelope.ResetEnvelope();
				break;
		}
	}

	/// <summary>Gets current output value.</summary>
	/// <returns>Last output (0-15).</returns>
	uint8_t GetOutput() {
		return _timer.GetLastOutput();
	}

	/// <summary>Gets channel state for debugging.</summary>
	/// <returns>Snapshot of all channel parameters.</returns>
	ApuNoiseState GetState() {
		ApuNoiseState state;
		state.Enabled = _envelope.LengthCounter.IsEnabled();
		state.Envelope = _envelope.GetState();
		state.Frequency = (double)NesConstants::GetClockRate(NesApu::GetApuRegion(_console)) / (_timer.GetPeriod() + 1) / (_modeFlag ? 93 : 1);
		state.LengthCounter = _envelope.LengthCounter.GetState();
		state.ModeFlag = _modeFlag;
		state.OutputVolume = _timer.GetLastOutput();
		state.Period = _timer.GetPeriod();
		state.Timer = _timer.GetTimer();
		state.ShiftRegister = _shiftRegister;
		return state;
	}

	/// <summary>Reads from register (returns 0, write-only).</summary>
	/// <param name="addr">Register address.</param>
	/// <returns>0 (registers are write-only).</returns>
	uint8_t ReadRam(uint16_t addr) override {
		return 0;
	}
};