#pragma once
#include "pch.h"
#include "Utilities/StringUtilities.h"

/// <summary>
/// Reads a 32-bit big-endian value from a byte array.
/// </summary>
/// <param name="value">4-byte array in big-endian order.</param>
/// <returns>32-bit value in native endianness.</returns>
uint32_t ReadBigEndian(uint8_t value[4]) {
	return value[3] | (value[2] << 8) | (value[1] << 16) | (value[0] << 24);
}

/// <summary>
/// GBX cartridge hardware description data.
/// </summary>
/// <remarks>
/// Describes the cartridge's mapper type and capabilities.
/// All multi-byte values are big-endian.
/// </remarks>
struct GbxCartridgeData {
	/// <summary>4-character mapper ID (e.g., "MBC1", "MBC5").</summary>
	char MapperId[4];

	/// <summary>Cartridge has battery-backed save (0/1).</summary>
	uint8_t HasBattery;

	/// <summary>Cartridge has rumble motor (0/1).</summary>
	uint8_t HasRumble;

	/// <summary>Cartridge has real-time clock (0/1).</summary>
	uint8_t HasRtc;

	/// <summary>Reserved for future use.</summary>
	uint8_t Unused;

	/// <summary>ROM size in bytes (big-endian).</summary>
	uint8_t RomSize[4];

	/// <summary>RAM size in bytes (big-endian).</summary>
	uint8_t RamSize[4];

	/// <summary>Mapper-specific configuration (8 × 32-bit values).</summary>
	uint8_t MapperVariables[8][4];
};

/// <summary>
/// GBX footer metadata section.
/// </summary>
/// <remarks>
/// Located at the very end of a GBX file.
/// Contains format version and footer size for parsing.
/// </remarks>
struct GbxMetadata {
	/// <summary>Total footer size in bytes (big-endian).</summary>
	uint8_t FooterSize[4];

	/// <summary>GBX format major version (big-endian).</summary>
	uint8_t MajorVersion[4];

	/// <summary>GBX format minor version (big-endian).</summary>
	uint8_t MinorVersion[4];

	/// <summary>Format signature ("GBX!" = valid).</summary>
	char Signature[4];

	/// <summary>
	/// Validates the GBX metadata.
	/// </summary>
	/// <returns>true if signature, version, and size are valid.</returns>
	bool IsValid() {
		return string(Signature, 4) == "GBX!" && ReadBigEndian(MajorVersion) == 1 && ReadBigEndian(FooterSize) == 0x40;
	}
};

/// <summary>
/// GBX (Game Boy Extended) ROM footer parser.
/// </summary>
/// <remarks>
/// **GBX Format Overview:**
/// GBX is an extended ROM format that appends metadata to standard
/// Game Boy ROMs. It provides explicit mapper information, eliminating
/// guesswork based on header codes.
///
/// **Why GBX?**
/// - Standard GB headers can be ambiguous (multiple mappers per code)
/// - Some homebrew/repros have incorrect or missing headers
/// - GBX explicitly states ROM/RAM sizes and mapper features
///
/// **File Structure:**
/// ```
/// [Standard ROM data] + [GBX Cartridge Data] + [GBX Metadata]
///                       └─────────────────────────────────────┘
///                                   Footer (64 bytes)
/// ```
///
/// **Mapper IDs:**
/// - "ROM ": No mapper (ROM only)
/// - "MBC1": MBC1
/// - "MBC2": MBC2
/// - "MBC3": MBC3
/// - "MBC5": MBC5
/// - "MBC6": MBC6
/// - "MBC7": MBC7
/// - "MMM1": MMM01 (multi-cart)
/// - "HUC1": Hudson HuC1
/// - "HUC3": Hudson HuC3
/// - "TAMA": Bandai TAMA5
/// </remarks>
struct GbxFooter {
private:
	/// <summary>Cartridge hardware description.</summary>
	GbxCartridgeData _cartridge;

	/// <summary>Footer metadata (at very end of file).</summary>
	GbxMetadata _metadata;

public:
	/// <summary>
	/// Initializes footer from ROM data and strips it.
	/// </summary>
	/// <param name="romData">ROM data (modified: footer removed if valid).</param>
	void Init(vector<uint8_t>& romData) {
		// Read metadata from end of file
		GbxMetadata meta = {};
		memcpy(&meta, romData.data() + romData.size() - sizeof(GbxMetadata), sizeof(GbxMetadata));

		if (meta.IsValid()) {
			// Read full footer and strip from ROM data
			memcpy(this, romData.data() + romData.size() - sizeof(GbxFooter), sizeof(GbxFooter));
			romData.resize(romData.size() - ReadBigEndian(_metadata.FooterSize));
		}
	}

	/// <summary>Checks if footer was successfully parsed.</summary>
	/// <returns>true if valid GBX footer found.</returns>
	bool IsValid() {
		return _metadata.IsValid();
	}

	/// <summary>Gets the mapper ID string.</summary>
	/// <returns>4-character mapper ID (e.g., "MBC5").</returns>
	[[nodiscard]] string GetMapperId() {
		return StringUtilities::GetString(_cartridge.MapperId, 4);
	}

	/// <summary>Gets ROM size from footer.</summary>
	/// <returns>ROM size in bytes.</returns>
	[[nodiscard]] uint32_t GetRomSize() {
		return ReadBigEndian(_cartridge.RomSize);
	}

	/// <summary>Gets RAM size from footer.</summary>
	/// <returns>RAM size in bytes (0 if no RAM).</returns>
	[[nodiscard]] uint32_t GetRamSize() {
		return ReadBigEndian(_cartridge.RamSize);
	}

	/// <summary>Checks if cartridge has battery backup.</summary>
	/// <returns>true if battery present.</returns>
	[[nodiscard]] bool HasBattery() {
		return (bool)_cartridge.HasBattery;
	}

	/// <summary>Checks if cartridge has RTC.</summary>
	/// <returns>true if real-time clock present.</returns>
	[[nodiscard]] bool HasRtc() {
		return (bool)_cartridge.HasRtc;
	}

	/// <summary>Checks if cartridge has rumble motor.</summary>
	/// <returns>true if rumble present.</returns>
	[[nodiscard]] bool HasRumble() {
		return (bool)_cartridge.HasRumble;
	}
};
