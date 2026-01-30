#pragma once

class BaseControlDevice;

/// <summary>
/// Interface for components that provide controller input to the emulation core.
/// Implemented by movie players, network clients, and input replay systems.
/// </summary>
/// <remarks>
/// Implementers:
/// - MovieManager: Playback from movie files (.msm, .fm2, .bk2, etc.)
/// - HistoryViewer: TAS history scrubbing and frame-by-frame playback
/// - GameClient: Network play client (receives input from server)
/// - GameServer: Network play server (provides host input)
///
/// Input flow:
/// 1. Emulation core requests input for controller port
/// 2. BaseControlManager checks if IInputProvider is registered
/// 3. Provider calls SetInput() to update device state
/// 4. Device state used for current frame
///
/// Priority order:
/// - Movie playback overrides user input
/// - Network input overrides local input
/// - History viewer overrides everything (TAS mode)
/// </remarks>
class IInputProvider {
public:
	/// <summary>
	/// Set controller input state for current frame.
	/// </summary>
	/// <param name="device">Controller device to update (caller owns memory)</param>
	/// <returns>True if input was provided, false if provider has no input for this device</returns>
	/// <remarks>
	/// Called once per frame per connected controller.
	/// Return false to allow input from lower-priority sources (e.g., user input).
	/// Return true to override all other input sources.
	/// </remarks>
	virtual bool SetInput(BaseControlDevice* device) = 0;
};