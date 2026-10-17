#pragma once
#include "pch.h"
#include "Utilities/Serializer.h"

/// <summary>
/// Game Boy audio channel DAC (Digital-to-Analog Converter) emulation.
/// </summary>
/// <remarks>
/// **DAC Hardware:**
/// Each of the 4 sound channels has its own DAC that converts
/// the digital sample values to analog output levels.
///
/// **DAC Fade Effect:**
/// When a channel's DAC is enabled or disabled, the output doesn't
/// instantly switch but gradually fades in/out. This is due to the
/// capacitor-coupled output in the hardware.
///
/// **Why This Matters:**
/// Without DAC fade emulation, games that rapidly toggle channels
/// produce audible pops and clicks. Games affected include:
/// - 3D Pocket Pool
/// - Ready 2 Rumble Boxing
/// - Cannon Fodder
///
/// **Implementation:**
/// - Volume ramps from 0% to 100% (or vice versa) over ~25000 cycles
/// - Counter runs at CPU clock rate
/// - Volume changes by 1% every 250 cycles
/// </remarks>
class GbChannelDac : ISerializable {
private:
	/// <summary>Countdown until next volume step (in CPU cycles).</summary>
	int16_t _counter = 0;

	/// <summary>Current DAC volume level (0-100%).</summary>
	uint16_t _volume = 0;

public:
	/// <summary>
	/// Gets the current DAC volume level.
	/// </summary>
	/// <returns>Volume percentage (0-100).</returns>
	[[nodiscard]] uint16_t GetDacVolume() {
		return _volume;
	}

	/// <summary>
	/// Advances DAC state, applying fade in/out effect.
	/// </summary>
	/// <param name="clocksToRun">CPU cycles elapsed.</param>
	/// <param name="enabled">Whether DAC should be at full volume.</param>
	/// <remarks>
	/// When enabled transitions:
	/// - false→true: Volume ramps up to 100%
	/// - true→false: Volume ramps down to 0%
	/// </remarks>
	void Exec(uint32_t clocksToRun, bool enabled) {
		_counter -= clocksToRun;

		if (_counter <= 0) {
			// Apply gradual fade in/out effect
			// Volume changes by 1% every 250 cycles (~6400 Hz update rate)
			if (enabled) {
				_volume = std::min(100, _volume + 1);
			} else {
				_volume = std::max(0, _volume - 1);
			}
			_counter += 250;
		}
	}

	/// <summary>Serializes DAC state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_counter);
		SV(_volume);
	}
};
