#pragma once
#include "pch.h"
#include "NES/NesConsole.h"
#include "NES/NesConstants.h"
#include "NES/APU/ApuTimer.h"
#include "NES/APU/ApuLengthCounter.h"
#include "NES/INesMemoryHandler.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

/// <summary>
/// NES APU triangle wave channel ($4008-$400B).
/// </summary>
/// <remarks>
/// **Waveform:**
/// The triangle channel produces a pseudo-triangle wave by stepping through
/// a 32-step sequence that ramps up 0-15 then down 15-0, creating a triangle shape.
/// Unlike pulse/noise, it has no volume control - it's either on at full volume or off.
///
/// **Output Sequence:**
/// ```
/// Step:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
/// Value: 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
///
/// Step: 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
/// Value: 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
/// ```
///
/// **Frequency:**
/// freq = CPU_clock / (32 * (period + 1))
/// Range: ~27.5 Hz to ~55.9 kHz (NTSC)
///
/// **Linear Counter:**
/// Unlike other channels, triangle uses a linear counter instead of envelope.
/// - 7-bit counter clocked by quarter-frame
/// - When 0, channel is silenced
/// - Control flag affects reload behavior
///
/// **High Frequency Filtering:**
/// Very high frequencies (period < 2) produce ultrasonic output that creates
/// audible "pops" due to DAC behavior. An option exists to silence these.
///
/// **Sound Character:**
/// Triangle waves sound smoother and more mellow than square waves.
/// Often used for bass lines and melodic content.
///
/// **Registers:**
/// - $4008: Linear counter control/reload value
/// - $400A: Timer low (period bits 0-7)
/// - $400B: Timer high (period bits 8-10) + length counter load
/// </remarks>
class TriangleChannel : public INesMemoryHandler, public ISerializable {
private:
	/// <summary>32-step triangle wave sequence (0-15-0).</summary>
	static constexpr uint8_t _sequence[32] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

	NesConsole* _console;
	ApuLengthCounter _lengthCounter;  ///< Length counter (automatic silencing)
	ApuTimer _timer;                  ///< Frequency timer

	uint8_t _linearCounter = 0;       ///< Current linear counter value (0-127)
	uint8_t _linearCounterReload = 0; ///< Linear counter reload value
	bool _linearReloadFlag = false;   ///< Linear counter reload flag
	bool _linearControlFlag = false;  ///< Linear counter control (also halts length)

	uint8_t _sequencePosition = 0;    ///< Current position in 32-step sequence

public:
	/// <summary>Constructs triangle channel.</summary>
	/// <param name="console">Parent NES console.</param>
	TriangleChannel(NesConsole* console) : _lengthCounter(AudioChannel::Triangle, console), _timer(AudioChannel::Triangle, console->GetSoundMixer()) {
		_console = console;
	}

	/// <summary>
	/// Runs triangle channel to target CPU cycle.
	/// </summary>
	/// <param name="targetCycle">CPU cycle to run to.</param>
	/// <remarks>
	/// Advances sequencer when both length counter and linear counter are non-zero.
	/// High-frequency filtering option silences periods < 2 to avoid pops.
	/// </remarks>
	void Run(uint32_t targetCycle) {
		while (_timer.Run(targetCycle)) {
			// The sequencer is clocked by the timer as long as both the linear counter and the length counter are nonzero.
			if (_lengthCounter.GetStatus() && _linearCounter > 0) {
				_sequencePosition = (_sequencePosition + 1) & 0x1F;

				if (_timer.GetPeriod() >= 2 || !_console->GetNesConfig().SilenceTriangleHighFreq) {
					// Disabling the triangle channel when period is < 2 removes "pops" in the audio that are caused by the ultrasonic frequencies
					// This is less "accurate" in terms of emulation, so this is an option (disabled by default)
					_timer.AddOutput(_sequence[_sequencePosition]);
				}
			}
		}
	}

	/// <summary>Resets triangle channel to initial state.</summary>
	/// <param name="softReset">True for soft reset, false for hard reset.</param>
	void Reset(bool softReset) {
		_timer.Reset(softReset);
		_lengthCounter.Reset(softReset);

		_linearCounter = 0;
		_linearCounterReload = 0;
		_linearReloadFlag = false;
		_linearControlFlag = false;

		_sequencePosition = 0;
	}

	/// <summary>Serializes triangle channel state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_linearCounter);
		SV(_linearCounterReload);
		SV(_linearReloadFlag);
		SV(_linearControlFlag);
		SV(_sequencePosition);
		SV(_timer);
		SV(_lengthCounter);
	}

	/// <summary>Gets memory ranges handled by this channel.</summary>
	/// <param name="ranges">Range collection to populate.</param>
	void GetMemoryRanges(MemoryRanges& ranges) override {
		ranges.AddHandler(MemoryOperation::Write, 0x4008, 0x400B);
	}

	/// <summary>Handles register writes ($4008-$400B).</summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteRam(uint16_t addr, uint8_t value) override {
		_console->GetApu()->Run();

		switch (addr & 0x03) {
			case 0: // $4008: Linear counter control
				_linearControlFlag = (value & 0x80) == 0x80;
				_linearCounterReload = value & 0x7F;
				_lengthCounter.InitializeLengthCounter(_linearControlFlag);
				break;

			case 2: // $400A: Timer low
				_timer.SetPeriod((_timer.GetPeriod() & 0xFF00) | value);
				break;

			case 3: // $400B: Timer high + length counter load
				_lengthCounter.LoadLengthCounter(value >> 3);
				_timer.SetPeriod((_timer.GetPeriod() & 0xFF) | ((value & 0x07) << 8));
				// Side effect: Sets the linear counter reload flag
				_linearReloadFlag = true;
				break;
		}
	}

	/// <summary>
	/// Advances linear counter by one quarter-frame clock.
	/// </summary>
	/// <remarks>
	/// If reload flag is set, loads counter from reload value.
	/// Otherwise decrements counter if > 0.
	/// Clears reload flag if control flag is clear.
	/// </remarks>
	void TickLinearCounter() {
		if (_linearReloadFlag) {
			_linearCounter = _linearCounterReload;
		} else if (_linearCounter > 0) {
			_linearCounter--;
		}

		if (!_linearControlFlag) {
			_linearReloadFlag = false;
		}
	}

	/// <summary>Advances length counter by one half-frame clock.</summary>
	void TickLengthCounter() {
		_lengthCounter.TickLengthCounter();
	}

	/// <summary>Reloads length counter from pending value.</summary>
	void ReloadLengthCounter() {
		_lengthCounter.ReloadCounter();
	}

	/// <summary>Resets cycle counter at frame boundary.</summary>
	void EndFrame() {
		_timer.EndFrame();
	}

	/// <summary>Sets channel enable state.</summary>
	/// <param name="enabled">True to enable, false to disable.</param>
	void SetEnabled(bool enabled) {
		_lengthCounter.SetEnabled(enabled);
	}

	/// <summary>Gets channel active status.</summary>
	/// <returns>True if length counter > 0.</returns>
	bool GetStatus() {
		return _lengthCounter.GetStatus();
	}

	/// <summary>Gets current output value.</summary>
	/// <returns>Last output (0-15).</returns>
	uint8_t GetOutput() {
		return _timer.GetLastOutput();
	}

	/// <summary>Gets channel state for debugging.</summary>
	/// <returns>Snapshot of all channel parameters.</returns>
	ApuTriangleState GetState() {
		ApuTriangleState state;
		state.Enabled = _lengthCounter.IsEnabled();
		state.Frequency = NesConstants::GetClockRate(NesApu::GetApuRegion(_console)) / 32.0 / (_timer.GetPeriod() + 1);
		state.LengthCounter = _lengthCounter.GetState();
		state.OutputVolume = _timer.GetLastOutput();
		state.Period = _timer.GetPeriod();
		state.Timer = _timer.GetTimer();
		state.SequencePosition = _sequencePosition;
		state.LinearCounterReload = _linearCounterReload;
		state.LinearCounter = _linearCounter;
		state.LinearReloadFlag = _linearReloadFlag;
		return state;
	}

	/// <summary>Reads from register (returns 0, write-only).</summary>
	/// <param name="addr">Register address.</param>
	/// <returns>0 (registers are write-only).</returns>
	uint8_t ReadRam(uint16_t addr) override {
		return 0;
	}
};