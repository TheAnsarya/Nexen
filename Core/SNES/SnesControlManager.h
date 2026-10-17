#pragma once

#include "pch.h"
#include "Utilities/SimpleLock.h"
#include "Utilities/ISerializable.h"
#include "Shared/BaseControlManager.h"
#include "Shared/SettingTypes.h"

class BaseControlDevice;
class IInputRecorder;
class IInputProvider;
class SnesConsole;
class Emulator;
class SystemActionManager;
struct ControllerData;
enum class ControllerType;

/// <summary>
/// SNES controller manager.
/// Handles input for 2 controller ports with multitap support (up to 8 controllers).
/// Implements the SNES joypad auto-read feature and serial I/O protocol.
/// </summary>
/// <remarks>
/// Controller Port Hardware:
/// - Two 7-pin controller ports
/// - Serial data protocol with Clock/Latch/Data lines
/// - Hardware auto-read during V-blank (reads to $4218-$421F)
/// - Manual read via $4016/$4017 I/O ports
/// 
/// Controller Types Supported:
/// - Standard SNES controller (D-pad, A/B/X/Y, L/R, Select/Start)
/// - SNES Mouse
/// - Super Scope light gun
/// - Konami Justifier light gun
/// - Multitap (up to 4 controllers per port)
/// 
/// Auto-Read Feature:
/// - Hardware automatically reads all controllers during V-blank
/// - Results available in $4218-$421F (16-bit values)
/// - HVBJOY flag ($4212 bit 0) indicates auto-read in progress
/// - Prevents timing-sensitive manual polling issues
/// 
/// Serial Protocol:
/// - Latch pulse captures button states
/// - 16 clock pulses shift out data bits
/// - Open bus returns 1s after all bits read
/// </remarks>
class SnesControlManager final : public BaseControlManager {
private:
	/// <summary>Previous configuration for detecting settings changes.</summary>
	SnesConfig _prevConfig = {};

protected:
	/// <summary>SNES console reference.</summary>
	SnesConsole* _console = nullptr;

	/// <summary>Last value written to $4016 for debugging.</summary>
	uint8_t _lastWriteValue = 0;

	/// <summary>Auto-read strobe state from PPU.</summary>
	bool _autoReadStrobe = 0;

public:
	/// <summary>
	/// Constructs the SNES controller manager.
	/// </summary>
	/// <param name="console">SNES console instance.</param>
	SnesControlManager(SnesConsole* console);

	/// <summary>Destructor - cleans up controller devices.</summary>
	virtual ~SnesControlManager();

	/// <summary>
	/// Resets the controller manager to initial state.
	/// </summary>
	/// <param name="softReset">True for soft reset, false for hard reset.</param>
	void Reset(bool softReset) override;

	/// <summary>
	/// Updates all connected control devices with current input state.
	/// </summary>
	void UpdateControlDevices() override;

	/// <summary>
	/// Gets the last value written to the controller port.
	/// Used for debugging and some controller types.
	/// </summary>
	/// <returns>Last written value.</returns>
	uint8_t GetLastWriteValue() { return _lastWriteValue; }

	/// <summary>
	/// Creates a controller device for the specified port and type.
	/// </summary>
	/// <param name="type">Controller type to create.</param>
	/// <param name="port">Port index (0-7 with multitaps).</param>
	/// <returns>Shared pointer to created controller device.</returns>
	shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override;

	/// <summary>
	/// Reads from controller I/O port.
	/// Returns serial data from controller shift registers.
	/// </summary>
	/// <param name="addr">Address ($4016 or $4017).</param>
	/// <param name="forAutoRead">True if called during hardware auto-read.</param>
	/// <returns>Controller data byte (bit 0-1 = data, others = open bus).</returns>
	uint8_t Read(uint16_t addr, bool forAutoRead = false);

	/// <summary>
	/// Writes to controller I/O port.
	/// Controls latch signal to all connected controllers.
	/// </summary>
	/// <param name="addr">Address ($4016).</param>
	/// <param name="value">Latch value (bit 0 = latch).</param>
	void Write(uint16_t addr, uint8_t value);

	/// <summary>
	/// Sets the auto-read strobe state from PPU.
	/// Called during V-blank when hardware auto-read begins.
	/// </summary>
	/// <param name="strobe">True to begin auto-read, false to end.</param>
	void SetAutoReadStrobe(bool strobe);

	/// <summary>Serializes controller manager state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override;
};
