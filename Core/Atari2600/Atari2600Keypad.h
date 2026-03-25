#pragma once
#include "pch.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/InputHud.h"
#include "Utilities/Serializer.h"

/// Atari 2600 keypad controller (CX50 / Video Touch Pad).
///
/// 12-button keypad (0-9, *, #) in a 4x3 matrix.
/// The console scans the keypad by writing row select bits to SWCHA
/// (DDR set to output), then reading column results from INPT0-3.
///
/// Matrix layout:
///          Col0(INPT0)  Col1(INPT1)  Col2(INPT2)
///   Row0:     1            2            3
///   Row1:     4            5            6
///   Row2:     7            8            9
///   Row3:     *            0            #
///
/// Row select via SWCHA nibble (active-low, one row at a time):
///   Row0: bit 0 low (0x0e), Row1: bit 1 low (0x0d),
///   Row2: bit 2 low (0x0c), Row3: bit 3 low (0x0b)
///
/// Column read: INPT bit 7: 0 = key pressed, 0x80 = not pressed
///
/// TAS key names: "123456789s0p" (s=star, p=pound, 12 chars)
class Atari2600Keypad : public BaseControlDevice {
protected:
	string GetKeyNames() override {
		return "123456789s0p";
	}

	void InternalSetStateFromInput() override {
		for (KeyMapping& keyMapping : _keyMappings) {
			SetPressedState(Buttons::Key1, keyMapping.CustomKeys[0]);
			SetPressedState(Buttons::Key2, keyMapping.CustomKeys[1]);
			SetPressedState(Buttons::Key3, keyMapping.CustomKeys[2]);
			SetPressedState(Buttons::Key4, keyMapping.CustomKeys[3]);
			SetPressedState(Buttons::Key5, keyMapping.CustomKeys[4]);
			SetPressedState(Buttons::Key6, keyMapping.CustomKeys[5]);
			SetPressedState(Buttons::Key7, keyMapping.CustomKeys[6]);
			SetPressedState(Buttons::Key8, keyMapping.CustomKeys[7]);
			SetPressedState(Buttons::Key9, keyMapping.CustomKeys[8]);
			SetPressedState(Buttons::KeyStar, keyMapping.CustomKeys[9]);
			SetPressedState(Buttons::Key0, keyMapping.CustomKeys[10]);
			SetPressedState(Buttons::KeyPound, keyMapping.CustomKeys[11]);
		}
	}

	void RefreshStateBuffer() override {}

public:
	enum Buttons {
		Key1 = 0,
		Key2,
		Key3,
		Key4,
		Key5,
		Key6,
		Key7,
		Key8,
		Key9,
		KeyStar,
		Key0,
		KeyPound
	};

	Atari2600Keypad(Emulator* emu, uint8_t port, KeyMappingSet keyMappings)
		: BaseControlDevice(emu, ControllerType::Atari2600Keypad, port, keyMappings) {
	}

	uint8_t ReadRam(uint16_t addr) override { return 0; }
	void WriteRam(uint16_t addr, uint8_t value) override {}

	/// Returns the INPT column state for the given row select value.
	/// row: 0-3 (which row is being scanned)
	/// colIndex: 0-2 (which INPT port: INPT0/1/2 or INPT2/3 depending on port)
	/// Returns: bit 7: 0 = pressed, 0x80 = not pressed
	uint8_t GetColumnState(uint8_t row, uint8_t colIndex) {
		// Map row + column to our button enum
		// Row 0: Key1(0), Key2(1), Key3(2)
		// Row 1: Key4(3), Key5(4), Key6(5)
		// Row 2: Key7(6), Key8(7), Key9(8)
		// Row 3: KeyStar(9), Key0(10), KeyPound(11)
		int buttonIdx = row * 3 + colIndex;
		if (buttonIdx < 0 || buttonIdx > 11) return 0x80;
		return IsPressed(static_cast<Buttons>(buttonIdx)) ? 0x00 : 0x80;
	}

	/// Returns the SWCHA nibble for this keypad.
	/// Keypads don't drive SWCHA as direction input — returns 0x0f (all high).
	uint8_t GetDirectionNibble() {
		return 0x0f; // No direction bits
	}

	void InternalDrawController(InputHud& hud) override {
		hud.DrawOutline(35, 47);

		// Draw 4x3 grid of buttons
		const char* labels[] = { "1","2","3","4","5","6","7","8","9","*","0","#" };
		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 3; col++) {
				int idx = row * 3 + col;
				hud.DrawButton(3 + col * 10, 3 + row * 10, 8, 8, IsPressed(static_cast<Buttons>(idx)));
			}
		}

		hud.DrawNumber(_port + 1, 16, 43);
	}

	vector<DeviceButtonName> GetKeyNameAssociations() override {
		return {
			{ "1",     Buttons::Key1 },
			{ "2",     Buttons::Key2 },
			{ "3",     Buttons::Key3 },
			{ "4",     Buttons::Key4 },
			{ "5",     Buttons::Key5 },
			{ "6",     Buttons::Key6 },
			{ "7",     Buttons::Key7 },
			{ "8",     Buttons::Key8 },
			{ "9",     Buttons::Key9 },
			{ "star",  Buttons::KeyStar },
			{ "0",     Buttons::Key0 },
			{ "pound", Buttons::KeyPound },
		};
	}
};
