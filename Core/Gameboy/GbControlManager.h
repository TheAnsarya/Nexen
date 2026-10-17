#pragma once
#include "pch.h"
#include <functional>
#include "Shared/BaseControlManager.h"
#include "Shared/SettingTypes.h"

class Emulator;
class Gameboy;
class BaseControlDevice;

/// <summary>
/// Game Boy/Game Boy Color controller manager.
/// Handles the 8-button input via the P1/JOYP register ($FF00).
/// Also manages link cable serial communication for multiplayer.
/// </summary>
/// <remarks>
/// Input Hardware:
/// - Single 8-direction D-pad
/// - 4 buttons: A, B, Select, Start
/// - P1 register ($FF00) uses column selection
/// - Bits 4-5 select D-pad or button matrix
/// - Bits 0-3 return selected input state (active low)
/// 
/// P1/JOYP Register ($FF00):
/// - Bit 5: Select D-pad (0 = selected)
/// - Bit 4: Select buttons (0 = selected)
/// - Bit 3: Down or Start (active low)
/// - Bit 2: Up or Select (active low)
/// - Bit 1: Left or B (active low)
/// - Bit 0: Right or A (active low)
/// 
/// Input IRQ:
/// - Joypad interrupt triggered on button press
/// - Any button going from high to low triggers INT $60
/// - Used for waking from STOP mode
/// 
/// Super Game Boy:
/// - Additional commands via packet protocol
/// - Multiplayer support for up to 4 controllers
/// </remarks>
class GbControlManager final : public BaseControlManager {
private:
	/// <summary>Emulator instance reference.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Game Boy console reference.</summary>
	Gameboy* _console = nullptr;

	/// <summary>Previous configuration for detecting settings changes.</summary>
	GameboyConfig _prevConfig = {};

	/// <summary>Current controller manager state.</summary>
	GbControlManagerState _state = {};

public:
	/// <summary>
	/// Constructs the Game Boy controller manager.
	/// </summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="console">Game Boy console instance.</param>
	GbControlManager(Emulator* emu, Gameboy* console);

	/// <summary>Gets copy of current controller manager state.</summary>
	/// <returns>Copy of state for debugging.</returns>
	GbControlManagerState GetState();

	/// <summary>
	/// Creates a controller device for the specified type.
	/// Game Boy only has one built-in controller.
	/// </summary>
	/// <param name="type">Controller type to create.</param>
	/// <param name="port">Port index (typically 0).</param>
	/// <returns>Shared pointer to created controller device.</returns>
	shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override;

	/// <summary>
	/// Updates all connected control devices with current input state.
	/// </summary>
	void UpdateControlDevices() override;

	/// <summary>
	/// Reads the P1/JOYP register.
	/// Returns D-pad or button states based on selection bits.
	/// </summary>
	/// <returns>Input state (bits 0-3 active low, bits 4-5 selection).</returns>
	uint8_t ReadInputPort();

	/// <summary>
	/// Writes to the P1/JOYP register.
	/// Sets the column selection for D-pad vs button matrix.
	/// </summary>
	/// <param name="value">Selection bits (bits 4-5).</param>
	void WriteInputPort(uint8_t value);

	/// <summary>
	/// Processes input changes with callback for state updates.
	/// Handles joypad interrupt generation on button press.
	/// </summary>
	/// <param name="inputUpdateCallback">Callback to invoke on input change.</param>
	void ProcessInputChange(std::function<void()> inputUpdateCallback);

	/// <summary>
	/// Updates internal input state before frame processing.
	/// </summary>
	void UpdateInputState() override;

	/// <summary>Serializes controller manager state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override;
};