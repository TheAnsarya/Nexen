#pragma once
#include "pch.h"

#include "NES/NesTypes.h"
#include "NES/NesHeader.h"
#include "Shared/RomInfo.h"

/// <summary>ROM header format version detection result.</summary>
enum class RomHeaderVersion {
	iNes = 0,      ///< Standard iNES format (most common)
	Nes2_0 = 1,    ///< NES 2.0 extended format
	OldiNes = 2    ///< Archaic iNES (bytes 7-15 may contain garbage)
};

/// <summary>
/// NSF (NES Sound Format) audio file header.
/// </summary>
/// <remarks>
/// **NSF Format:**
/// NSF files contain NES music/sound data that can be played
/// without the full game ROM. Used for game music rips and
/// chiptune compositions.
///
/// **Memory Layout:**
/// - LoadAddress: Where the NSF data is loaded in NES memory
/// - InitAddress: Called to initialize song (A = song number)
/// - PlayAddress: Called once per frame to advance playback
///
/// **Bank Switching:**
/// BankSetup[8] defines initial bank configuration for ROMs > 4KB.
/// Each slot maps a 4KB page. Value $00 means no banking.
///
/// **Sound Chips:**
/// - Bit 0: VRC6 (Konami)
/// - Bit 1: VRC7 (Konami, FM synthesis)
/// - Bit 2: FDS (Famicom Disk System wavetable)
/// - Bit 3: MMC5 (extra pulse channels)
/// - Bit 4: Namco 163 (wavetable)
/// - Bit 5: Sunsoft 5B (AY-3-8910)
///
/// **NSFe Extensions:**
/// Extended format adds per-track names, lengths, and fade times.
/// </remarks>
struct NsfHeader {
	char Header[5] = {};           ///< "NESM" + $1A for NSF, "NSFE" for NSFe
	uint8_t Version = 0;           ///< NSF version (usually 1)
	uint8_t TotalSongs = 0;        ///< Number of songs in file
	uint8_t StartingSong = 0;      ///< Default song to play (1-based)
	uint16_t LoadAddress = 0;      ///< Memory address to load data ($8000+)
	uint16_t InitAddress = 0;      ///< Init routine address
	uint16_t PlayAddress = 0;      ///< Play routine address (called each frame)
	char SongName[256] = {};       ///< Song/album name (null-terminated)
	char ArtistName[256] = {};     ///< Artist name (null-terminated)
	char CopyrightHolder[256] = {};///< Copyright holder (null-terminated)
	uint16_t PlaySpeedNtsc = 0;    ///< NTSC playback speed in microseconds
	uint8_t BankSetup[8] = {};     ///< Initial bank configuration
	uint16_t PlaySpeedPal = 0;     ///< PAL playback speed in microseconds
	uint8_t Flags = 0;             ///< Region flags (bit 0=PAL, bit 1=dual)
	uint8_t SoundChips = 0;        ///< Expansion audio chip flags
	uint8_t Padding[4] = {};       ///< Reserved padding bytes

	// NSFe extensions
	char RipperName[256] = {};     ///< NSFe: Person who ripped the music
	vector<string> TrackNames;     ///< NSFe: Per-track names
	int32_t TrackLength[256] = {}; ///< NSFe: Track length in ms (-1 = unknown)
	int32_t TrackFade[256] = {};   ///< NSFe: Fade-out time in ms
};

/// <summary>
/// Game database entry with verified ROM information.
/// </summary>
/// <remarks>
/// **Purpose:**
/// Provides accurate ROM metadata from curated databases like
/// NES Cart DB, bootgod's database, or No-Intro. Allows correct
/// emulation even when ROM headers are incorrect or missing.
///
/// **Key Fields:**
/// - MapperID: Correct mapper (overrides header)
/// - Board/Pcb/Chip: Hardware identification
/// - RAM sizes: Exact PRG/CHR RAM amounts
/// - BusConflicts: ROM/mapper bus conflict handling
/// </remarks>
struct GameInfo {
	uint32_t Crc = 0;              ///< CRC32 for ROM identification
	string System;                 ///< System name (NES, Famicom, etc.)
	string Board;                  ///< PCB board name (e.g., "SNROM")
	string Pcb;                    ///< Specific PCB identifier
	string Chip;                   ///< Mapper chip name (e.g., "MMC1B2")
	uint16_t MapperID = 0;         ///< Correct mapper number
	uint32_t PrgRomSize = 0;       ///< PRG-ROM size in bytes
	uint32_t ChrRomSize = 0;       ///< CHR-ROM size in bytes
	uint32_t ChrRamSize = 0;       ///< CHR-RAM size in bytes
	uint32_t WorkRamSize = 0;      ///< Work RAM size in bytes
	uint32_t SaveRamSize = 0;      ///< Battery-backed RAM size
	bool HasBattery = false;       ///< Has battery backup
	string Mirroring;              ///< Mirroring type string
	GameInputType InputType = {};  ///< Input device type
	string BusConflicts;           ///< Bus conflict handling
	string SubmapperID;            ///< Submapper identifier
	VsSystemType VsType = {};      ///< Vs. System type
	PpuModel VsPpuModel = {};      ///< Vs. System PPU model
};

/// <summary>
/// Parsed NES ROM information combining header, database, and analysis.
/// </summary>
/// <remarks>
/// **Information Sources:**
/// 1. ROM header (iNES/NES 2.0)
/// 2. Database lookup (by CRC32)
/// 3. Heuristic analysis (for headerless ROMs)
///
/// Database entries override header information when available.
/// </remarks>
struct NesRomInfo {
	string RomName;                ///< Game name (from database or filename)
	string Filename;               ///< Original filename
	RomFormat Format = {};         ///< File format (iNES, FDS, NSF, etc.)

	bool IsNes20Header = false;    ///< True if NES 2.0 header detected
	bool IsInDatabase = false;     ///< True if found in game database
	bool IsHeaderlessRom = false;  ///< True if ROM has no header

	uint32_t FilePrgOffset = 0;    ///< Offset to PRG data in file

	uint16_t MapperID = 0;         ///< Final mapper ID (database or header)
	uint8_t SubMapperID = 0;       ///< Submapper ID (NES 2.0)

	GameSystem System = GameSystem::Unknown;  ///< Target system
	VsSystemType VsType = VsSystemType::Default;  ///< Vs. System variant
	GameInputType InputType = GameInputType::Unspecified;  ///< Input device
	PpuModel VsPpuModel = PpuModel::Ppu2C02;  ///< PPU model for Vs. games

	bool HasChrRam = false;        ///< True if using CHR-RAM (no CHR-ROM)
	bool HasBattery = false;       ///< Has battery-backed saves
	bool HasEpsm = false;          ///< Has EPSM audio expansion
	bool HasTrainer = false;       ///< Has 512-byte trainer data
	MirroringType Mirroring = MirroringType::Horizontal;  ///< Nametable mirroring
	BusConflictType BusConflicts = BusConflictType::Default;  ///< Bus conflict mode

	HashInfo Hash = {};            ///< ROM hash values (CRC32, MD5, SHA1)

	NesHeader Header = {};         ///< Parsed ROM header
	NsfHeader NsfInfo = {};        ///< NSF header (if audio file)
	GameInfo DatabaseInfo = {};    ///< Database entry (if matched)
};

/// <summary>
/// Study Box page data structure.
/// </summary>
/// <remarks>
/// Study Box was a Famicom educational peripheral that used
/// audio cassettes with embedded data for interactive lessons.
/// Each page contains graphical data and audio cues.
/// </remarks>
struct PageInfo {
	uint32_t LeadInOffset = 0;     ///< Offset to lead-in tone
	uint32_t AudioOffset = 0;      ///< Offset to audio data
	vector<uint8_t> Data;          ///< Page graphical/program data
};

/// <summary>
/// Study Box cassette data container.
/// </summary>
struct StudyBoxData {
	string FileName;               ///< Original cassette filename
	vector<uint8_t> AudioFile;     ///< Full audio track data
	vector<PageInfo> Pages;        ///< Individual page data
};

/// <summary>
/// Complete parsed ROM data ready for emulation.
/// </summary>
/// <remarks>
/// **Contents:**
/// This structure contains all data needed to initialize emulation:
/// - PRG-ROM: Program code and data
/// - CHR-ROM: Graphics tile data (optional, some games use CHR-RAM)
/// - Trainer: Optional 512-byte code loaded at $7000
/// - FDS disk data: For Famicom Disk System games
///
/// **RAM Sizes:**
/// -1 indicates "use default for mapper". The mapper decides
/// the appropriate RAM size based on board type. Explicit sizes
/// override defaults (from NES 2.0 headers or database).
///
/// **Error Handling:**
/// - Error: ROM failed to load (corrupt, unsupported format)
/// - BiosMissing: Required BIOS file not found (FDS, PlayChoice)
/// </remarks>
struct RomData {
	NesRomInfo Info = {};          ///< Parsed ROM information

	int32_t ChrRamSize = -1;       ///< CHR-RAM size (-1 = use default)
	int32_t SaveChrRamSize = -1;   ///< Battery-backed CHR-RAM size
	int32_t SaveRamSize = -1;      ///< Battery-backed PRG-RAM size
	int32_t WorkRamSize = -1;      ///< Work RAM size (-1 = use default)

	vector<uint8_t> PrgRom;        ///< PRG-ROM data (program code)
	vector<uint8_t> ChrRom;        ///< CHR-ROM data (graphics, may be empty)
	vector<uint8_t> TrainerData;   ///< 512-byte trainer (if present)
	vector<vector<uint8_t>> FdsDiskData;     ///< FDS disk side data
	vector<vector<uint8_t>> FdsDiskHeaders;  ///< FDS disk headers
	StudyBoxData StudyBox = {};    ///< Study Box cassette data

	vector<uint8_t> RawData;       ///< Original file data (for hashing)

	bool Error = false;            ///< True if ROM failed to load
	bool BiosMissing = false;      ///< True if required BIOS is missing
};
