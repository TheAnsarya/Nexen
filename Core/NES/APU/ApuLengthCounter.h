#pragma once
#include "pch.h"
#include "NES/NesConsole.h"
#include "NES/APU/NesApu.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

/// <summary>
/// APU length counter - automatic note duration control for channels.
/// </summary>
/// <remarks>
/// **Purpose:**
/// The length counter provides automatic channel silencing after a
/// programmable number of frame counter clocks. This allows music
/// engines to set note durations without manual tracking.
///
/// **Lookup Table:**
/// The 5-bit index ($00-$1F) maps to various durations:
/// ```
/// Index  Value    Index  Value    Index  Value    Index  Value
/// $00    10       $08    160      $10    12       $18    192
/// $01    254      $09    8        $11    16       $19    24
/// $02    20       $0A    60       $12    24       $1A    72
/// $03    2        $0B    10       $13    18       $1B    26
/// $04    40       $0C    14       $14    48       $1C    16
/// $05    4        $0D    12       $15    20       $1D    28
/// $06    80       $0E    26       $16    96       $1E    32
/// $07    6        $0F    14       $17    22       $1F    30
/// ```
///
/// **Clocking:**
/// Clocked by frame counter half-frame signal (~120 Hz in 4-step mode).
/// Decrements counter if not halted and counter > 0.
/// Channel is silenced when counter = 0.
///
/// **Halt Flag:**
/// When halt flag is set, counter doesn't decrement.
/// Shared with envelope loop flag for pulse/noise channels.
/// For triangle, this is the linear counter control flag.
///
/// **Enable Flag:**
/// When channel is disabled via $4015, counter is forced to 0.
/// Writing to $4003/$4007/$400B/$400F reloads counter from table
/// (but only if channel is enabled).
///
/// **Quirks:**
/// - Loading counter during length counter clock has special behavior
/// - Reset behavior differs for triangle vs other channels
/// </remarks>
class ApuLengthCounter : public ISerializable {
private:
	/// <summary>Length counter lookup table (32 entries).</summary>
	static constexpr uint8_t _lcLookupTable[32] = {10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14, 12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

	NesConsole* _console = nullptr;     ///< Parent console reference
	AudioChannel _channel = AudioChannel::Square1;  ///< Channel identifier
	bool _newHaltValue = false;         ///< Pending halt flag value

protected:
	bool _enabled = false;    ///< Channel enabled ($4015 bit)
	bool _halt = false;       ///< Halt flag (stops counting when true)
	uint8_t _counter = 0;     ///< Current counter value
	uint8_t _reloadValue = 0; ///< Pending reload value
	uint8_t _previousValue = 0; ///< Value before reload (for quirk handling)

public:
	/// <summary>
	/// Initializes halt flag from register write.
	/// </summary>
	/// <param name="haltFlag">New halt flag value.</param>
	void InitializeLengthCounter(bool haltFlag) {
		_console->GetApu()->SetNeedToRun();
		_newHaltValue = haltFlag;
	}

	/// <summary>
	/// Loads counter from lookup table.
	/// </summary>
	/// <param name="value">5-bit table index (0-31).</param>
	void LoadLengthCounter(uint8_t value) {
		if (_enabled) {
			_reloadValue = _lcLookupTable[value];
			_previousValue = _counter;
			_console->GetApu()->SetNeedToRun();
		}
	}

	/// <summary>Constructs length counter for specified channel.</summary>
	/// <param name="channel">Audio channel this counter belongs to.</param>
	/// <param name="console">Parent NES console.</param>
	ApuLengthCounter(AudioChannel channel, NesConsole* console) {
		_channel = channel;
		_console = console;
	}

	/// <summary>Resets length counter to initial state.</summary>
	/// <param name="softReset">True for soft reset, false for hard reset.</param>
	/// <remarks>
	/// On soft reset, triangle channel is unaffected but others are cleared.
	/// "At reset, length counters should be enabled, triangle unaffected"
	/// </remarks>
	void Reset(bool softReset) {
		if (softReset) {
			_enabled = false;
			if (_channel != AudioChannel::Triangle) {
				// Triangle channel unaffected by soft reset
				_halt = false;
				_counter = 0;
				_newHaltValue = false;
				_reloadValue = 0;
				_previousValue = 0;
			}
		} else {
			_enabled = false;
			_halt = false;
			_counter = 0;
			_newHaltValue = false;
			_reloadValue = 0;
			_previousValue = 0;
		}
	}

	/// <summary>Serializes length counter state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_enabled);
		SV(_halt);
		SV(_newHaltValue);
		SV(_counter);
		SV(_previousValue);
		SV(_reloadValue);
	}

	/// <summary>Gets channel active status.</summary>
	/// <returns>True if counter > 0 (channel produces output).</returns>
	bool GetStatus() {
		return _counter > 0;
	}

	/// <summary>Gets halt flag state.</summary>
	/// <returns>True if counting is halted.</returns>
	bool IsHalted() {
		return _halt;
	}

	/// <summary>
	/// Reloads counter from pending value (called after length clock).
	/// </summary>
	/// <remarks>
	/// Handles the quirk where reload during length clock has special behavior.
	/// </remarks>
	void ReloadCounter() {
		if (_reloadValue) {
			if (_counter == _previousValue) {
				_counter = _reloadValue;
			}
			_reloadValue = 0;
		}

		_halt = _newHaltValue;
	}

	/// <summary>
	/// Advances length counter by one frame counter clock.
	/// </summary>
	/// <remarks>
	/// Decrements counter if not halted and counter > 0.
	/// </remarks>
	void TickLengthCounter() {
		if (_counter > 0 && !_halt) {
			_counter--;
		}
	}

	/// <summary>
	/// Sets channel enable state from $4015 write.
	/// </summary>
	/// <param name="enabled">True to enable, false to disable and clear counter.</param>
	void SetEnabled(bool enabled) {
		if (!enabled) {
			_counter = 0;
		}
		_enabled = enabled;
	}

	bool IsEnabled() {
		return _enabled;
	}

	ApuLengthCounterState GetState() {
		ApuLengthCounterState state;
		state.Counter = _counter;
		state.Halt = _halt;
		state.ReloadValue = _reloadValue;
		return state;
	}
};
