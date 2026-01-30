#pragma once
#include "pch.h"
#include <cstring>
#include "Shared/SettingTypes.h"

/// <summary>
/// Represents current button/input state for a controller or input device.
/// State format is device-specific and opaque to the emulation core.
/// </summary>
/// <remarks>
/// Different controller types encode button state differently:
/// - Standard gamepad: Each byte represents different buttons/axes
/// - Mouse: Position coordinates + button flags
/// - Keyboard: Scancode matrix
/// - Zapper: X/Y coordinates + trigger state
///
/// State vector size varies by device type.
/// Comparison uses memcmp for efficient byte-wise equality check.
/// </remarks>
struct ControlDeviceState {
	/// <summary>Raw device state bytes (device-specific encoding)</summary>
	vector<uint8_t> State;

	/// <summary>
	/// Compare device states for inequality.
	/// </summary>
	/// <param name="other">Other device state to compare against</param>
	/// <returns>True if states differ in size or content</returns>
	/// <remarks>
	/// Uses memcmp for efficient byte-wise comparison.
	/// Returns true if either size differs OR any byte differs.
	/// Used for detecting input changes frame-to-frame.
	/// </remarks>
	bool operator!=(ControlDeviceState& other) {
		return State.size() != other.State.size() || memcmp(State.data(), other.State.data(), State.size()) != 0;
	}
};

/// <summary>
/// Complete controller data packet including device type, state, and port assignment.
/// Used for input recording, movie playback, and network play.
/// </summary>
/// <remarks>
/// Combines all information needed to reconstruct controller input:
/// - Type: Determines state format and behavior
/// - State: Current button/axis values
/// - Port: Which physical/virtual port this controller is connected to
///
/// Common use cases:
/// - Movie files (.mmo): Record/replay controller input sequences
/// - Network play: Transmit controller state to remote emulator
/// - Input display: Show pressed buttons on OSD
/// </remarks>
struct ControllerData {
	/// <summary>Controller type (gamepad, mouse, zapper, etc.)</summary>
	ControllerType Type;

	/// <summary>Current input state (device-specific encoding)</summary>
	ControlDeviceState State;

	/// <summary>Port number (0-based, typically 0-3 for most systems)</summary>
	uint8_t Port;
};