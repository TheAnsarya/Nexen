#pragma once
#include "pch.h"
#include "Lynx/LynxTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;
class LynxConsole;

/// <summary>
/// Atari Lynx EEPROM — 93C46/93C66/93C86 Microwire serial protocol.
///
/// The EEPROM is accessed via three signals from Suzy hardware:
///   CS  (chip select) — directly, active high
///   CLK (clock)       — directly
///   DI  (data in)     — directly
///   DO  (data out)    — directly
///
/// Protocol:
///   1. CS goes high — starts a new command sequence
///   2. Start bit (1) clocked in
///   3. 2-bit opcode clocked in
///   4. Address bits clocked in (6/8/10 depending on chip)
///   5. Data bits clocked in/out (16 bits)
///   6. CS goes low — ends sequence
///
/// Opcodes (after start bit):
///   10 + addr = READ    — reads 16-bit word, output on DO
///   01 + addr = WRITE   — writes 16-bit word from DI
///   11 + addr = ERASE   — erases word (sets to 0xFFFF)
///   00 + extended:
///     00 + 00xxxx = EWDS  (write disable)
///     00 + 01xxxx = WRAL  (write all) + 16-bit data
///     00 + 10xxxx = ERAL  (erase all)
///     00 + 11xxxx = EWEN  (write enable)
/// </summary>
class LynxEeprom final : public ISerializable {
private:
	Emulator* _emu = nullptr;
	LynxConsole* _console = nullptr;

	LynxEepromSerialState _state = {};

	std::unique_ptr<uint8_t[]> _data; // 16-bit word array stored as bytes (LE)
	uint32_t _dataSize = 0;      // Size in bytes

	/// <summary>Get the number of address bits for the current chip type</summary>
	[[nodiscard]] uint8_t GetAddressBits() const;

	/// <summary>Get total word count (data size / 2)</summary>
	[[nodiscard]] uint16_t GetWordCount() const;

	/// <summary>Read a 16-bit word from EEPROM storage</summary>
	[[nodiscard]] uint16_t ReadWord(uint16_t wordAddr) const;

	/// <summary>Write a 16-bit word to EEPROM storage</summary>
	void WriteWord(uint16_t wordAddr, uint16_t value);

	/// <summary>Process completed command</summary>
	void ExecuteCommand();

public:
	LynxEeprom(Emulator* emu, LynxConsole* console);
	~LynxEeprom() = default;

	void Init(LynxEepromType type);

	/// <summary>Set chip select line</summary>
	void SetChipSelect(bool active);

	/// <summary>Clock a data bit in/out. Returns the DO (data out) state.</summary>
	[[nodiscard]] bool ClockData(bool dataIn);

	/// <summary>Get current data out pin state</summary>
	[[nodiscard]] bool GetDataOut() const { return _state.DataOut; }

	/// <summary>Get internal state for debugger</summary>
	[[nodiscard]] LynxEepromSerialState& GetState() { return _state; }

	/// <summary>Load battery-backed EEPROM data</summary>
	void LoadBattery();

	/// <summary>Save battery-backed EEPROM data</summary>
	void SaveBattery();

	void Serialize(Serializer& s) override;
};
