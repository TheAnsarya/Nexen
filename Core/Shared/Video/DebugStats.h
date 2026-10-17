#pragma once
#include "pch.h"

class Emulator;

/// <summary>
/// Frame timing statistics display for performance monitoring.
/// Tracks frame duration min/max/average for debugging and optimization.
/// </summary>
/// <remarks>
/// Displays on-screen stats in debug builds or when performance overlay is enabled.
/// Tracks rolling 60-frame window for smooth average calculations.
/// Used to diagnose frame drops, stuttering, and emulation slowdowns.
/// </remarks>
class DebugStats {
private:
	double _frameDurations[60] = {};  ///< Rolling frame time buffer (60 frames)
	uint32_t _frameDurationIndex = 0; ///< Circular buffer write index
	double _lastFrameMin = 9999;      ///< Minimum frame time in last window
	double _lastFrameMax = 0;         ///< Maximum frame time in last window

public:
	/// <summary>
	/// Display performance statistics overlay on current frame.
	/// </summary>
	/// <param name="emu">Emulator instance for OSD rendering</param>
	/// <param name="lastFrameTime">Duration of last frame in milliseconds</param>
	/// <remarks>
	/// Calculates and displays:
	/// - Current FPS (frames per second)
	/// - Average frame time over 60-frame window
	/// - Min/Max frame times
	/// - Frame drops (if any)
	/// </remarks>
	void DisplayStats(Emulator* emu, double lastFrameTime);
};