#pragma once
#include "pch.h"

/// <summary>
/// Interface for custom battery (save data) loading providers.
/// </summary>
class IBatteryProvider {
public:
	/// <summary>
	/// Load battery data with specified file extension.
	/// </summary>
	/// <param name="extension">File extension (e.g., ".sav", ".srm")</param>
	/// <returns>Battery file contents as byte vector, or empty vector if not found</returns>
	virtual vector<uint8_t> LoadBattery(const string& extension) = 0;
};

/// <summary>
/// Interface for recording battery save/load operations (for movies, replays, etc.).
/// </summary>
class IBatteryRecorder {
public:
	/// <summary>
	/// Callback invoked when battery data is loaded.
	/// </summary>
	/// <param name="extension">File extension that was loaded</param>
	/// <param name="batteryData">Loaded battery data contents</param>
	virtual void OnLoadBattery(const string& extension, vector<uint8_t> batteryData) = 0;
};

/// <summary>
/// Manages battery-backed save data (SRAM, EEPROM, Flash) with optional custom providers.
/// Uses std::span for efficient memory views (C++20 feature) and [[nodiscard]] for safety.
/// </summary>
/// <remarks>
/// Battery data is typically stored as .sav files in the same directory as the ROM.
/// Supports custom providers for alternative storage (network, archives, etc.).
/// Uses weak_ptr to avoid circular dependencies with emulator core.
/// </remarks>
class BatteryManager {
private:
	string _romName;          ///< ROM name for constructing save file paths
	string _perRomSaveDir;    ///< Per-ROM save directory override (set by C# GameDataManager on ROM load)
	bool _hasBattery = false; ///< Battery-backed save flag

	std::weak_ptr<IBatteryProvider> _provider; ///< Optional custom battery provider
	std::weak_ptr<IBatteryRecorder> _recorder; ///< Optional battery operation recorder

	/// <summary>
	/// Construct base file path for battery file with extension.
	/// </summary>
	/// <param name="extension">File extension (including dot)</param>
	/// <returns>Full path to battery file</returns>
	string GetBasePath(const string& extension);

public:
	/// <summary>
	/// Initialize battery manager with ROM name.
	/// </summary>
	/// <param name="romName">Name of ROM file (without path or extension)</param>
	/// <param name="setBatteryFlag">If true, sets internal battery flag (for games with save support)</param>
	void Initialize(const string& romName, bool setBatteryFlag = false);

	/// <summary>
	/// Check if battery-backed save support is enabled.
	/// </summary>
	/// <returns>True if battery flag is set</returns>
	/// <remarks>
	/// [[nodiscard]] prevents accidentally discarding the boolean result.
	/// </remarks>
	[[nodiscard]] bool HasBattery() { return _hasBattery; }

	/// <summary>
	/// Set custom battery provider for alternative storage.
	/// </summary>
	/// <param name="provider">Shared pointer to provider implementation</param>
	/// <remarks>
	/// Uses weak_ptr internally to avoid circular references.
	/// </remarks>
	void SetBatteryProvider(shared_ptr<IBatteryProvider> provider);

	/// <summary>
	/// Set battery recorder for tracking save/load operations.
	/// </summary>
	/// <param name="recorder">Shared pointer to recorder implementation</param>
	/// <remarks>
	/// Used for movie recording to capture exact save data used during playback.
	/// </remarks>
	void SetBatteryRecorder(shared_ptr<IBatteryRecorder> recorder);

	/// <summary>
	/// Save battery data to file using std::span for efficient memory view.
	/// </summary>
	/// <param name="extension">File extension (e.g., ".sav", ".srm")</param>
	/// <param name="data">Read-only span of battery data to save</param>
	/// <remarks>
	/// std::span avoids copying data - operates on original buffer.
	/// Uses const qualifier to prevent accidental modification.
	/// C++20 feature for modern zero-cost abstraction.
	/// </remarks>
	void SaveBattery(const string& extension, std::span<const uint8_t> data);

	/// <summary>
	/// Load battery data from file into new vector.
	/// </summary>
	/// <param name="extension">File extension to load</param>
	/// <returns>Vector containing loaded battery data, or empty vector if file not found</returns>
	vector<uint8_t> LoadBattery(const string& extension);

	/// <summary>
	/// Load battery data directly into existing buffer using std::span.
	/// </summary>
	/// <param name="extension">File extension to load</param>
	/// <param name="data">Writable span to fill with battery data</param>
	/// <remarks>
	/// std::span allows filling caller's buffer without extra allocation.
	/// Buffer size must match file size (or file will be truncated/padded).
	/// More efficient than vector-returning variant when destination buffer pre-allocated.
	/// </remarks>
	void LoadBattery(const string& extension, std::span<uint8_t> data);

	/// <summary>
	/// Get size of battery file without loading contents.
	/// </summary>
	/// <param name="extension">File extension to check</param>
	/// <returns>File size in bytes, or 0 if file does not exist</returns>
	uint32_t GetBatteryFileSize(const string& extension);

	/// <summary>
	/// Set per-ROM save directory override for battery files.
	/// Called from C# GameDataManager on ROM load to redirect saves to per-game folders.
	/// </summary>
	/// <param name="path">Full path to per-ROM save directory, or empty string to use default</param>
	void SetPerRomSaveDirectory(const string& path);
};
