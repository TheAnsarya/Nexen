#pragma once
#include "pch.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/InputHud.h"
#include "Utilities/Serializer.h"

class LynxConsole;

/// <summary>
/// Atari Lynx controller device — handles physical input from the built-in
/// gamepad and provides movie/TAS serialization.
///
/// The Lynx has a fixed controller with 9 inputs:
///   - D-pad: Up, Down, Left, Right
///   - Face buttons: A, B
///   - Option buttons: Option1, Option2 (mapped as L/R in settings)
///   - Pause (mapped as Start in settings, triggers Mikey IRQ)
///
/// Input is read via two Suzy registers (active-low encoding):
///   - JOYSTICK ($FCB0): D-pad + A/B + Option1/2
///     Bit layout: [A][B][Opt2][Opt1][Up][Down][Left][Right]
///   - SWITCHES ($FCB1): Pause, cart control bits
///
/// TAS key names string: "UDLRabOoP" (9 characters per frame in BK2 format)
/// This allows full serialization of Lynx input state for movie playback.
///
/// The Lynx supports hardware rotation (0°, 90° left, 90° right) which
/// games can use to swap button mappings for left-handed play. Rotation
/// is detected from the LNX header or game database.
///
/// References:
///   - ~docs/plans/lynx-subsystems-deep-dive.md (Section: Input System)
///   - Epyx hardware reference: monlynx.de/lynx/hardware.html
/// </summary>
class LynxController : public BaseControlDevice {
private:
	uint32_t _turboSpeed = 0;

protected:
	string GetKeyNames() override {
		return "UDLRabOoP";
	}

	void InternalSetStateFromInput() override {
		for (KeyMapping& keyMapping : _keyMappings) {
			SetPressedState(Buttons::A, keyMapping.A);
			SetPressedState(Buttons::B, keyMapping.B);
			SetPressedState(Buttons::Option1, keyMapping.L);
			SetPressedState(Buttons::Option2, keyMapping.R);
			SetPressedState(Buttons::Pause, keyMapping.Start);
			SetPressedState(Buttons::Up, keyMapping.Up);
			SetPressedState(Buttons::Down, keyMapping.Down);
			SetPressedState(Buttons::Left, keyMapping.Left);
			SetPressedState(Buttons::Right, keyMapping.Right);

			uint8_t turboFreq = 1 << (4 - _turboSpeed);
			bool turboOn = static_cast<uint8_t>(_emu->GetFrameCount() % turboFreq) < turboFreq / 2;
			if (turboOn) {
				SetPressedState(Buttons::A, keyMapping.TurboA);
				SetPressedState(Buttons::B, keyMapping.TurboB);
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
		A,
		B,
		Option1,
		Option2,
		Pause
	};

	LynxController(Emulator* emu, uint8_t port, KeyMappingSet keyMappings) : BaseControlDevice(emu, ControllerType::LynxController, port, keyMappings) {
		_turboSpeed = keyMappings.TurboSpeed;
	}

	uint8_t ReadRam(uint16_t addr) override {
		return 0;
	}

	void WriteRam(uint16_t addr, uint8_t value) override {
	}

	// Returns joystick byte: active-low, bit mapping:
	// Bit 0: Right, 1: Left, 2: Down, 3: Up
	// Bit 4: Option1, 5: Option2, 6: B, 7: A
	uint8_t GetJoystickState() {
		uint8_t value = 0xff; // All released (active-low)
		if (IsPressed(Buttons::Right))   value &= ~0x01;
		if (IsPressed(Buttons::Left))    value &= ~0x02;
		if (IsPressed(Buttons::Down))    value &= ~0x04;
		if (IsPressed(Buttons::Up))      value &= ~0x08;
		if (IsPressed(Buttons::Option1)) value &= ~0x10;
		if (IsPressed(Buttons::Option2)) value &= ~0x20;
		if (IsPressed(Buttons::B))       value &= ~0x40;
		if (IsPressed(Buttons::A))       value &= ~0x80;
		return value;
	}

	// Returns switches byte, Pause button on bit 0
	uint8_t GetSwitchesState() {
		uint8_t value = 0xff;
		if (IsPressed(Buttons::Pause)) value &= ~0x01;
		return value;
	}

	void InternalDrawController(InputHud& hud) override {
		hud.DrawOutline(35, 24);

		// D-pad
		hud.DrawButton(5, 2, 3, 3, IsPressed(Buttons::Up));
		hud.DrawButton(5, 8, 3, 3, IsPressed(Buttons::Down));
		hud.DrawButton(2, 5, 3, 3, IsPressed(Buttons::Left));
		hud.DrawButton(8, 5, 3, 3, IsPressed(Buttons::Right));

		// Face buttons
		hud.DrawButton(25, 5, 3, 3, IsPressed(Buttons::A));
		hud.DrawButton(29, 2, 3, 3, IsPressed(Buttons::B));

		// Option buttons
		hud.DrawButton(14, 19, 4, 2, IsPressed(Buttons::Option1));
		hud.DrawButton(19, 19, 4, 2, IsPressed(Buttons::Option2));

		// Pause
		hud.DrawButton(16, 2, 3, 2, IsPressed(Buttons::Pause));

		hud.DrawNumber(_port + 1, 16, 12);
	}

	vector<DeviceButtonName> GetKeyNameAssociations() override {
		return {
			{"a",       Buttons::A      },
			{"b",       Buttons::B      },
			{"option1", Buttons::Option1},
			{"option2", Buttons::Option2},
			{"pause",   Buttons::Pause  },
			{"up",      Buttons::Up     },
			{"down",    Buttons::Down   },
			{"left",    Buttons::Left   },
			{"right",   Buttons::Right  },
		};
	}
};
