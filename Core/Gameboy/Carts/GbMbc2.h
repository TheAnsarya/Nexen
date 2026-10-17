#pragma once
#include "pch.h"
#include "Gameboy/Carts/GbCart.h"
#include "Gameboy/GbMemoryManager.h"
#include "Utilities/Serializer.h"

/// <summary>
/// MBC2 (Memory Bank Controller 2) with built-in 512×4-bit RAM.
/// Used in games like Final Fantasy Adventure.
/// </summary>
/// <remarks>
/// **MBC2 Specifications:**
/// - ROM: Up to 256KB (16 banks × 16KB)
/// - RAM: Built-in 512 × 4-bit (nibbles), mirrored in $A000-$BFFF
/// - Games: Final Fantasy Adventure, Kirby's Dream Land 2
///
/// **Unique Features:**
/// - RAM is 4-bit only (upper 4 bits read as $F / open bus)
/// - RAM is built into the MBC chip (not separate IC)
/// - 512 bytes mirrored 16 times across $A000-$BFFF
///
/// **Register Map:**
/// - $0000-$3FFF: Dual-purpose based on A8 bit
///   - A8=0: RAM Enable (write $0A to enable)
///   - A8=1: ROM Bank Select (4-bit, 1-15)
///
/// **RAM Behavior:**
/// - Only 4 bits usable per byte (values 0-15)
/// - Reads return $Fx (upper nibble from open bus)
/// - Writes only store lower nibble
/// </remarks>
class GbMbc2 : public GbCart {
private:
	/// <summary>RAM access enabled.</summary>
	bool _ramEnabled = false;

	/// <summary>Current PRG bank (4-bit, 1-15).</summary>
	uint8_t _prgBank = 1;

public:
	/// <summary>
	/// Initializes MBC2 and prepares 4-bit RAM.
	/// </summary>
	void InitCart() override {
		_memoryManager->MapRegisters(0x0000, 0x3FFF, RegisterAccess::Write);

		for (int i = 0; i < 512; i++) {
			// Ensure cart RAM contains $F in the upper nibble, no matter the contents of save ram
			// This simulates open bus on the unused upper 4 bits
			_cartRam[i] |= 0xF0;
		}
	}

	/// <summary>
	/// Updates memory mappings. RAM mirrors 512 bytes 16 times.
	/// </summary>
	void RefreshMappings() override {
		constexpr int prgBankSize = 0x4000;  // 16KB PRG banks

		// Bank 0 fixed at $0000-$3FFF
		Map(0x0000, 0x3FFF, GbMemoryType::PrgRom, 0, true);
		// Switchable bank at $4000-$7FFF
		Map(0x4000, 0x7FFF, GbMemoryType::PrgRom, _prgBank * prgBankSize, true);

		if (_ramEnabled) {
			// Mirror 512-byte RAM 16 times across $A000-$BFFF
			for (int i = 0; i < 16; i++) {
				Map(0xA000 + 0x200 * i, 0xA1FF + 0x200 * i, GbMemoryType::CartRam, 0, false);
			}
			// Use write handler to mask writes to 4 bits
			_memoryManager->MapRegisters(0xA000, 0xBFFF, RegisterAccess::Write);
		} else {
			// Disabled RAM returns $FF
			Unmap(0xA000, 0xBFFF);
			_memoryManager->MapRegisters(0xA000, 0xBFFF, RegisterAccess::Read);
		}
	}

	/// <summary>
	/// Reads from disabled RAM (returns $FF).
	/// </summary>
	/// <param name="addr">Address being read.</param>
	/// <returns>$FF when RAM is disabled.</returns>
	uint8_t ReadRegister(uint16_t addr) override {
		// Disabled RAM returns 0xFF on reads
		return 0xFF;
	}

	/// <summary>
	/// Writes to MBC2 registers or 4-bit RAM.
	/// </summary>
	/// <param name="addr">Register/RAM address.</param>
	/// <param name="value">Value to write.</param>
	/// <remarks>
	/// ROM area: A8=0 controls RAM enable, A8=1 controls ROM bank.
	/// RAM area: Only lower nibble stored, upper nibble set to $F.
	/// </remarks>
	void WriteRegister(uint16_t addr, uint8_t value) override {
		if (addr >= 0xA000 && addr <= 0xBFFF) {
			// Cart RAM write: only store lower 4 bits
			// Set top nibble to $F to mimic open bus behavior
			_cartRam[addr & 0x1FF] = (value & 0x0F) | 0xF0;
		} else {
			// Register write: A8 determines function
			switch (addr & 0x100) {
				case 0x000:
					// A8=0: RAM enable
					_ramEnabled = ((value & 0x0F) == 0x0A);
					break;
				case 0x100:
					// A8=1: ROM bank (0→1 like MBC1)
					_prgBank = std::max(1, value & 0x0F);
					break;
			}
			RefreshMappings();
		}
	}

	/// <summary>Serializes MBC2 state for save states.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_ramEnabled);
		SV(_prgBank);
	}
};