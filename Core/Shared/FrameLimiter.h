#pragma once
#include "Utilities/Timer.h"

/// <summary>
/// Frame rate limiter using precise timing to maintain target FPS.
/// Synchronizes emulation speed to real-time (60Hz for NTSC, 50Hz for PAL, etc.).
/// </summary>
/// <remarks>
/// How it works:
/// 1. ProcessFrame() called after each emulated frame - advances target time
/// 2. WaitForNextFrame() sleeps until target time reached
/// 3. Automatically recovers from timing drift and emulation pauses
/// 
/// Target delay calculation:
/// - 60 FPS (NTSC):  16.667ms per frame
/// - 50 FPS (PAL):   20.000ms per frame
/// - 120 FPS (2x):    8.333ms per frame
/// 
/// Auto-reset scenarios:
/// - Target frame rate changed (speed multiplier changed)
/// - Emulation paused/reset (debugger, power cycle)
/// - Timing drift > 100ms (safety net for lag spikes)
/// 
/// Usage:
/// <code>
/// FrameLimiter limiter(16.667); // 60 FPS
/// while (running) {
///     EmulateFrame();
///     limiter.ProcessFrame();
///     limiter.WaitForNextFrame();
/// }
/// </code>
/// 
/// Thread safety: Not thread-safe - use from emulation thread only.
/// </remarks>
class FrameLimiter {
private:
	Timer _clockTimer;      ///< High-resolution timer for frame timing
	double _targetTime;     ///< Next frame target time in milliseconds
	double _delay;          ///< Delay per frame in milliseconds
	bool _resetRunTimers;   ///< Flag to reset timers on next frame

public:
	/// <summary>
	/// Construct frame limiter with target delay.
	/// </summary>
	/// <param name="delay">Milliseconds per frame (e.g., 16.667 for 60 FPS)</param>
	FrameLimiter(double delay) {
		_delay = delay;
		_targetTime = _delay;
		_resetRunTimers = false;
	}

	/// <summary>
	/// Change target frame rate.
	/// </summary>
	/// <param name="delay">New milliseconds per frame</param>
	/// <remarks>
	/// Triggers timer reset on next ProcessFrame() call.
	/// Used when speed multiplier changes (1x, 2x, 0.5x, etc.).
	/// </remarks>
	void SetDelay(double delay) {
		_delay = delay;
		_resetRunTimers = true;
	}

	/// <summary>
	/// Process frame completion and advance target time.
	/// </summary>
	/// <remarks>
	/// Call after each emulated frame.
	/// Advances target time by delay amount.
	/// Auto-resets timers if:
	/// - Delay changed (frame rate changed)
	/// - Timing drift > 100ms (emulation paused, debugger break, etc.)
	/// </remarks>
	void ProcessFrame() {
		if (_resetRunTimers || (_clockTimer.GetElapsedMS() - _targetTime) > 100) {
			// Reset the timers, this can happen in 3 scenarios:
			// 1) Target frame rate changed
			// 2) The console was reset/power cycled or the emulation was paused (with or without the debugger)
			// 3) As a satefy net, if we overshoot our target by over 100 milliseconds, the timer is reset, too.
			//    This can happen when something slows the emulator down severely (or when breaking execution in VS when debugging Mesen itself, etc.)
			_clockTimer.Reset();
			_targetTime = 0;
			_resetRunTimers = false;
		}

		_targetTime += _delay;
	}

	/// <summary>
	/// Wait until next frame time.
	/// </summary>
	/// <returns>True if long sleep was interrupted (for early exit check), false if normal wait</returns>
	/// <remarks>
	/// Sleeps until target time reached.
	/// For slow speeds (<= 25%), sleeps in 40ms chunks to allow early exit.
	/// For normal speeds, sleeps precise amount until target time.
	/// Call after ProcessFrame() to maintain consistent frame rate.
	/// </remarks>
	bool WaitForNextFrame() {
		if (_targetTime - _clockTimer.GetElapsedMS() > 50) {
			// When sleeping for a long time (e.g <= 25% speed), sleep in small chunks and check to see if we need to stop sleeping between each sleep call
			_clockTimer.WaitUntil(_clockTimer.GetElapsedMS() + 40);
			return true;
		}

		_clockTimer.WaitUntil(_targetTime);
		return false;
	}
};