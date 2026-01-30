#pragma once

#include "pch.h"

/// <summary>
/// Interface for displaying messages to the user.
/// Implemented by the frontend UI layer.
/// </summary>
/// <remarks>
/// Implementers:
/// - Windows UI: MessageBox, toast notifications
/// - Linux UI: GTK dialog, notification daemon
/// - macOS UI: NSAlert, notification center
/// - SDL UI: Console output, SDL_ShowMessageBox
///
/// Used for:
/// - Error messages (missing files, unsupported formats)
/// - Warnings (overclock detection, incompatible settings)
/// - Information (netplay connection status, recording started)
/// - Firmware/BIOS missing notifications
///
/// Thread safety:
/// - May be called from any thread
/// - Implementation must dispatch to UI thread if needed
/// </remarks>
class IMessageManager {
public:
	/// <summary>
	/// Display message dialog to user.
	/// </summary>
	/// <param name="title">Message box title</param>
	/// <param name="message">Message content (may contain newlines)</param>
	/// <remarks>
	/// Blocking vs non-blocking:
	/// - Implementation may block or return immediately
	/// - Emulation typically paused while message is displayed
	/// - Critical errors should block until user acknowledges
	/// </remarks>
	virtual void DisplayMessage(string title, string message) = 0;
};
