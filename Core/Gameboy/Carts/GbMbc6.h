#pragma once
#include "pch.h"
#include "Gameboy/Carts/GbCart.h"
#include "Gameboy/GbMemoryManager.h"
#include "Utilities/Serializer.h"

/// <summary>
/// MBC6 mapper with Flash ROM support.
/// Used exclusively by Net de Get: Minigame @ 100.
/// </summary>
/// <remarks>
/// **MBC6 Specifications:**
/// - ROM: Up to 2MB (split into 8KB banks)
/// - RAM: Up to 32KB (split into 4KB banks)
/// - Flash: 1Mbit (128KB) writable flash memory
/// - Game: Net de Get: Minigame @ 100 (Japan-only)
///
/// **Unique Features:**
/// - Two independent 8KB ROM bank slots ($4000/$6000)
/// - Two independent 4KB RAM bank slots ($A000/$B000)
/// - Flash ROM can be mapped in place of regular ROM
/// - Flash supports sector erase and byte programming
///
/// **Memory Map:**
/// - $0000-$3FFF: Fixed ROM bank 0
/// - $4000-$5FFF: Switchable ROM/Flash bank A (8KB)
/// - $6000-$7FFF: Switchable ROM/Flash bank B (8KB)
/// - $A000-$AFFF: RAM bank A (4KB)
/// - $B000-$BFFF: RAM bank B (4KB)
///
/// **Register Map:**
/// - $0000-$03FF: RAM enable ($0A = enable)
/// - $0400-$07FF: RAM bank A (3-bit)
/// - $0800-$0BFF: RAM bank B (3-bit)
/// - $0C00-$0FFF: Flash enable (bit 0, requires write enable)
/// - $1000-$13FF: Flash write enable (bit 0)
/// - $2000-$27FF: ROM bank A low (7-bit)
/// - $2800-$2FFF: Flash bank A enable ($08)
/// - $3000-$37FF: ROM bank B low (7-bit)
/// - $3800-$3FFF: Flash bank B enable ($08)
///
/// **Flash Programming:**
/// Flash write support not fully implemented (TODO).
/// Real hardware uses command sequences for erase/program.
/// </remarks>
class GbMbc6 : public GbCart {
private:
	/// <summary>RAM access enabled.</summary>
	bool _ramEnabled = false;

	/// <summary>Flash ROM enabled (global).</summary>
	bool _flashEnabled = false;

	/// <summary>Flash write operations enabled.</summary>
	bool _flashWriteEnabled = false;

	/// <summary>Flash bank enabled for each slot [A, B].</summary>
	bool _flashBankEnabled[2] = {};

	/// <summary>PRG bank numbers for slots [A, B].</summary>
	uint8_t _prgBanks[2] = {};

	/// <summary>RAM bank numbers for slots [A, B].</summary>
	uint8_t _ramBanks[2] = {};

public:
	/// <summary>
	/// Initializes MBC6 register mappings.
	/// </summary>
	void InitCart() override {
		_memoryManager->MapRegisters(0x0000, 0x3FFF, RegisterAccess::Write);
	}

	/// <summary>
	/// Updates memory mappings for dual ROM/Flash and RAM slots.
	/// </summary>
	void RefreshMappings() override {
		// Fixed bank 0 at $0000-$3FFF
		Map(0x0000, 0x3FFF, GbMemoryType::PrgRom, 0, true);

		// Bank A ($4000-$5FFF): Flash or ROM
		if (_flashBankEnabled[0]) {
			// Flash mode - route to register handler
			_memoryManager->MapRegisters(0x4000, 0x5FFF, RegisterAccess::ReadWrite);
		} else {
			// ROM mode - 8KB bank
			Map(0x4000, 0x5FFF, GbMemoryType::PrgRom, _prgBanks[0] * 0x2000, true);
			_memoryManager->MapRegisters(0x4000, 0x5FFF, RegisterAccess::None);
		}

		// Bank B ($6000-$7FFF): Flash or ROM
		if (_flashBankEnabled[1]) {
			_memoryManager->MapRegisters(0x6000, 0x7FFF, RegisterAccess::ReadWrite);
		} else {
			Map(0x6000, 0x7FFF, GbMemoryType::PrgRom, _prgBanks[1] * 0x2000, true);
			_memoryManager->MapRegisters(0x6000, 0x7FFF, RegisterAccess::None);
		}

		// RAM slots ($A000-$AFFF and $B000-$BFFF)
		if (_ramEnabled) {
			Map(0xA000, 0xAFFF, GbMemoryType::CartRam, _ramBanks[0] * 0x1000, false);
			Map(0xB000, 0xBFFF, GbMemoryType::CartRam, _ramBanks[1] * 0x1000, false);
		} else {
			Unmap(0xA000, 0xBFFF);
		}
	}

	/// <summary>
	/// Reads from Flash ROM.
	/// </summary>
	/// <param name="addr">Flash address.</param>
	/// <returns>Flash data or $FF.</returns>
	uint8_t ReadRegister(uint16_t addr) override {
		// TODO - Flash reads
		return 0xFF;
	}

	/// <summary>
	/// Writes to MBC6 registers or Flash ROM.
	/// </summary>
	/// <param name="addr">Register/Flash address.</param>
	/// <param name="value">Value to write.</param>
	void WriteRegister(uint16_t addr, uint8_t value) override {
		if (addr < 0x4000) {
			// MBC control registers
			switch (addr & 0x3C00) {
				case 0x0000:
					// RAM enable
					_ramEnabled = (value == 0x0A);
					break;
				case 0x0400:
					// RAM bank A (3-bit)
					_ramBanks[0] = value & 0x07;
					break;
				case 0x0800:
					// RAM bank B (3-bit)
					_ramBanks[1] = value & 0x07;
					break;

				case 0x0C00:
					// Flash enable (requires write enable first)
					if (_flashWriteEnabled) {
						_flashEnabled = (value & 0x01) != 0;
					}
					break;

				case 0x1000:
					// Flash write enable
					_flashWriteEnabled = (value & 0x01) != 0;
					break;

				case 0x2000:
				case 0x2400:
					// ROM bank A (7-bit)
					_prgBanks[0] = value & 0x7F;
					break;

				case 0x2800:
				case 0x2C00:
					// Flash bank A enable ($08 = flash, else ROM)
					_flashBankEnabled[0] = (value == 0x08);
					break;

				case 0x3000:
				case 0x3400:
					// ROM bank B (7-bit)
					_prgBanks[1] = value & 0x7F;
					break;

				case 0x3800:
				case 0x3C00:
					// Flash bank B enable
					_flashBankEnabled[1] = (value == 0x08);
					break;
			}
			RefreshMappings();
		} else {
			// TODO - Flash writes (command sequences for erase/program)
		}
	}

	/// <summary>Serializes MBC6 state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_ramEnabled);
		SV(_prgBanks[0]);
		SV(_prgBanks[1]);
		SV(_ramBanks[0]);
		SV(_ramBanks[1]);

		SV(_flashEnabled);
		SV(_flashWriteEnabled);
		SV(_flashBankEnabled[0]);
		SV(_flashBankEnabled[1]);
	}
};