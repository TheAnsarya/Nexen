#pragma once
#include "pch.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/InputHud.h"
#include "Shared/KeyManager.h"
#include "Utilities/Serializer.h"

/// Atari 2600 paddle controller (CX30).
///
/// Each port supports 2 paddles. Port 0 → paddles 0+1, Port 1 → paddles 2+3.
/// - Analog position read via TIA INPT0-3 ($08-$0b): capacitor charge timing
/// - Fire button read via TIA INPT4/INPT5 ($0c/$0d)
/// - Uses mouse X movement to control paddle position (like Arkanoid)
///
/// This class represents a SINGLE paddle. Two instances are used per port.
/// Paddle A uses INPT0 (port 0) or INPT2 (port 1).
/// Paddle B uses INPT1 (port 0) or INPT3 (port 1).
///
/// TAS key names: "Pf" (P = position byte via coordinates, f = fire)
class Atari2600Paddle : public BaseControlDevice {
private:
	uint8_t _paddleIndex = 0; // 0-3: which INPT port this paddle maps to
	uint32_t _turboSpeed = 0;

protected:
	string GetKeyNames() override {
		return "f";
	}

	bool HasCoordinates() override { return true; }

	void InternalSetStateFromInput() override {
		for (KeyMapping& keyMapping : _keyMappings) {
			SetPressedState(Buttons::Fire, keyMapping.A);

			uint8_t turboFreq = 1 << (4 - _turboSpeed);
			bool turboOn = static_cast<uint8_t>(_emu->GetFrameCount() % turboFreq) < turboFreq / 2;
			if (turboOn) {
				SetPressedState(Buttons::Fire, keyMapping.TurboA);
			}
		}

		// Use mouse X movement for paddle position
		SetMovement(KeyManager::GetMouseMovement(_emu, _emu->GetSettings()->GetInputConfig().MouseSensitivity));
	}

	void RefreshStateBuffer() override {
		MouseMovement mov = GetMovement();
		MousePosition pos = GetCoordinates();

		// Clamp position to 0-255 range
		int newX = static_cast<int>(pos.X) + mov.dx;
		if (newX < 0) newX = 0;
		if (newX > 255) newX = 255;

		SetCoordinates({ static_cast<int16_t>(newX), 0 });
	}

public:
	enum Buttons {
		Fire = 0
	};

	Atari2600Paddle(Emulator* emu, uint8_t port, uint8_t paddleIndex, KeyMappingSet keyMappings)
		: BaseControlDevice(emu, ControllerType::Atari2600Paddle, port, keyMappings),
		  _paddleIndex(paddleIndex) {
		_turboSpeed = keyMappings.TurboSpeed;
	}

	uint8_t ReadRam(uint16_t addr) override { return 0; }
	void WriteRam(uint16_t addr, uint8_t value) override {}

	/// Returns the paddle position (0-255).
	/// 0 = fully clockwise, 255 = fully counter-clockwise.
	uint8_t GetPaddlePosition() {
		return static_cast<uint8_t>(GetCoordinates().X & 0xff);
	}

	/// Returns the INPT index this paddle maps to (0-3).
	[[nodiscard]] uint8_t GetPaddleIndex() const { return _paddleIndex; }

	/// Returns INPT value for this paddle.
	/// Bit 7: set when capacitor has charged past threshold.
	/// Simplified: map position [0,255] to threshold comparison.
	uint8_t GetInptState(uint8_t scanlineCounter) {
		// Simple threshold model: position determines when bit 7 goes high
		// Higher position value = later threshold = longer to charge
		uint8_t pos = GetPaddlePosition();
		return (scanlineCounter >= pos) ? 0x80 : 0x00;
	}

	/// Returns TIA INPT4/5 value for fire button.
	uint8_t GetFireState() {
		return IsPressed(Buttons::Fire) ? 0x00 : 0x80;
	}

	void InternalDrawController(InputHud& hud) override {
		hud.DrawOutline(35, 24);

		// Paddle knob (draw as a horizontal bar showing position)
		uint8_t pos = GetPaddlePosition();
		int barX = 3 + (pos * 25 / 255); // Scale to fit in 25px range
		hud.DrawButton(barX, 3, 5, 8, true);

		// Fire button
		hud.DrawButton(25, 14, 5, 5, IsPressed(Buttons::Fire));

		hud.DrawNumber(_paddleIndex, 16, 14);
	}

	vector<DeviceButtonName> GetKeyNameAssociations() override {
		return {
			{ "fire", Buttons::Fire },
		};
	}
};
