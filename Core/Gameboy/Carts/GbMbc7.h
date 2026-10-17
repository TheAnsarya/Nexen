#pragma once
#include "pch.h"
#include "Gameboy/Carts/GbCart.h"
#include "Gameboy/Carts/Eeprom93Lc56.h"
#include "Gameboy/Carts/GbMbc7Accelerometer.h"
#include "Gameboy/GbMemoryManager.h"
#include "Utilities/Serializer.h"

/// <summary>
/// MBC7 mapper with accelerometer and EEPROM - used for tilt-controlled games.
/// </summary>
/// <remarks>
/// **MBC7 Specifications:**
/// - ROM: Up to 2MB (128 banks × 16KB)
/// - Save: 256 bytes EEPROM (93LC56)
/// - Sensors: ADXL202E dual-axis accelerometer
/// - Game: Kirby Tilt 'n' Tumble
///
/// **Register Map:**
/// - $0000-$1FFF: RAM Enable 1 (write $0A)
/// - $2000-$3FFF: ROM Bank (7-bit)
/// - $4000-$5FFF: RAM Enable 2 (write $40)
///
/// **Memory-Mapped I/O ($A000-$AFFF):**
/// - $A000-$A01F: Accelerometer latch control
/// - $A020-$A05F: Accelerometer X/Y data (16-bit)
/// - $A060-$A07F: Always returns 0
/// - $A080-$A08F: EEPROM interface
/// - $B000-$BFFF: Returns $FF
///
/// **Accelerometer:**
/// - X axis: Tilt left/right (inverted in hardware)
/// - Y axis: Tilt forward/back
/// - 10-bit unsigned values, $200 = center
/// - Write $55 then $AA to $A000 to latch values
///
/// **EEPROM (93LC56):**
/// - 256 bytes organized as 128 × 16-bit words
/// - Serial interface: DO/DI/CLK/CS lines
/// - Commands: READ, WRITE, ERASE, EWEN, EWDS
///
/// **Dual RAM Enable:**
/// Both _ramEnabled ($0A) and _ramEnabled2 ($40) must be set
/// to access accelerometer and EEPROM registers.
/// </remarks>
class GbMbc7 : public GbCart {
private:
	/// <summary>Accelerometer sensor device.</summary>
	shared_ptr<GbMbc7Accelerometer> _accelerometer;

	/// <summary>93LC56 256-byte EEPROM.</summary>
	Eeprom93Lc56 _eeprom;

	/// <summary>First RAM enable flag ($0A at $0000-$1FFF).</summary>
	bool _ramEnabled = false;

	/// <summary>Second RAM enable flag ($40 at $4000-$5FFF).</summary>
	bool _ramEnabled2 = false;

	/// <summary>Current PRG bank (7-bit).</summary>
	uint16_t _prgBank = 1;

public:
	/// <summary>
	/// Initializes MBC7 with accelerometer and EEPROM.
	/// </summary>
	void InitCart() override {
		// Create accelerometer as a system control device
		_accelerometer = std::make_unique<GbMbc7Accelerometer>(_gameboy->GetEmulator());
		_gameboy->GetControlManager()->AddSystemControlDevice(_accelerometer);

		// Point EEPROM at cart RAM for storage
		_eeprom.SetRam(_cartRam);
		_memoryManager->MapRegisters(0x0000, 0x5FFF, RegisterAccess::Write);
	}

	/// <summary>
	/// Updates memory mappings. I/O only available when both RAM enables set.
	/// </summary>
	void RefreshMappings() override {
		Map(0x0000, 0x3FFF, GbMemoryType::PrgRom, 0, true);
		Map(0x4000, 0x7FFF, GbMemoryType::PrgRom, _prgBank * 0x4000, true);

		// Both RAM enable flags must be set for register access
		_memoryManager->MapRegisters(0xA000, 0xBFFF, _ramEnabled && _ramEnabled2 ? RegisterAccess::ReadWrite : RegisterAccess::None);
	}

	/// <summary>
	/// Reads from accelerometer or EEPROM registers.
	/// </summary>
	/// <param name="addr">Register address ($A000-$BFFF).</param>
	/// <returns>Sensor data, EEPROM data, or $FF.</returns>
	uint8_t ReadRegister(uint16_t addr) override {
		if (addr >= 0xB000) {
			// Upper range always returns $FF
			return 0xFF;
		}

		switch (addr & 0xF0) {
			case 0x20:
			case 0x30:
			case 0x40:
			case 0x50:
				// Accelerometer X/Y values (16-bit each)
				return _accelerometer->Read(addr);

			case 0x60:
				// Fixed zero return
				return 0;

			case 0x80:
				// EEPROM data output (DO line)
				return _eeprom.Read();

			default:
				return 0xFF;
		}
	}

	/// <summary>
	/// Writes to MBC7 registers, accelerometer, or EEPROM.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteRegister(uint16_t addr, uint8_t value) override {
		if (addr < 0x6000) {
			// MBC control registers
			switch (addr & 0x6000) {
				case 0x0000:
					// First RAM enable
					_ramEnabled = (value == 0x0A);
					break;
				case 0x2000:
					// ROM bank select (7-bit)
					_prgBank = value & 0x7F;
					break;
				case 0x4000:
					// Second RAM enable (special value $40)
					_ramEnabled2 = (value == 0x40);
					break;
			}
			RefreshMappings();
		} else {
			// Sensor/EEPROM I/O registers
			switch (addr & 0xF0) {
				case 0x00:
				case 0x10:
					// Accelerometer latch control ($55/$AA sequence)
					_accelerometer->Write(addr, value);
					break;

				case 0x80:
					// EEPROM control (DI/CLK/CS lines)
					_eeprom.Write(value);
					break;
			}
		}
	}

	/// <summary>Serializes MBC7 state including EEPROM and accelerometer.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_ramEnabled);
		SV(_ramEnabled2);
		SV(_prgBank);
		SV(_eeprom);
		SV(_accelerometer);
	}
};
