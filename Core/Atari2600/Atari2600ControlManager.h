#pragma once
#include "pch.h"
#include "Shared/BaseControlManager.h"
#include "Shared/SettingTypes.h"

class Emulator;

class Atari2600ControlManager final : public BaseControlManager {
private:
	Atari2600Config _prevConfig = {};
	// Cached input state for hardware to read
	uint8_t _swcha = 0xff;     // Joystick/controller directions (active-low, all released)
	uint8_t _swchb = 0xff;     // Console switches
	uint8_t _fireP0 = 0x80;    // P0 fire button (bit 7: 0=pressed, 1=released)
	uint8_t _fireP1 = 0x80;    // P1 fire button
	uint8_t _inpt[4] = { 0x80, 0x80, 0x80, 0x80 }; // INPT0-3 state (paddles/keypad/booster)

public:
	explicit Atari2600ControlManager(Emulator* emu);

	shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override;
	void UpdateControlDevices() override;
	void UpdateInputState() override;

	/// Returns INPT0-3 state for TIA read. Replaces hardcoded 0x80.
	/// For keypads, scans column based on current SWCHA row select.
	uint8_t GetInpt(uint8_t index);

	[[nodiscard]] uint8_t GetSwcha() const { return _swcha; }
	[[nodiscard]] uint8_t GetSwchb() const { return _swchb; }
	[[nodiscard]] uint8_t GetFireP0() const { return _fireP0; }
	[[nodiscard]] uint8_t GetFireP1() const { return _fireP1; }

	void Serialize(Serializer& s) override;
};
