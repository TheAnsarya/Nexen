#pragma once

#include "pch.h"
#include <array>
#include <deque>
#include "Core/Shared/Interfaces/INotificationListener.h"
#include "Utilities/AutoResetEvent.h"

class VirtualFile;
class Emulator;

/// <summary>ROM test result states</summary>
enum class RomTestState {
	Failed,            ///< Test failed (mismatched frames)
	Passed,            ///< Test passed (all frames matched)
	PassedWithWarnings ///< Test passed with warnings (some tolerances exceeded)
};

/// <summary>ROM test execution result</summary>
struct RomTestResult {
	RomTestState State; ///< Test outcome
	int32_t ErrorCode;  ///< Error code (0 = success, negative = failure)
};

/// <summary>
/// Automated ROM test recorder and validator.
/// Records video frame hashes during gameplay, validates playback matches recording.
/// </summary>
/// <remarks>
/// Purpose:
/// - Detect emulation regressions (timing, PPU, CPU bugs)
/// - Automated test suite for ROM compatibility
/// - TAS movie validation
///
/// Recording process:
/// 1. Record(filename, reset) - Start recording
/// 2. ProcessNotification(EndOfFrame) - Hash each frame (MD5)
/// 3. RLE compression for duplicate frames
/// 4. Save() - Write test file
///
/// Playback/validation:
/// 1. Run(filename) - Load test file
/// 2. ProcessNotification(EndOfFrame) - Hash each frame
/// 3. Compare against recorded hashes
/// 4. Return result (Passed/Failed/PassedWithWarnings)
///
/// File format:
/// - Frame hash (MD5, 16 bytes)
/// - Repetition count (RLE compressed)
/// - Deque of hashes with counts
///
/// Failure detection:
/// - _badFrameCount tracks consecutive mismatches
/// - Tolerance: 10 bad frames before failure
/// - PassedWithWarnings if some frames differ but within tolerance
///
/// Usage:
/// <code>
/// auto test = make_shared<RecordedRomTest>(&emu, false);
/// test->Record("test.mtp", true);
/// // ... play game ...
/// test->Stop();
///
/// // Later, validate:
/// RomTestResult result = test->Run("test.mtp");
/// if (result.State == RomTestState::Passed) { ... }
/// </code>
///
/// Thread safety: Uses AutoResetEvent for synchronization between threads.
/// </remarks>
class RecordedRomTest : public INotificationListener, public std::enable_shared_from_this<RecordedRomTest> {
private:
	Emulator* _emu; ///< Emulator instance

	bool _inBackground = false;    ///< Run in background flag
	bool _recording = false;       ///< Currently recording flag
	bool _runningTest = false;     ///< Currently running test flag
	int _badFrameCount = 0;        ///< Consecutive bad frame counter
	bool _isLastFrameGood = false; ///< Last frame matched flag

	uint8_t _previousHash[16] = {};                        ///< Previous frame MD5 hash
	std::deque<std::array<uint8_t, 16>> _screenshotHashes; ///< Recorded frame hashes (no manual memory management)
	std::deque<uint8_t> _repetitionCount;                  ///< RLE repetition counts
	uint8_t _currentCount = 0;                             ///< Current repetition count

	string _filename; ///< Test file path
	ofstream _file;   ///< Output file stream

	AutoResetEvent _signal; ///< Synchronization event

private:
	/// <summary>Reset test state (clear hashes, counters)</summary>
	void Reset();

	/// <summary>Validate current frame against recorded hash</summary>
	void ValidateFrame();

	/// <summary>Save current frame hash to recording</summary>
	void SaveFrame();

	/// <summary>Write test file to disk</summary>
	void Save();

public:
	/// <summary>
	/// Construct ROM test recorder/validator.
	/// </summary>
	/// <param name="console">Emulator instance</param>
	/// <param name="inBackground">Run in background mode if true</param>
	RecordedRomTest(Emulator* console, bool inBackground);

	/// <summary>Destructor - stops recording/testing</summary>
	virtual ~RecordedRomTest();

	/// <summary>
	/// Process emulator notifications (EndOfFrame, etc.).
	/// </summary>
	void ProcessNotification(ConsoleNotificationType type, void* parameter) override;

	/// <summary>
	/// Start recording test.
	/// </summary>
	/// <param name="filename">Output test filename</param>
	/// <param name="reset">Reset emulator before recording if true</param>
	void Record(string filename, bool reset);

	/// <summary>
	/// Run validation test.
	/// </summary>
	/// <param name="filename">Test file to validate against</param>
	/// <returns>Test result (Passed/Failed/PassedWithWarnings)</returns>
	RomTestResult Run(string filename);

	/// <summary>Stop recording or testing</summary>
	void Stop();
};