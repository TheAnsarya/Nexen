#include "pch.h"
#include "Shared/NotificationManager.h"

void NotificationManager::RegisterNotificationListener(shared_ptr<INotificationListener> notificationListener) {
	auto lock = _lock.AcquireSafe();
	bool isAlreadyRegistered = false;

	// Prune expired listeners and detect duplicates in a single pass.
	auto writeIt = _listeners.begin();
	for (auto readIt = _listeners.begin(); readIt != _listeners.end(); readIt++) {
		shared_ptr<INotificationListener> existing = readIt->lock();
		if (!existing) {
			continue;
		}

		*writeIt = *readIt;
		writeIt++;
		if (existing == notificationListener) {
			isAlreadyRegistered = true;
		}
	}
	_listeners.erase(writeIt, _listeners.end());

	if (isAlreadyRegistered) {
		return;
	}

	_listeners.push_back(notificationListener);
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
