#include "pch.h"
#include "Shared/NotificationManager.h"

void NotificationManager::RegisterNotificationListener(shared_ptr<INotificationListener> notificationListener) {
	auto lock = _lock.AcquireSafe();

	// Cleanup expired listeners while lock is held to avoid lock re-entry in hot registration paths.
	std::erase_if(_listeners, [](const weak_ptr<INotificationListener>& ptr) { return ptr.expired(); });

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
	std::erase_if(_listeners, [](const weak_ptr<INotificationListener>& ptr) { return ptr.expired(); });
}

void NotificationManager::SendNotification(ConsoleNotificationType type, void* parameter) {
	// Build a strong snapshot under lock and prune expired listeners in one pass.
	// This keeps callback execution lock-free while reducing weak_ptr lock churn.
	// Reuses _snapshot member to avoid heap allocation on every notification.
	{
		auto lock = _lock.AcquireSafe();
		_snapshot.clear();
		_snapshot.reserve(_listeners.size());

		auto writeIt = _listeners.begin();
		for (auto readIt = _listeners.begin(); readIt != _listeners.end(); readIt++) {
			shared_ptr<INotificationListener> listener = readIt->lock();
			if (!listener) {
				continue;
			}

			*writeIt = *readIt;
			writeIt++;
			_snapshot.push_back(std::move(listener));
		}

		_listeners.erase(writeIt, _listeners.end());
	}

	for (size_t i = 0; i < _snapshot.size(); i++) {
		_snapshot[i]->ProcessNotification(type, parameter);
	}

	// Release shared_ptr refs after callbacks complete
	_snapshot.clear();
}
