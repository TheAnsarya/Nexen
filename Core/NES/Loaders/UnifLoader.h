#pragma once
#include "pch.h"
#include <algorithm>
#include <array>
#include <string_view>
#include "NES/Loaders/BaseLoader.h"
#include "NES/Loaders/UnifBoards.h"

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
	using BoardEntry = std::pair<std::string_view, int>;

	/// <summary>Maps UNIF board names to iNES mapper numbers (sorted for binary search).</summary>
	static constexpr std::array<BoardEntry, 167> _boardMappings = {{
		{"10-24-C-A1",                  UnifBoards::UnknownBoard},
		{"11160",                       299},
		{"12-IN-1",                     331},
		{"13in1JY110",                  UnifBoards::UnknownBoard},
		{"158B",                        258},
		{"190in1",                      300},
		{"22211",                       132},
		{"255in1",                      UnifBoards::Unl255in1},
		{"3D-BLOCK",                    UnifBoards::UnknownBoard},
		{"411120-C",                    287},
		{"42in1ResetSwitch",            226},
		{"43272",                       227},
		{"603-5052",                    238},
		{"60311C",                      289},
		{"64in1NoRepeat",               314},
		{"70in1",                       236},
		{"70in1B",                      236},
		{"8-IN-1",                      333},
		{"80013-B",                     274},
		{"81-01-31-C",                  UnifBoards::UnknownBoard},
		{"810544-C-A1",                 261},
		{"8157",                        301},
		{"8237",                        215},
		{"8237A",                       UnifBoards::Unl8237A},
		{"830118C",                     348},
		{"830425C-4391T",               320},
		{"A65AS",                       285},
		{"AC08",                        UnifBoards::Ac08},
		{"ANROM",                       7},
		{"AX5705",                      530},
		{"BB",                          108},
		{"BS-5",                        286},
		{"CC-21",                       UnifBoards::Cc21},
		{"CHINA_ER_SAN2",               19},
		{"CITYFIGHT",                   266},
		{"CNROM",                       3},
		{"COOLBOY",                     268},
		{"CPROM",                       13},
		{"D1038",                       59},
		{"DANCE",                       UnifBoards::UnknownBoard},
		{"DANCE2000",                   518},
		{"DRAGONFIGHTER",               292},
		{"DREAMTECH01",                 521},
		{"DRIPGAME",                    284},
		{"EDU2000",                     329},
		{"EH8813A",                     519},
		{"EKROM",                       5},
		{"ELROM",                       5},
		{"ETROM",                       5},
		{"EWROM",                       5},
		{"F-15",                        259},
		{"FARID_SLROM_8-IN-1",          323},
		{"FARID_UNROM_8-IN-1",          324},
		{"FK23C",                       176},
		{"FK23CA",                      176},
		{"FS304",                       162},
		{"G-146",                       349},
		{"GK-192",                      58},
		{"GS-2004",                     283},
		{"GS-2013",                     UnifBoards::Gs2013},
		{"Ghostbusters63in1",           UnifBoards::Ghostbusters63in1},
		{"H2288",                       123},
		{"HKROM",                       4},
		{"HP2018A",                     260},
		{"HP898F",                      319},
		{"HPxx",                        260},
		{"K-3046",                      336},
		{"KOF97",                       263},
		{"KONAMI-QTAI",                 190},
		{"KS7010",                      UnifBoards::UnknownBoard},
		{"KS7012",                      346},
		{"KS7013B",                     312},
		{"KS7016",                      306},
		{"KS7017",                      303},
		{"KS7030",                      UnifBoards::UnknownBoard},
		{"KS7031",                      305},
		{"KS7032",                      142},
		{"KS7037",                      307},
		{"KS7057",                      302},
		{"LE05",                        UnifBoards::UnknownBoard},
		{"LH10",                        522},
		{"LH32",                        125},
		{"LH51",                        309},
		{"LH53",                        UnifBoards::UnknownBoard},
		{"MALISB",                      325},
		{"MARIO1-MALEE2",               UnifBoards::Malee},
		{"MHROM",                       66},
		{"N625092",                     221},
		{"NROM",                        0},
		{"NROM-128",                    0},
		{"NROM-256",                    0},
		{"NTBROM",                      68},
		{"NTD-03",                      290},
		{"NovelDiamond9999999in1",      201},
		{"OneBus",                      UnifBoards::UnknownBoard},
		{"PEC-586",                     UnifBoards::UnknownBoard},
		{"PUZZLE",                      UnifBoards::UnlPuzzle},
		{"RESET-TXROM",                 313},
		{"RET-CUFROM",                  29},
		{"RROM",                        0},
		{"RROM-128",                    0},
		{"RT-01",                       328},
		{"SA-002",                      136},
		{"SA-0036",                     149},
		{"SA-0037",                     148},
		{"SA-009",                      160},
		{"SA-016-1M",                   146},
		{"SA-72007",                    145},
		{"SA-72008",                    133},
		{"SA-9602B",                    513},
		{"SA-NROM",                     143},
		{"SAROM",                       1},
		{"SB-2000",                     UnifBoards::UnknownBoard},
		{"SBROM",                       1},
		{"SC-127",                      35},
		{"SCROM",                       1},
		{"SEROM",                       1},
		{"SGROM",                       1},
		{"SHERO",                       262},
		{"SKROM",                       1},
		{"SL12",                        116},
		{"SL1632",                      14},
		{"SL1ROM",                      1},
		{"SLROM",                       1},
		{"SMB2J",                       304},
		{"SNROM",                       1},
		{"SOROM",                       1},
		{"SSS-NROM-256",                UnifBoards::SssNrom256},
		{"SUNSOFT_UNROM",               93},
		{"Sachen-74LS374N",             150},
		{"Sachen-74LS374NA",            243},
		{"Sachen-8259A",                141},
		{"Sachen-8259B",                138},
		{"Sachen-8259C",                139},
		{"Sachen-8259D",                137},
		{"Super24in1SC03",              176},
		{"SuperHIK8in1",                45},
		{"Supervision16in1",            53},
		{"T-227-1",                     UnifBoards::UnknownBoard},
		{"T-230",                       529},
		{"T-262",                       265},
		{"TBROM",                       4},
		{"TC-U01-1.5M",                 147},
		{"TEK90",                       90},
		{"TEROM",                       4},
		{"TF1201",                      298},
		{"TFROM",                       4},
		{"TGROM",                       4},
		{"TKROM",                       4},
		{"TKSROM",                      4},
		{"TLROM",                       4},
		{"TLSROM",                      4},
		{"TQROM",                       4},
		{"TR1ROM",                      4},
		{"TSROM",                       4},
		{"TVROM",                       4},
		{"Transformer",                 UnifBoards::UnknownBoard},
		{"UNROM",                       2},
		{"UNROM-512-16",                30},
		{"UNROM-512-32",                30},
		{"UNROM-512-8",                 30},
		{"UOROM",                       2},
		{"VRC7",                        85},
		{"WAIXING-FS005",               UnifBoards::UnknownBoard},
		{"WAIXING-FW01",                227},
		{"WS",                          332},
		{"YOKO",                        264},
	}};

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
	static int32_t GetMapperID(const string& mapperName);

	/// <summary>
	/// Loads a UNIF ROM file.
	/// </summary>
	/// <param name="romData">Output ROM data structure.</param>
	/// <param name="romFile">Input UNIF file bytes.</param>
	/// <param name="databaseEnabled">Whether to validate with ROM database.</param>
	void LoadRom(RomData& romData, vector<uint8_t>& romFile, bool databaseEnabled);
};
