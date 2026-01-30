#pragma once
#include "pch.h"
#include <deque>
#include "Shared/Interfaces/INotificationListener.h"
#include "Shared/RewindData.h"
#include "Shared/Interfaces/IInputProvider.h"
#include "Shared/Interfaces/IInputRecorder.h"

class Emulator;
class EmuSettings;
struct RenderedFrame;

/// <summary>Rewind state machine states</summary>
enum class RewindState {
	Stopped = 0,  ///< Not rewinding
	Stopping = 1, ///< Stopping rewind (transitioning to normal)
	Starting = 2, ///< Starting rewind (transitioning to rewind)
	Started = 3,  ///< Actively rewinding
	Debugging = 4 ///< Rewind for debugger step-back
};

/// <summary>
/// Video frame snapshot for rewind playback.
/// Stores rendered frame data and input state for replay.
/// </summary>
struct VideoFrame {
	vector<uint32_t> Data;            ///< RGBA pixel data
	uint32_t Width = 0;               ///< Frame width
	uint32_t Height = 0;              ///< Frame height
	double Scale = 0;                 ///< Display scale factor
	uint32_t FrameNumber = 0;         ///< Frame sequence number
	vector<ControllerData> InputData; ///< Input state for this frame
};

/// <summary>Statistics about rewind buffer state</summary>
struct RewindStats {
	uint32_t MemoryUsage;     ///< Total memory usage in bytes
	uint32_t HistorySize;     ///< Number of savestate snapshots
	uint32_t HistoryDuration; ///< Duration of history in seconds
};

/// <summary>
/// Rewind system for TAS-style frame-by-frame replay and instant rewind.
/// Saves compressed savestates + video/audio snapshots for playback.
/// </summary>
/// <remarks>
/// Rewind architecture:
/// - Saves full savestate every 30 frames (BufferSize)
/// - XOR-delta compression between consecutive states
/// - Video frames buffered separately for smooth playback
/// - Audio samples buffered for continuous playback during rewind
///
/// Memory usage:
/// - Configurable history duration (default 5-10 seconds)
/// - Compressed savestates (~10-50KB each depending on console)
/// - Video frames (uncompressed RGBA, ~300KB per frame at 256x240)
/// - Audio samples (16-bit stereo PCM)
///
/// Usage patterns:
/// 1. Normal play: Records savestate+video/audio every 30 frames
/// 2. Rewinding: Loads states backward, plays buffered video/audio
/// 3. Debugger step-back: Single-frame rewind for debugging
///
/// Performance:
/// - Minimal overhead during normal play (~3% CPU for savestate compression)
/// - Fast rewind (instant state loading, pre-rendered frames)
///
/// Thread safety: Accessed from emulation thread only.
/// </remarks>
class RewindManager : public INotificationListener, public IInputProvider, public IInputRecorder {
public:
	/// <summary>Savestate interval in frames (30 = every 0.5 seconds at 60 FPS)</summary>
	static constexpr int32_t BufferSize = 30;

private:
	Emulator* _emu = nullptr;         ///< Emulator instance
	EmuSettings* _settings = nullptr; ///< Settings reference

	bool _hasHistory = false; ///< History data available

	deque<RewindData> _history;       ///< Savestate history (main timeline)
	deque<RewindData> _historyBackup; ///< Backup history (for resume after rewind)
	RewindData _currentHistory = {};  ///< Current savestate being built

	RewindState _rewindState = RewindState::Stopped; ///< Current rewind state
	int32_t _framesToFastForward = 0;                ///< Frames to skip when resuming

	deque<VideoFrame> _videoHistory;         ///< Video frame snapshots
	vector<VideoFrame> _videoHistoryBuilder; ///< Buffer for current video segment
	deque<int16_t> _audioHistory;            ///< Audio sample history
	vector<int16_t> _audioHistoryBuilder;    ///< Buffer for current audio segment

	/// <summary>Add completed history block to deque</summary>
	void AddHistoryBlock();

	/// <summary>Remove oldest history block to free memory</summary>
	void PopHistory();

	/// <summary>Start rewind with state machine transition</summary>
	void Start(bool forDebugger);

	/// <summary>Internal start implementation</summary>
	void InternalStart(bool forDebugger);

	/// <summary>Stop rewind and return to normal play</summary>
	void Stop();

	/// <summary>Force stop rewind and optionally delete future data</summary>
	/// <param name="deleteFutureData">Delete history ahead of rewind point if true</param>
	void ForceStop(bool deleteFutureData);

	/// <summary>Process frame for recording or playback</summary>
	void ProcessFrame(RenderedFrame& frame, bool forRewind);

	/// <summary>
	/// Process audio for recording or playback.
	/// </summary>
	/// <returns>True if audio data provided (from history), false if recording</returns>
	bool ProcessAudio(int16_t* soundBuffer, uint32_t sampleCount);

	/// <summary>Clear all history buffers and reset state</summary>
	void ClearBuffer();

public:
	/// <summary>Construct rewind manager for emulator</summary>
	RewindManager(Emulator* emu);

	/// <summary>Destructor - cleans up history data</summary>
	virtual ~RewindManager();

	/// <summary>Initialize history tracking (allocate buffers)</summary>
	void InitHistory();

	/// <summary>Reset rewind state (clear history, stop rewinding)</summary>
	void Reset();

	/// <summary>Handle emulator notifications (pause, reset, etc.)</summary>
	void ProcessNotification(ConsoleNotificationType type, void* parameter) override;

	/// <summary>Called at end of each emulated frame - records or plays back</summary>
	void ProcessEndOfFrame();

	/// <summary>Record input state for replay (IInputRecorder)</summary>
	void RecordInput(vector<shared_ptr<BaseControlDevice>> devices) override;

	/// <summary>Set input from history during rewind (IInputProvider)</summary>
	bool SetInput(BaseControlDevice* device) override;

	/// <summary>
	/// Start rewinding (backward playback).
	/// </summary>
	/// <param name="forDebugger">True for debugger step-back (single frame)</param>
	void StartRewinding(bool forDebugger = false);

	/// <summary>
	/// Stop rewinding and resume normal play.
	/// </summary>
	/// <param name="forDebugger">True if debugger step-back</param>
	/// <param name="deleteFutureData">Delete history ahead of rewind point if true</param>
	void StopRewinding(bool forDebugger = false, bool deleteFutureData = false);

	/// <summary>Check if currently rewinding</summary>
	bool IsRewinding();

	/// <summary>Check if in debugger step-back mode</summary>
	bool IsStepBack();

	/// <summary>
	/// Rewind specified number of seconds.
	/// </summary>
	/// <param name="seconds">Seconds to rewind (limited by available history)</param>
	void RewindSeconds(uint32_t seconds);

	/// <summary>Check if any history data available</summary>
	bool HasHistory();

	/// <summary>Get copy of history deque (for UI/debugging)</summary>
	deque<RewindData> GetHistory();

	/// <summary>Get rewind buffer statistics</summary>
	RewindStats GetStats();

	/// <summary>Send video frame to display or history</summary>
	void SendFrame(RenderedFrame& frame, bool forRewind);

	/// <summary>
	/// Send audio to output or history.
	/// </summary>
	/// <returns>True if audio from history, false if recording</returns>
	bool SendAudio(int16_t* soundBuffer, uint32_t sampleCount);
};