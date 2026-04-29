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
		: BaseControlDevice(emu, ControllerType::GenesisController, port, keyMappings) {
		_turboSpeed = keyMappings.TurboSpeed;
	}

	uint8_t ReadRam(uint16_t addr) override { return 0; }
	void WriteRam(uint16_t addr, uint8_t value) override {}

protected:
	string GetKeyNames() override {
		return "UDLRABCSxyzm";
	}

	bool HasCustomGenesisMapping(const KeyMapping& keyMapping) {
		for (uint32_t i = 0; i < 12; i++) {
			if (keyMapping.CustomKeys[i] != 0) {
				return true;
			}
		}
		return false;
	}

	void InternalSetStateFromInput() override {
		for (KeyMapping& keyMapping : _keyMappings) {
			if (HasCustomGenesisMapping(keyMapping)) {
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
			} else {
				SetPressedState(Buttons::Up, keyMapping.Up);
				SetPressedState(Buttons::Down, keyMapping.Down);
				SetPressedState(Buttons::Left, keyMapping.Left);
				SetPressedState(Buttons::Right, keyMapping.Right);
				SetPressedState(Buttons::A, keyMapping.A);
				SetPressedState(Buttons::B, keyMapping.B);
				SetPressedState(Buttons::C, keyMapping.X);
				SetPressedState(Buttons::Start, keyMapping.Start);
				SetPressedState(Buttons::X, keyMapping.Y);
				SetPressedState(Buttons::Y, keyMapping.L);
				SetPressedState(Buttons::Z, keyMapping.R);
				SetPressedState(Buttons::Mode, keyMapping.GenericKey1);

				uint8_t turboFreq = 1 << (4 - _turboSpeed);
				bool turboOn = (uint8_t)(_emu->GetFrameCount() % turboFreq) < turboFreq / 2;
				if (turboOn) {
					SetPressedState(Buttons::A, keyMapping.TurboA);
					SetPressedState(Buttons::B, keyMapping.TurboB);
					SetPressedState(Buttons::C, keyMapping.TurboX);
					SetPressedState(Buttons::X, keyMapping.TurboY);
					SetPressedState(Buttons::Y, keyMapping.TurboL);
					SetPressedState(Buttons::Z, keyMapping.TurboR);
				}
			}
		}
	}

private:
	uint32_t _turboSpeed = 0;
};
