#pragma once
#include "pch.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/Emulator.h"

class GenesisController : public BaseControlDevice {
public:
	enum Buttons {
		Up = 0, Down, Left, Right,
		A, B, C, Start,
		X, Y, Z, Mode
	};

	GenesisController(Emulator* emu, uint8_t port, KeyMappingSet keyMappings)
		: BaseControlDevice(emu, ControllerType::None, port, keyMappings) {
	}

	uint8_t ReadRam(uint16_t addr) override { return 0; }
	void WriteRam(uint16_t addr, uint8_t value) override {}

protected:
	string GetKeyNames() override {
		return "UDLRABCSxyzm";
	}

	void InternalSetStateFromInput() override {
		for (KeyMapping& keyMapping : _keyMappings) {
			SetPressedState(Buttons::Up, keyMapping.CustomKeys[Buttons::Up]);
			SetPressedState(Buttons::Down, keyMapping.CustomKeys[Buttons::Down]);
			SetPressedState(Buttons::Left, keyMapping.CustomKeys[Buttons::Left]);
			SetPressedState(Buttons::Right, keyMapping.CustomKeys[Buttons::Right]);
			SetPressedState(Buttons::A, keyMapping.CustomKeys[Buttons::A]);
			SetPressedState(Buttons::B, keyMapping.CustomKeys[Buttons::B]);
			SetPressedState(Buttons::C, keyMapping.CustomKeys[Buttons::C]);
			SetPressedState(Buttons::Start, keyMapping.CustomKeys[Buttons::Start]);
			SetPressedState(Buttons::X, keyMapping.CustomKeys[Buttons::X]);
			SetPressedState(Buttons::Y, keyMapping.CustomKeys[Buttons::Y]);
			SetPressedState(Buttons::Z, keyMapping.CustomKeys[Buttons::Z]);
			SetPressedState(Buttons::Mode, keyMapping.CustomKeys[Buttons::Mode]);
		}
	}
};
