#pragma once
#include "pch.h"

/// <summary>
/// Game Boy Color compatibility mode indicators.
/// </summary>
/// <remarks>
/// Located at cartridge header offset $143 (CGB Flag).
/// Determines whether game runs in DMG, CGB, or both modes.
/// </remarks>
enum class CgbCompat : uint8_t {
	/// <summary>DMG only (original Game Boy).</summary>
	Gameboy = 0x00,

	/// <summary>CGB-enhanced, also runs on DMG.</summary>
	GameboyColorSupport = 0x80,

	/// <summary>CGB-only, won't run on DMG.</summary>
	GameboyColorExclusive = 0xC0,
};

/// <summary>
/// Game Boy ROM cartridge header structure.
/// </summary>
/// <remarks>
/// **Header Location:** $0100-$014F in ROM
/// - $0100-$0103: Entry point (usually NOP + JP $0150)
/// - $0104-$0133: Nintendo logo (must match for boot ROM)
/// - $0134-$0143: Title (ASCII, up to 16 bytes, shorter on CGB)
/// - $0144-$0145: New licensee code (if $014B == $33)
/// - $0146: SGB flag ($03 = SGB functions supported)
/// - $0147: Cartridge type (MBC/features)
/// - $0148: ROM size (code: 32KB << code)
/// - $0149: RAM size (code → size lookup)
/// - $014A: Destination (0=Japan, 1=International)
/// - $014B: Old licensee code ($33 = use new code)
/// - $014C: Mask ROM version (usually $00)
/// - $014D: Header checksum (validated by boot ROM)
/// - $014E-$014F: Global checksum (not validated)
///
/// **Cartridge Types ($0147):**
/// - $00: ROM only
/// - $01-$03: MBC1
/// - $05-$06: MBC2
/// - $0F-$13: MBC3
/// - $19-$1E: MBC5
/// - $20: MBC6
/// - $22: MBC7
/// - $FC: Pocket Camera
/// - $FD: Bandai TAMA5
/// - $FE: HuC3
/// - $FF: HuC1
/// </remarks>
struct GameboyHeader {
	/// <summary>Game title (ASCII, up to 11 chars on CGB).</summary>
	/// <remarks>Starts at header offset $0134.</remarks>
	char Title[11];

	/// <summary>Manufacturer code (4 chars, CGB only).</summary>
	char ManufacturerCode[4];

	/// <summary>CGB compatibility flag ($143).</summary>
	CgbCompat CgbFlag;

	/// <summary>New licensee code (ASCII, 2 chars).</summary>
	char LicenseeCode[2];

	/// <summary>SGB flag ($03 = SGB supported).</summary>
	uint8_t SgbFlag;

	/// <summary>Cartridge type/MBC code ($147).</summary>
	uint8_t CartType;

	/// <summary>ROM size code (32KB << value).</summary>
	uint8_t PrgRomSize;

	/// <summary>Cart RAM size code.</summary>
	uint8_t CartRamSize;

	/// <summary>Destination code (0=Japan, 1=International).</summary>
	uint8_t DestCode;

	/// <summary>Old licensee code ($33 = use new code).</summary>
	uint8_t OldLicenseeCode;

	/// <summary>Mask ROM version number.</summary>
	uint8_t MaskRomVersion;

	/// <summary>Header checksum (validated by boot ROM).</summary>
	uint8_t HeaderChecksum;

	/// <summary>Global checksum (big-endian, not validated).</summary>
	uint8_t GlobalChecksum[2];

	/// <summary>
	/// Gets actual cartridge RAM size in bytes.
	/// </summary>
	/// <returns>RAM size in bytes (0 if none).</returns>
	/// <remarks>
	/// Special cases:
	/// - MBC2 ($05/$06): Built-in 512×4-bit RAM (returns $200)
	/// - MBC7 ($22): 256-byte EEPROM (returns $100)
	/// </remarks>
	[[nodiscard]] uint32_t GetCartRamSize() {
		if (CartType == 5 || CartType == 6) {
			// MBC2 has 512x4bits of cart ram
			return 0x200;
		}

		if (CartType == 0x22) {
			// MBC7 has a 256-byte eeprom
			return 0x100;
		}

		// Standard RAM size lookup table
		switch (CartRamSize) {
			case 0: return 0;        // No RAM
			case 1: return 0x800;    // 2 KB (unused in practice)
			case 2: return 0x2000;   // 8 KB (1 bank)
			case 3: return 0x8000;   // 32 KB (4 banks)
			case 4: return 0x20000;  // 128 KB (16 banks)
			case 5: return 0x10000;  // 64 KB (8 banks)
		}
		return 0;
	}

	/// <summary>
	/// Checks if cartridge has battery-backed save RAM.
	/// </summary>
	/// <returns>true if battery present.</returns>
	[[nodiscard]] bool HasBattery() {
		switch (CartType) {
			case 0x03:  // MBC1+RAM+BATTERY
			case 0x06:  // MBC2+BATTERY
			case 0x09:  // ROM+RAM+BATTERY
			case 0x0D:  // MMM01+RAM+BATTERY
			case 0x0F:  // MBC3+TIMER+BATTERY
			case 0x10:  // MBC3+TIMER+RAM+BATTERY
			case 0x13:  // MBC3+RAM+BATTERY
			case 0x1B:  // MBC5+RAM+BATTERY
			case 0x1E:  // MBC5+RUMBLE+RAM+BATTERY
			case 0x22:  // MBC7+SENSOR+RUMBLE+RAM+BATTERY
			case 0xFF:  // HuC1+RAM+BATTERY
				return true;
		}

		return false;
	}

	/// <summary>
	/// Gets the game title as a trimmed string.
	/// </summary>
	/// <returns>Title without trailing spaces/nulls.</returns>
	string GetCartName() {
		// Find null terminator or use full length
		int nameLength = 11;
		for (int i = 0; i < 11; i++) {
			if (Title[i] == 0) {
				nameLength = i;
				break;
			}
		}
		string name = string(Title, nameLength);

		// Remove trailing spaces
		size_t lastNonSpace = name.find_last_not_of(' ');
		if (lastNonSpace != string::npos) {
			return name.substr(0, lastNonSpace + 1);
		} else {
			return name;
		}
	}
};
