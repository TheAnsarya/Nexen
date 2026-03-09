#include "pch.h"
#include <algorithm>
#include "Shared/NotificationManager.h"

void NotificationManager::RegisterNotificationListener(shared_ptr<INotificationListener> notificationListener) {
	auto lock = _lock.AcquireSafe();

	// Cleanup expired listeners while lock is held to avoid lock re-entry in hot registration paths.
	_listeners.erase(
		std::remove_if(
			_listeners.begin(),
			_listeners.end(),
			[](const weak_ptr<INotificationListener>& ptr) { return ptr.expired(); }),
		_listeners.end());

	for (const weak_ptr<INotificationListener>& listener : _listeners) {
		shared_ptr<INotificationListener> existing = listener.lock();
		if (existing && existing == notificationListener) {
			// This listener is already registered, do nothing
			return;
		}
	}

	_listeners.push_back(notificationListener);
}

void NotificationManager::CleanupNotificationListeners() {
	auto lock = _lock.AcquireSafe();

	// Remove expired listeners
	_listeners.erase(
	    std::remove_if(
	        _listeners.begin(),
	        _listeners.end(),
	        [](const weak_ptr<INotificationListener>& ptr) { return ptr.expired(); }),
	    _listeners.end());
}

void NotificationManager::SendNotification(ConsoleNotificationType type, void* parameter) {
	// Snapshot listener list under lock, then iterate outside lock.
	// This minimizes lock hold time — callbacks execute without contention.
	vector<weak_ptr<INotificationListener>> snapshot;
	{
		auto lock = _lock.AcquireSafe();
		snapshot = _listeners;
	}

	for (size_t i = 0; i < snapshot.size(); i++) {
		shared_ptr<INotificationListener> listener = snapshot[i].lock();
		if (listener) {
			listener->ProcessNotification(type, parameter);
		}
	}
}
