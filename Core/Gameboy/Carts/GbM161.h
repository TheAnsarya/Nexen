#pragma once
#include "pch.h"
#include "Gameboy/Carts/GbCart.h"
#include "Gameboy/GbMemoryManager.h"
#include "Utilities/Serializer.h"

/// <summary>
/// M161 mapper for multi-game cartridges with one-time bank selection.
/// Used in Mani 4-in-1 compilation cartridge.
/// </summary>
/// <remarks>
/// **M161 Mapper Specifications:**
/// - ROM: Multiple 32KB games
/// - RAM: None
/// - Multi-cart: One-time game selection at startup
///
/// **Unique Feature - One-Time Write:**
/// - First write to $0000-$7FFF selects the game bank
/// - After first write, register is disabled permanently
/// - Only a reset allows selecting a different game
///
/// **Bank Select:**
/// Write game number (0-15) to any ROM address.
/// The entire 32KB ($0000-$7FFF) switches to selected game.
///
/// **Boot Process:**
/// 1. Cartridge powers up with bank 0 (menu/first game)
/// 2. Menu writes desired game number
/// 3. Mapper locks - no further bank changes possible
/// 4. Selected game runs in its own 32KB space
///
/// This design prevents games from interfering with each other
/// since bank selection is permanently locked after first write.
/// </remarks>
class GbM161 : public GbCart {
private:
	/// <summary>Current 32KB bank (4-bit, set once).</summary>
	uint8_t _prgBank = 0;

public:
	/// <summary>
	/// Initializes M161 register mappings.
	/// </summary>
	void InitCart() override {
		_memoryManager->MapRegisters(0x0000, 0x7FFF, RegisterAccess::Write);
	}

	/// <summary>
	/// Updates memory mapping (32KB at a time).
	/// </summary>
	void RefreshMappings() override {
		Map(0x0000, 0x7FFF, GbMemoryType::PrgRom, _prgBank * 0x8000, true);
	}

	/// <summary>
	/// Writes to bank select register. Disables itself after first write.
	/// </summary>
	/// <param name="addr">Register address (any ROM address).</param>
	/// <param name="value">Bank number (bits 0-3).</param>
	void WriteRegister(uint16_t addr, uint8_t value) override {
		// Select 32KB bank
		_prgBank = value & 0x0F;
		RefreshMappings();

		// Block all subsequent writes - one-time selection only
		_memoryManager->MapRegisters(0x0000, 0x7FFF, RegisterAccess::None);
	}

	/// <summary>Serializes M161 state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_prgBank);
	}
};