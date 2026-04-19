#pragma once
#include "pch.h"
#include "Utilities/Serializer.h"

/// <summary>
/// SPC700 timer implementation for the SNES audio subsystem.
/// </summary>
/// <typeparam name="rate">Clock divider rate (128 for timers 0-1, 16 for timer 2).</typeparam>
/// <remarks>
/// The SPC700 has three timers:
/// - Timer 0 and 1: 8KHz base rate (128 divider from ~1MHz master clock)
/// - Timer 2: 64KHz base rate (16 divider)
/// 
/// Each timer has a programmable target value (1-256, where 0=256).
/// The output counter increments each time the internal counter reaches the target,
/// wrapping at 15 (4-bit counter). Reading the output resets it to 0.
/// </remarks>
template <uint8_t rate>
class SpcTimer {
private:
	/// <summary>Timer enabled via CONTROL register.</summary>
	bool _enabled = false;

	/// <summary>Global timer enable (via TEST register).</summary>
	bool _timersEnabled = true;

	/// <summary>
	/// Output counter (0-15), increments when stage2 reaches target.
	/// Power-on value is $F, reset value is $0.
	/// </summary>
	uint8_t _output = 0x0F;

	/// <summary>Stage 0: Sub-divider accumulator (counts up to rate).</summary>
	uint8_t _stage0 = 0;

	/// <summary>Stage 1: Toggle state (0/1), used for edge detection.</summary>
	uint8_t _stage1 = 0;

	/// <summary>Previous stage 1 value for edge detection.</summary>
	uint8_t _prevStage1 = 0;

	/// <summary>Stage 2: Main timer counter (0 to target-1).</summary>
	uint8_t _stage2 = 0;

	/// <summary>Target value (1-256, where 0 means 256).</summary>
	uint8_t _target = 0;

	/// <summary>
	/// Clocks the timer on 1→0 edge transitions of stage1.
	/// </summary>
	void ClockTimer() {
		uint8_t currentState = _stage1;
		if (!_timersEnabled) {
			// All timers are disabled via TEST register
			currentState = 0;
		}

		uint8_t prevState = _prevStage1;
		_prevStage1 = currentState;
		if (!_enabled || !prevState || currentState) {
			// Only clock on 1->0 transitions, when the timer is enabled
			return;
		}

		if (++_stage2 == _target) {
			_stage2 = 0;
			_output++;
		}
	}

public:
	/// <summary>
	/// Resets the timer output counter to 0 (soft reset behavior).
	/// </summary>
	void Reset() {
		_output = 0;
	}

	/// <summary>
	/// Enables or disables this timer via CONTROL register ($F1).
	/// </summary>
	/// <param name="enabled">True to enable the timer.</param>
	/// <remarks>
	/// Enabling a disabled timer resets stage2 and output to 0.
	/// </remarks>
	void SetEnabled(bool enabled) {
		if (!_enabled && enabled) {
			_stage2 = 0;
			_output = 0;
		}
		_enabled = enabled;
	}

	/// <summary>
	/// Sets the global timer enable state via TEST register ($F0).
	/// </summary>
	/// <param name="enabled">True to enable all timers globally.</param>
	void SetGlobalEnabled(bool enabled) {
		_timersEnabled = enabled;
		ClockTimer();
	}

	/// <summary>
	/// Advances the timer by the specified number of clock cycles.
	/// </summary>
	/// <param name="step">Number of SPC clock cycles elapsed.</param>
	void Run(uint8_t step) {
		_stage0 += step;
		if (_stage0 >= rate) [[unlikely]] {
			_stage1 ^= 0x01;
			_stage0 -= rate;

			ClockTimer();
		}
	}

	/// <summary>
	/// Sets the timer target value.
	/// </summary>
	/// <param name="target">Target value (0=256, otherwise 1-255).</param>
	void SetTarget(uint8_t target) {
		_target = target;
	}

	/// <summary>
	/// Reads the output counter without resetting it (for debugging).
	/// </summary>
	/// <returns>Current output counter value (0-15).</returns>
	uint8_t DebugRead() {
		return _output & 0x0F;
	}

	/// <summary>
	/// Reads and resets the output counter.
	/// </summary>
	/// <returns>Output counter value before reset (0-15).</returns>
	uint8_t GetOutput() {
		uint8_t value = _output & 0x0F;
		_output = 0;
		return value;
	}

	/// <summary>
	/// Sets the output counter directly (used when loading SPC files).
	/// </summary>
	/// <param name="value">Output counter value to set.</param>
	void SetOutput(uint8_t value) {
		_output = value;
	}

	/// <summary>
	/// Serializes timer state for save states.
	/// </summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) {
		SV(_stage0);
		SV(_stage1);
		SV(_stage2);
		SV(_output);
		SV(_target);
		SV(_enabled);
		SV(_timersEnabled);
		SV(_prevStage1);
	}
};