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

	GenesisController(Emulator* emu, uint8_t port, KeyMappingSet keyMappings, bool sixButton = true)
		: BaseControlDevice(emu, sixButton ? ControllerType::GenesisController : ControllerType::GenesisController3Buttons, port, keyMappings) {
		_turboSpeed = keyMappings.TurboSpeed;
		_sixButton = sixButton;
	}

	bool IsSixButtonController() const {
		return _sixButton;
	}

	uint32_t GetButtonMask() {
		enum : uint32_t {
			MaskUp = 1u << 0,
			MaskDown = 1u << 1,
			MaskLeft = 1u << 2,
			MaskRight = 1u << 3,
			MaskA = 1u << 4,
			MaskB = 1u << 5,
			MaskC = 1u << 6,
			MaskStart = 1u << 7,
			MaskX = 1u << 8,
			MaskY = 1u << 9,
			MaskZ = 1u << 10,
			MaskMode = 1u << 11,
		};

		uint32_t mask = 0;
		if (IsPressed(Buttons::Up)) {
			mask |= MaskUp;
		}
		if (IsPressed(Buttons::Down)) {
			mask |= MaskDown;
		}
		if (IsPressed(Buttons::Left)) {
			mask |= MaskLeft;
		}
		if (IsPressed(Buttons::Right)) {
			mask |= MaskRight;
		}
		if (IsPressed(Buttons::A)) {
			mask |= MaskA;
		}
		if (IsPressed(Buttons::B)) {
			mask |= MaskB;
		}
		if (IsPressed(Buttons::C)) {
			mask |= MaskC;
		}
		if (IsPressed(Buttons::Start)) {
			mask |= MaskStart;
		}
		if (_sixButton) {
			if (IsPressed(Buttons::X)) {
				mask |= MaskX;
			}
			if (IsPressed(Buttons::Y)) {
				mask |= MaskY;
			}
			if (IsPressed(Buttons::Z)) {
				mask |= MaskZ;
			}
			if (IsPressed(Buttons::Mode)) {
				mask |= MaskMode;
			}
		}
		return mask;
	}

	uint8_t ReadRam(uint16_t addr) override { return 0; }
	void WriteRam(uint16_t addr, uint8_t value) override {}

protected:
	string GetKeyNames() override {
		return _sixButton ? "UDLRABCSxyzm" : "UDLRABCS";
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
				if (_sixButton) {
					SetPressedState(Buttons::X, keyMapping.CustomKeys[Buttons::X]);
					SetPressedState(Buttons::Y, keyMapping.CustomKeys[Buttons::Y]);
					SetPressedState(Buttons::Z, keyMapping.CustomKeys[Buttons::Z]);
					SetPressedState(Buttons::Mode, keyMapping.CustomKeys[Buttons::Mode]);
				}
			} else {
				SetPressedState(Buttons::Up, keyMapping.Up);
				SetPressedState(Buttons::Down, keyMapping.Down);
				SetPressedState(Buttons::Left, keyMapping.Left);
				SetPressedState(Buttons::Right, keyMapping.Right);
				SetPressedState(Buttons::A, keyMapping.A);
				SetPressedState(Buttons::B, keyMapping.B);
				SetPressedState(Buttons::C, keyMapping.X);
				SetPressedState(Buttons::Start, keyMapping.Start);
				if (_sixButton) {
					SetPressedState(Buttons::X, keyMapping.Y);
					SetPressedState(Buttons::Y, keyMapping.L);
					SetPressedState(Buttons::Z, keyMapping.R);
					SetPressedState(Buttons::Mode, keyMapping.GenericKey1);
				}

				uint8_t turboFreq = 1 << (4 - _turboSpeed);
				bool turboOn = (uint8_t)(_emu->GetFrameCount() % turboFreq) < turboFreq / 2;
				if (turboOn) {
					SetPressedState(Buttons::A, keyMapping.TurboA);
					SetPressedState(Buttons::B, keyMapping.TurboB);
					SetPressedState(Buttons::C, keyMapping.TurboX);
					if (_sixButton) {
						SetPressedState(Buttons::X, keyMapping.TurboY);
						SetPressedState(Buttons::Y, keyMapping.TurboL);
						SetPressedState(Buttons::Z, keyMapping.TurboR);
					}
				}
			}

			if (IsPressed(Buttons::Up) && IsPressed(Buttons::Down)) {
				ClearBit(Buttons::Up);
				ClearBit(Buttons::Down);
			}
			if (IsPressed(Buttons::Left) && IsPressed(Buttons::Right)) {
				ClearBit(Buttons::Left);
				ClearBit(Buttons::Right);
			}
		}
	}

private:
	uint32_t _turboSpeed = 0;
	bool _sixButton = true;
};
