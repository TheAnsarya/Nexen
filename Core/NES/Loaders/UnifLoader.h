#pragma once
#include "pch.h"
#include <unordered_map>
#include "NES/Loaders/BaseLoader.h"

struct RomData;

/// <summary>
/// UNIF (Universal NES Image Format) ROM loader.
/// Parses the chunk-based UNIF format used for complex mapper documentation.
/// </summary>
/// <remarks>
/// **UNIF Format Overview:**
/// UNIF was designed as an alternative to iNES for better mapper documentation.
/// It uses a chunk-based format similar to IFF/RIFF with FourCC identifiers.
///
/// **File Structure:**
/// - Header: "UNIF" magic (4 bytes) + version (4 bytes)
/// - Chunks: FourCC (4 bytes) + size (4 bytes) + data
///
/// **Standard Chunks:**
/// - "MAPR": Board/mapper name string
/// - "PRG0"-"PRGF": PRG ROM data chunks (up to 16)
/// - "CHR0"-"CHRF": CHR ROM data chunks (up to 16)
/// - "NAME": Game title string
/// - "MIRR": Mirroring mode
/// - "BATR": Battery presence flag
/// - "TVCI": TV standard (NTSC/PAL/Dual)
/// - "CTRL": Controller types supported
///
/// **Board Name Mapping:**
/// - Uses human-readable board names (e.g., "NES-NROM-256")
/// - _boardMappings translates to iNES mapper numbers
/// - GetMapperID() performs the translation
///
/// **Multi-Chunk PRG/CHR:**
/// - PRG/CHR data can be split across multiple chunks
/// - Chunks numbered 0-F (hexadecimal)
/// - Concatenated in order during loading
///
/// **Limitations:**
/// - Less widely supported than iNES/NES 2.0
/// - Board name database may be incomplete
/// - Mostly used for unusual/undocumented mappers
/// </remarks>
class UnifLoader : public BaseLoader {
private:
	/// <summary>Maps UNIF board names to iNES mapper numbers.</summary>
	static std::unordered_map<string, int> _boardMappings;

	/// <summary>PRG ROM chunks (PRG0-PRGF).</summary>
	vector<uint8_t> _prgChunks[16];

	/// <summary>CHR ROM chunks (CHR0-CHRF).</summary>
	vector<uint8_t> _chrChunks[16];

	/// <summary>Board/mapper name from MAPR chunk.</summary>
	string _mapperName;

	/// <summary>Reads a single byte from the chunk stream.</summary>
	void Read(uint8_t*& data, uint8_t& dest);

	/// <summary>Reads a 32-bit value from the chunk stream.</summary>
	void Read(uint8_t*& data, uint32_t& dest);

	/// <summary>Reads a byte array from the chunk stream.</summary>
	void Read(uint8_t*& data, uint8_t* dest, size_t len);

	/// <summary>Reads a null-terminated string from the chunk.</summary>
	string ReadString(uint8_t*& data, uint8_t* chunkEnd);

	/// <summary>Reads a 4-character chunk identifier.</summary>
	string ReadFourCC(uint8_t*& data);

	/// <summary>
	/// Reads and processes a single UNIF chunk.
	/// </summary>
	/// <param name="data">Current read position (updated).</param>
	/// <param name="dataEnd">End of file pointer.</param>
	/// <param name="romData">ROM data being populated.</param>
	/// <returns>True if chunk was valid, false on error.</returns>
	bool ReadChunk(uint8_t*& data, uint8_t* dataEnd, RomData& romData);

public:
	using BaseLoader::BaseLoader;

	/// <summary>
	/// Translates a UNIF board name to an iNES mapper ID.
	/// </summary>
	/// <param name="mapperName">UNIF board name (e.g., "NES-CNROM").</param>
	/// <returns>Mapper ID, or -1 if unknown.</returns>
	static int32_t GetMapperID(string mapperName);

	/// <summary>
	/// Loads a UNIF ROM file.
	/// </summary>
	/// <param name="romData">Output ROM data structure.</param>
	/// <param name="romFile">Input UNIF file bytes.</param>
	/// <param name="databaseEnabled">Whether to validate with ROM database.</param>
	void LoadRom(RomData& romData, vector<uint8_t>& romFile, bool databaseEnabled);
};