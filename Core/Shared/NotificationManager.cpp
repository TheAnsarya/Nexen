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
	// Build a strong snapshot under lock and prune expired listeners in one pass.
	// This keeps callback execution lock-free while reducing weak_ptr lock churn.
	vector<shared_ptr<INotificationListener>> snapshot;
	{
		auto lock = _lock.AcquireSafe();
		snapshot.reserve(_listeners.size());

		auto writeIt = _listeners.begin();
		for (auto readIt = _listeners.begin(); readIt != _listeners.end(); readIt++) {
			shared_ptr<INotificationListener> listener = readIt->lock();
			if (!listener) {
				continue;
			}

			*writeIt = *readIt;
			writeIt++;
			snapshot.push_back(std::move(listener));
		}

		_listeners.erase(writeIt, _listeners.end());
	}

	for (const shared_ptr<INotificationListener>& listener : snapshot) {
		listener->ProcessNotification(type, parameter);
	}
}
