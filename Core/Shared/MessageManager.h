#pragma once

#include "pch.h"

#include "Core/Shared/Interfaces/IMessageManager.h"
#include <unordered_map>
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
	static std::unordered_map<string, string> _enResources; ///< English localization strings

	static bool _osdEnabled;        ///< OSD messages enabled flag
	static bool _outputToStdout;    ///< Log to stdout flag
	static SimpleLock _logLock;     ///< Log access synchronization
	static SimpleLock _messageLock; ///< Message access synchronization
	static std::list<string> _log;  ///< In-memory log buffer

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
	static string Localize(string key);

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
	static void DisplayMessage(string title, string message, string param1 = "", string param2 = "");

	/// <summary>
	/// Log message to memory buffer and optionally stdout.
	/// </summary>
	/// <param name="message">Log message (empty string logs blank line)</param>
	/// <remarks>
	/// Thread-safe. Logs accumulate in memory until ClearLog() called.
	/// Use LogDebug() macro for debug-only logging.
	/// </remarks>
	static void Log(string message = "");

	/// <summary>Clear in-memory log buffer</summary>
	static void ClearLog();

	/// <summary>
	/// Get full log history as newline-separated string.
	/// </summary>
	/// <returns>Complete log contents</returns>
	static string GetLog();
};
