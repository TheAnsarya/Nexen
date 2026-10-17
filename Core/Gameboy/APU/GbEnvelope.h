#pragma once
#include "pch.h"

/// <summary>
/// Game Boy APU volume envelope emulation.
/// </summary>
/// <remarks>
/// **Envelope Overview:**
/// The volume envelope automatically adjusts channel volume over time.
/// Used for fade-in, fade-out, and decay effects in sound.
///
/// **NRx2 Register ($FF12/$FF17/$FF21):**
/// ```
/// Bit 7-4: Initial volume (0-15)
/// Bit 3:   Direction (1=increase, 0=decrease)
/// Bit 2-0: Period (0=disabled, 1-7=sweep interval)
/// ```
///
/// **Envelope Timing:**
/// - Clocked at 64 Hz (every 16384 CPU cycles)
/// - Period value sets clocks between volume changes
/// - Period 0 disables automatic envelope changes
///
/// **Zombie Mode (Hardware Quirk):**
/// Writing to NRx2 while envelope is active causes "zombie mode"
/// behavior where volume is modified in unexpected ways.
/// This is a real hardware quirk that some games exploit.
///
/// **Zombie Mode Rules:**
/// - If direction flipped (add↔sub): Volume = 16 - volume
/// - If period was 0 and not stopped: Volume incremented/decremented
/// - Only low 4 bits of volume are kept after modifications
///
/// **DAC Control:**
/// - If upper 5 bits of NRx2 are 0, DAC is disabled
/// - DAC off = channel produces no output
/// </remarks>
class GbEnvelope {
public:
	/// <summary>
	/// Clocks the volume envelope for one step.
	/// </summary>
	/// <typeparam name="T">Channel state type.</typeparam>
	/// <typeparam name="U">Channel class type.</typeparam>
	/// <param name="state">Channel state with envelope fields.</param>
	/// <param name="channel">Channel instance for output update.</param>
	/// <remarks>
	/// Called at 64 Hz. Period value determines how many 64 Hz clocks
	/// pass between volume changes. Volume stops at 0 or 15.
	/// </remarks>
	template <typename T, typename U>
	static void ClockEnvelope(T& state, U& channel) {
		uint8_t timer = state.EnvTimer;

		if (state.EnvTimer == 0 || --state.EnvTimer == 0) {
			if (state.EnvPeriod > 0 && !state.EnvStopped) {
				// Adjust volume based on direction
				if (state.EnvRaiseVolume && state.Volume < 0x0F) {
					state.Volume++;
				} else if (!state.EnvRaiseVolume && state.Volume > 0) {
					state.Volume--;
				} else {
					// Reached min/max, stop envelope
					state.EnvStopped = true;
				}

				// Clocking envelope should update output immediately
				// (based on div_trigger_volume/channel_4_volume_div tests)
				channel.UpdateOutput();

				// Reload timer with period
				state.EnvTimer = state.EnvPeriod;

				if (timer == 0) {
					// When timer was already 0 (period was 0), next clock
					// occurs earlier than expected - fixes channel_1_nrx2_glitch
					state.EnvTimer--;
				}
			}
		}
	}

	/// <summary>
	/// Handles writes to NRx2 volume envelope register.
	/// </summary>
	/// <typeparam name="T">Channel state type.</typeparam>
	/// <typeparam name="U">Channel class type.</typeparam>
	/// <param name="state">Channel state.</param>
	/// <param name="value">Register value being written.</param>
	/// <param name="channel">Channel instance.</param>
	/// <remarks>
	/// Implements "zombie mode" hardware quirk where writing NRx2
	/// while envelope is running causes unexpected volume changes.
	/// Based on channel_1_nrx2_glitch test and SameBoy's implementation.
	/// </remarks>
	template <typename T, typename U>
	static void WriteRegister(T& state, uint8_t value, U& channel) {
		bool raiseVolume = (value & 0x08) != 0;
		uint8_t period = value & 0x07;

		// Check if DAC should be disabled (upper 5 bits = 0)
		if ((value & 0xF8) == 0) {
			state.Enabled = false;
			state.Output = 0;
		} else {
			// Zombie mode implementation (differs from gbdev wiki description)
			// Based on channel_1_nrx2_glitch test and SameBoy behavior
			bool preventIncrement = false;

			if (raiseVolume != state.EnvRaiseVolume) {
				// Direction changed - apply zombie mode volume modification
				if (raiseVolume) {
					// Changed to increase mode
					if (!state.EnvStopped && state.EnvPeriod == 0) {
						state.Volume ^= 0x0F;
					} else {
						state.Volume = 14 - state.Volume;
					}
					preventIncrement = true;
				} else {
					// Changed to decrease mode
					// "If mode changed, volume is set to 16 - volume"
					state.Volume = 16 - state.Volume;
				}

				// Only low 4 bits kept
				state.Volume &= 0xF;
			}

			// Additional zombie mode increment/decrement
			if (!state.EnvStopped && !preventIncrement) {
				if (state.EnvPeriod == 0 && (period || raiseVolume)) {
					// "If old period was zero and envelope still doing updates,
					// volume is incremented/decremented by 1"
					if (raiseVolume) {
						state.Volume++;
					} else {
						state.Volume--;
					}
					state.Volume &= 0xF;
				}
			}
		}

		// Store new envelope parameters
		state.EnvPeriod = period;
		state.EnvRaiseVolume = raiseVolume;
		state.EnvVolume = (value & 0xF0) >> 4;

		channel.UpdateOutput();
	}
};