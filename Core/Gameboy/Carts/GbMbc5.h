#pragma once
#include "pch.h"
#include "Gameboy/Carts/GbCart.h"
#include "Gameboy/GbMemoryManager.h"
#include "Utilities/Serializer.h"
#include "Shared/KeyManager.h"

/// <summary>
/// MBC5 (Memory Bank Controller 5) - Most advanced standard Game Boy mapper.
/// Supports up to 8MB ROM, 128KB RAM, and optional rumble motor.
/// </summary>
/// <remarks>
/// **MBC5 Specifications:**
/// - ROM: Up to 8MB (512 banks × 16KB) via 9-bit bank register
/// - RAM: Up to 128KB (16 banks × 8KB)
/// - Rumble: Optional vibration motor control
/// - Games: Pokémon Yellow/R/S/E, Zelda DX, most GBC games
///
/// **Register Map:**
/// - $0000-$1FFF: RAM Enable (write $0A to enable)
/// - $2000-$2FFF: ROM Bank Low (bits 0-7)
/// - $3000-$3FFF: ROM Bank High (bit 8)
/// - $4000-$5FFF: RAM Bank (+ Rumble control)
///
/// **Improvements over MBC1:**
/// - No bank 0 quirk (writing 0 gives bank 0)
/// - 9-bit ROM bank (512 banks vs 127)
/// - 4-bit RAM bank (16 banks vs 4)
/// - Simpler banking model (no modes)
///
/// **Rumble Variant:**
/// - RAM bank bits 0-2: RAM bank select (0-7)
/// - RAM bank bit 3: Rumble motor on/off
/// - Reduces RAM to 64KB (8 banks)
/// </remarks>
class GbMbc5 : public GbCart {
private:
	/// <summary>Cartridge has rumble motor.</summary>
	bool _hasRumble = false;

	/// <summary>RAM access enabled.</summary>
	bool _ramEnabled = false;

	/// <summary>Current PRG bank (9-bit, 0-511).</summary>
	uint16_t _prgBank = 1;

	/// <summary>Current RAM bank (4-bit, 0-15).</summary>
	uint8_t _ramBank = 0;

public:
	/// <summary>
	/// Creates an MBC5 mapper instance.
	/// </summary>
	/// <param name="hasRumble">True if cartridge has rumble motor.</param>
	GbMbc5(bool hasRumble) {
		_hasRumble = hasRumble;
	}

	/// <summary>
	/// Initializes MBC5 register mappings.
	/// </summary>
	void InitCart() override {
		_memoryManager->MapRegisters(0x0000, 0x5FFF, RegisterAccess::Write);
	}

	/// <summary>
	/// Updates memory mappings based on current bank registers.
	/// </summary>
	void RefreshMappings() override {
		constexpr int prgBankSize = 0x4000;  // 16KB PRG banks
		constexpr int ramBankSize = 0x2000;  // 8KB RAM banks

		// Bank 0 always at $0000-$3FFF
		Map(0x0000, 0x3FFF, GbMemoryType::PrgRom, 0, true);
		// Switchable bank at $4000-$7FFF (9-bit bank number)
		Map(0x4000, 0x7FFF, GbMemoryType::PrgRom, _prgBank * prgBankSize, true);

		if (_ramEnabled) {
			Map(0xA000, 0xBFFF, GbMemoryType::CartRam, _ramBank * ramBankSize, false);
			_memoryManager->MapRegisters(0xA000, 0xBFFF, RegisterAccess::None);
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
	/// Writes to MBC5 control registers.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteRegister(uint16_t addr, uint8_t value) override {
		switch (addr & 0x7000) {
			case 0x0000:
			case 0x1000:
				// RAM enable: exact $0A required (stricter than MBC1)
				_ramEnabled = (value == 0x0A);
				break;

			case 0x2000:
				// ROM bank low byte (bits 0-7)
				_prgBank = (value & 0xFF) | (_prgBank & 0x100);
				break;

			case 0x3000:
				// ROM bank high bit (bit 8)
				_prgBank = (_prgBank & 0xFF) | ((value & 0x01) << 8);
				break;

			case 0x4000:
			case 0x5000:
				if (_hasRumble) {
					// Rumble variant: bits 0-2 = RAM bank, bit 3 = rumble
					_ramBank = value & 0x07;
					KeyManager::SetForceFeedback((value & 0x08) ? 0xFFFF : 0);
				} else {
					// Standard: 4-bit RAM bank (0-15)
					_ramBank = value & 0x0F;
				}
				break;
		}
		RefreshMappings();
	}

	/// <summary>Serializes MBC5 state for save states.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_ramEnabled);
		SV(_prgBank);
		SV(_ramBank);
	}
};