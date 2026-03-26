#pragma once
#include "pch.h"
#include "Shared/BaseControlManager.h"

class Emulator;

class ChannelFControlManager final : public BaseControlManager {
public:
	explicit ChannelFControlManager(Emulator* emu);

	shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override;
	void UpdateControlDevices() override;
	void UpdateInputState() override;
};
