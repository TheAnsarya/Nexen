#pragma once
#include "pch.h"
#include "Shared/BaseControlManager.h"

class Emulator;
class ChannelFController;
class ChannelFConsolePanel;

class ChannelFControlManager final : public BaseControlManager {
private:
	shared_ptr<ChannelFController> _controller1;
	shared_ptr<ChannelFController> _controller2;
	shared_ptr<ChannelFConsolePanel> _consolePanel;

public:
	explicit ChannelFControlManager(Emulator* emu);

	shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override;
	void UpdateControlDevices() override;
	void UpdateInputState() override;

	// Controller port values for memory manager I/O
	[[nodiscard]] uint8_t GetController1Value();
	[[nodiscard]] uint8_t GetController2Value();
	[[nodiscard]] uint8_t GetConsoleButtonValue();
};
