#pragma once

#include "pch.h"
#include "NES/INesMemoryHandler.h"
#include "Shared/BaseControlManager.h"
#include "Shared/SettingTypes.h"
#include "Utilities/SimpleLock.h"
#include "Utilities/ISerializable.h"

class BaseControlDevice;
class SystemActionManager;
class IInputRecorder;
class IInputProvider;
class Emulator;
class NesConsole;
enum class ControllerType;

/// <summary>
/// NES/Famicom controller manager.
/// Handles input for both controller ports and expansion port devices.
/// Implements the $4016/$4017 I/O register interface.
/// </summary>
/// <remarks>
/// Hardware Interface:
/// - $4016: Controller 1 read / Controller strobe write
/// - $4017: Controller 2 read (and APU frame counter write)
/// - Read returns controller data with open bus on unused bits
/// - Write to $4016 controls strobe signal to all controllers
/// 
/// Controller Types Supported:
/// - Standard NES controller (D-pad, A, B, Select, Start)
/// - NES Zapper light gun
/// - NES Arkanoid paddle
/// - Power Pad / Family Trainer
/// - Four Score / NES Satellite multitaps
/// 
/// Famicom Expansion Port:
/// - Family BASIC Keyboard
/// - Famicom 3D System glasses
/// - Various special controllers
/// 
/// Open Bus Behavior:
/// - Unused bits return open bus values
/// - Mask varies by controller type (typically D0-D4 used)
/// </remarks>
class NesControlManager : public INesMemoryHandler, public BaseControlManager {
private:
	/// <summary>Previous configuration for detecting settings changes.</summary>
	NesConfig _prevConfig = {};

	/// <summary>Pending write address for deferred controller writes.</summary>
	uint16_t _writeAddr = 0;

	/// <summary>Pending write value for deferred controller writes.</summary>
	uint8_t _writeValue = 0;

	/// <summary>Counter for pending writes to process.</summary>
	uint8_t _writePending = 0;

protected:
	/// <summary>NES console reference.</summary>
	NesConsole* _console;

	/// <summary>Serializes controller manager state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	virtual void Serialize(Serializer& s) override;

	/// <summary>
	/// Remaps controller buttons based on current configuration.
	/// Handles turbo buttons and button remapping.
	/// </summary>
	virtual void RemapControllerButtons();

public:
	/// <summary>
	/// Constructs the NES controller manager.
	/// </summary>
	/// <param name="console">NES console instance.</param>
	NesControlManager(NesConsole* console);

	/// <summary>Destructor - cleans up controller devices.</summary>
	virtual ~NesControlManager();

	/// <summary>
	/// Gets the open bus mask for a controller port.
	/// Determines which bits return actual controller data vs open bus.
	/// </summary>
	/// <param name="port">Port number (0 or 1).</param>
	/// <returns>Bitmask of bits that return controller data.</returns>
	virtual uint8_t GetOpenBusMask(uint8_t port);

	/// <summary>
	/// Updates all connected control devices with current input state.
	/// </summary>
	void UpdateControlDevices() override;

	/// <summary>
	/// Updates internal input state before frame processing.
	/// </summary>
	void UpdateInputState() override;

	/// <summary>
	/// Saves battery-backed controller data (e.g., Datach barcode data).
	/// </summary>
	void SaveBattery();

	/// <summary>
	/// Resets the controller manager to initial state.
	/// </summary>
	/// <param name="softReset">True for soft reset, false for hard reset.</param>
	void Reset(bool softReset) override;

	/// <summary>
	/// Checks if a keyboard controller is connected.
	/// </summary>
	/// <returns>True if Family BASIC Keyboard or similar is connected.</returns>
	bool IsKeyboardConnected() override;

	/// <summary>
	/// Creates a controller device for the specified port and type.
	/// </summary>
	/// <param name="type">Controller type to create.</param>
	/// <param name="port">Port index.</param>
	/// <returns>Shared pointer to created controller device.</returns>
	shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override;

	/// <summary>
	/// Registers memory address ranges handled by this component.
	/// $4016-$4017 read, $4016 write.
	/// </summary>
	/// <param name="ranges">Memory ranges to populate.</param>
	void GetMemoryRanges(MemoryRanges& ranges) override {
		ranges.AddHandler(MemoryOperation::Read, 0x4016, 0x4017);
		ranges.AddHandler(MemoryOperation::Write, 0x4016);
	}

	/// <summary>
	/// Reads from controller I/O port.
	/// Returns controller data with open bus on unused bits.
	/// </summary>
	/// <param name="addr">Address ($4016 or $4017).</param>
	/// <returns>Controller data byte.</returns>
	uint8_t ReadRam(uint16_t addr) override;

	/// <summary>
	/// Writes to controller I/O port.
	/// Controls strobe signal to all connected controllers.
	/// </summary>
	/// <param name="addr">Address ($4016).</param>
	/// <param name="value">Strobe value (bit 0 = strobe).</param>
	void WriteRam(uint16_t addr, uint8_t value) override;

	/// <summary>
	/// Processes any pending controller writes.
	/// Used for deferred write handling.
	/// </summary>
	__noinline void ProcessWrites();

	/// <summary>
	/// Checks if there are pending controller writes to process.
	/// </summary>
	/// <returns>True if writes are pending.</returns>
	__forceinline bool HasPendingWrites() { return _writePending > 0; }
};
