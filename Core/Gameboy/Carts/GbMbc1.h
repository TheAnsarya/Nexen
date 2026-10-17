#pragma once
#include "pch.h"
#include "Gameboy/Carts/GbCart.h"
#include "Gameboy/GbMemoryManager.h"
#include "Utilities/Serializer.h"

/// <summary>
/// MBC1 (Memory Bank Controller 1) - The most common Game Boy mapper.
/// Supports up to 2MB ROM and 32KB RAM with two banking modes.
/// </summary>
/// <remarks>
/// **MBC1 Specifications:**
/// - ROM: Up to 2MB (128 banks × 16KB)
/// - RAM: Up to 32KB (4 banks × 8KB)
/// - Games: Zelda, Metroid II, Final Fantasy Legend, etc.
///
/// **Register Map:**
/// - $0000-$1FFF: RAM Enable (write $0A to enable)
/// - $2000-$3FFF: ROM Bank Number (bits 0-4)
/// - $4000-$5FFF: RAM Bank / Upper ROM bits
/// - $6000-$7FFF: Banking Mode Select
///
/// **Banking Modes:**
/// - Mode 0 (Simple): ROM banking at $4000, RAM bank 0 only
/// - Mode 1 (Advanced): $0000 affected by RAM bank, full RAM banking
///
/// **MBC1M Variant:**
/// Multi-cart variant with 4-bit lower bank (Momotaro Collection, etc.)
/// Uses bit 4 shift instead of bit 5 for upper bits.
///
/// **Bank 0 Quirk:**
/// Writing $00 to ROM bank register selects bank 1, not bank 0.
/// Games can access bank 0 through $0000-$3FFF region.
/// </remarks>
class GbMbc1 : public GbCart {
private:
	/// <summary>RAM access enabled ($0A written to $0000-$1FFF).</summary>
	bool _ramEnabled = false;

	/// <summary>Current PRG bank number (5-bit, 1-31).</summary>
	uint8_t _prgBank = 1;

	/// <summary>RAM bank / upper ROM bits (2-bit).</summary>
	uint8_t _ramBank = 0;

	/// <summary>Banking mode: false=simple (mode 0), true=advanced (mode 1).</summary>
	bool _mode = false;

	/// <summary>MBC1M multi-cart variant flag.</summary>
	bool _isMbc1m = false;

public:
	/// <summary>
	/// Creates an MBC1 mapper instance.
	/// </summary>
	/// <param name="isMbc1m">True for MBC1M multi-cart variant.</param>
	GbMbc1(bool isMbc1m) {
		_isMbc1m = isMbc1m;
	}

	/// <summary>
	/// Initializes MBC1 by enabling write-only register mapping in ROM area.
	/// </summary>
	void InitCart() override {
		_memoryManager->MapRegisters(0x0000, 0x7FFF, RegisterAccess::Write);
	}

	/// <summary>
	/// Updates memory mappings based on current bank registers and mode.
	/// </summary>
	/// <remarks>
	/// Mode 0: Fixed bank 0 at $0000-$3FFF, switchable at $4000-$7FFF
	/// Mode 1: Upper bits affect $0000 bank, full RAM banking
	/// </remarks>
	void RefreshMappings() override {
		constexpr int prgBankSize = 0x4000;  // 16KB PRG banks
		constexpr int ramBankSize = 0x2000;  // 8KB RAM banks

		// MBC1M uses 4-bit shift (multi-cart), standard uses 5-bit
		int shift = _isMbc1m ? 4 : 5;
		int mask = _isMbc1m ? 0x0F : 0x1F;

		// Mode 1: $0000-$3FFF uses upper bits; Mode 0: always bank 0
		Map(0x0000, 0x3FFF, GbMemoryType::PrgRom, (_mode ? (_ramBank << shift) : 0) * prgBankSize, true);

		// Combine lower bank bits with upper bits for $4000-$7FFF
		uint8_t prgBank = (_prgBank & mask) | (_ramBank << shift);
		Map(0x4000, 0x7FFF, GbMemoryType::PrgRom, prgBank * prgBankSize, true);

		if (_ramEnabled) {
			// Mode 1: full RAM banking; Mode 0: bank 0 only
			uint8_t ramBank = _mode ? _ramBank : 0;
			Map(0xA000, 0xBFFF, GbMemoryType::CartRam, ramBank * ramBankSize, false);
			_memoryManager->MapRegisters(0xA000, 0xBFFF, RegisterAccess::None);
		} else {
			// Disabled RAM returns $FF through read handler
			Unmap(0xA000, 0xBFFF);
			_memoryManager->MapRegisters(0xA000, 0xBFFF, RegisterAccess::Read);
		}
	}

	/// <summary>
	/// Reads from disabled RAM region (returns $FF).
	/// </summary>
	/// <param name="addr">Address being read.</param>
	/// <returns>$FF when RAM is disabled.</returns>
	uint8_t ReadRegister(uint16_t addr) override {
		// Disabled RAM returns 0xFF on reads
		return 0xFF;
	}

	/// <summary>
	/// Writes to MBC1 control registers.
	/// </summary>
	/// <param name="addr">Register address (decoded via bits 13-14).</param>
	/// <param name="value">Value to write.</param>
	/// <remarks>
	/// $0000-$1FFF: RAM enable (value $0A enables)
	/// $2000-$3FFF: ROM bank (forces 0→1)
	/// $4000-$5FFF: RAM bank / upper ROM bits
	/// $6000-$7FFF: Mode select
	/// </remarks>
	void WriteRegister(uint16_t addr, uint8_t value) override {
		switch (addr & 0x6000) {
			case 0x0000:
				// RAM enable: writing $0A enables, anything else disables
				_ramEnabled = ((value & 0x0F) == 0x0A);
				break;
			case 0x2000:
				// ROM bank: bank 0 maps to bank 1 (hardware quirk)
				_prgBank = std::max(1, value & 0x1F);
				break;
			case 0x4000:
				// Upper bits: RAM bank in mode 1, upper ROM bits in mode 0
				_ramBank = value & 0x03;
				break;
			case 0x6000:
				// Mode: 0 = simple banking, 1 = advanced banking
				_mode = (value & 0x01) != 0;
				break;
		}
		RefreshMappings();
	}

	/// <summary>Serializes MBC1 state for save states.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_ramEnabled);
		SV(_prgBank);
		SV(_ramBank);
		SV(_mode);
	}
};