#pragma once
#include "pch.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"
#include "NES/INesMemoryHandler.h"
#include "NES/NesConsole.h"
#include "NES/NesSoundMixer.h"

/// <summary>
/// APU timer - frequency generator for audio channels.
/// </summary>
/// <remarks>
/// **Purpose:**
/// The timer generates the base frequency for audio channels.
/// It counts down from a period value and signals when it reaches 0.
///
/// **Operation:**
/// - Decrements each CPU cycle
/// - When timer reaches 0, reloads from period and returns true
/// - Period determines output frequency: freq = CPU_clock / (period + 1)
///
/// **Frequencies:**
/// For pulse channels (doubled period): freq = CPU_clock / (2 * (period + 1))
/// NTSC example: 1789773 / (2 * 254) ≈ 3520 Hz = A7
///
/// **Delta Output:**
/// Sends volume changes to mixer as deltas rather than absolute values.
/// Only sends delta when output changes, reducing mixer load.
///
/// **Frame Boundary:**
/// EndFrame() resets the previous cycle counter for accurate timing
/// across audio frames.
/// </remarks>
class ApuTimer : public ISerializable {
private:
	uint32_t _previousCycle;     ///< CPU cycle at last update
	uint16_t _timer = 0;         ///< Current countdown timer value
	uint16_t _period = 0;        ///< Timer reload value (frequency)
	int8_t _lastOutput = 0;      ///< Last output value sent to mixer

	AudioChannel _channel = AudioChannel::Square1;  ///< Channel identifier
	NesSoundMixer* _mixer = nullptr;  ///< Audio mixer reference

public:
	/// <summary>Constructs timer for specified channel.</summary>
	/// <param name="channel">Audio channel identifier.</param>
	/// <param name="mixer">Sound mixer for delta output.</param>
	ApuTimer(AudioChannel channel, NesSoundMixer* mixer) {
		_channel = channel;
		_mixer = mixer;
		Reset(false);
	}

	/// <summary>Resets timer to initial state.</summary>
	/// <param name="softReset">True for soft reset, false for hard reset.</param>
	void Reset(bool softReset) {
		_timer = 0;
		_period = 0;
		_previousCycle = 0;
		_lastOutput = 0;
	}

	/// <summary>Serializes timer state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		if (!s.IsSaving()) {
			_previousCycle = 0;
		}

		SV(_timer);
		SV(_period);
		SV(_lastOutput);
	}

	/// <summary>
	/// Adds delta output to mixer if value changed.
	/// </summary>
	/// <param name="output">New output value.</param>
	/// <remarks>
	/// Only sends delta when output changes to reduce mixer load.
	/// Mixer expects deltas, not absolute values.
	/// </remarks>
	__forceinline void AddOutput(int8_t output) {
		if (output != _lastOutput) {
			_mixer->AddDelta(_channel, _previousCycle, output - _lastOutput);
			_lastOutput = output;
		}
	}

	/// <summary>Gets last output value sent to mixer.</summary>
	/// <returns>Last output value.</returns>
	int8_t GetLastOutput() {
		return _lastOutput;
	}

	/// <summary>
	/// Runs timer to target cycle, returns true if timer fired.
	/// </summary>
	/// <param name="targetCycle">CPU cycle to run to.</param>
	/// <returns>True if timer reached 0 (output should update).</returns>
	/// <remarks>
	/// Efficiently handles both small and large cycle advances.
	/// Returns true at most once per call (single timer fire).
	/// </remarks>
	__forceinline bool Run(uint32_t targetCycle) {
		int32_t cyclesToRun = targetCycle - _previousCycle;

		if (cyclesToRun > _timer) {
			_previousCycle += _timer + 1;
			_timer = _period;
			return true;
		}

		_timer -= cyclesToRun;
		_previousCycle = targetCycle;
		return false;
	}

	/// <summary>Sets timer period (frequency).</summary>
	/// <param name="period">New period value.</param>
	void SetPeriod(uint16_t period) {
		_period = period;
	}

	/// <summary>Gets timer period.</summary>
	/// <returns>Current period value.</returns>
	uint16_t GetPeriod() {
		return _period;
	}

	/// <summary>Gets current timer countdown value.</summary>
	/// <returns>Current timer value.</returns>
	uint16_t GetTimer() {
		return _timer;
	}

	/// <summary>Sets timer countdown value directly.</summary>
	/// <param name="timer">New timer value.</param>
	void SetTimer(uint16_t timer) {
		_timer = timer;
	}

	/// <summary>
	/// Resets cycle counter at end of audio frame.
	/// </summary>
	/// <remarks>
	/// Called at audio frame boundary to reset cycle tracking.
	/// </remarks>
	__forceinline void EndFrame() {
		_previousCycle = 0;
	}
};