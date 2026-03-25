#pragma once
#include "pch.h"
#include "Genesis/GenesisTypes.h"
#include "Shared/BaseControlManager.h"
#include "Shared/SettingTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;
class GenesisConsole;

class GenesisControlManager final : public BaseControlManager {
private:
	GenesisConsole* _console = nullptr;
	GenesisConfig _prevConfig = {};

	// Per-port TH state (directly controlled by 68k via data port writes)
	uint8_t _dataPortWrite[2] = {};
	uint8_t _thState[2] = {};
	uint8_t _thCount[2] = {};

	// Controller button states
	GenesisControllerState _padState[2] = {};

public:
	GenesisControlManager(Emulator* emu, GenesisConsole* console);

	shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override;
	void UpdateControlDevices() override;

	uint8_t ReadDataPort(uint8_t port);
	void WriteDataPort(uint8_t port, uint8_t value);

	void Serialize(Serializer& s) override;
};
