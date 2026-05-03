#pragma once
#include "pch.h"
#include "Core/Shared/Interfaces/INotificationListener.h"
#include "Core/Shared/NotificationManager.h"
#include "Core/Shared/MessageManager.h"

typedef void(__stdcall* NotificationListenerCallback)(int, void*);

class InteropNotificationListener : public INotificationListener {
	NotificationListenerCallback _callback;

public:
	InteropNotificationListener(NotificationListenerCallback callback) {
		_callback = callback;
	}

	virtual ~InteropNotificationListener() {
	}

	void ProcessNotification(ConsoleNotificationType type, void* parameter) override {
		if (!_callback) {
			return;
		}

		try {
			_callback((int)type, parameter);
		} catch (...) {
			MessageManager::Log("[InteropDLL] Notification callback threw C++ exception; disabling callback");
			_callback = nullptr;
		}
	}
};
