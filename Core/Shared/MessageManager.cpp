#include "pch.h"
#include "Shared/MessageManager.h"

std::deque<string> MessageManager::_log;
SimpleLock MessageManager::_logLock;
SimpleLock MessageManager::_messageLock;
bool MessageManager::_osdEnabled = true;
bool MessageManager::_outputToStdout = false;
IMessageManager* MessageManager::_messageManager = nullptr;

void MessageManager::RegisterMessageManager(IMessageManager* messageManager) {
	auto lock = _messageLock.AcquireSafe();
	if (MessageManager::_messageManager == nullptr) {
		MessageManager::_messageManager = messageManager;
	}
}

void MessageManager::UnregisterMessageManager(IMessageManager* messageManager) {
	auto lock = _messageLock.AcquireSafe();
	if (MessageManager::_messageManager == messageManager) {
		MessageManager::_messageManager = nullptr;
	}
}

void MessageManager::SetOptions(bool osdEnabled, bool outputToStdout) {
	_osdEnabled = osdEnabled;
	_outputToStdout = outputToStdout;
}

string MessageManager::Localize(const string& key) {
	// Binary search on sorted constexpr array — zero heap allocation
	auto it = std::lower_bound(
		_enResources.begin(), _enResources.end(), key,
		[](const auto& pair, const std::string& k) { return pair.first < k; }
	);
	if (it != _enResources.end() && it->first == key) {
		return string(it->second);
	}

	return string(key);
}

void MessageManager::DisplayMessage(const string& title, const string& message, const string& param1, const string& param2) {
	if (MessageManager::_messageManager) {
		auto lock = _messageLock.AcquireSafe();
		if (!MessageManager::_messageManager) {
			return;
		}

		string localTitle = Localize(title);
		string localMessage = Localize(message);

		size_t startPos = localMessage.find("%1");
		if (startPos != std::string::npos) {
			localMessage.replace(startPos, 2, param1);
		}

		startPos = localMessage.find("%2");
		if (startPos != std::string::npos) {
			localMessage.replace(startPos, 2, param2);
		}

		if (_osdEnabled) {
			MessageManager::_messageManager->DisplayMessage(localTitle, localMessage);
		} else {
			MessageManager::Log("[" + localTitle + "] " + localMessage);
		}
	}
}

void MessageManager::Log(const string& message) {
	auto lock = _logLock.AcquireSafe();
	if (message.empty()) {
		_log.push_back("------------------------------------------------------");
	} else {
		_log.push_back(message);
	}
	if (_log.size() > 1000) {
		_log.pop_front();
	}

	if (_outputToStdout) {
		std::cout << (message.empty() ? _log.back() : message) << '\n';
	}
}

void MessageManager::Log(string&& message) {
	auto lock = _logLock.AcquireSafe();
	if (message.empty()) {
		_log.push_back("------------------------------------------------------");
	} else {
		_log.push_back(std::move(message));
	}
	if (_log.size() > 1000) {
		_log.pop_front();
	}

	if (_outputToStdout) {
		std::cout << _log.back() << '\n';
	}
}

void MessageManager::ClearLog() {
	auto lock = _logLock.AcquireSafe();
	_log.clear();
}

string MessageManager::GetLog() {
	auto lock = _logLock.AcquireSafe();
	string result;
	size_t totalSize = 0;
	for (const string& msg : _log) {
		totalSize += msg.size() + 1;
	}
	result.reserve(totalSize);
	for (const string& msg : _log) {
		result.append(msg);
		result += '\n';
	}
	return result;
}
