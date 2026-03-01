#include "pch.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxControlManager.h"
#include "Lynx/LynxController.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"

LynxControlManager::LynxControlManager(Emulator* emu, LynxConsole* console) : BaseControlManager(emu, CpuType::Lynx) {
	_console = console;
}

shared_ptr<BaseControlDevice> LynxControlManager::CreateControllerDevice(ControllerType type, uint8_t port) {
	shared_ptr<BaseControlDevice> device;

	LynxConfig& cfg = _emu->GetSettings()->GetLynxConfig();

	switch (type) {
		default:
		case ControllerType::None:
			break;

		case ControllerType::LynxController:
			device.reset(new LynxController(_emu, port, cfg.Controller.Keys));
			break;
	}

	return device;
}

void LynxControlManager::UpdateControlDevices() {
	LynxConfig cfg = _emu->GetSettings()->GetLynxConfig();
	if (_emu->GetSettings()->IsEqual(_prevConfig, cfg) && _controlDevices.size() > 0) {
		return;
	}

	_prevConfig = cfg;

	auto lock = _deviceLock.AcquireSafe();

	ClearDevices();

	shared_ptr<BaseControlDevice> device(CreateControllerDevice(ControllerType::LynxController, 0));
	if (device) {
		RegisterControlDevice(device);
	}
}

void LynxControlManager::UpdateInputState() {
	BaseControlManager::UpdateInputState();

	// Read controller state and update registers
	_state.Joystick = ReadJoystick();
	_state.Switches = ReadSwitches();
}

uint8_t LynxControlManager::ReadJoystick() {
	for (shared_ptr<BaseControlDevice>& controller : _controlDevices) {
		if (controller->GetPort() == 0 && controller->GetControllerType() == ControllerType::LynxController) {
			auto* lynxCtrl = static_cast<LynxController*>(controller.get());
			return lynxCtrl->GetJoystickState();
		}
	}
	return 0xff; // All released
}

uint8_t LynxControlManager::ReadSwitches() {
	for (shared_ptr<BaseControlDevice>& controller : _controlDevices) {
		if (controller->GetPort() == 0 && controller->GetControllerType() == ControllerType::LynxController) {
			auto* lynxCtrl = static_cast<LynxController*>(controller.get());
			return lynxCtrl->GetSwitchesState();
		}
	}
	return 0xff;
}

void LynxControlManager::Serialize(Serializer& s) {
	SV(_state.Joystick);
	SV(_state.Switches);
}
