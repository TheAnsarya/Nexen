#pragma once
#include "pch.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/InputHud.h"
#include "Utilities/Serializer.h"

/// <summary>
/// Atari 2600 joystick controller device — handles physical input from a
/// standard joystick and provides movie/TAS serialization.
///
/// The Atari 2600 joystick has 5 inputs:
///   - D-pad: Up, Down, Left, Right
///   - Fire button (single button)
///
/// Joystick directions are read via RIOT SWCHA register ($0280):
///   Port 0 (player 1): bits 4-7 (active-low)
///     Bit 4: Up, Bit 5: Down, Bit 6: Left, Bit 7: Right
///   Port 1 (player 2): bits 0-3 (active-low)
///     Bit 0: Up, Bit 1: Down, Bit 2: Left, Bit 3: Right
///
/// Fire button is read via TIA INPT4/INPT5 ($0C/$0D):
///   Bit 7: Fire state (0 = pressed in dumped mode)
///
/// TAS key names string: "UDLRf" (5 characters per port per frame)
/// </summary>
class Atari2600Controller : public BaseControlDevice {
private:
	uint32_t _turboSpeed = 0;

protected:
	string GetKeyNames() override {
		return "UDLRf";
	}

	void InternalSetStateFromInput() override {
		for (KeyMapping& keyMapping : _keyMappings) {
			SetPressedState(Buttons::Up, keyMapping.Up);
			SetPressedState(Buttons::Down, keyMapping.Down);
			SetPressedState(Buttons::Left, keyMapping.Left);
			SetPressedState(Buttons::Right, keyMapping.Right);
			SetPressedState(Buttons::Fire, keyMapping.A);

			uint8_t turboFreq = 1 << (4 - _turboSpeed);
			bool turboOn = static_cast<uint8_t>(_emu->GetFrameCount() % turboFreq) < turboFreq / 2;
			if (turboOn) {
				SetPressedState(Buttons::Fire, keyMapping.TurboA);
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
		Fire
	};

	Atari2600Controller(Emulator* emu, uint8_t port, KeyMappingSet keyMappings) : BaseControlDevice(emu, ControllerType::Atari2600Joystick, port, keyMappings) {
		_turboSpeed = keyMappings.TurboSpeed;
	}

	uint8_t ReadRam(uint16_t addr) override {
		return 0;
	}

	void WriteRam(uint16_t addr, uint8_t value) override {
	}

	/// Returns the SWCHA nibble for this port (active-low).
	/// For port 0: bits 4-7 of SWCHA
	/// For port 1: bits 0-3 of SWCHA
	/// Bit layout per nibble: [Right][Left][Down][Up] (high to low)
	uint8_t GetDirectionNibble() {
		uint8_t value = 0x0f; // All released (active-low)
		if (IsPressed(Buttons::Up))    value &= ~0x01;
		if (IsPressed(Buttons::Down))  value &= ~0x02;
		if (IsPressed(Buttons::Left))  value &= ~0x04;
		if (IsPressed(Buttons::Right)) value &= ~0x08;
		return value;
	}

	/// Returns TIA INPT4/5 value for fire button.
	/// Bit 7: 0 = pressed, 1 = released (dumped/grounded mode)
	uint8_t GetFireState() {
		return IsPressed(Buttons::Fire) ? 0x00 : 0x80;
	}

	void InternalDrawController(InputHud& hud) override {
		hud.DrawOutline(35, 24);

		// D-pad
		hud.DrawButton(5, 2, 3, 3, IsPressed(Buttons::Up));
		hud.DrawButton(5, 8, 3, 3, IsPressed(Buttons::Down));
		hud.DrawButton(2, 5, 3, 3, IsPressed(Buttons::Left));
		hud.DrawButton(8, 5, 3, 3, IsPressed(Buttons::Right));

		// Fire button (large, single button)
		hud.DrawButton(25, 4, 5, 5, IsPressed(Buttons::Fire));

		hud.DrawNumber(_port + 1, 16, 12);
	}

	vector<DeviceButtonName> GetKeyNameAssociations() override {
		return {
			{"fire",  Buttons::Fire },
			{"up",    Buttons::Up   },
			{"down",  Buttons::Down },
			{"left",  Buttons::Left },
			{"right", Buttons::Right},
		};
	}
};
