#pragma once
#include "pch.h"
#include <ctime>

class Emulator;
struct RenderedFrame;

/// <summary>
/// Origin category for save state files.
/// Determines the colored badge shown in UI and the save state's purpose.
/// </summary>
/// <remarks>
/// Badge colors:
/// - Auto (0): Blue - Background periodic saves (every 20-30 min)
/// - Save (1): Green - User-initiated saves (Quick Save with Ctrl+S)
/// - Recent (2): Red - Recent play checkpoints (automatic 5-min interval queue)
/// - Lua (3): Yellow - Script-created saves
///
/// File naming:
/// - Auto: {RomName}_auto.nexen-save
/// - Save: {RomName}_{YYYY-MM-DD}_{HH-mm-ss}.nexen-save
/// - Recent: {RomName}_recent_{01-12}.nexen-save
/// - Lua: {RomName}_lua_{timestamp}.nexen-save
/// </remarks>
enum class SaveStateOrigin : uint8_t {
	Auto = 0,    ///< Auto-save (blue badge) - periodic background saves
	Save = 1,    ///< User save (green badge) - Quick Save (Ctrl+S)
	Recent = 2,  ///< Recent play (red badge) - 5-min interval queue
	Lua = 3      ///< Lua script (yellow badge) - script-created saves
};

/// <summary>
/// Metadata for a single save state file.
/// Used by GetSaveStateList() to return save state information to the UI.
/// </summary>
struct SaveStateInfo {
	string filepath;        ///< Full path to the save state file
	string romName;         ///< ROM name this save is for
	time_t timestamp;       ///< Unix timestamp when save was created (from filename)
	uint32_t fileSize;      ///< File size in bytes
	SaveStateOrigin origin; ///< Origin category (Auto/Save/Recent/Lua)
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
/// Save Categories:
/// - Designated Save: Single user-selected save for quick load (F4)
/// - Quick Save: User-initiated timestamped saves (Ctrl+S)
/// - Recent Play: Automatic 5-min interval rotating queue (12 saves max)
/// - Auto Save: Periodic background saves (20-30 min intervals)
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
	static constexpr uint32_t MaxIndex = 10; ///< Maximum slot index (0-9) [LEGACY]

	// Recent Play Queue constants
	static constexpr uint32_t RecentPlayMaxSlots = 12;      ///< Maximum Recent Play saves
	static constexpr uint32_t RecentPlayIntervalSec = 300;  ///< 5 minutes between saves

	atomic<uint32_t> _lastIndex;      ///< Last used save state slot [LEGACY]
	atomic<uint32_t> _recentPlaySlot; ///< Current Recent Play slot (0-11, wraps)
	time_t _lastRecentPlayTime;       ///< Last Recent Play save timestamp
	string _designatedSavePath;       ///< Path to user-designated quick-load save
	string _perRomSaveStateDir;       ///< Per-ROM save state directory (set by C# GameDataManager)
	Emulator* _emu;                   ///< Emulator instance

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
	/// Format: {SaveStateFolder}/{RomName}/{RomName}_{YYYY-MM-DD}_{HH-mm-ss}.nexen-save
	/// </summary>
	/// <returns>Full path for new timestamped save state</returns>
	string GetTimestampedFilepath();

	/// <summary>
	/// Generate filepath for a Recent Play slot.
	/// Format: {SaveStateFolder}/{RomName}/{RomName}_recent_{01-12}.nexen-save
	/// </summary>
	/// <param name="slotIndex">Slot index (0-11)</param>
	/// <returns>Full path for the Recent Play slot</returns>
	string GetRecentPlayFilepath(uint32_t slotIndex);

	/// <summary>
	/// Generate filepath for the Auto Save state.
	/// Format: {SaveStateFolder}/{RomName}/{RomName}_auto.nexen-save
	/// </summary>
	/// <returns>Full path for the Auto Save state</returns>
	string GetAutoSaveFilepath();

	/// <summary>
	/// Parse timestamp from a timestamped save state filename.
	/// </summary>
	/// <param name="filename">Filename (without path) to parse</param>
	/// <returns>Unix timestamp, or 0 if parsing failed</returns>
	time_t ParseTimestampFromFilename(const string& filename);

	/// <summary>Save screenshot to stream (PNG compressed)</summary>
	void SaveVideoData(ostream& stream);

	/// <summary>
	/// Load screenshot from stream.
	/// </summary>
	/// <param name="out">Output PNG data</param>
	/// <param name="frame">Frame info (width, height)</param>
	/// <param name="stream">Input stream</param>
	/// <returns>True if screenshot loaded successfully</returns>
	bool GetVideoData(vector<uint8_t>& out, RenderedFrame& frame, istream& stream);

	/// <summary>Write 32-bit value to stream (little-endian)</summary>
	void WriteValue(ostream& stream, uint32_t value);

	/// <summary>Read 32-bit value from stream (little-endian)</summary>
	uint32_t ReadValue(istream& stream);

public:
	static constexpr uint32_t FileFormatVersion = 4;       ///< Current save state version
	static constexpr uint32_t MinimumSupportedVersion = 3; ///< Oldest loadable version
	static constexpr uint32_t AutoSaveStateIndex = 11;     ///< Auto-save slot index

	/// <summary>Construct save state manager for emulator</summary>
	SaveStateManager(Emulator* emu);

	/// <summary>Save state to last used slot</summary>
	void SaveState();

	/// <summary>Load state from last used slot</summary>
	bool LoadState();

	/// <summary>Write save state header to stream (without state data)</summary>
	void GetSaveStateHeader(ostream& stream);

	/// <summary>
	/// Save complete state to stream.
	/// </summary>
	/// <param name="stream">Output stream</param>
	void SaveState(ostream& stream);

	/// <summary>
	/// Save state to file.
	/// </summary>
	/// <param name="filepath">Output file path</param>
	/// <param name="showSuccessMessage">Display success message if true</param>
	bool SaveState(string filepath, bool showSuccessMessage = true);

	/// <summary>
	/// Save state to numbered slot.
	/// </summary>
	/// <param name="stateIndex">Slot index (0-11)</param>
	/// <param name="displayMessage">Display message if true</param>
	void SaveState(int stateIndex, bool displayMessage = true);

	/// <summary>
	/// Load state from stream.
	/// </summary>
	/// <param name="stream">Input stream</param>
	bool LoadState(istream& stream);

	/// <summary>
	/// Load state from file.
	/// </summary>
	/// <param name="filepath">Input file path</param>
	/// <param name="showSuccessMessage">Display success message if true</param>
	bool LoadState(string filepath, bool showSuccessMessage = true);

	/// <summary>
	/// Load state from numbered slot.
	/// </summary>
	/// <param name="stateIndex">Slot index (0-11)</param>
	bool LoadState(int stateIndex);

	/// <summary>
	/// Save recent game info for quick resume.
	/// </summary>
	/// <param name="romName">ROM name</param>
	/// <param name="romPath">ROM file path</param>
	/// <param name="patchPath">Patch file path (if any)</param>
	void SaveRecentGame(string romName, string romPath, string patchPath);

	/// <summary>
	/// Load recent game.
	/// </summary>
	/// <param name="filename">Recent game filename</param>
	/// <param name="resetGame">Reset game after loading if true</param>
	void LoadRecentGame(string filename, bool resetGame);

	/// <summary>
	/// Get save state screenshot preview.
	/// </summary>
	/// <param name="saveStatePath">Save state file path</param>
	/// <param name="pngData">Output PNG data buffer</param>
	/// <returns>PNG data size in bytes, or -1 on error</returns>
	int32_t GetSaveStatePreview(string saveStatePath, uint8_t* pngData);

	/// <summary>
	/// Select save state slot for next save/load.
	/// </summary>
	/// <param name="slotIndex">Slot index (0-11)</param>
	void SelectSaveSlot(int slotIndex);

	/// <summary>Move to next save slot (wraps around)</summary>
	void MoveToNextSlot();

	/// <summary>Move to previous save slot (wraps around)</summary>
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

	// ========== Recent Play Queue Methods ==========

	/// <summary>
	/// Save a Recent Play checkpoint.
	/// Uses rotating slots (1-12) with FIFO replacement.
	/// Called automatically every 5 minutes during gameplay.
	/// </summary>
	/// <returns>Full path to the saved file, or empty string on failure</returns>
	string SaveRecentPlayState();

	/// <summary>
	/// Check if enough time has passed for a new Recent Play save.
	/// </summary>
	/// <returns>True if 5+ minutes since last Recent Play save</returns>
	bool ShouldSaveRecentPlay();

	/// <summary>
	/// Reset the Recent Play timer (e.g., when loading a ROM).
	/// </summary>
	void ResetRecentPlayTimer();

	/// <summary>
	/// Get Recent Play saves only, sorted newest first.
	/// </summary>
	/// <returns>Vector of SaveStateInfo with origin=Recent</returns>
	vector<SaveStateInfo> GetRecentPlayStates();

	// ========== Designated Save Methods ==========

	/// <summary>
	/// Set the designated save state for quick loading (F4).
	/// The save state at the given path becomes the "quick load" target.
	/// </summary>
	/// <param name="filepath">Path to save state to designate</param>
	void SetDesignatedSave(const string& filepath);

	/// <summary>
	/// Get the current designated save path.
	/// </summary>
	/// <returns>Path to designated save, or empty if none set</returns>
	string GetDesignatedSave() const;

	/// <summary>
	/// Load the designated save state (F4 action).
	/// </summary>
	/// <returns>True if load succeeded</returns>
	bool LoadDesignatedState();

	/// <summary>
	/// Check if a designated save is set and valid.
	/// </summary>
	/// <returns>True if designated save exists and file is valid</returns>
	bool HasDesignatedSave() const;

	/// <summary>
	/// Clear the designated save (unset).
	/// </summary>
	void ClearDesignatedSave();

	// ========== Per-ROM Directory Override ==========

	/// <summary>
	/// Set the per-ROM save state directory (called from C# on ROM load).
	/// When set, all timestamped saves, recent plays, and auto-saves go here
	/// instead of the default {SaveStateFolder}/{RomName}/ path.
	/// </summary>
	/// <param name="path">Full path to per-ROM save state directory, or empty to use default</param>
	void SetPerRomSaveStateDirectory(const string& path);
};
