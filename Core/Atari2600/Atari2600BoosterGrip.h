#pragma once
#include "pch.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/InputHud.h"
#include "Utilities/Serializer.h"

/// Atari 2600 Booster Grip controller.
///
/// Extension of the standard joystick with 2 additional buttons:
///   - Trigger: read via INPT1 (port 0) or INPT3 (port 1)
///   - Booster: read via INPT0 (port 0) or INPT2 (port 1)
///
/// Standard joystick inputs are preserved:
///   - Directions via SWCHA nibble (same as CX40)
///   - Fire via INPT4/INPT5 (same as CX40)
///
/// INPT0-3 behavior: bit 7 = 0 means pressed, 0x80 = not pressed
/// (uses latched mode, same as paddle read but digital)
///
/// TAS key names: "UDLRftb" (up/down/left/right/fire/trigger/booster)
class Atari2600BoosterGrip : public BaseControlDevice {
private:
	uint32_t _turboSpeed = 0;

protected:
	string GetKeyNames() override {
		return "UDLRftb";
	}

	void InternalSetStateFromInput() override {
		for (KeyMapping& keyMapping : _keyMappings) {
			SetPressedState(Buttons::Up, keyMapping.Up);
			SetPressedState(Buttons::Down, keyMapping.Down);
			SetPressedState(Buttons::Left, keyMapping.Left);
			SetPressedState(Buttons::Right, keyMapping.Right);
			SetPressedState(Buttons::Fire, keyMapping.A);
			SetPressedState(Buttons::Trigger, keyMapping.B);
			SetPressedState(Buttons::Booster, keyMapping.X);

			uint8_t turboFreq = 1 << (4 - _turboSpeed);
			bool turboOn = static_cast<uint8_t>(_emu->GetFrameCount() % turboFreq) < turboFreq / 2;
			if (turboOn) {
				SetPressedState(Buttons::Fire, keyMapping.TurboA);
				SetPressedState(Buttons::Trigger, keyMapping.TurboB);
				SetPressedState(Buttons::Booster, keyMapping.TurboX);
			}
		}
	}

	void RefreshStateBuffer() override {}

public:
	enum Buttons {
		Up = 0,
		Down,
		Left,
		Right,
		Fire,
		Trigger,
		Booster
	};

	Atari2600BoosterGrip(Emulator* emu, uint8_t port, KeyMappingSet keyMappings)
		: BaseControlDevice(emu, ControllerType::Atari2600BoosterGrip, port, keyMappings) {
		_turboSpeed = keyMappings.TurboSpeed;
	}

	uint8_t ReadRam(uint16_t addr) override { return 0; }
	void WriteRam(uint16_t addr, uint8_t value) override {}

	/// Returns the SWCHA nibble for this port (same as joystick).
	uint8_t GetDirectionNibble() {
		uint8_t value = 0x0f;
		if (IsPressed(Buttons::Up))    value &= ~0x01;
		if (IsPressed(Buttons::Down))  value &= ~0x02;
		if (IsPressed(Buttons::Left))  value &= ~0x04;
		if (IsPressed(Buttons::Right)) value &= ~0x08;
		return value;
	}

	/// Returns TIA INPT4/5 value for fire button.
	uint8_t GetFireState() {
		return IsPressed(Buttons::Fire) ? 0x00 : 0x80;
	}

	/// Returns TIA INPT1/INPT3 value for trigger button.
	/// Port 0 → INPT1, Port 1 → INPT3
	uint8_t GetTriggerState() {
		return IsPressed(Buttons::Trigger) ? 0x00 : 0x80;
	}

	/// Returns TIA INPT0/INPT2 value for booster button.
	/// Port 0 → INPT0, Port 1 → INPT2
	uint8_t GetBoosterState() {
		return IsPressed(Buttons::Booster) ? 0x00 : 0x80;
	}

	void InternalDrawController(InputHud& hud) override {
		hud.DrawOutline(35, 30);

		// D-pad
		hud.DrawButton(5, 2, 3, 3, IsPressed(Buttons::Up));
		hud.DrawButton(5, 8, 3, 3, IsPressed(Buttons::Down));
		hud.DrawButton(2, 5, 3, 3, IsPressed(Buttons::Left));
		hud.DrawButton(8, 5, 3, 3, IsPressed(Buttons::Right));

		// Fire button (large)
		hud.DrawButton(25, 2, 5, 5, IsPressed(Buttons::Fire));

		// Trigger and Booster (smaller, below fire)
		hud.DrawButton(22, 10, 4, 4, IsPressed(Buttons::Trigger));
		hud.DrawButton(28, 10, 4, 4, IsPressed(Buttons::Booster));

		hud.DrawNumber(_port + 1, 16, 18);
	}

	vector<DeviceButtonName> GetKeyNameAssociations() override {
		return {
			{ "fire",    Buttons::Fire },
			{ "trigger", Buttons::Trigger },
			{ "booster", Buttons::Booster },
			{ "up",      Buttons::Up },
			{ "down",    Buttons::Down },
			{ "left",    Buttons::Left },
			{ "right",   Buttons::Right },
		};
	}
};
