#pragma once
#include "pch.h"
#include "Gameboy/Carts/GbCart.h"
#include "Gameboy/GbMemoryManager.h"
#include "Utilities/Serializer.h"

/// <summary>
/// Wisdom Tree mapper for unlicensed religious Game Boy games.
/// Simple 32KB banking with address-based bank selection.
/// </summary>
/// <remarks>
/// **Wisdom Tree Mapper Specifications:**
/// - ROM: Up to 512KB (16 × 32KB banks)
/// - RAM: None
/// - Games: King James Bible, Spiritual Warfare, etc.
///
/// **Unique Banking:**
/// - Entire $0000-$7FFF is a single 32KB switchable bank
/// - Bank selected by writing to $0000-$3FFF
/// - Bank number comes from address bits 0-3 (not data!)
///
/// **Bank Select:**
/// Write any value to address $0xxx where xxx encodes the bank.
/// Only bits 0-3 of the address are used: addr & $0F.
///
/// **Example:**
/// - Write to $0000: Select bank 0
/// - Write to $0001: Select bank 1
/// - Write to $000F: Select bank 15
///
/// This unusual design made the cartridges cheaper to produce
/// (fewer components needed) for the budget religious market.
/// </remarks>
class GbWisdomTree : public GbCart {
private:
	/// <summary>Current 32KB bank (4-bit, 0-15).</summary>
	uint8_t _prgBank = 0;

public:
	/// <summary>
	/// Initializes Wisdom Tree register mappings.
	/// </summary>
	void InitCart() override {
		_memoryManager->MapRegisters(0x0000, 0x3FFF, RegisterAccess::Write);
	}

	/// <summary>
	/// Updates memory mapping (32KB at a time).
	/// </summary>
	void RefreshMappings() override {
		// Map full 32KB bank to $0000-$7FFF
		Map(0x0000, 0x7FFF, GbMemoryType::PrgRom, _prgBank * 0x8000, true);
	}

	/// <summary>
	/// Writes to bank select register. Bank number is in address, not data.
	/// </summary>
	/// <param name="addr">Address determines bank (bits 0-3).</param>
	/// <param name="value">Ignored - bank comes from address.</param>
	void WriteRegister(uint16_t addr, uint8_t value) override {
		// Bank number encoded in address bits 0-3
		_prgBank = addr & 0x0F;
		RefreshMappings();
	}

	/// <summary>Serializes Wisdom Tree state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_prgBank);
	}
};