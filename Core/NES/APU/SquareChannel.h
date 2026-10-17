#pragma once
#include "pch.h"
#include "NES/APU/ApuEnvelope.h"
#include "NES/APU/ApuTimer.h"
#include "NES/APU/NesApu.h"
#include "NES/NesConstants.h"
#include "NES/NesConsole.h"
#include "NES/INesMemoryHandler.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

/// <summary>
/// NES APU pulse/square wave channel.
/// </summary>
/// <remarks>
/// **Overview:**
/// The NES has two pulse wave channels ($4000-$4003 and $4004-$4007).
/// Each produces a square wave with variable duty cycle, frequency,
/// and volume. They're the primary melodic voices of the NES.
///
/// **Duty Cycle Sequences:**
/// The duty cycle determines the waveform shape (ratio of high to low):
/// ```
/// Duty  Sequence        Ratio   Sound Character
/// 0     01111111        12.5%   Thin, reedy
/// 1     00111111        25%     Classic NES sound
/// 2     00001111        50%     Hollow, pure square
/// 3     11000000        25%     Same as 1, inverted (sounds identical)
/// ```
///
/// **Frequency:**
/// freq = CPU_clock / (16 * (period + 1))
/// Range: ~54 Hz to ~12.4 kHz (NTSC)
/// Periods < 8 are muted to prevent aliasing artifacts.
///
/// **Sweep Unit:**
/// Automatic frequency adjustment for portamento/vibrato effects:
/// - Period: Divider rate (1-8)
/// - Shift: Amount to shift period by (0-7)
/// - Negate: Add or subtract shifted value
/// - Channel 1 has different negation behavior than channel 2
///
/// **Muting Conditions:**
/// Channel is silenced when:
/// - Period < 8 (too high frequency)
/// - Sweep target period > $7FF (overflow)
/// - Length counter = 0
///
/// **MMC5 Variant:**
/// MMC5 expansion audio has two additional square channels.
/// These lack the sweep unit but are otherwise identical.
///
/// **Registers:**
/// - $4000/$4004: Duty, envelope control
/// - $4001/$4005: Sweep unit control
/// - $4002/$4006: Timer low (period bits 0-7)
/// - $4003/$4007: Timer high (period bits 8-10) + length counter load
/// </remarks>
class SquareChannel : public INesMemoryHandler, public ISerializable {
protected:
	/// <summary>
	/// Duty cycle sequences (4 patterns × 8 steps).
	/// Output is 1 when step value is 1, 0 otherwise.
	/// </summary>
	static constexpr uint8_t _dutySequences[4][8] = {
		{0, 0, 0, 0, 0, 0, 0, 1},  ///< 12.5% duty
		{0, 0, 0, 0, 0, 0, 1, 1},  ///< 25% duty
		{0, 0, 0, 0, 1, 1, 1, 1},  ///< 50% duty
		{1, 1, 1, 1, 1, 1, 0, 0}   ///< 25% duty (inverted)
	};

	NesConsole* _console = nullptr;
	ApuEnvelope _envelope;       ///< Volume envelope generator
	ApuTimer _timer;             ///< Frequency timer

	bool _isChannel1 = false;    ///< True for channel 1 ($4000-$4003)
	bool _isMmc5Square = false;  ///< True if MMC5 expansion channel

	uint8_t _duty = 0;           ///< Duty cycle selector (0-3)
	uint8_t _dutyPos = 0;        ///< Current position in duty sequence (0-7)

	// Sweep unit state
	bool _sweepEnabled = false;  ///< Sweep unit enabled
	uint8_t _sweepPeriod = 0;    ///< Sweep divider period (1-8)
	bool _sweepNegate = false;   ///< True = subtract, false = add
	uint8_t _sweepShift = 0;     ///< Shift amount (0-7)
	bool _reloadSweep = false;   ///< Reload sweep divider flag
	uint8_t _sweepDivider = 0;   ///< Sweep divider counter
	uint32_t _sweepTargetPeriod = 0;  ///< Calculated target period
	uint16_t _realPeriod = 0;    ///< Actual timer period

	/// <summary>Checks if channel output is muted.</summary>
	/// <returns>True if period < 8 or sweep target overflows.</returns>
	bool IsMuted() {
		// A period of t < 8, either set explicitly or via a sweep period update, silences the corresponding pulse channel.
		return _realPeriod < 8 || (!_sweepNegate && _sweepTargetPeriod > 0x7FF);
	}

	/// <summary>
	/// Initializes sweep unit from register value.
	/// </summary>
	/// <param name="regValue">Sweep register value ($4001/$4005).</param>
	virtual void InitializeSweep(uint8_t regValue) {
		_sweepEnabled = (regValue & 0x80) == 0x80;
		_sweepNegate = (regValue & 0x08) == 0x08;

		// The divider's period is set to P + 1
		_sweepPeriod = ((regValue & 0x70) >> 4) + 1;
		_sweepShift = (regValue & 0x07);

		UpdateTargetPeriod();

		// Side effects: Sets the reload flag
		_reloadSweep = true;
	}

	/// <summary>
	/// Recalculates sweep target period.
	/// </summary>
	/// <remarks>
	/// Target = Period ± (Period >> Shift)
	/// Channel 1 uses one's complement for negation (subtracts extra 1).
	/// Channel 2 uses two's complement.
	/// </remarks>
	void UpdateTargetPeriod() {
		uint16_t shiftResult = (_realPeriod >> _sweepShift);
		if (_sweepNegate) {
			_sweepTargetPeriod = _realPeriod - shiftResult;
			if (_isChannel1) {
				// As a result, a negative sweep on pulse channel 1 will subtract the shifted period value minus 1
				_sweepTargetPeriod--;
			}
		} else {
			_sweepTargetPeriod = _realPeriod + shiftResult;
		}
	}

	/// <summary>
	/// Sets timer period and updates sweep target.
	/// </summary>
	/// <param name="newPeriod">New period value (11-bit).</param>
	void SetPeriod(uint16_t newPeriod) {
		_realPeriod = newPeriod;
		_timer.SetPeriod((_realPeriod * 2) + 1);
		UpdateTargetPeriod();
	}

	/// <summary>
	/// Updates output based on current duty step and envelope.
	/// </summary>
	void UpdateOutput() {
		if (IsMuted()) {
			_timer.AddOutput(0);
		} else {
			_timer.AddOutput(_dutySequences[_duty][_dutyPos] * _envelope.GetVolume());
		}
	}

public:
	SquareChannel(AudioChannel channel, NesConsole* console, bool isChannel1) : _envelope(channel, console), _timer(channel, console->GetSoundMixer()) {
		_console = console;
		_isChannel1 = isChannel1;
	}

	void Run(uint32_t targetCycle) {
		while (_timer.Run(targetCycle)) {
			_dutyPos = (_dutyPos - 1) & 0x07;
			UpdateOutput();
		}
	}

	void Reset(bool softReset) {
		_envelope.Reset(softReset);
		_timer.Reset(softReset);

		_duty = 0;
		_dutyPos = 0;

		_realPeriod = 0;

		_sweepEnabled = false;
		_sweepPeriod = 0;
		_sweepNegate = false;
		_sweepShift = 0;
		_reloadSweep = false;
		_sweepDivider = 0;
		_sweepTargetPeriod = 0;
		UpdateTargetPeriod();
	}

	void Serialize(Serializer& s) override {
		SV(_realPeriod);
		SV(_duty);
		SV(_dutyPos);
		SV(_sweepEnabled);
		SV(_sweepPeriod);
		SV(_sweepNegate);
		SV(_sweepShift);
		SV(_reloadSweep);
		SV(_sweepDivider);
		SV(_sweepTargetPeriod);
		SV(_timer);
		SV(_envelope);
	}

	void GetMemoryRanges(MemoryRanges& ranges) override {
		if (_isChannel1) {
			ranges.AddHandler(MemoryOperation::Write, 0x4000, 0x4003);
		} else {
			ranges.AddHandler(MemoryOperation::Write, 0x4004, 0x4007);
		}
	}

	void WriteRam(uint16_t addr, uint8_t value) override {
		_console->GetApu()->Run();
		switch (addr & 0x03) {
			case 0: // 4000 & 4004
				_envelope.InitializeEnvelope(value);

				_duty = (value & 0xC0) >> 6;
				if (_console->GetNesConfig().SwapDutyCycles) {
					_duty = ((_duty & 0x02) >> 1) | ((_duty & 0x01) << 1);
				}
				break;

			case 1: // 4001 & 4005
				InitializeSweep(value);
				break;

			case 2: // 4002 & 4006
				SetPeriod((_realPeriod & 0x0700) | value);
				break;

			case 3: // 4003 & 4007
				_envelope.LengthCounter.LoadLengthCounter(value >> 3);

				SetPeriod((_realPeriod & 0xFF) | ((value & 0x07) << 8));

				// The sequencer is restarted at the first value of the current sequence.
				_dutyPos = 0;

				// The envelope is also restarted.
				_envelope.ResetEnvelope();
				break;
		}

		if (!_isMmc5Square) {
			UpdateOutput();
		}
	}

	void TickSweep() {
		_sweepDivider--;
		if (_sweepDivider == 0) {
			if (_sweepShift > 0 && _sweepEnabled && _realPeriod >= 8 && _sweepTargetPeriod <= 0x7FF) {
				SetPeriod(_sweepTargetPeriod);
			}
			_sweepDivider = _sweepPeriod;
		}

		if (_reloadSweep) {
			_sweepDivider = _sweepPeriod;
			_reloadSweep = false;
		}
	}

	void TickEnvelope() {
		_envelope.TickEnvelope();
	}

	void TickLengthCounter() {
		_envelope.LengthCounter.TickLengthCounter();
	}

	void ReloadLengthCounter() {
		_envelope.LengthCounter.ReloadCounter();
	}

	void EndFrame() {
		_timer.EndFrame();
	}

	void SetEnabled(bool enabled) {
		_envelope.LengthCounter.SetEnabled(enabled);
	}

	bool GetStatus() {
		return _envelope.LengthCounter.GetStatus();
	}

	uint8_t GetOutput() {
		return _timer.GetLastOutput();
	}

	ApuSquareState GetState() {
		ApuSquareState state;
		state.Duty = _duty;
		state.DutyPosition = _dutyPos;
		state.Enabled = _envelope.LengthCounter.IsEnabled();
		state.Envelope = _envelope.GetState();
		state.Frequency = NesConstants::GetClockRate(NesApu::GetApuRegion(_console)) / 16.0 / (_realPeriod + 1);
		state.LengthCounter = _envelope.LengthCounter.GetState();
		state.OutputVolume = _timer.GetLastOutput();
		state.Period = _realPeriod;
		state.Timer = _timer.GetTimer() / 2;
		state.SweepEnabled = _sweepEnabled;
		state.SweepNegate = _sweepNegate;
		state.SweepPeriod = _sweepPeriod;
		state.SweepShift = _sweepShift;
		return state;
	}

	uint8_t ReadRam(uint16_t addr) override {
		return 0;
	}
};