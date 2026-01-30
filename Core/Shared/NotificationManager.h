#pragma once
#include "pch.h"
#include "Shared/Interfaces/INotificationListener.h"
#include "Utilities/SimpleLock.h"

/// <summary>
/// Event notification broadcast system for emulator events.
/// Notifies registered listeners about console state changes and important events.
/// </summary>
/// <remarks>
/// Observer pattern implementation:
/// - Listeners register via RegisterNotificationListener()
/// - SendNotification() broadcasts to all listeners
/// - Weak pointers prevent listener lifecycle issues
///
/// Notification types (ConsoleNotificationType enum):
/// - GameLoaded, GameStopped, GamePaused, GameResumed
/// - StateLoaded, StateSaved
/// - CodeBreak (debugger)
/// - Cheats added/removed
/// - Many more...
///
/// Usage:
/// <code>
/// class MyListener : public INotificationListener {
///     void ProcessNotification(ConsoleNotificationType type, void* param) override {
///         // Handle events
///     }
/// };
/// auto listener = make_shared<MyListener>();
/// notificationMgr.RegisterNotificationListener(listener);
/// </code>
///
/// Cleanup:
/// - Automatic cleanup of dead weak_ptr references
/// - Safe to destroy listeners without unregistering
/// - Thread-safe registration and broadcast
///
/// Thread safety: All methods synchronized via SimpleLock.
/// </remarks>
class NotificationManager {
private:
	SimpleLock _lock;                                   ///< Thread synchronization lock
	vector<weak_ptr<INotificationListener>> _listeners; ///< Registered listeners (weak refs)

	/// <summary>Remove dead listener references (expired weak_ptr)</summary>
	void CleanupNotificationListeners();

public:
	/// <summary>
	/// Register notification listener.
	/// </summary>
	/// <param name="notificationListener">Listener to register (as shared_ptr)</param>
	/// <remarks>
	/// Stores weak_ptr - safe if listener destroyed without unregistering.
	/// Listener receives all future notifications via ProcessNotification().
	/// </remarks>
	void RegisterNotificationListener(shared_ptr<INotificationListener> notificationListener);

	/// <summary>
	/// Broadcast notification to all registered listeners.
	/// </summary>
	/// <param name="type">Notification type (event category)</param>
	/// <param name="parameter">Optional event-specific data pointer</param>
	/// <remarks>
	/// Calls ProcessNotification() on each live listener.
	/// Automatically cleans up dead listeners before broadcasting.
	/// Thread-safe - can be called from any thread.
	/// </remarks>
	void SendNotification(ConsoleNotificationType type, void* parameter = nullptr);
};
