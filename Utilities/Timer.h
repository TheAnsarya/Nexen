#pragma once
#include "pch.h"
#include <chrono>
using namespace std::chrono;

/// <summary>
/// High-resolution timer using std::chrono for precise elapsed time measurement.
/// Suitable for performance profiling, frame timing, and timeout handling.
/// </summary>
/// <remarks>
/// Uses high_resolution_clock for best available system clock precision.
/// Common use cases:
/// - Frame limiter timing (60 Hz = 16.67ms per frame)
/// - Performance profiling (measure function execution time)
/// - Timeout detection (check if operation exceeded time limit)
/// - Animation timing
/// 
/// Resolution depends on platform:
/// - Windows: ~100ns (QueryPerformanceCounter)
/// - Linux: ~1ns (CLOCK_MONOTONIC)
/// - macOS: ~1ns (mach_absolute_time)
/// </remarks>
class Timer {
private:
	/// <summary>Timer start point captured at construction/reset</summary>
	high_resolution_clock::time_point _start;

public:
	/// <summary>
	/// Construct timer and start counting from current time.
	/// </summary>
	Timer();
	
	/// <summary>
	/// Reset timer to current time (restart counting from now).
	/// </summary>
	void Reset();
	
	/// <summary>
	/// Get elapsed time since construction or last Reset().
	/// </summary>
	/// <returns>Elapsed time in milliseconds (fractional)</returns>
	/// <remarks>
	/// Returns double for sub-millisecond precision.
	/// Example: 1.234ms, 16.667ms, 0.001ms
	/// </remarks>
	double GetElapsedMS() const;
	
	/// <summary>
	/// Sleep until target elapsed time is reached.
	/// </summary>
	/// <param name="targetMillisecond">Target elapsed time in milliseconds</param>
	/// <remarks>
	/// Used for frame limiting: WaitUntil(16.667) waits until 16.667ms elapsed.
	/// Does nothing if target time already passed.
	/// Uses std::this_thread::sleep_for for efficient waiting.
	/// </remarks>
	void WaitUntil(double targetMillisecond) const;
};