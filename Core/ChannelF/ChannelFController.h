#pragma once
#include "pch.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/InputHud.h"
#include "Utilities/Serializer.h"

/// <summary>
/// Fairchild Channel F hand controller device.
///
/// Each hand controller has an 8-way joystick plus push, pull,
/// twist clockwise, and twist counter-clockwise actions.
///
/// Controller port values are active-low:
///   Bit 0: Right
///   Bit 1: Left
///   Bit 2: Back (away from player)
///   Bit 3: Forward (toward player)
///   Bit 4: Twist counter-clockwise
///   Bit 5: Twist clockwise
///   Bit 6: Pull up
///   Bit 7: Push down
///
/// TAS key names: "RLBFtTpP" (8 buttons per controller)
/// </summary>
class ChannelFController : public BaseControlDevice {
private:
	uint32_t _turboSpeed = 0;

protected:
	string GetKeyNames() override {
		return "RLBFtTpP";
	}

	void InternalSetStateFromInput() override {
		for (KeyMapping& keyMapping : _keyMappings) {
			SetPressedState(Buttons::Right, keyMapping.Right);
			SetPressedState(Buttons::Left, keyMapping.Left);
			SetPressedState(Buttons::Back, keyMapping.Up);
			SetPressedState(Buttons::Forward, keyMapping.Down);
			SetPressedState(Buttons::TwistCCW, keyMapping.L);
			SetPressedState(Buttons::TwistCW, keyMapping.R);
			SetPressedState(Buttons::Pull, keyMapping.B);
			SetPressedState(Buttons::Push, keyMapping.A);

			uint8_t turboFreq = 1 << (4 - _turboSpeed);
			bool turboOn = static_cast<uint8_t>(_emu->GetFrameCount() % turboFreq) < turboFreq / 2;
			if (turboOn) {
				SetPressedState(Buttons::Push, keyMapping.TurboA);
				SetPressedState(Buttons::Pull, keyMapping.TurboB);
			}
		}
	}

	void RefreshStateBuffer() override {}

public:
	enum Buttons {
		Right = 0,
		Left,
		Back,
		Forward,
		TwistCCW,
		TwistCW,
		Pull,
		Push
	};

	ChannelFController(Emulator* emu, uint8_t port, KeyMappingSet keyMappings) : BaseControlDevice(emu, ControllerType::ChannelFController, port, keyMappings) {
		_turboSpeed = keyMappings.TurboSpeed;
	}

	uint8_t ReadRam(uint16_t addr) override { return 0; }
	void WriteRam(uint16_t addr, uint8_t value) override {}

	/// Returns the port value for this controller (active-low).
	/// All bits high = no input, clear bits for pressed buttons.
	uint8_t GetPortValue() {
		uint8_t value = 0xff;
		if (IsPressed(Buttons::Right))    value &= ~0x01;
		if (IsPressed(Buttons::Left))     value &= ~0x02;
		if (IsPressed(Buttons::Back))     value &= ~0x04;
		if (IsPressed(Buttons::Forward))  value &= ~0x08;
		if (IsPressed(Buttons::TwistCCW)) value &= ~0x10;
		if (IsPressed(Buttons::TwistCW))  value &= ~0x20;
		if (IsPressed(Buttons::Pull))     value &= ~0x40;
		if (IsPressed(Buttons::Push))     value &= ~0x80;
		return value;
	}

	void InternalDrawController(InputHud& hud) override {
		hud.DrawOutline(35, 30);

		// D-pad (8-way)
		hud.DrawButton(5, 2, 3, 3, IsPressed(Buttons::Back));
		hud.DrawButton(5, 8, 3, 3, IsPressed(Buttons::Forward));
		hud.DrawButton(2, 5, 3, 3, IsPressed(Buttons::Left));
		hud.DrawButton(8, 5, 3, 3, IsPressed(Buttons::Right));

		// Action buttons
		hud.DrawButton(18, 3, 5, 3, IsPressed(Buttons::Push));
		hud.DrawButton(25, 3, 5, 3, IsPressed(Buttons::Pull));

		// Twist indicators
		hud.DrawButton(18, 8, 5, 3, IsPressed(Buttons::TwistCCW));
		hud.DrawButton(25, 8, 5, 3, IsPressed(Buttons::TwistCW));

		hud.DrawNumber(_port + 1, 14, 16);
	}

	vector<DeviceButtonName> GetKeyNameAssociations() override {
		return {
			{"right",   Buttons::Right   },
			{"left",    Buttons::Left    },
			{"back",    Buttons::Back    },
			{"forward", Buttons::Forward },
			{"twistccw", Buttons::TwistCCW},
			{"twistcw",  Buttons::TwistCW },
			{"pull",    Buttons::Pull    },
			{"push",    Buttons::Push    },
		};
	}
};
