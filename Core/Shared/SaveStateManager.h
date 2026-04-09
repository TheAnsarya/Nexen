#pragma once
#include "pch.h"
#include <ctime>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

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
/// - Designated (4): Purple - Designated quick-access slots (F2-F4)
///
/// File naming:
/// - Auto: {RomName}_auto.nexen-save
/// - Save: {RomName}_{YYYY-MM-DD}_{HH-mm-ss}.nexen-save
/// - Recent: {RomName}_recent_{01-12}.nexen-save
/// - Lua: {RomName}_lua_{timestamp}.nexen-save
/// - Designated: {RomName}_designated_{1-3}_{YYYY-MM-DD}_{HH-mm-ss}.nexen-save
/// </remarks>
enum class SaveStateOrigin : uint8_t {
	Auto = 0,        ///< Auto-save (blue badge) - periodic background saves
	Save = 1,        ///< User save (green badge) - Quick Save (Ctrl+S)
	Recent = 2,      ///< Recent play (red badge) - 5-min interval queue
	Lua = 3,         ///< Lua script (yellow badge) - script-created saves
	Designated = 4   ///< Designated slot (purple badge) - quick-access F2-F4 saves
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
	bool isPaused = false;  ///< Whether the emulator was paused when this state was saved (v5+)
};

/// <summary>
/// Captured state snapshot for background writing.
/// Contains all data needed to write a save state file without holding the emulator lock.
/// </summary>
struct SaveStateSnapshot {
	vector<uint8_t> stateData;    ///< Serialized emulator state (uncompressed)
	vector<uint8_t> frameBuffer;  ///< Raw framebuffer copy for screenshot
	uint32_t frameBufferSize = 0; ///< Frame buffer size in bytes
	uint32_t frameWidth = 0;      ///< Frame width in pixels
	uint32_t frameHeight = 0;     ///< Frame height in pixels
	uint32_t frameScale100 = 0;   ///< Frame scale * 100 (e.g., 200 = 2.0x)
	uint32_t emuVersion = 0;      ///< Emulator version at time of capture
	uint32_t consoleType = 0;     ///< Console type (cast from ConsoleType)
	string romName;               ///< ROM filename without path
	string filepath;              ///< Target save state file path
	bool showSuccessMessage = false; ///< Display success message after write
	bool isPaused = false;        ///< Whether emulator was paused at time of capture
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
	string _perRomSaveStateDir;       ///< Per-ROM save state directory (set by C# GameDataManager)
	Emulator* _emu;                   ///< Emulator instance

	static constexpr uint32_t DesignatedSlotCount = 3; ///< Number of designated save slots (F2-F4)

	/// <summary>Persistent compression buffer for SaveVideoData (avoids ~150KB alloc per save)</summary>
	vector<uint8_t> _compressBuffer;

	/// <summary>Persistent decompression buffer for GetVideoData (avoids ~150KB alloc per load/rewind)</summary>
	vector<uint8_t> _decompressBuffer;

	/// <summary>Persistent buffer for reading compressed video data (avoids ~150KB alloc per load/rewind)</summary>
	vector<uint8_t> _compressedReadBuffer;

	/// <summary>Persistent compression buffer for background frame writes (single-threaded, no lock needed)</summary>
	vector<uint8_t> _bgCompressFrameBuffer;

	/// <summary>Persistent compression buffer for background state writes (single-threaded, no lock needed)</summary>
	vector<uint8_t> _bgCompressStateBuffer;

	// ========== Async Write Infrastructure ==========
	std::thread _writeThread;
	std::queue<SaveStateSnapshot> _writeQueue;
	std::mutex _writeMutex;
	std::condition_variable _writeCv;
	bool _shutdownRequested = false;

	/// <summary>Background thread loop — dequeues snapshots, compresses, writes to disk</summary>
	void BackgroundWriteLoop();

	/// <summary>Write a single snapshot to disk (compression + file I/O)</summary>
	void WriteSnapshotToDisk(SaveStateSnapshot& snapshot);

	/// <summary>
	/// Get filesystem path for save state slot (legacy mode).
	/// </summary>
	/// <param name="stateIndex">Slot index (0-11)</param>
	[[nodiscard]] string GetStateFilepath(int stateIndex);

	/// <summary>
	/// Get the per-ROM save state directory path.
	/// Creates the directory if it doesn't exist.
	/// </summary>
	/// <returns>Path to ROM-specific save state folder</returns>
	[[nodiscard]] string GetRomSaveStateDirectory();

	/// <summary>
	/// Generate a timestamped filepath for a new save state.
	/// Format: {SaveStateFolder}/{RomName}/{RomName}_{YYYY-MM-DD}_{HH-mm-ss}.nexen-save
	/// </summary>
	/// <returns>Full path for new timestamped save state</returns>
	[[nodiscard]] string GetTimestampedFilepath();

	/// <summary>
	/// Generate filepath for a Recent Play slot.
	/// Format: {SaveStateFolder}/{RomName}/{RomName}_recent_{01-12}.nexen-save
	/// </summary>
	/// <param name="slotIndex">Slot index (0-11)</param>
	/// <returns>Full path for the Recent Play slot</returns>
	[[nodiscard]] string GetRecentPlayFilepath(uint32_t slotIndex);

	/// <summary>
	/// Generate filepath for the Auto Save state.
	/// Format: {SaveStateFolder}/{RomName}/{RomName}_auto.nexen-save
	/// </summary>
	/// <returns>Full path for the Auto Save state</returns>
	[[nodiscard]] string GetAutoSaveFilepath();


	/// <summary>Save screenshot to stream (PNG compressed)</summary>
	void SaveVideoData(ostream& stream);

	/// <summary>
	/// Load screenshot from stream.
	/// </summary>
	/// <param name="out">Output PNG data</param>
	/// <param name="frame">Frame info (width, height)</param>
	/// <param name="stream">Input stream</param>
	/// <returns>True if screenshot loaded successfully</returns>
	[[nodiscard]] bool GetVideoData(vector<uint8_t>& out, RenderedFrame& frame, istream& stream);

	/// <summary>Write 32-bit value to stream (little-endian)</summary>
	void WriteValue(ostream& stream, uint32_t value);

	/// <summary>Read 32-bit value from stream (little-endian)</summary>
	[[nodiscard]] uint32_t ReadValue(istream& stream);

public:
	static constexpr uint32_t FileFormatVersion = 5;       ///< Current save state version
	static constexpr uint32_t MinimumSupportedVersion = 3; ///< Oldest loadable version
	static constexpr uint32_t AutoSaveStateIndex = 11;     ///< Auto-save slot index

	/// <summary>
	/// Parse timestamp from a timestamped save state filename.
	/// </summary>
	/// <param name="filename">Filename (without path) to parse</param>
	/// <returns>Unix timestamp, or 0 if parsing failed</returns>
	[[nodiscard]] static time_t ParseTimestampFromFilename(const string& filename);

	/// <summary>Construct save state manager for emulator</summary>
	SaveStateManager(Emulator* emu);

	/// <summary>Save state to last used slot</summary>
	void SaveState();

	/// <summary>Load state from last used slot</summary>
	[[nodiscard]] bool LoadState();

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
	[[nodiscard]] bool SaveState(const string& filepath, bool showSuccessMessage = true);

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
	[[nodiscard]] bool LoadState(istream& stream);

	/// <summary>
	/// Load state from file.
	/// </summary>
	/// <param name="filepath">Input file path</param>
	/// <param name="showSuccessMessage">Display success message if true</param>
	[[nodiscard]] bool LoadState(const string& filepath, bool showSuccessMessage = true);

	/// <summary>
	/// Load state from numbered slot.
	/// </summary>
	/// <param name="stateIndex">Slot index (0-11)</param>
	[[nodiscard]] bool LoadState(int stateIndex);

	/// <summary>
	/// Save recent game info for quick resume.
	/// </summary>
	/// <param name="romName">ROM name</param>
	/// <param name="romPath">ROM file path</param>
	/// <param name="patchPath">Patch file path (if any)</param>
	void SaveRecentGame(const string& romName, const string& romPath, const string& patchPath);

	/// <summary>
	/// Load recent game.
	/// </summary>
	/// <param name="filename">Recent game filename</param>
	/// <param name="resetGame">Reset game after loading if true</param>
	void LoadRecentGame(const string& filename, bool resetGame);

	/// <summary>
	/// Get save state screenshot preview.
	/// </summary>
	/// <param name="saveStatePath">Save state file path</param>
	/// <param name="pngData">Output PNG data buffer</param>
	/// <returns>PNG data size in bytes, or -1 on error</returns>
	[[nodiscard]] int32_t GetSaveStatePreview(const string& saveStatePath, uint8_t* pngData);

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
	[[nodiscard]] string SaveTimestampedState();

	/// <summary>
	/// Get list of all save states for the current ROM.
	/// Returns saves from the ROM's subdirectory, sorted by timestamp (newest first).
	/// </summary>
	/// <returns>Vector of SaveStateInfo structs</returns>
	[[nodiscard]] vector<SaveStateInfo> GetSaveStateList();

	/// <summary>
	/// Delete a specific save state file.
	/// </summary>
	/// <param name="filepath">Full path to the save state file to delete</param>
	/// <returns>True if deletion succeeded</returns>
	[[nodiscard]] bool DeleteSaveState(const string& filepath);

	/// <summary>
	/// Get the number of save states for the current ROM.
	/// </summary>
	/// <returns>Count of save state files</returns>
	[[nodiscard]] uint32_t GetSaveStateCount();

	// ========== Recent Play Queue Methods ==========

	/// <summary>
	/// Save a Recent Play checkpoint.
	/// Uses rotating slots (1-12) with FIFO replacement.
	/// Called automatically every 5 minutes during gameplay.
	/// </summary>
	/// <returns>Full path to the saved file, or empty string on failure</returns>
	[[nodiscard]] string SaveRecentPlayState();

	/// <summary>
	/// Check if enough time has passed for a new Recent Play save.
	/// </summary>
	/// <returns>True if 5+ minutes since last Recent Play save</returns>
	[[nodiscard]] bool ShouldSaveRecentPlay();

	/// <summary>
	/// Reset the Recent Play timer (e.g., when loading a ROM).
	/// </summary>
	void ResetRecentPlayTimer();

	/// <summary>
	/// Get Recent Play saves only, sorted newest first.
	/// </summary>
	/// <returns>Vector of SaveStateInfo with origin=Recent</returns>
	[[nodiscard]] vector<SaveStateInfo> GetRecentPlayStates();

	// ========== Designated Save Methods ==========

	/// <summary>
	/// Get a new timestamped filepath for a designated save slot.
	/// Format: {SaveStateDirectory}/{RomName}_designated_{slot+1}_{YYYY-MM-DD}_{HH-mm-ss}.nexen-save
	/// Each save creates a new file; old saves remain as history.
	/// </summary>
	/// <param name="slot">Slot index (0-2)</param>
	/// <returns>Full path for a new designated save</returns>
	[[nodiscard]] string GetDesignatedSaveFilepath(uint32_t slot);

	/// <summary>
	/// Find the latest (most recent) designated save for a slot.
	/// Searches the save directory for files matching the designated pattern
	/// and returns the one with the newest timestamp.
	/// </summary>
	/// <param name="slot">Slot index (0-2)</param>
	/// <returns>Path to latest designated save, or empty if none found</returns>
	[[nodiscard]] string FindLatestDesignatedSave(uint32_t slot) const;

	/// <summary>
	/// Save state to a designated slot (F2-F4 action).
	/// Creates a new timestamped file; previous saves remain accessible in history.
	/// </summary>
	/// <param name="slot">Slot index (0-2)</param>
	void SaveDesignatedState(uint32_t slot);

	/// <summary>
	/// Load the most recent designated save state for a slot (F2-F4 action).
	/// </summary>
	/// <param name="slot">Slot index (0-2), defaults to 0</param>
	/// <returns>True if load succeeded</returns>
	[[nodiscard]] bool LoadDesignatedState(uint32_t slot = 0);

	/// <summary>
	/// Check if a designated save slot has any valid save files.
	/// </summary>
	/// <param name="slot">Slot index (0-2), defaults to 0</param>
	/// <returns>True if at least one designated save exists for this slot</returns>
	[[nodiscard]] bool HasDesignatedSave(uint32_t slot = 0) const;

	/// <summary>
	/// Set the designated save state for quick loading (legacy compatibility).
	/// The save state at the given path becomes the "quick load" target for slot 0.
	/// </summary>
	/// <param name="filepath">Path to save state to designate</param>
	void SetDesignatedSave(const string& filepath);

	/// <summary>
	/// Get the current designated save path for slot 0 (legacy compatibility).
	/// </summary>
	/// <returns>Path to designated save, or empty if none set</returns>
	[[nodiscard]] string GetDesignatedSave() const;

	/// <summary>
	/// Clear all designated saves.
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

	/// <summary>Block until all pending background writes are complete</summary>
	void FlushPendingWrites();

	/// <summary>Shut down the background writer thread (called on destruction)</summary>
	~SaveStateManager();
};
