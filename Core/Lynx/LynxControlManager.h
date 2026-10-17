#pragma once
#include "pch.h"
#include "Shared/SettingTypes.h"
#include "Shared/BaseControlManager.h"
#include "Lynx/LynxTypes.h"

class LynxConsole;

class LynxControlManager final : public BaseControlManager {
private:
	LynxControlManagerState _state = {};
	LynxConfig _prevConfig = {};
	LynxConsole* _console = nullptr;

public:
	LynxControlManager(Emulator* emu, LynxConsole* console);

	[[nodiscard]] LynxControlManagerState& GetState() { return _state; }

	shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override;
	void UpdateControlDevices() override;
	void UpdateInputState() override;

	[[nodiscard]] uint8_t ReadJoystick();
	[[nodiscard]] uint8_t ReadSwitches();

	void Serialize(Serializer& s) override;
};
