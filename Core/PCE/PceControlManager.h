#pragma once
#include "pch.h"
#include "Shared/BaseControlManager.h"
#include "Shared/SettingTypes.h"
#include "PCE/PceTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;

/// <summary>
/// PC Engine/TurboGrafx-16 controller manager.
/// Handles input for the PC Engine multitap (up to 5 controllers) and
/// various controller types including standard pads and 6-button pads.
/// </summary>
/// <remarks>
/// Controller Port Features:
/// - Single controller port on console (directly or via multitap)
/// - Multitap supports up to 5 controllers
/// - Controller select via CLR/SEL lines
/// - 4-bit data read per input poll
/// 
/// Controller Types Supported:
/// - Standard 2-button pad (Run, Select, D-pad, I, II)
/// - Avenue Pad 6 (6-button pad with III, IV, V, VI)
/// - PC Engine Mouse
/// - Turbo switches for auto-fire on I/II buttons
/// </remarks>
class PceControlManager final : public BaseControlManager {
private:
	/// <summary>Current controller manager state including select lines.</summary>
	PceControlManagerState _state = {};

	/// <summary>Previous configuration for detecting settings changes.</summary>
	PcEngineConfig _prevConfig = {};

public:
	/// <summary>
	/// Constructs the PC Engine controller manager.
	/// </summary>
	/// <param name="emu">Emulator instance for settings and input polling.</param>
	PceControlManager(Emulator* emu);

	/// <summary>Gets mutable reference to controller manager state.</summary>
	/// <returns>Reference to state for serialization/debugging.</returns>
	PceControlManagerState& GetState();

	/// <summary>
	/// Creates a controller device for the specified port and type.
	/// </summary>
	/// <param name="type">Type of controller to create.</param>
	/// <param name="port">Port index (0-4 with multitap).</param>
	/// <returns>Shared pointer to the created controller device.</returns>
	shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override;

	/// <summary>
	/// Reads the input port value.
	/// Returns 4-bit controller data based on current SEL/CLR state.
	/// </summary>
	/// <returns>4-bit input data in lower nibble.</returns>
	uint8_t ReadInputPort();

	/// <summary>
	/// Writes to the input port to control SEL/CLR lines.
	/// SEL selects which nibble (directions or buttons) to read.
	/// CLR resets the multitap controller index.
	/// </summary>
	/// <param name="value">Control value with SEL (bit 0) and CLR (bit 1).</param>
	void WriteInputPort(uint8_t value);

	/// <summary>
	/// Updates all connected control devices with current input state.
	/// Called before input is read to refresh controller states.
	/// </summary>
	void UpdateControlDevices() override;

	/// <summary>Serializes controller manager state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override;
};