#pragma once
#include "pch.h"
#include <deque>
#include "Shared/BaseControlDevice.h"

class Emulator;

/// <summary>
/// Savestate snapshot with XOR delta compression for rewind system.
/// Stores compressed savestate and input logs for frame-perfect replay.
/// </summary>
/// <remarks>
/// Compression strategy:
/// - First snapshot: Full savestate (uncompressed or zlib)
/// - Subsequent: XOR delta against previous state (much smaller)
/// - XOR compression: 10-100x size reduction for consecutive frames
///
/// Storage format:
/// - _saveStateData: Compressed state (full or XOR delta)
/// - _uncompressedData: Temporary buffer for decompression
/// - InputLogs: Controller input for each port (for replay)
///
/// Reconstruction:
/// 1. LoadState() walks back through prevStates deque
/// 2. Finds most recent full state (IsFullState=true)
/// 3. Applies XOR deltas forward to target position
/// 4. Deserializes final state into emulator
///
/// Segment markers:
/// - EndOfSegment: Boundary between rewind blocks (30 frames)
/// - IsFullState: Full savestate vs delta
///
/// Thread safety: Accessed from emulation thread only.
/// </remarks>
class RewindData {
private:
	vector<uint8_t> _saveStateData;    ///< Compressed savestate (full or XOR delta)
	vector<uint8_t> _uncompressedData; ///< Temporary decompression buffer

	/// <summary>
	/// Process XOR compression/decompression against previous states.
	/// </summary>
	/// <param name="data">Current state data to compress/decompress</param>
	/// <param name="prevStates">History of previous states</param>
	/// <param name="position">Position in history (-1 = current)</param>
	template <typename T>
	void ProcessXorState(T& data, deque<RewindData>& prevStates, int32_t position);

public:
	/// <summary>Input logs per controller port (for replay)</summary>
	std::deque<ControlDeviceState> InputLogs[BaseControlDevice::PortCount];

	int32_t FrameCount = 0;    ///< Number of frames in this block
	bool EndOfSegment = false; ///< Marks end of 30-frame segment
	bool IsFullState = false;  ///< True = full state, false = XOR delta

	/// <summary>
	/// Get decompressed state data as stream.
	/// </summary>
	/// <param name="stateData">Output stream for state data</param>
	/// <param name="prevStates">Previous states for XOR reconstruction</param>
	/// <param name="position">Position in history (-1 = current)</param>
	void GetStateData(stringstream& stateData, deque<RewindData>& prevStates, int32_t position);

	/// <summary>Get compressed state size in bytes</summary>
	[[nodiscard]] uint32_t GetStateSize() { return (uint32_t)_saveStateData.size(); }

	/// <summary>
	/// Load this savestate into emulator.
	/// </summary>
	/// <param name="emu">Emulator instance</param>
	/// <param name="prevStates">Previous states for XOR reconstruction</param>
	/// <param name="position">Position in history (-1 = current)</param>
	/// <param name="sendNotification">Send state loaded notification if true</param>
	/// <remarks>
	/// Reconstructs full state from XOR deltas if needed.
	/// Walks backward through prevStates to find full state, applies deltas forward.
	/// </remarks>
	void LoadState(Emulator* emu, deque<RewindData>& prevStates, int32_t position = -1, bool sendNotification = true);

	/// <summary>
	/// Save current emulator state to this snapshot.
	/// </summary>
	/// <param name="emu">Emulator instance</param>
	/// <param name="prevStates">Previous states for XOR compression</param>
	/// <param name="position">Position in history (-1 = current)</param>
	/// <remarks>
	/// First save is full state, subsequent saves are XOR deltas.
	/// XOR compression: data[i] = current[i] ^ previous[i] (only stores differences).
	/// </remarks>
	void SaveState(Emulator* emu, deque<RewindData>& prevStates, int32_t position = -1);
};
