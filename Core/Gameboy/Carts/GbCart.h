#pragma once
#include "pch.h"
#include "Gameboy/Gameboy.h"
#include "Gameboy/GbMemoryManager.h"
#include "Shared/MessageManager.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/ISerializable.h"

/// <summary>
/// Base class for Game Boy cartridge implementations (MBC chips).
/// Provides common functionality for ROM/RAM banking and memory mapping.
/// </summary>
/// <remarks>
/// **Game Boy Cartridge Types:**
/// - **No MBC (ROM only)**: Up to 32KB ROM, no banking
/// - **MBC1**: Up to 2MB ROM + 32KB RAM, mode switching
/// - **MBC2**: Up to 256KB ROM + 512×4-bit RAM (built-in)
/// - **MBC3**: Up to 2MB ROM + 32KB RAM + RTC
/// - **MBC5**: Up to 8MB ROM + 128KB RAM, rumble support
/// - **MBC6**: Flash ROM support (Net de Get)
/// - **MBC7**: Accelerometer + EEPROM (Kirby Tilt 'n' Tumble)
/// - **MMM01**: Multi-cart support
/// - **HuC1**: IR communication
/// - **HuC3**: IR + RTC + speaker
/// - **TAMA5**: Complex mapper with RTC
/// - **Camera**: Built-in camera sensor
///
/// **Memory Map (standard):**
/// - $0000-$3FFF: ROM Bank 0 (fixed)
/// - $4000-$7FFF: ROM Bank 1-N (switchable)
/// - $A000-$BFFF: External RAM (if present)
///
/// **MBC Register Interface:**
/// Writes to ROM area ($0000-$7FFF) control the MBC:
/// - RAM enable/disable
/// - ROM bank selection
/// - RAM bank selection
/// - Mode selection (MBC1)
///
/// **Battery Saves:**
/// - Cart RAM can be battery-backed for saves
/// - RTC state also saved for MBC3/HuC3
/// </remarks>
class GbCart : public ISerializable {
protected:
	/// <summary>Game Boy console reference.</summary>
	Gameboy* _gameboy = nullptr;

	/// <summary>Memory manager for bank mapping.</summary>
	GbMemoryManager* _memoryManager = nullptr;

	/// <summary>Cartridge RAM buffer (external RAM).</summary>
	uint8_t* _cartRam = nullptr;

	/// <summary>
	/// Maps a memory region to a specific source.
	/// </summary>
	/// <param name="start">Start address (inclusive).</param>
	/// <param name="end">End address (inclusive).</param>
	/// <param name="type">Memory type to map.</param>
	/// <param name="offset">Offset within the memory type.</param>
	/// <param name="readonly">True if writes should be blocked.</param>
	void Map(uint16_t start, uint16_t end, GbMemoryType type, uint32_t offset, bool readonly) {
		_memoryManager->Map(start, end, type, offset, readonly);
	}

	/// <summary>
	/// Unmaps a memory region (reads return open bus).
	/// </summary>
	/// <param name="start">Start address (inclusive).</param>
	/// <param name="end">End address (inclusive).</param>
	void Unmap(uint16_t start, uint16_t end) {
		_memoryManager->Unmap(start, end);
	}

public:
	/// <summary>Virtual destructor for polymorphic deletion.</summary>
	virtual ~GbCart() {
	}

	/// <summary>
	/// Initializes the cartridge with console and memory manager references.
	/// </summary>
	/// <param name="gameboy">Game Boy console instance.</param>
	/// <param name="memoryManager">Memory manager for banking.</param>
	void Init(Gameboy* gameboy, GbMemoryManager* memoryManager) {
		_gameboy = gameboy;
		_memoryManager = memoryManager;
		_cartRam = gameboy->DebugGetMemory(MemoryType::GbCartRam);
	}

	/// <summary>
	/// Initializes cartridge-specific state.
	/// Called after ROM is loaded.
	/// </summary>
	virtual void InitCart() {
	}

	/// <summary>
	/// Saves battery-backed data to file.
	/// Includes cart RAM and RTC state.
	/// </summary>
	virtual void SaveBattery() {
	}

	/// <summary>
	/// Updates memory mappings based on current bank state.
	/// Default implementation maps 32KB ROM without banking.
	/// </summary>
	virtual void RefreshMappings() {
		Map(0x0000, 0x7FFF, GbMemoryType::PrgRom, 0, true);
	}

	/// <summary>
	/// Reads from cartridge registers.
	/// Used for special hardware (RTC, sensors).
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <returns>Register value.</returns>
	virtual uint8_t ReadRegister(uint16_t addr) {
		LogDebug("[Debug] GB - Missing read handler: $" + HexUtilities::ToHex(addr));
		return 0;
	}

	/// <summary>
	/// Writes to cartridge registers.
	/// Controls MBC banking and special hardware.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	virtual void WriteRegister(uint16_t addr, uint8_t value) {
	}

	/// <summary>Serializes cartridge state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override {
	}
};
