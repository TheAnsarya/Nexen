#pragma once
#include "pch.h"
#include "Gameboy/Carts/GbCart.h"
#include "Gameboy/GbMemoryManager.h"
#include "Utilities/Serializer.h"

/// <summary>
/// MMM01 (Multi-ROM MBC) mapper for multi-game cartridges.
/// Provides a two-stage boot process for game selection.
/// </summary>
/// <remarks>
/// **MMM01 Specifications:**
/// - ROM: Multiple games in one cartridge
/// - RAM: Up to 32KB shared between games
/// - Multi-cart: Game selection menu + individual games
/// - Games: Momotaro Collection 2, Taito Variety Pack
///
/// **Boot Process:**
/// 1. Initially unlocked: maps last 32KB of ROM (menu)
/// 2. First write to $0000-$1FFF locks mapper
/// 3. Once locked: operates like MBC1 within selected game
///
/// **Unlocked Mode (Menu):**
/// - $0000-$7FFF: Last 32KB of ROM (selection menu)
/// - $0000-$1FFF write: Locks mapper (transitions to locked)
/// - $2000-$3FFF write: Sets outer bank (game base offset)
///
/// **Locked Mode (Game Running):**
/// - $0000-$3FFF: Fixed bank at outer offset
/// - $4000-$7FFF: Switchable bank (outer + inner bank)
/// - $0000-$1FFF write: RAM enable ($0A)
/// - $2000-$3FFF write: Inner ROM bank
/// - $4000-$5FFF write: RAM bank
///
/// **Game Isolation:**
/// Each game operates within its allocated ROM region.
/// Outer bank sets the base, inner bank adds offset.
/// </remarks>
class GbMmm01 : public GbCart {
private:
	/// <summary>RAM access enabled (in locked mode).</summary>
	bool _ramEnabled = false;

	/// <summary>Inner PRG bank (within selected game).</summary>
	uint8_t _prgBank = 0;

	/// <summary>RAM bank select.</summary>
	uint8_t _ramBank = 0;

	/// <summary>Mapper locked (game selected, menu exited).</summary>
	bool _locked = false;

	/// <summary>Outer PRG bank (game base offset).</summary>
	uint8_t _outerPrgBank = 0;

public:
	/// <summary>
	/// Creates an MMM01 mapper instance.
	/// </summary>
	GbMmm01() {
	}

	/// <summary>
	/// Initializes MMM01 register mappings.
	/// </summary>
	void InitCart() override {
		_memoryManager->MapRegisters(0x0000, 0x7FFF, RegisterAccess::Write);
	}

	/// <summary>
	/// Updates memory mappings based on lock state.
	/// </summary>
	void RefreshMappings() override {
		constexpr int prgBankSize = 0x4000;  // 16KB banks
		constexpr int ramBankSize = 0x2000;  // 8KB RAM banks

		if (_locked) {
			// Locked mode: MBC1-like within selected game region
			Map(0x0000, 0x3FFF, GbMemoryType::PrgRom, _outerPrgBank * prgBankSize, true);
			Map(0x4000, 0x7FFF, GbMemoryType::PrgRom, (_outerPrgBank + _prgBank) * prgBankSize, true);
		} else {
			// Unlocked mode: map last 32KB (menu) for game selection
			Map(0x0000, 0x7FFF, GbMemoryType::PrgRom, _gameboy->DebugGetMemorySize(MemoryType::GbPrgRom) - 0x8000, true);
		}

		if (_ramEnabled) {
			Map(0xA000, 0xBFFF, GbMemoryType::CartRam, _ramBank * ramBankSize, false);
			_memoryManager->MapRegisters(0xA000, 0xBFFF, RegisterAccess::None);
		} else {
			Unmap(0xA000, 0xBFFF);
			_memoryManager->MapRegisters(0xA000, 0xBFFF, RegisterAccess::Read);
		}
	}

	/// <summary>
	/// Reads from disabled RAM (returns $FF).
	/// </summary>
	/// <param name="addr">Address being read.</param>
	/// <returns>$FF when RAM disabled.</returns>
	uint8_t ReadRegister(uint16_t addr) override {
		// Disabled RAM returns 0xFF on reads
		return 0xFF;
	}

	/// <summary>
	/// Writes to MMM01 registers. Behavior changes based on lock state.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteRegister(uint16_t addr, uint8_t value) override {
		switch (addr & 0x6000) {
			case 0x0000:
				if (_locked) {
					// Locked: RAM enable
					_ramEnabled = ((value & 0x0F) == 0x0A);
				} else {
					// Unlocked: any write locks the mapper
					_locked = true;
				}
				break;

			case 0x2000:
				if (_locked) {
					// Locked: inner ROM bank
					_prgBank = value;
				} else {
					// Unlocked: outer ROM bank (game selection)
					_outerPrgBank = value;
				}
				break;

			case 0x4000:
				if (_locked) {
					// Locked: RAM bank
					_ramBank = value;
				}
				// Unlocked: ignored
				break;

			case 0x6000:
				// Unknown/unused
				break;
		}

		RefreshMappings();
	}

	/// <summary>Serializes MMM01 state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_ramEnabled);
		SV(_prgBank);
		SV(_ramBank);
		SV(_outerPrgBank);
		SV(_locked);
	}
};