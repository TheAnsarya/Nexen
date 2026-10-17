#pragma once
#include "pch.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/InputHud.h"
#include "Utilities/Serializer.h"

/// <summary>
/// Game Boy standard controller input device.
/// </summary>
/// <remarks>
/// **Physical Layout:**
/// The Game Boy controller is built into the console:
/// - D-Pad: Up, Down, Left, Right (left side)
/// - Buttons: A, B (right side)
/// - Control: Start, Select (center bottom)
///
/// **Hardware Interface:**
/// The controller is read via the $FF00 (P1/JOYP) register:
/// ```
/// Bit 5: Select D-Pad (active low)
/// Bit 4: Select Buttons (active low)
/// Bit 3: Down or Start (active low)
/// Bit 2: Up or Select (active low)
/// Bit 1: Left or B (active low)
/// Bit 0: Right or A (active low)
/// ```
///
/// **Reading Process:**
/// 1. Write to select D-Pad or buttons
/// 2. Read back to get 4 button states
/// 3. Switch selection to read other 4 buttons
///
/// **Invalid Input Handling:**
/// By default, simultaneous opposite D-Pad presses (Up+Down or Left+Right)
/// are filtered out as they're physically impossible on real hardware
/// and can cause game glitches. Can be disabled in settings.
///
/// **Turbo Feature:**
/// Emulator adds turbo A/B buttons that auto-fire at configurable rates.
/// Turbo speed affects how many frames between on/off cycles.
/// </remarks>
class GbController : public BaseControlDevice {
private:
	/// <summary>Turbo auto-fire speed setting (0-4).</summary>
	uint32_t _turboSpeed = 0;

protected:
	/// <summary>
	/// Returns button character codes for state representation.
	/// </summary>
	/// <returns>String "UDLRSsBA" (Up/Down/Left/Right/Start/Select/B/A).</returns>
	string GetKeyNames() override {
		return "UDLRSsBA";
	}

	/// <summary>
	/// Updates button states from input device mappings.
	/// </summary>
	/// <remarks>
	/// Applies key mappings, turbo auto-fire, and optionally filters
	/// invalid simultaneous opposite D-Pad presses.
	/// </remarks>
	void InternalSetStateFromInput() override {
		for (KeyMapping& keyMapping : _keyMappings) {
			// Map standard buttons
			SetPressedState(Buttons::A, keyMapping.A);
			SetPressedState(Buttons::B, keyMapping.B);
			SetPressedState(Buttons::Start, keyMapping.Start);
			SetPressedState(Buttons::Select, keyMapping.Select);
			SetPressedState(Buttons::Up, keyMapping.Up);
			SetPressedState(Buttons::Down, keyMapping.Down);
			SetPressedState(Buttons::Left, keyMapping.Left);
			SetPressedState(Buttons::Right, keyMapping.Right);

			// Apply turbo auto-fire (on/off based on frame count)
			uint8_t turboFreq = 1 << (4 - _turboSpeed);
			bool turboOn = (uint8_t)(_emu->GetFrameCount() % turboFreq) < turboFreq / 2;
			if (turboOn) {
				SetPressedState(Buttons::A, keyMapping.TurboA);
				SetPressedState(Buttons::B, keyMapping.TurboB);
			}

			// Filter physically impossible inputs (unless allowed)
			if (!_emu->GetSettings()->GetGameboyConfig().AllowInvalidInput) {
				// Up+Down pressed = treat as neither pressed
				if (IsPressed(Buttons::Up) && IsPressed(Buttons::Down)) {
					ClearBit(Buttons::Down);
					ClearBit(Buttons::Up);
				}
				// Left+Right pressed = treat as neither pressed
				if (IsPressed(Buttons::Left) && IsPressed(Buttons::Right)) {
					ClearBit(Buttons::Left);
					ClearBit(Buttons::Right);
				}
			}
		}
	}

	/// <summary>Refreshes internal state buffer (no-op for GB).</summary>
	void RefreshStateBuffer() override {
	}

public:
	/// <summary>
	/// Button bit indices for state tracking.
	/// </summary>
	enum Buttons {
		Up = 0,     ///< D-Pad up
		Down,       ///< D-Pad down
		Left,       ///< D-Pad left
		Right,      ///< D-Pad right
		Start,      ///< Start button
		Select,     ///< Select button
		B,          ///< B button
		A           ///< A button
	};

	/// <summary>
	/// Creates a Game Boy controller.
	/// </summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="port">Controller port (0=P1).</param>
	/// <param name="keyMappings">Key binding configuration.</param>
	GbController(Emulator* emu, uint8_t port, KeyMappingSet keyMappings) : BaseControlDevice(emu, ControllerType::GameboyController, port, keyMappings) {
		_turboSpeed = keyMappings.TurboSpeed;
	}

	/// <summary>Reads from controller address (returns 0).</summary>
	/// <param name="addr">Address being read.</param>
	/// <returns>Always 0 (actual reads go through memory manager).</returns>
	uint8_t ReadRam(uint16_t addr) override {
		return 0;
	}

	/// <summary>Writes to controller address (no-op).</summary>
	/// <param name="addr">Address being written.</param>
	/// <param name="value">Value being written.</param>
	void WriteRam(uint16_t addr, uint8_t value) override {
	}

	/// <summary>
	/// Draws controller state on input HUD overlay.
	/// </summary>
	/// <param name="hud">HUD drawing interface.</param>
	void InternalDrawController(InputHud& hud) override {
		hud.DrawOutline(35, 14);

		// D-Pad (cross shape)
		hud.DrawButton(5, 3, 3, 3, IsPressed(Buttons::Up));
		hud.DrawButton(5, 9, 3, 3, IsPressed(Buttons::Down));
		hud.DrawButton(2, 6, 3, 3, IsPressed(Buttons::Left));
		hud.DrawButton(8, 6, 3, 3, IsPressed(Buttons::Right));
		hud.DrawButton(5, 6, 3, 3, false);  // Center (not pressable)

		// A/B buttons
		hud.DrawButton(30, 7, 3, 3, IsPressed(Buttons::A));
		hud.DrawButton(25, 7, 3, 3, IsPressed(Buttons::B));

		// Start/Select
		hud.DrawButton(13, 9, 4, 2, IsPressed(Buttons::Select));
		hud.DrawButton(18, 9, 4, 2, IsPressed(Buttons::Start));

		// Player number
		hud.DrawNumber(_port + 1, 16, 2);
	}

	/// <summary>
	/// Gets button name to enum associations for scripting/config.
	/// </summary>
	/// <returns>Vector of name/button pairs.</returns>
	vector<DeviceButtonName> GetKeyNameAssociations() override {
		return {
		    {"a",      Buttons::A     },
		    {"b",      Buttons::B     },
		    {"start",  Buttons::Start },
		    {"select", Buttons::Select},
		    {"up",     Buttons::Up    },
		    {"down",   Buttons::Down  },
		    {"left",   Buttons::Left  },
		    {"right",  Buttons::Right },
		};
	}
};