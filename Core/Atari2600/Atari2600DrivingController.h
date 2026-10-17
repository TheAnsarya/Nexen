#pragma once
#include "pch.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/InputHud.h"
#include "Utilities/Serializer.h"

/// Atari 2600 driving controller (CX20).
///
/// Continuous rotation via 2-bit gray code on SWCHA bits 1:0.
/// Gray code sequence (clockwise): 00 → 01 → 11 → 10 → 00
/// Gray code sequence (counter-clockwise): 00 → 10 → 11 → 01 → 00
///
/// Port 0: Gray code in SWCHA bits 5:4, fire via INPT4
/// Port 1: Gray code in SWCHA bits 1:0, fire via INPT5
///
/// The gray code maps to the SWCHA nibble as:
///   Bit 0 (Up position): gray bit 0
///   Bit 1 (Down position): gray bit 1
///   Bits 2-3 (Left/Right): unused, set high (1)
///
/// TAS key names: "LRf" (L=rotate-left, R=rotate-right, f=fire)
class Atari2600DrivingController : public BaseControlDevice {
private:
	uint8_t _grayCodePosition = 0; // 0-3, indexes into gray code table
	uint32_t _turboSpeed = 0;

	static constexpr uint8_t GrayCodeTable[4] = { 0x00, 0x01, 0x03, 0x02 };

protected:
	string GetKeyNames() override {
		return "LRf";
	}

	void InternalSetStateFromInput() override {
		for (KeyMapping& keyMapping : _keyMappings) {
			SetPressedState(Buttons::RotateLeft, keyMapping.Left);
			SetPressedState(Buttons::RotateRight, keyMapping.Right);
			SetPressedState(Buttons::Fire, keyMapping.A);

			uint8_t turboFreq = 1 << (4 - _turboSpeed);
			bool turboOn = static_cast<uint8_t>(_emu->GetFrameCount() % turboFreq) < turboFreq / 2;
			if (turboOn) {
				SetPressedState(Buttons::Fire, keyMapping.TurboA);
			}
		}
	}

	void RefreshStateBuffer() override {
		// Advance gray code position based on rotation input
		if (IsPressed(Buttons::RotateRight)) {
			_grayCodePosition = (_grayCodePosition + 1) & 0x03;
		} else if (IsPressed(Buttons::RotateLeft)) {
			_grayCodePosition = (_grayCodePosition - 1) & 0x03;
		}
	}

public:
	enum Buttons {
		RotateLeft = 0,
		RotateRight,
		Fire
	};

	Atari2600DrivingController(Emulator* emu, uint8_t port, KeyMappingSet keyMappings)
		: BaseControlDevice(emu, ControllerType::Atari2600DrivingController, port, keyMappings) {
		_turboSpeed = keyMappings.TurboSpeed;
	}

	uint8_t ReadRam(uint16_t addr) override { return 0; }
	void WriteRam(uint16_t addr, uint8_t value) override {}

	/// Returns the SWCHA nibble for this port.
	/// Gray code in bits 1:0, bits 3:2 set high (unused).
	uint8_t GetDirectionNibble() {
		uint8_t gray = GrayCodeTable[_grayCodePosition];
		// Bits 3:2 unused (set high/released), bits 1:0 = gray code
		return 0x0c | gray;
	}

	/// Returns TIA INPT4/5 value for fire button.
	uint8_t GetFireState() {
		return IsPressed(Buttons::Fire) ? 0x00 : 0x80;
	}

	void InternalDrawController(InputHud& hud) override {
		hud.DrawOutline(35, 24);

		// Draw rotation indicator (arrow direction based on position)
		hud.DrawButton(5, 5, 3, 3, IsPressed(Buttons::RotateLeft));  // Left arrow
		hud.DrawButton(11, 5, 3, 3, IsPressed(Buttons::RotateRight)); // Right arrow

		// Rotation position indicator
		hud.DrawNumber(_grayCodePosition, 8, 12);

		// Fire button
		hud.DrawButton(25, 4, 5, 5, IsPressed(Buttons::Fire));

		hud.DrawNumber(_port + 1, 16, 18);
	}

	vector<DeviceButtonName> GetKeyNameAssociations() override {
		return {
			{ "rotateLeft",  Buttons::RotateLeft },
			{ "rotateRight", Buttons::RotateRight },
			{ "fire",        Buttons::Fire },
		};
	}

	void Serialize(Serializer& s) override {
		BaseControlDevice::Serialize(s);
		SV(_grayCodePosition);
	}
};
