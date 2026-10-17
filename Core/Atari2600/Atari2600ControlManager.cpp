#include "pch.h"
#include "Atari2600/Atari2600ControlManager.h"
#include "Atari2600/Atari2600Controller.h"
#include "Atari2600/Atari2600Paddle.h"
#include "Atari2600/Atari2600Keypad.h"
#include "Atari2600/Atari2600DrivingController.h"
#include "Atari2600/Atari2600BoosterGrip.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Utilities/Serializer.h"

Atari2600ControlManager::Atari2600ControlManager(Emulator* emu)
	: BaseControlManager(emu, CpuType::Atari2600) {
}

shared_ptr<BaseControlDevice> Atari2600ControlManager::CreateControllerDevice(ControllerType type, uint8_t port) {
	shared_ptr<BaseControlDevice> device;
	Atari2600Config& cfg = _emu->GetSettings()->GetAtari2600Config();
	KeyMappingSet& keys = (port == 0) ? cfg.Port1.Keys : cfg.Port2.Keys;

	switch (type) {
		default:
		case ControllerType::None:
			break;
		case ControllerType::Atari2600Joystick:
			device = std::make_shared<Atari2600Controller>(_emu, port, keys);
			break;
		case ControllerType::Atari2600Paddle:
			// Create paddle A for this port (index 0 or 2)
			device = std::make_shared<Atari2600Paddle>(_emu, port, port * 2, keys);
			break;
		case ControllerType::Atari2600Keypad:
			device = std::make_shared<Atari2600Keypad>(_emu, port, keys);
			break;
		case ControllerType::Atari2600DrivingController:
			device = std::make_shared<Atari2600DrivingController>(_emu, port, keys);
			break;
		case ControllerType::Atari2600BoosterGrip:
			device = std::make_shared<Atari2600BoosterGrip>(_emu, port, keys);
			break;
	}
	return device;
}

void Atari2600ControlManager::UpdateControlDevices() {
	Atari2600Config cfg = _emu->GetSettings()->GetAtari2600Config();
	if (_emu->GetSettings()->IsEqual(_prevConfig, cfg) && _controlDevices.size() > 0) {
		return;
	}
	_prevConfig = cfg;

	auto lock = _deviceLock.AcquireSafe();

	// Rebuild device list: re-register valid system devices and add controllers.
	// ClearDevices() is not used here because _systemDevices may contain a
	// null entry when the Emulator was not fully initialized (e.g., unit tests
	// that create a bare Emulator without calling Initialize()).
	_controlDevices.clear();
	for (const shared_ptr<BaseControlDevice>& sysDevice : _systemDevices) {
		if (sysDevice) {
			RegisterControlDevice(sysDevice);
		}
	}

	// Port 1 controller
	shared_ptr<BaseControlDevice> dev0(CreateControllerDevice(cfg.Port1.Type, 0));
	if (dev0) {
		RegisterControlDevice(dev0);
	}

	// Port 2 controller
	shared_ptr<BaseControlDevice> dev1(CreateControllerDevice(cfg.Port2.Type, 1));
	if (dev1) {
		RegisterControlDevice(dev1);
	}
}

void Atari2600ControlManager::UpdateInputState() {
	BaseControlManager::UpdateInputState();

	uint8_t swcha = 0xff;
	_fireP0 = 0x80;
	_fireP1 = 0x80;
	_inpt[0] = 0x80;
	_inpt[1] = 0x80;
	_inpt[2] = 0x80;
	_inpt[3] = 0x80;

	for (shared_ptr<BaseControlDevice>& controller : _controlDevices) {
		uint8_t port = controller->GetPort();
		ControllerType type = controller->GetControllerType();

		switch (type) {
			case ControllerType::Atari2600Joystick: {
				auto* joystick = static_cast<Atari2600Controller*>(controller.get());
				uint8_t nibble = joystick->GetDirectionNibble();
				if (port == 0) {
					swcha = (uint8_t)((swcha & 0x0f) | (nibble << 4));
					_fireP0 = joystick->GetFireState();
				} else if (port == 1) {
					swcha = (uint8_t)((swcha & 0xf0) | nibble);
					_fireP1 = joystick->GetFireState();
				}
				break;
			}

			case ControllerType::Atari2600Paddle: {
				auto* paddle = static_cast<Atari2600Paddle*>(controller.get());
				uint8_t idx = paddle->GetPaddleIndex();
				if (idx < 4) {
					// Simplified: position maps directly to INPT threshold
					// Full accuracy would use scanline-based charge timing
					_inpt[idx] = paddle->GetInptState(128); // Mid-frame estimate
				}
				if (port == 0) {
					_fireP0 = paddle->GetFireState();
				} else if (port == 1) {
					_fireP1 = paddle->GetFireState();
				}
				break;
			}

			case ControllerType::Atari2600Keypad: {
				// Keypad INPT values are handled per-scanline via GetKeypadColumnState()
				// SWCHA is not driven by keypad (no direction nibble)
				break;
			}

			case ControllerType::Atari2600DrivingController: {
				auto* driving = static_cast<Atari2600DrivingController*>(controller.get());
				uint8_t nibble = driving->GetDirectionNibble();
				if (port == 0) {
					swcha = (uint8_t)((swcha & 0x0f) | (nibble << 4));
					_fireP0 = driving->GetFireState();
				} else if (port == 1) {
					swcha = (uint8_t)((swcha & 0xf0) | nibble);
					_fireP1 = driving->GetFireState();
				}
				break;
			}

			case ControllerType::Atari2600BoosterGrip: {
				auto* grip = static_cast<Atari2600BoosterGrip*>(controller.get());
				uint8_t nibble = grip->GetDirectionNibble();
				if (port == 0) {
					swcha = (uint8_t)((swcha & 0x0f) | (nibble << 4));
					_fireP0 = grip->GetFireState();
					_inpt[0] = grip->GetBoosterState();
					_inpt[1] = grip->GetTriggerState();
				} else if (port == 1) {
					swcha = (uint8_t)((swcha & 0xf0) | nibble);
					_fireP1 = grip->GetFireState();
					_inpt[2] = grip->GetBoosterState();
					_inpt[3] = grip->GetTriggerState();
				}
				break;
			}

			default:
				break;
		}
	}
	_swcha = swcha;

	// Build SWCHB from config
	// Bit 0: Reset (1=not pressed), Bit 1: Select (1=not pressed)
	// Bit 3: P0 Difficulty (1=B/Amateur, 0=A/Pro)
	// Bit 6: P1 Difficulty (1=B/Amateur, 0=A/Pro)
	// Bit 7: Color/BW (1=Color, 0=BW)
	Atari2600Config& cfg = _emu->GetSettings()->GetAtari2600Config();
	uint8_t swchb = 0x37; // Reset=1, Select=1, unused bits=1
	if (_consoleSwitchReset) swchb &= ~0x01;  // Active-low: pressed clears bit
	if (_consoleSwitchSelect) swchb &= ~0x02; // Active-low: pressed clears bit
	if (cfg.P0DifficultyB) swchb |= 0x08;
	if (cfg.P1DifficultyB) swchb |= 0x40;
	if (cfg.ColorMode) swchb |= 0x80;

	_swchb = swchb;
}

uint8_t Atari2600ControlManager::GetInpt(uint8_t index) {
	if (index > 3) return 0x80;

	// Check if a keypad is connected on the corresponding port
	uint8_t keypadPort = (index < 2) ? 0 : 1; // INPT0-1 → port 0, INPT2-3 → port 1
	for (shared_ptr<BaseControlDevice>& controller : _controlDevices) {
		if (controller->GetControllerType() == ControllerType::Atari2600Keypad &&
			controller->GetPort() == keypadPort) {
			auto* keypad = static_cast<Atari2600Keypad*>(controller.get());
			// Determine which row is selected from SWCHA
			uint8_t portNibble = (keypadPort == 0) ? ((_swcha >> 4) & 0x0f) : (_swcha & 0x0f);
			// Find which row has its bit pulled low (active-low scan)
			uint8_t colIndex = index - (keypadPort * 2); // 0 or 1 within port
			// Note: keypad only uses 3 columns, but INPT0-3 only gives us
			// 2 per port (INPT0/1 for port 0, INPT2/3 for port 1)
			// Actual 2600 hardware: Port 0 keypad uses INPT0/1 for cols,
			// port 1 keypad uses INPT2/3 for cols
			for (uint8_t row = 0; row < 4; row++) {
				if (!(portNibble & (1 << row))) {
					// This row is selected (active low)
					return keypad->GetColumnState(row, colIndex);
				}
			}
			return 0x80; // No row selected
		}
	}

	return _inpt[index];
}

void Atari2600ControlManager::SetConsoleSwitchState(bool select, bool reset) {
	_consoleSwitchSelect = select;
	_consoleSwitchReset = reset;
}

void Atari2600ControlManager::Serialize(Serializer& s) {
	BaseControlManager::Serialize(s);
	SV(_swcha);
	SV(_swchb);
	SV(_fireP0);
	SV(_fireP1);
	SVArray(_inpt, 4);
	SV(_consoleSwitchSelect);
	SV(_consoleSwitchReset);
}
