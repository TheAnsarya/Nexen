#pragma once
#include "pch.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/Emulator.h"
#include "Shared/InputHud.h"

/// <summary>
/// Fairchild Channel F console panel buttons.
///
/// The console has 4 front panel buttons:
///   Bit 0: Time (Reset)
///   Bit 1: Mode (Select game variation)
///   Bit 2: Hold (Pause)
///   Bit 3: Start
///
/// Read through port 0 (active-low, bits 0-3).
/// TAS key names: "TMHS" (4 console buttons)
/// </summary>
class ChannelFConsolePanel : public BaseControlDevice {
protected:
	string GetKeyNames() override {
		return "TMHS";
	}

	void InternalSetStateFromInput() override {
		for (KeyMapping& keyMapping : _keyMappings) {
			SetPressedState(Buttons::Time, keyMapping.Start);
			SetPressedState(Buttons::Mode, keyMapping.Select);
			SetPressedState(Buttons::Hold, keyMapping.L);
			SetPressedState(Buttons::Start, keyMapping.R);
		}
	}

	void RefreshStateBuffer() override {}

public:
	enum Buttons {
		Time = 0,
		Mode,
		Hold,
		Start
	};

	ChannelFConsolePanel(Emulator* emu, KeyMappingSet keyMappings) : BaseControlDevice(emu, ControllerType::ChannelFConsolePanel, BaseControlDevice::ConsoleInputPort, keyMappings) {
	}

	uint8_t ReadRam(uint16_t addr) override { return 0; }
	void WriteRam(uint16_t addr, uint8_t value) override {}

	/// Returns console button state (active-low, bits 0-3).
	uint8_t GetConsoleButtons() {
		uint8_t value = 0xff;
		if (IsPressed(Buttons::Time))  value &= ~0x01;
		if (IsPressed(Buttons::Mode))  value &= ~0x02;
		if (IsPressed(Buttons::Hold))  value &= ~0x04;
		if (IsPressed(Buttons::Start)) value &= ~0x08;
		return value;
	}

	void InternalDrawController(InputHud& hud) override {
		hud.DrawOutline(35, 14);
		hud.DrawButton(2, 2, 5, 3, IsPressed(Buttons::Time));
		hud.DrawButton(9, 2, 5, 3, IsPressed(Buttons::Mode));
		hud.DrawButton(16, 2, 5, 3, IsPressed(Buttons::Hold));
		hud.DrawButton(23, 2, 5, 3, IsPressed(Buttons::Start));
	}

	vector<DeviceButtonName> GetKeyNameAssociations() override {
		return {
			{"time",  Buttons::Time },
			{"mode",  Buttons::Mode },
			{"hold",  Buttons::Hold },
			{"start", Buttons::Start},
		};
	}
};
