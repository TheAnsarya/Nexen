#pragma once
#include "pch.h"
#include <ctime>

class Emulator;
struct RenderedFrame;

/// <summary>
/// Metadata for a single save state file.
/// Used by GetSaveStateList() to return save state information to the UI.
/// </summary>
struct SaveStateInfo {
	string filepath;    ///< Full path to the save state file
	string romName;     ///< ROM name this save is for
	time_t timestamp;   ///< Unix timestamp when save was created (from filename)
	uint32_t fileSize;  ///< File size in bytes
};

/// <summary>
/// Save state manager for creating/loading/managing save states.
/// Supports file-based and stream-based save states with screenshot previews.
/// </summary>
/// <remarks>
/// Save state format:
/// - Header with file format version, emulator version, ROM info
/// - Screenshot (PNG compressed) for preview
/// - Compressed emulator state (zlib)
/// - Settings (optional)
///
/// Storage modes:
/// - Legacy slot mode: 10 numbered slots (0-9) + 1 auto-save slot (index 11)
/// - Timestamped mode: Unlimited saves with datetime-based naming
///   - Files stored in per-ROM subdirectories
///   - Format: {RomName}/{RomName}_{YYYY-MM-DD}_{HH-mm-ss}.mss
///
/// Save state features:
/// - Full emulator state (CPU, PPU, APU, memory, etc.)
/// - Versioning support (backward compatible to version 3)
/// - Screenshot preview extraction
/// - Settings preservation option
///
/// Performance:
/// - Atomic _lastIndex for thread-safe slot tracking
/// - Compression reduces state size (~10-50KB depending on console)
/// - Fast save/load (~1-2ms for NES, ~5-10ms for SNES)
///
/// Thread safety: All methods should be called with EmulatorLock held.
/// </remarks>
class SaveStateManager {
private:
	static constexpr uint32_t MaxIndex = 10;

	atomic<uint32_t> _lastIndex;
	Emulator* _emu;

	/// <summary>
	/// Get filesystem path for save state slot (legacy mode).
	/// </summary>
	/// <param name="stateIndex">Slot index (0-11)</param>
	string GetStateFilepath(int stateIndex);

	/// <summary>
	/// Get the per-ROM save state directory path.
	/// Creates the directory if it doesn't exist.
	/// </summary>
	/// <returns>Path to ROM-specific save state folder</returns>
	string GetRomSaveStateDirectory();

	/// <summary>
	/// Generate a timestamped filepath for a new save state.
	/// Format: {SaveStateFolder}/{RomName}/{RomName}_{YYYY-MM-DD}_{HH-mm-ss}.mss
	/// </summary>
	/// <returns>Full path for new timestamped save state</returns>
	string GetTimestampedFilepath();

	/// <summary>
	/// Parse timestamp from a timestamped save state filename.
	/// </summary>
	/// <param name="filename">Filename (without path) to parse</param>
	/// <returns>Unix timestamp, or 0 if parsing failed</returns>
	time_t ParseTimestampFromFilename(const string& filename);

	/// <summary>Save screenshot to stream (PNG compressed)</summary>
	void SaveVideoData(ostream& stream);
	bool GetVideoData(vector<uint8_t>& out, RenderedFrame& frame, istream& stream);

	void WriteValue(ostream& stream, uint32_t value);
	uint32_t ReadValue(istream& stream);

public:
	static constexpr uint32_t FileFormatVersion = 4;
	static constexpr uint32_t MinimumSupportedVersion = 3;
	static constexpr uint32_t AutoSaveStateIndex = 11;

	SaveStateManager(Emulator* emu);

	void SaveState();
	bool LoadState();

	void GetSaveStateHeader(ostream & stream);

	void SaveState(ostream &stream);
	bool SaveState(string filepath, bool showSuccessMessage = true);
	void SaveState(int stateIndex, bool displayMessage = true);
	bool LoadState(istream &stream);
	bool LoadState(string filepath, bool showSuccessMessage = true);
	bool LoadState(int stateIndex);

	void SaveRecentGame(string romName, string romPath, string patchPath);
	void LoadRecentGame(string filename, bool resetGame);

	int32_t GetSaveStatePreview(string saveStatePath, uint8_t* pngData);

	void SelectSaveSlot(int slotIndex);
	void MoveToNextSlot();
	void MoveToPreviousSlot();

	// ========== Timestamped Save State Methods (New) ==========

	/// <summary>
	/// Save a new timestamped save state.
	/// Creates a save state with datetime-based filename in the ROM's subdirectory.
	/// </summary>
	/// <returns>Full path to the saved file, or empty string on failure</returns>
	string SaveTimestampedState();

	/// <summary>
	/// Get list of all save states for the current ROM.
	/// Returns saves from the ROM's subdirectory, sorted by timestamp (newest first).
	/// </summary>
	/// <returns>Vector of SaveStateInfo structs</returns>
	vector<SaveStateInfo> GetSaveStateList();

	/// <summary>
	/// Delete a specific save state file.
	/// </summary>
	/// <param name="filepath">Full path to the save state file to delete</param>
	/// <returns>True if deletion succeeded</returns>
	bool DeleteSaveState(const string& filepath);

	/// <summary>
	/// Get the number of save states for the current ROM.
	/// </summary>
	/// <returns>Count of save state files</returns>
	uint32_t GetSaveStateCount();
};