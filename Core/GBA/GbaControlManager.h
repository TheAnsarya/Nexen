#pragma once
#include "Shared/BaseControlManager.h"
#include "Shared/SettingTypes.h"
#include "GBA/GbaTypes.h"

class Emulator;
class GbaConsole;
class GbaMemoryManager;
class BaseControlDevice;

/// <summary>
/// GBA controller manager.
/// Handles the 10-button input via KEYINPUT ($4000130) and KEYCNT ($4000132) registers.
/// Supports input-triggered interrupts for power-saving modes.
/// </summary>
/// <remarks>
/// Input Hardware:
/// - D-pad (Up, Down, Left, Right)
/// - 4 face buttons (A, B)
/// - 2 shoulder buttons (L, R)
/// - 2 system buttons (Start, Select)
/// - All buttons directly readable via KEYINPUT register
/// 
/// KEYINPUT Register ($4000130):
/// - Bit 0: A button (0 = pressed)
/// - Bit 1: B button (0 = pressed)
/// - Bit 2: Select (0 = pressed)
/// - Bit 3: Start (0 = pressed)
/// - Bit 4: Right (0 = pressed)
/// - Bit 5: Left (0 = pressed)
/// - Bit 6: Up (0 = pressed)
/// - Bit 7: Down (0 = pressed)
/// - Bit 8: R shoulder (0 = pressed)
/// - Bit 9: L shoulder (0 = pressed)
/// - All bits active low (0 = pressed, 1 = released)
/// 
/// KEYCNT Register ($4000132) - Key Interrupt Control:
/// - Bits 0-9: Button select mask (which buttons trigger IRQ)
/// - Bit 14: IRQ enable (1 = enabled)
/// - Bit 15: IRQ condition (0 = OR/any, 1 = AND/all)
/// - Allows wake from HALT/STOP on button press
/// </remarks>
class GbaControlManager final : public BaseControlManager {
private:
	/// <summary>GBA console reference.</summary>
	GbaConsole* _console = nullptr;

	/// <summary>Memory manager for IRQ signaling.</summary>
	GbaMemoryManager* _memoryManager = nullptr;

	/// <summary>Previous configuration for detecting settings changes.</summary>
	GbaConfig _prevConfig = {};

	/// <summary>Current controller manager state.</summary>
	GbaControlManagerState _state = {};

	/// <summary>
	/// Reads raw controller state.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <returns>Button state bits.</returns>
	uint8_t ReadController(uint32_t addr);

	/// <summary>
	/// Checks if input interrupt condition is met and triggers IRQ if needed.
	/// </summary>
	void CheckForIrq();

public:
	/// <summary>
	/// Constructs the GBA controller manager.
	/// </summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="console">GBA console instance.</param>
	GbaControlManager(Emulator* emu, GbaConsole* console);

	/// <summary>
	/// Initializes the controller manager with memory manager reference.
	/// </summary>
	/// <param name="memoryManager">Memory manager for IRQ handling.</param>
	void Init(GbaMemoryManager* memoryManager);

	/// <summary>Gets mutable reference to controller manager state.</summary>
	/// <returns>Reference to state.</returns>
	GbaControlManagerState& GetState();

	/// <summary>
	/// Updates internal input state before frame processing.
	/// Checks for input-triggered interrupts.
	/// </summary>
	void UpdateInputState() override;

	/// <summary>
	/// Creates a controller device for the specified type.
	/// GBA has a single built-in controller.
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
	/// Reads KEYINPUT or KEYCNT register.
	/// </summary>
	/// <param name="addr">Register address ($4000130 or $4000132).</param>
	/// <returns>Register value.</returns>
	uint8_t ReadInputPort(uint32_t addr);

	/// <summary>
	/// Checks if input interrupt condition is currently met.
	/// Used for HALT exit condition checking.
	/// </summary>
	/// <returns>True if interrupt condition is satisfied.</returns>
	__noinline bool CheckInputCondition();

	/// <summary>
	/// Writes to KEYCNT register.
	/// Sets interrupt mask and condition.
	/// </summary>
	/// <param name="mode">Access mode (byte/half/word).</param>
	/// <param name="addr">Register address ($4000132).</param>
	/// <param name="value">Value to write.</param>
	void WriteInputPort(GbaAccessModeVal mode, uint32_t addr, uint8_t value);

	/// <summary>Serializes controller manager state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override;
};