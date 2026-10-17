#pragma once
#include "pch.h"
#include "NES/APU/ApuLengthCounter.h"
#include "NES/NesConsole.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

/// <summary>
/// APU envelope generator for volume control on pulse and noise channels.
/// </summary>
/// <remarks>
/// **Envelope Operation:**
/// The envelope provides automatic volume decay or constant volume output.
/// It's clocked by the frame counter's quarter-frame signal (~240 Hz).
///
/// **Modes:**
/// - Constant Volume: Output = volume parameter (0-15)
/// - Decay Mode: Output = counter, decrements each clock
///
/// **Loop Behavior:**
/// When loop flag is set and counter reaches 0, it wraps to 15.
/// Otherwise, counter stays at 0 (silence).
///
/// **Start Flag:**
/// Writing to $4003/$4007/$400F sets the start flag.
/// On next envelope clock, counter resets to 15 and divider resets.
///
/// **Divider:**
/// Divider period = volume parameter + 1 (1-16 frames).
/// Counter decrements when divider reaches 0.
///
/// **Integration with Length Counter:**
/// The envelope also contains the length counter for this channel.
/// Both share the loop/halt flag (loop = envelope, halt = length).
/// </remarks>
class ApuEnvelope : public ISerializable {
private:
	bool _constantVolume = false;  ///< True = constant volume, false = decay mode
	uint8_t _volume = 0;           ///< Volume/divider period parameter (0-15)

	bool _start = false;           ///< Start flag (triggers envelope reset)
	int8_t _divider = 0;           ///< Divider counter (clocks envelope)
	uint8_t _counter = 0;          ///< Envelope counter (0-15), actual output

public:
	/// <summary>Associated length counter for this channel.</summary>
	ApuLengthCounter LengthCounter;

	/// <summary>Constructs envelope with associated length counter.</summary>
	/// <param name="channel">Audio channel identifier.</param>
	/// <param name="console">Parent NES console.</param>
	ApuEnvelope(AudioChannel channel, NesConsole* console) : LengthCounter(channel, console) {
	}

	/// <summary>
	/// Initializes envelope from register value (bits 0-5).
	/// </summary>
	/// <param name="regValue">Register value with volume/loop/constant flags.</param>
	/// <remarks>
	/// Bit layout: --LC VVVV
	/// - L: Loop/halt flag (shared with length counter)
	/// - C: Constant volume flag
	/// - V: Volume/period (0-15)
	/// </remarks>
	void InitializeEnvelope(uint8_t regValue) {
		LengthCounter.InitializeLengthCounter((regValue & 0x20) == 0x20);
		_constantVolume = (regValue & 0x10) == 0x10;
		_volume = regValue & 0x0F;
	}

	/// <summary>Sets start flag to trigger envelope reset on next clock.</summary>
	void ResetEnvelope() {
		_start = true;
	}

	/// <summary>
	/// Gets current volume output.
	/// </summary>
	/// <returns>Volume (0-15), or 0 if length counter is zero.</returns>
	uint32_t GetVolume() {
		if (LengthCounter.GetStatus()) {
			if (_constantVolume) {
				return _volume;
			} else {
				return _counter;
			}
		} else {
			return 0;
		}
	}

	/// <summary>Resets envelope to initial state.</summary>
	/// <param name="softReset">True for soft reset, false for hard reset.</param>
	void Reset(bool softReset) {
		LengthCounter.Reset(softReset);
		_constantVolume = false;
		_volume = 0;
		_start = false;
		_divider = 0;
		_counter = 0;
	}

	/// <summary>Serializes envelope state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_constantVolume);
		SV(_volume);
		SV(_start);
		SV(_divider);
		SV(_counter);
		SV(LengthCounter);
	}

	/// <summary>
	/// Advances envelope by one frame counter clock.
	/// </summary>
	/// <remarks>
	/// Called at ~240 Hz by the frame counter.
	/// If start flag is set, resets counter to 15 and divider to volume.
	/// Otherwise, decrements divider. When divider reaches 0:
	/// - Reloads divider from volume
	/// - Decrements counter (or wraps to 15 if loop flag set)
	/// </remarks>
	void TickEnvelope() {
		if (!_start) {
			_divider--;
			if (_divider < 0) {
				_divider = _volume;
				if (_counter > 0) {
					_counter--;
				} else if (LengthCounter.IsHalted()) {
					// Loop mode: wrap counter to 15
					_counter = 15;
				}
			}
		} else {
			// Start flag set: reset envelope
			_start = false;
			_counter = 15;
			_divider = _volume;
		}
	}

	/// <summary>Gets envelope state for debugging.</summary>
	/// <returns>Snapshot of all envelope parameters.</returns>
	ApuEnvelopeState GetState() {
		ApuEnvelopeState state;
		state.ConstantVolume = _constantVolume;
		state.Counter = _counter;
		state.Divider = _divider;
		state.Loop = LengthCounter.IsHalted();
		state.StartFlag = _start;
		state.Volume = _volume;
		return state;
	}
};
