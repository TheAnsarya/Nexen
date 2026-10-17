#pragma once
#include "pch.h"
#include "Gameboy/Carts/GbCart.h"
#include "Gameboy/GbMemoryManager.h"
#include "Utilities/Serializer.h"

/// <summary>
/// HuC1 (Hudson Soft Controller 1) mapper with infrared communication.
/// Used in games with multiplayer via IR link.
/// </summary>
/// <remarks>
/// **HuC1 Specifications:**
/// - ROM: Up to 1MB (64 banks × 16KB)
/// - RAM: Up to 32KB (4 banks × 8KB)
/// - IR: Infrared transceiver for short-range communication
/// - Games: Pocket Family series, Robot Ponkottsu
///
/// **Register Map:**
/// - $0000-$1FFF: IR/RAM mode select ($0E = IR mode)
/// - $2000-$3FFF: ROM bank (6-bit)
/// - $4000-$5FFF: RAM bank (2-bit)
/// - $6000-$7FFF: Unused
/// - $A000-$BFFF: RAM access or IR register
///
/// **Infrared Communication:**
/// - Write $0E to $0000-$1FFF to enable IR mode
/// - $A000: IR status (read) / IR output (write)
/// - Read bit 0: 0 = no light detected, 1 = light detected
/// - Write bit 0: 0 = LED off, 1 = LED on
///
/// **IR Protocol:**
/// Games implement their own protocols using IR LED pulses.
/// Typically used for trading items or multiplayer minigames.
/// Link requires two Game Boys pointed at each other.
/// </remarks>
class GbHuc1 : public GbCart {
private:
	/// <summary>IR mode enabled (vs RAM mode).</summary>
	bool _irEnabled = false;

	/// <summary>IR LED output state.</summary>
	bool _irOutputEnabled = false;

	/// <summary>Current ROM bank (6-bit).</summary>
	uint8_t _prgBank = 1;

	/// <summary>Current RAM bank (2-bit).</summary>
	uint8_t _ramBank = 0;

public:
	/// <summary>
	/// Initializes HuC1 register mappings.
	/// </summary>
	void InitCart() override {
		_memoryManager->MapRegisters(0x0000, 0x7FFF, RegisterAccess::Write);
	}

	/// <summary>
	/// Updates memory mappings. IR mode unmaps RAM and enables register access.
	/// </summary>
	void RefreshMappings() override {
		// Fixed bank 0 at $0000-$3FFF
		Map(0x0000, 0x3FFF, GbMemoryType::PrgRom, 0, true);
		// Switchable ROM bank at $4000-$7FFF
		Map(0x4000, 0x7FFF, GbMemoryType::PrgRom, _prgBank * 0x4000, true);

		if (_irEnabled) {
			// IR mode: unmap RAM, enable IR register at $A000
			Unmap(0xA000, 0xBFFF);
			_memoryManager->MapRegisters(0xA000, 0xBFFF, RegisterAccess::ReadWrite);
		} else {
			// RAM mode: map cartridge RAM
			Map(0xA000, 0xBFFF, GbMemoryType::CartRam, _ramBank * 0x2000, false);
			_memoryManager->MapRegisters(0xA000, 0xBFFF, RegisterAccess::None);
		}
	}

	/// <summary>
	/// Reads IR sensor state (light detection).
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <returns>$C0 = no light, $C1 = light detected.</returns>
	uint8_t ReadRegister(uint16_t addr) override {
		// IR sensor: bit 0 indicates light detection
		// TODO: Implement actual IR link communication
		return 0xC0;  // No light detected
	}

	/// <summary>
	/// Writes to HuC1 control registers.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteRegister(uint16_t addr, uint8_t value) override {
		switch (addr & 0xE000) {
			case 0x0000:
				// $0E enables IR mode, other values select RAM mode
				_irEnabled = ((value & 0x0F) == 0x0E);
				break;
			case 0x2000:
				// ROM bank select (6-bit)
				_prgBank = value & 0x3F;
				break;
			case 0x4000:
				// RAM bank select (2-bit)
				_ramBank = value & 0x03;
				break;
			case 0x6000:
				// Unused
				break;
			case 0xA000:
				// IR LED output control
				_irOutputEnabled = (value & 0x01) != 0;
				break;
		}
		RefreshMappings();
	}

	/// <summary>Serializes HuC1 state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_irEnabled);
		SV(_irOutputEnabled);
		SV(_prgBank);
		SV(_ramBank);
	}
};