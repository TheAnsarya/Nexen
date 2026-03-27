#include "pch.h"
#include "ChannelF/ChannelFControlManager.h"
#include "ChannelF/ChannelFController.h"
#include "ChannelF/ChannelFConsolePanel.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"

ChannelFControlManager::ChannelFControlManager(Emulator* emu)
	: BaseControlManager(emu, CpuType::ChannelF) {
}

shared_ptr<BaseControlDevice> ChannelFControlManager::CreateControllerDevice(ControllerType type, uint8_t port) {
	shared_ptr<BaseControlDevice> device;
	KeyMappingSet keys = {};

	switch (type) {
		case ControllerType::ChannelFController:
			device = std::make_shared<ChannelFController>(_emu, port, keys);
			break;
		case ControllerType::ChannelFConsolePanel:
			device = std::make_shared<ChannelFConsolePanel>(_emu, keys);
			break;
		default:
			break;
	}

	return device;
}

void ChannelFControlManager::UpdateControlDevices() {
	// Port 0: right controller (player 1)
	if (HasControlDevice(ControllerType::ChannelFController)) {
		return;
	}

	KeyMappingSet keys1 = {};
	KeyMappingSet keys2 = {};
	KeyMappingSet panelKeys = {};
	auto ctrl1 = std::make_shared<ChannelFController>(_emu, 0, keys1);
	auto ctrl2 = std::make_shared<ChannelFController>(_emu, 1, keys2);
	auto panel = std::make_shared<ChannelFConsolePanel>(_emu, panelKeys);

	_controller1 = ctrl1;
	_controller2 = ctrl2;
	_consolePanel = panel;

	RegisterControlDevice(ctrl1);
	RegisterControlDevice(ctrl2);
	RegisterControlDevice(panel);
}

void ChannelFControlManager::UpdateInputState() {
	BaseControlManager::UpdateInputState();
}

uint8_t ChannelFControlManager::GetController1Value() {
	return _controller1 ? _controller1->GetPortValue() : 0xff;
}

uint8_t ChannelFControlManager::GetController2Value() {
	return _controller2 ? _controller2->GetPortValue() : 0xff;
}

uint8_t ChannelFControlManager::GetConsoleButtonValue() {
	return _consolePanel ? _consolePanel->GetConsoleButtons() : 0xff;
}
