#pragma once
#include "pch.h"
#include <deque>
#include "Shared/Interfaces/IInputProvider.h"
#include "Shared/RewindData.h"

class Emulator;
class BaseControlDevice;

/// <summary>Current history viewer playback state</summary>
struct HistoryViewerState {
	uint32_t Position = 0; ///< Current position in history (frames)
	uint32_t Length = 0;   ///< Total history length (frames)
	uint32_t Volume = 0;   ///< Audio volume (0-100)
	double Fps = 60.0;     ///< Playback speed (FPS)
	bool IsPaused = false; ///< Playback paused flag

	uint32_t SegmentCount = 0;    ///< Number of savestate segments
	uint32_t Segments[1000] = {}; ///< Segment frame numbers
};

/// <summary>History viewer configuration</summary>
struct HistoryViewerOptions {
	bool IsPaused = false; ///< Start paused if true
	uint32_t Volume = 100; ///< Audio volume (0-100)
	uint32_t Width = 256;  ///< Display width
	uint32_t Height = 240; ///< Display height
};

/// <summary>
/// TAS movie history viewer and editor.
/// Allows scrubbing through recorded gameplay, editing inputs, and exporting.
/// </summary>
/// <remarks>
/// Architecture:
/// - Separate Emulator instance from main emulator
/// - Copies rewind history for non-destructive playback
/// - IInputProvider interface provides recorded inputs
/// - Can export to save states or movie files
///
/// Use cases:
/// 1. Review TAS movie frame-by-frame
/// 2. Scrub timeline to find specific moments
/// 3. Export segments to save states
/// 4. Save edited movie files
/// 5. Resume gameplay from any point
///
/// Playback:
/// - SeekTo(position) jumps to frame
/// - ProcessEndOfFrame() advances playback
/// - Pausing supported via SetOptions
///
/// Export:
/// - CreateSaveState(file, position) - Save state at frame
/// - SaveMovie(file, start, end) - Export movie segment
/// - ResumeGameplay(position) - Continue from frame in main emulator
///
/// Performance:
/// - Fast seeking via savestate snapshots (every 30 frames)
/// - Reuses existing RewindData compression
/// - Separate thread avoids blocking main emulator
///
/// Thread safety: History viewer runs in separate emulation thread.
/// </remarks>
class HistoryViewer : public IInputProvider {
private:
	Emulator* _emu = nullptr;     ///< History viewer emulator instance
	Emulator* _mainEmu = nullptr; ///< Main emulator reference
	deque<RewindData> _history;   ///< Copied rewind history
	vector<uint32_t> _segmentFrames; ///< Cached segment boundary frame numbers (built once at init)
	uint32_t _position = 0;       ///< Current playback position (frames)
	uint32_t _pollCounter = 0;    ///< Input poll counter

public:
	/// <summary>Construct history viewer for emulator</summary>
	HistoryViewer(Emulator* emu);

	/// <summary>Destructor - releases resources</summary>
	virtual ~HistoryViewer();

	/// <summary>
	/// Initialize history viewer with main emulator's history.
	/// </summary>
	/// <param name="mainEmu">Main emulator to copy history from</param>
	/// <returns>True if initialized successfully</returns>
	bool Initialize(Emulator* mainEmu);

	/// <summary>Set playback options (volume, pause, resolution)</summary>
	void SetOptions(HistoryViewerOptions options);

	/// <summary>Get current playback state</summary>
	HistoryViewerState GetState();

	/// <summary>
	/// Seek to specific frame in history.
	/// </summary>
	/// <param name="seekPosition">Target frame number</param>
	/// <remarks>
	/// Uses nearest savestate snapshot and emulates forward to exact position.
	/// Fast for positions near savestate boundaries (every 30 frames).
	/// </remarks>
	void SeekTo(uint32_t seekPosition);

	/// <summary>
	/// Create save state file at specific position.
	/// </summary>
	/// <param name="outputFile">Save state filename</param>
	/// <param name="position">Frame position</param>
	/// <returns>True if save state created successfully</returns>
	bool CreateSaveState(string outputFile, uint32_t position);

	/// <summary>
	/// Save movie file for frame range.
	/// </summary>
	/// <param name="movieFile">Movie filename (.fm2, .fm3, etc.)</param>
	/// <param name="startPosition">Start frame</param>
	/// <param name="endPosition">End frame (inclusive)</param>
	/// <returns>True if movie saved successfully</returns>
	bool SaveMovie(string movieFile, uint32_t startPosition, uint32_t endPosition);

	/// <summary>
	/// Resume gameplay in main emulator from history position.
	/// </summary>
	/// <param name="resumePosition">Frame to resume from</param>
	/// <remarks>
	/// Loads savestate into main emulator and continues execution.
	/// Useful for TAS editing (scrub to position, edit, continue).
	/// </remarks>
	void ResumeGameplay(uint32_t resumePosition);

	/// <summary>Process end of frame (advance playback)</summary>
	void ProcessEndOfFrame();

	/// <summary>
	/// Provide input from recorded history (IInputProvider implementation).
	/// </summary>
	/// <param name="device">Controller device to set input for</param>
	/// <returns>True if input provided from history</returns>
	bool SetInput(BaseControlDevice* device) override;
};
