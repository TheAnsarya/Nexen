#pragma once

#include "pch.h"

class BaseControlDevice;

/// <summary>
/// Interface for controller hub devices that expose multiple sub-ports.
/// Examples: SNES Multitap (5 controllers), NES Four Score (4 controllers).
/// </summary>
/// <remarks>
/// Controller hubs allow more than the standard number of controllers by multiplexing
/// multiple devices through a single physical port on the console.
///
/// Implementations handle:
/// - Reading input from multiple connected controllers
/// - Multiplexing controller data in hardware-accurate manner
/// - Updating internal hub state when polling occurs
/// </remarks>
class IControllerHub {
public:
	/// <summary>
	/// Maximum number of sub-ports supported by any hub implementation.
	/// </summary>
	/// <remarks>
	/// SNES Multitap supports 5 controllers (4 players + 1 for hub itself).
	/// constexpr allows compile-time array sizing.
	/// </remarks>
	static constexpr int MaxSubPorts = 5;

	/// <summary>
	/// Update internal hub state when controller polling occurs.
	/// </summary>
	/// <remarks>
	/// Called when the emulated game reads controller data.
	/// Hub implementations update internal registers to prepare for sequential reads.
	/// </remarks>
	virtual void RefreshHubState() = 0;

	/// <summary>
	/// Get number of active controller ports on this hub.
	/// </summary>
	/// <returns>Number of sub-ports (typically 4 or 5)</returns>
	virtual int GetHubPortCount() = 0;

	/// <summary>
	/// Get controller device at specific hub port index.
	/// </summary>
	/// <param name="index">Port index (0-based, up to GetHubPortCount()-1)</param>
	/// <returns>Shared pointer to controller device, or nullptr if no controller connected</returns>
	virtual shared_ptr<BaseControlDevice> GetController(int index) = 0;
};