#pragma once
#include "pch.h"

class BaseControlDevice;

/// <summary>
/// Interface for components that record controller input.
/// Implemented by movie recorders, network servers, and input logging systems.
/// </summary>
/// <remarks>
/// Implementers:
/// - MovieManager: Recording to movie files (.msm format)
/// - GameServer: Broadcasting input to network clients
/// - RecordedRomTest: Recording test runs for regression testing
///
/// Recording flow:
/// 1. BaseControlManager polls all connected controllers
/// 2. Calls RecordInput() on registered recorders
/// 3. Recorders serialize input state to storage/network
///
/// Input format:
/// - Each frame records all controller states
/// - Includes button presses, analog axes, mouse coordinates
/// - Timestamp/frame number for synchronization
/// </remarks>
class IInputRecorder {
public:
	/// <summary>
	/// Record controller input for current frame.
	/// </summary>
	/// <param name="devices">All connected controller devices (shared ownership)</param>
	/// <remarks>
	/// Called once per frame with all active controllers.
	/// Device states reflect post-processing (after lag reduction, auto-fire, etc.).
	/// Recorder must clone/serialize state immediately (devices may be modified after call).
	/// </remarks>
	virtual void RecordInput(const vector<shared_ptr<BaseControlDevice>>& devices) = 0;
};
