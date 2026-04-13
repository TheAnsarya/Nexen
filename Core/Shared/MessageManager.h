#pragma once

#include "pch.h"

#include "Core/Shared/Interfaces/IMessageManager.h"
#include <algorithm>
#include <array>
#include <deque>
#include <string_view>
#include "Utilities/SimpleLock.h"

/// <summary>Debug logging macro - compiles to nothing in Release builds</summary>
#ifdef _DEBUG
#define LogDebug(msg) MessageManager::Log(msg);
#define LogDebugIf(cond, msg)     \
	if (cond) {                   \
		MessageManager::Log(msg); \
	}
#else
#define LogDebug(msg)
#define LogDebugIf(cond, msg)
#endif

/// <summary>
/// Global message and logging system for Nexen.
/// Handles OSD messages, localization, and debug logging.
/// </summary>
/// <remarks>
/// Features:
/// - OSD (On-Screen Display) messages to user
/// - Localization system (key → translated string)
/// - Debug logging to memory buffer and stdout
/// - Thread-safe access to log and message systems
///
/// Message flow:
/// 1. DisplayMessage() called with localization key
/// 2. Localize() looks up translated string
/// 3. IMessageManager receives message for UI display
///
/// Logging:
/// - Log() appends to in-memory buffer
/// - Optional stdout output for console debugging
/// - GetLog() retrieves full log history
/// - Thread-safe (SimpleLock for multi-thread access)
///
/// Usage:
/// <code>
/// MessageManager::DisplayMessage("Cheats", "CheatsApplied", "5");
/// MessageManager::Log("Emulation started");
/// LogDebug("Debug info"); // DEBUG only
/// </code>
///
/// Thread safety: All methods are thread-safe via SimpleLock.
/// </remarks>
class MessageManager {
private:
	static IMessageManager* _messageManager;                ///< UI message handler

	/// English localization strings — sorted constexpr array for binary search lookup.
	/// 80 entries, sorted alphabetically by key at compile time.
	static constexpr std::array<std::pair<std::string_view, std::string_view>, 80> _enResources = {{
		{"ApplyingPatch",                 "Applying patch: %1"},
		{"CheatApplied",                  "1 cheat applied."},
		{"Cheats",                        "Cheats"},
		{"CheatsApplied",                 "%1 cheats applied."},
		{"CheatsDisabled",                "All cheats disabled."},
		{"ClockRate",                     "Clock Rate"},
		{"CoinInsertedSlot",              "Coin inserted (slot %1)"},
		{"ConnectedToServer",             "Connected to server."},
		{"ConnectionLost",                "Connection to server lost."},
		{"CouldNotConnect",               "Could not connect to the server."},
		{"CouldNotFindRom",               "Could not find matching game ROM. (%1)"},
		{"CouldNotInitializeAudioSystem", "Could not initialize audio system"},
		{"CouldNotLoadFile",              "Could not load file: %1"},
		{"CouldNotWriteToFile",           "Could not write to file: %1"},
		{"Debug",                         "Debug"},
		{"EmulationMaximumSpeed",         "Maximum speed"},
		{"EmulationSpeed",                "Emulation Speed"},
		{"EmulationSpeedPercent",         "%1%"},
		{"Error",                         "Error"},
		{"FdsDiskInserted",               "Disk %1 Side %2 inserted."},
		{"Frame",                         "Frame"},
		{"GameCrash",                     "Game has crashed (%1)"},
		{"GameInfo",                      "Game Info"},
		{"GameLoaded",                    "Game loaded"},
		{"Input",                         "Input"},
		{"KeyboardModeDisabled",          "Keyboard mode disabled."},
		{"KeyboardModeEnabled",           "Keyboard connected - shortcut keys disabled."},
		{"Lag",                           "Lag"},
		{"Mapper",                        "Mapper: %1, SubMapper: %2"},
		{"MovieEnded",                    "Movie ended."},
		{"MovieIncompatibleVersion",      "This movie is incompatible with this version of Nexen."},
		{"MovieIncorrectConsole",         "This movie was recorded on another console (%1) and can't be loaded."},
		{"MovieInvalid",                  "Invalid movie file."},
		{"MovieMissingRom",               "Missing ROM required (%1) to play movie."},
		{"MovieNewerVersion",             "Cannot load movies created by a more recent version of Nexen. Please download the latest version."},
		{"MoviePlaying",                  "Playing movie: %1"},
		{"MovieRecordingTo",              "Recording to: %1"},
		{"MovieSaved",                    "Movie saved to file: %1"},
		{"MovieStopped",                  "Movie stopped."},
		{"Movies",                        "Movies"},
		{"NetPlay",                       "Net Play"},
		{"NetplayNotAllowed",             "This action is not allowed while connected to a server."},
		{"NetplayVersionMismatch",        "Netplay client is not running the same version of Nexen and has been disconnected."},
		{"Overclock",                     "Overclock"},
		{"OverclockDisabled",             "Overclocking disabled."},
		{"OverclockEnabled",              "Overclocking enabled."},
		{"Patch",                         "Patch"},
		{"PatchFailed",                   "Failed to apply patch: %1"},
		{"PrgSizeWarning",                "PRG size is smaller than 32kb"},
		{"Region",                        "Region"},
		{"SaveStateEmpty",                "Slot is empty."},
		{"SaveStateIncompatibleVersion",  "Save state is incompatible with this version of Nexen."},
		{"SaveStateInvalidFile",          "Invalid save state file."},
		{"SaveStateLoaded",               "State #%1 loaded."},
		{"SaveStateLoadedFile",           "State loaded: %1"},
		{"SaveStateMissingRom",           "Missing ROM required (%1) to load save state."},
		{"SaveStateNewerVersion",         "Cannot load save states created by a more recent version of Nexen. Please download the latest version."},
		{"SaveStateSaved",                "State #%1 saved."},
		{"SaveStateSavedFile",            "State saved: %1"},
		{"SaveStateSavedTime",            "State saved at %1"},
		{"SaveStateSlotSelected",         "Slot #%1 selected."},
		{"SaveStateWrongSystem",          "Error: State cannot be loaded (wrong console type)"},
		{"SaveStates",                    "Save States"},
		{"ScanlineTimingWarning",         "PPU timing has been changed."},
		{"ScreenshotSaved",               "Screenshot Saved"},
		{"ServerStarted",                 "Server started (Port: %1)"},
		{"ServerStopped",                 "Server stopped"},
		{"SoundRecorder",                 "Audio"},
		{"SoundRecorderStarted",          "Recording Audio"},
		{"SoundRecorderStopped",          "Stopped Audio Recording"},
		{"Test",                          "Test"},
		{"TestFileSavedTo",               "Test file saved to: %1"},
		{"UnexpectedError",               "Unexpected error: %1"},
		{"UnsupportedMapper",             "Unsupported mapper (%1), cannot load game."},
		{"VideoRecorder",                 "Video"},
		{"VideoRecorderStarted",          "Recording Video"},
		{"VideoRecorderStopped",          "Stopped Video Recording"},
	}};

	static bool _osdEnabled;        ///< OSD messages enabled flag
	static bool _outputToStdout;    ///< Log to stdout flag
	static SimpleLock _logLock;     ///< Log access synchronization
	static SimpleLock _messageLock; ///< Message access synchronization
	static std::deque<string> _log; ///< In-memory log buffer (deque for O(1) push/pop with cache locality)

public:
	/// <summary>
	/// Configure message manager options.
	/// </summary>
	/// <param name="osdEnabled">Enable OSD messages</param>
	/// <param name="outputToStdout">Output logs to stdout</param>
	static void SetOptions(bool osdEnabled, bool outputToStdout);

	/// <summary>
	/// Localize string key to translated text.
	/// </summary>
	/// <param name="key">Localization key (e.g., "Cheats.CheatsApplied")</param>
	/// <returns>Localized string or key if not found</returns>
	[[nodiscard]] static string Localize(const string& key);

	/// <summary>Register UI message handler</summary>
	static void RegisterMessageManager(IMessageManager* messageManager);

	/// <summary>Unregister UI message handler</summary>
	static void UnregisterMessageManager(IMessageManager* messageManager);

	/// <summary>
	/// Display localized message to user via OSD.
	/// </summary>
	/// <param name="title">Title localization key</param>
	/// <param name="message">Message localization key</param>
	/// <param name="param1">Optional parameter for string formatting</param>
	/// <param name="param2">Optional second parameter</param>
	/// <remarks>
	/// Message strings can contain {0}, {1} placeholders for param1, param2.
	/// Example: "CheatsApplied" = "{0} cheats loaded"
	/// </remarks>
	static void DisplayMessage(const string& title, const string& message, const string& param1 = "", const string& param2 = "");

	/// <summary>
	/// Log message to memory buffer and optionally stdout.
	/// </summary>
	/// <param name="message">Log message (empty string logs blank line)</param>
	/// <remarks>
	/// Thread-safe. Logs accumulate in memory until ClearLog() called.
	/// Use LogDebug() macro for debug-only logging.
	/// </remarks>
	static void Log(const string& message = "");

	/// <summary>Move-overload for Log() to avoid copying temporary strings.</summary>
	static void Log(string&& message);

	/// <summary>Clear in-memory log buffer</summary>
	static void ClearLog();

	/// <summary>
	/// Get full log history as newline-separated string.
	/// </summary>
	/// <returns>Complete log contents</returns>
	[[nodiscard]] static string GetLog();
};
