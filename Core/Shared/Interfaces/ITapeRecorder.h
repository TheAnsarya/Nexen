#pragma once
#include "pch.h"

/// <summary>
/// Cassette tape recorder actions.
/// </summary>
enum class TapeRecorderAction {
	Play,        ///< Play existing tape file
	StartRecord, ///< Start recording to new tape
	StopRecord   ///< Stop recording and finalize tape
};

/// <summary>
/// Interface for cassette tape emulation (Famicom Data Recorder, etc.).
/// </summary>
/// <remarks>
/// Supported systems:
/// - Famicom: Data Recorder (Family BASIC, Excitebike track editor)
/// - Game Boy: Not used (no cassette peripherals)
///
/// Tape format:
/// - WAV audio file (PCM samples)
/// - FSK modulation (frequency-shift keying)
/// - Typical baud rates: 300-1200 bps
///
/// Recording flow:
/// 1. StartRecord - Create new tape file
/// 2. Console writes data via audio output
/// 3. Tape encoder converts to FSK audio
/// 4. StopRecord - Finalize and close file
///
/// Playback flow:
/// 1. Play - Load existing tape file
/// 2. Tape decoder converts FSK to data
/// 3. Console reads data via audio input
///
/// Thread model:
/// - ProcessTapeRecorderAction() called from UI thread
/// - Tape I/O happens on emulation thread (audio callbacks)
/// </remarks>
class ITapeRecorder {
public:
	/// <summary>
	/// Process tape recorder action (play, record, stop).
	/// </summary>
	/// <param name="action">Action to perform</param>
	/// <param name="filename">Tape file path (.wav extension)</param>
	/// <remarks>
	/// Filename usage:
	/// - Play: Path to existing tape file
	/// - StartRecord: Path for new recording
	/// - StopRecord: Ignored (uses path from StartRecord)
	/// </remarks>
	virtual void ProcessTapeRecorderAction(TapeRecorderAction action, string filename) = 0;

	/// <summary>
	/// Check if currently recording.
	/// </summary>
	/// <returns>True if recording in progress</returns>
	virtual bool IsRecording() = 0;
};
