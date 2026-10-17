#pragma once
#include "pch.h"
#include "Shared/MessageManager.h"
#include "Netplay/NetMessage.h"
#include "Netplay/NetplayTypes.h"
#include "Utilities/FolderUtilities.h"

class GameInformationMessage : public NetMessage {
private:
	string _romFilename;
	uint32_t _crc32 = 0;
	NetplayControllerInfo _controller = {};
	bool _paused = false;

protected:
	void Serialize(Serializer& s) override {
		SV(_romFilename);
		SV(_crc32);
		SV(_controller.Port);
		SV(_controller.SubPort);
		SV(_paused);
	}

public:
	GameInformationMessage(void* buffer, uint32_t length) : NetMessage(buffer, length) {}

	GameInformationMessage(const string& filepath, uint32_t crc32, NetplayControllerInfo controller, bool paused) : NetMessage(MessageType::GameInformation) {
		_romFilename = FolderUtilities::GetFilename(filepath, true);
		_crc32 = crc32;
		_controller = controller;
		_paused = paused;
	}

	[[nodiscard]] NetplayControllerInfo GetPort() {
		return _controller;
	}

	[[nodiscard]] string GetRomFilename() {
		return _romFilename;
	}

	[[nodiscard]] uint32_t GetCrc32() {
		return _crc32;
	}

	[[nodiscard]] bool IsPaused() {
		return _paused;
	}
};
