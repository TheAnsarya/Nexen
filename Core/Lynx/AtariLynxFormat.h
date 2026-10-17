#pragma once
#include "pch.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// .atari-lynx file format constants and structures.
///
/// The .atari-lynx format is a container for Atari Lynx ROM data with rich
/// metadata. It replaces the limited LNX format and metadata-less LYX/O formats.
/// See docs/ATARI-LYNX-FORMAT.md for the full specification.
///
/// File layout:  [32-byte header] [ROM data] [optional metadata section]
/// Magic:        "LYNXROM\0" (8 bytes)
/// Version:      0.1.0
/// </summary>
namespace AtariLynxFormatConstants {
	/// <summary>8-byte magic: "LYNXROM\0"</summary>
	constexpr uint8_t Magic[8] = { 'L', 'Y', 'N', 'X', 'R', 'O', 'M', '\0' };
	constexpr uint32_t MagicSize = 8;

	constexpr uint8_t VersionMajor = 0;
	constexpr uint8_t VersionMinor = 1;
	constexpr uint8_t VersionPatch = 0;

	constexpr uint32_t HeaderSize = 32;

	/// <summary>Metadata section magic: "META"</summary>
	constexpr uint8_t MetaMagic[4] = { 'M', 'E', 'T', 'A' };

	// Flag bits
	constexpr uint8_t FlagCompressed = 0x01;
	constexpr uint8_t FlagHasMetadata = 0x02;
}

/// <summary>
/// Fixed 32-byte header for .atari-lynx files.
/// All multi-byte fields are little-endian.
/// </summary>
struct AtariLynxHeader {
	uint8_t Magic[8];       // "LYNXROM\0"
	uint8_t VersionMajor;   // 0
	uint8_t VersionMinor;   // 1
	uint8_t VersionPatch;   // 0
	uint8_t Flags;          // Bit 0: compressed, Bit 1: has-metadata
	uint32_t RomSize;       // Raw ROM data size in bytes
	uint32_t RomCrc32;      // CRC32 of raw ROM data
	uint32_t TotalFileSize; // Total file size
	uint32_t MetadataOffset;// Byte offset of metadata section (0 if none)
	uint32_t Reserved;      // Must be 0
};
static_assert(sizeof(AtariLynxHeader) == 32, "AtariLynxHeader must be exactly 32 bytes");

/// <summary>
/// Metadata extracted from (or destined for) an .atari-lynx file.
/// Strings are stored as std::string (UTF-8).
/// </summary>
struct AtariLynxMetadata {
	std::string CartName;
	std::string Manufacturer;
	uint8_t Rotation = 0;        // 0=none, 1=left, 2=right
	uint16_t Bank0PageSize = 0;  // In 256-byte pages
	uint16_t Bank1PageSize = 0;
	uint8_t EepromType = 0;      // 0=none, 1-5 = 93Cxx
	uint8_t EepromFlags = 0;     // Bit 0: SD, Bit 1: 8-bit org
	uint8_t HardwareModel = 0;   // 0=auto, 1=Lynx I, 2=Lynx II
	uint16_t YearOfRelease = 0;  // 0=unknown
	uint8_t PlayerCount = 0;     // 0=unknown
	uint8_t Handedness = 0;      // 0=right, 1=left, 2=ambidextrous
	uint8_t ComLynxSupport = 0;  // 0=no, 1=yes
	uint16_t CountryCode = 0;    // ISO 3166-1 numeric, 0=unknown
	std::string Language;        // ISO 639-1, e.g., "en"
};

/// <summary>
/// Result of loading an .atari-lynx file.
/// Contains the raw ROM data and optional metadata.
/// </summary>
struct AtariLynxLoadResult {
	bool Success = false;
	std::string Error;

	std::vector<uint8_t> RomData;
	AtariLynxMetadata Metadata;
	uint32_t RomCrc32 = 0;

	/// <summary>Whether the file had a metadata section</summary>
	bool HasMetadata = false;
};

/// <summary>
/// Reader/writer for the .atari-lynx file format.
///
/// Handles loading and saving .atari-lynx container files.
/// Also provides conversion utilities from LNX/LYX/O formats.
/// </summary>
class AtariLynxFormat {
public:
	/// <summary>Check if a buffer starts with the .atari-lynx magic bytes.</summary>
	[[nodiscard]] static bool IsAtariLynxFormat(const uint8_t* data, size_t size);

	/// <summary>Check if a buffer starts with an LNX header ("LYNX" magic).</summary>
	[[nodiscard]] static bool IsLnxFormat(const uint8_t* data, size_t size);

	/// <summary>
	/// Load an .atari-lynx file from a byte buffer.
	/// Validates the header, extracts ROM data, and parses metadata if present.
	/// </summary>
	[[nodiscard]] static AtariLynxLoadResult Load(const uint8_t* data, size_t size);

	/// <summary>
	/// Save ROM data and metadata to .atari-lynx format.
	/// Returns the complete file as a byte vector.
	/// </summary>
	[[nodiscard]] static std::vector<uint8_t> Save(const uint8_t* romData, uint32_t romSize, const AtariLynxMetadata& metadata);

	/// <summary>
	/// Save ROM data without metadata (header + ROM only).
	/// </summary>
	[[nodiscard]] static std::vector<uint8_t> SaveHeaderOnly(const uint8_t* romData, uint32_t romSize);

	/// <summary>
	/// Convert an LNX file (with 64-byte header) to .atari-lynx format.
	/// Extracts metadata from the LNX header.
	/// </summary>
	[[nodiscard]] static AtariLynxLoadResult ConvertFromLnx(const uint8_t* data, size_t size);

	/// <summary>
	/// Convert a headerless ROM (LYX/O format) to .atari-lynx format.
	/// Metadata is inferred from ROM size and game database (if available).
	/// </summary>
	[[nodiscard]] static AtariLynxLoadResult ConvertFromRaw(const uint8_t* data, size_t size);

	/// <summary>
	/// Convert .atari-lynx back to LNX format (64-byte header + ROM data).
	/// Useful for compatibility with other emulators/tools.
	/// </summary>
	[[nodiscard]] static std::vector<uint8_t> ConvertToLnx(const uint8_t* romData, uint32_t romSize, const AtariLynxMetadata& metadata);

private:
	/// <summary>Serialize metadata section to bytes.</summary>
	static void WriteMetadata(std::vector<uint8_t>& output, const AtariLynxMetadata& metadata);

	/// <summary>Parse metadata section from bytes.</summary>
	static bool ReadMetadata(const uint8_t* data, size_t size, AtariLynxMetadata& metadata);

	/// <summary>Write a length-prefixed UTF-8 string.</summary>
	static void WriteString(std::vector<uint8_t>& output, const std::string& str);

	/// <summary>Read a length-prefixed UTF-8 string. Returns bytes consumed.</summary>
	static size_t ReadString(const uint8_t* data, size_t available, std::string& out);

	/// <summary>Write a little-endian uint16.</summary>
	static void WriteU16(std::vector<uint8_t>& output, uint16_t value);

	/// <summary>Write a little-endian uint32.</summary>
	static void WriteU32(std::vector<uint8_t>& output, uint32_t value);

	/// <summary>Read a little-endian uint16 from buffer.</summary>
	[[nodiscard]] static uint16_t ReadU16(const uint8_t* data);

	/// <summary>Read a little-endian uint32 from buffer.</summary>
	[[nodiscard]] static uint32_t ReadU32(const uint8_t* data);
};
