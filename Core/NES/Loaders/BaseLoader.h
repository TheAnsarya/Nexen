#pragma once
#include "pch.h"
#include "Shared/MessageManager.h"

class BaseLoader {
protected:
	void Log(const string& message) {
		MessageManager::Log(message);
	}

public:
	BaseLoader() {
	}
};
