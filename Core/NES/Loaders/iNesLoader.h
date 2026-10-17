#pragma once
#include "pch.h"
#include "NES/Loaders/BaseLoader.h"

struct RomData;
struct NesHeader;

/// <summary>
/// iNES/NES 2.0 ROM format loader.
/// Parses the iNES header format used by most NES ROM distributions.
/// </summary>
/// <remarks>
/// **iNES Header Format (16 bytes):**
/// - Bytes 0-3: "NES\x1A" magic number
/// - Byte 4: PRG ROM size in 16KB units
/// - Byte 5: CHR ROM size in 8KB units (0 = CHR RAM)
/// - Byte 6: Flags 6 (mapper low nibble, mirroring, battery, trainer)
/// - Byte 7: Flags 7 (mapper high nibble, NES 2.0 identifier)
/// - Bytes 8-15: Extended header (varies by format)
///
/// **NES 2.0 Extensions (Byte 7 bits 2-3 = %10):**
/// - Extended mapper number (12 bits)
/// - Submapper number (4 bits)
/// - Extended PRG/CHR ROM sizes
/// - PRG/CHR RAM sizes (non-volatile and volatile)
/// - TV system and special features
/// - Default expansion device
///
/// **Mirroring Types:**
/// - Horizontal (CIRAM A10 = PPU A11)
/// - Vertical (CIRAM A10 = PPU A10)
/// - Four-screen (cart provides 4KB nametable RAM)
/// - Single-screen (fixed to one nametable)
/// - Mapper-controlled (complex patterns)
///
/// **Database Integration:**
/// - Optionally validates against NES ROM database
/// - Can override header with database values for accuracy
/// - Handles incorrect/broken headers in older dumps
/// </remarks>
class iNesLoader : public BaseLoader {
public:
	using BaseLoader::BaseLoader;

	/// <summary>
	/// Loads and parses an iNES/NES 2.0 ROM file.
	/// </summary>
	/// <param name="romData">Output structure for parsed ROM data.</param>
	/// <param name="romFile">Raw ROM file bytes including header.</param>
	/// <param name="preloadedHeader">Optional pre-parsed header (for embedded ROMs).</param>
	/// <param name="databaseEnabled">Whether to validate/override with ROM database.</param>
	void LoadRom(RomData& romData, vector<uint8_t>& romFile, NesHeader* preloadedHeader, bool databaseEnabled);
};