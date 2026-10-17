#pragma once
#include "pch.h"
#include "SMS/SmsTypes.h"
#include "Shared/SettingTypes.h"
#include "Shared/BaseControlManager.h"

class SmsConsole;
class SmsVdp;

/// <summary>
/// Sega Master System/Game Gear controller manager.
/// Handles input for SMS, Game Gear, SG-1000, and ColecoVision modes.
/// Manages two controller ports with various device types.
/// </summary>
/// <remarks>
/// Controller Port Hardware:
/// - Two 9-pin controller ports (Port A and Port B)
/// - TH (pin 7) and TR (pin 9) lines for light gun and paddle support
/// - Port DC ($DC) returns D-pad and buttons for both ports
/// - Port DD ($DD) returns additional lines and reset button
/// 
/// Controller Types Supported:
/// - SMS Control Pad (D-pad, 1, 2 buttons)
/// - Game Gear built-in controls (D-pad, 1, 2, Start)
/// - Light Phaser (light gun using TH line for light detection)
/// - Paddle Controller (analog paddle using TH line)
/// - Sports Pad (trackball controller)
/// - ColecoVision controllers (keypad + joystick)
/// 
/// System Variations:
/// - SMS: Two ports, pause button triggers NMI
/// - Game Gear: Built-in controls, Start button instead of Pause
/// - SG-1000: Joystick and keypad support
/// - ColecoVision: Keypad controllers with special port handling
/// </remarks>
class SmsControlManager : public BaseControlManager {
private:
	/// <summary>SMS console reference for system state access.</summary>
	SmsConsole* _console = nullptr;

	/// <summary>VDP reference for light gun coordinate detection.</summary>
	SmsVdp* _vdp = nullptr;

	/// <summary>Current controller manager state.</summary>
	SmsControlManagerState _state = {};

	/// <summary>Previous SMS config for detecting settings changes.</summary>
	SmsConfig _prevConfig = {};

	/// <summary>Previous ColecoVision config for detecting settings changes.</summary>
	CvConfig _prevCvConfig = {};

	/// <summary>
	/// Gets the TH (pin 7) line state for a controller port.
	/// Used for light gun and paddle timing.
	/// </summary>
	/// <param name="portB">True for port B, false for port A.</param>
	/// <returns>TH line state.</returns>
	bool GetTh(bool portB);

	/// <summary>
	/// Gets the TR (pin 9) line state for a controller port.
	/// </summary>
	/// <param name="portB">True for port B, false for port A.</param>
	/// <returns>TR line state.</returns>
	bool GetTr(bool portB);

	/// <summary>
	/// Internal read of controller port data.
	/// </summary>
	/// <param name="port">Port number (0=$DC, 1=$DD).</param>
	/// <returns>Port data value.</returns>
	uint8_t InternalReadPort(uint8_t port);

	/// <summary>
	/// Reads ColecoVision controller port.
	/// Handles keypad and joystick multiplexing.
	/// </summary>
	/// <param name="port">Port number.</param>
	/// <returns>Controller data.</returns>
	uint8_t ReadColecoVisionPort(uint8_t port);

	/// <summary>
	/// Writes to ColecoVision control port.
	/// Selects between keypad and joystick mode.
	/// </summary>
	/// <param name="value">Control value.</param>
	void WriteColecoVisionPort(uint8_t value);

public:
	/// <summary>
	/// Constructs the SMS controller manager.
	/// </summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="console">SMS console reference.</param>
	/// <param name="vdp">VDP reference for light gun support.</param>
	SmsControlManager(Emulator* emu, SmsConsole* console, SmsVdp* vdp);

	/// <summary>
	/// Creates a controller device for the specified port and type.
	/// </summary>
	/// <param name="type">Controller type to create.</param>
	/// <param name="port">Port index (0-1).</param>
	/// <returns>Shared pointer to created controller device.</returns>
	shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override;

	/// <summary>Gets mutable reference to controller manager state.</summary>
	/// <returns>Reference to state.</returns>
	SmsControlManagerState& GetState() { return _state; }

	/// <summary>
	/// Updates all connected control devices with current input state.
	/// </summary>
	void UpdateControlDevices() override;

	/// <summary>
	/// Checks if the Pause button is pressed.
	/// On SMS, Pause triggers NMI; on Game Gear, Start is handled differently.
	/// </summary>
	/// <returns>True if pause/start is pressed.</returns>
	bool IsPausePressed();

	/// <summary>
	/// Reads a controller I/O port.
	/// </summary>
	/// <param name="port">Port number ($DC or $DD).</param>
	/// <returns>Port data value.</returns>
	uint8_t ReadPort(uint8_t port);

	/// <summary>
	/// Writes to the control port register.
	/// Controls TH/TR output direction and values.
	/// </summary>
	/// <param name="value">Control register value.</param>
	void WriteControlPort(uint8_t value);

	/// <summary>Serializes controller manager state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override;
};