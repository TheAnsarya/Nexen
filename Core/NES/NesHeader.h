#pragma once
#include "NesTypes.h"

enum class RomHeaderVersion;

/// <summary>
/// Parses and interprets iNES and NES 2.0 ROM headers.
/// </summary>
/// <remarks>
/// **iNES Header Format (16 bytes):**
/// The iNES format is the de facto standard for NES ROM distribution.
/// Created by Marat Fayzullin in 1996 for his iNES emulator.
///
/// **Header Evolution:**
/// ```
/// Byte    Archaic          iNES                          NES 2.0
/// -------------------------------------------------------------------------
/// 0-3     "NES" + $1A      "NES" + $1A                   "NES" + $1A
/// 4       PRG-ROM (16KB)   PRG-ROM (16KB)                PRG-ROM (16KB)
/// 5       CHR-ROM (8KB)    CHR-ROM (8KB)                 CHR-ROM (8KB)
/// 6       Mapper/Mirror    Mapper low, Mirror/Bat/Train  Same
/// 7       Unused           Mapper high, Vs/PC            NES 2.0 sig, Mapper
/// 8       Unused           PRG-RAM size                  Mapper/Submapper
/// 9       Unused           TV system                     ROM size upper bits
/// 10      Unused           Unused                        PRG-RAM (log2)
/// 11      Unused           Unused                        CHR-RAM (log2)
/// 12      Unused           Unused                        TV system
/// 13      Unused           Unused                        Vs. PPU variant
/// 14-15   Unused           Unused                        Unused
/// ```
///
/// **Byte 6 Flags:**
/// - Bit 0: Mirroring (0=horizontal, 1=vertical)
/// - Bit 1: Battery-backed PRG-RAM
/// - Bit 2: 512-byte trainer at $7000-$71FF
/// - Bit 3: Four-screen VRAM (ignore mirroring bit)
/// - Bits 4-7: Mapper number lower nibble
///
/// **Byte 7 Flags:**
/// - Bits 0-1: Vs. Unisystem / PlayChoice-10
/// - Bits 2-3: NES 2.0 signature (must be %10 for NES 2.0)
/// - Bits 4-7: Mapper number upper nibble
///
/// **NES 2.0 Extensions:**
/// - Submapper IDs for variants (e.g., MMC1 vs SNROM)
/// - Exact PRG/CHR RAM sizes (battery and non-battery)
/// - Extended console types (Vs., PlayChoice, FDS, etc.)
/// - Extended input device specification
///
/// **Mapper ID Calculation:**
/// - iNES: (Byte7 & 0xF0) | (Byte6 >> 4) = 0-255
/// - NES 2.0: ((Byte8 & 0x0F) << 8) | above = 0-4095
/// </remarks>
struct NesHeader {
	char NES[4];        ///< Magic number: "NES" + $1A
	uint8_t PrgCount;   ///< PRG-ROM size in 16KB units (or NES 2.0 extended)
	uint8_t ChrCount;   ///< CHR-ROM size in 8KB units (0 = CHR-RAM)
	uint8_t Byte6;      ///< Mapper low nibble, mirroring, battery, trainer
	uint8_t Byte7;      ///< Mapper high nibble, NES 2.0 signature, Vs./PC
	uint8_t Byte8;      ///< iNES: PRG-RAM, NES 2.0: Mapper/submapper
	uint8_t Byte9;      ///< iNES: TV system, NES 2.0: ROM size upper bits
	uint8_t Byte10;     ///< NES 2.0: PRG-RAM size (log2, battery + non-battery)
	uint8_t Byte11;     ///< NES 2.0: CHR-RAM size (log2, battery + non-battery)
	uint8_t Byte12;     ///< NES 2.0: CPU/PPU timing mode
	uint8_t Byte13;     ///< NES 2.0: Vs. system type or extended console type
	uint8_t Byte14;     ///< Reserved (should be zero)
	uint8_t Byte15;     ///< Reserved (should be zero)

	/// <summary>Gets full mapper ID (0-255 for iNES, 0-4095 for NES 2.0).</summary>
	[[nodiscard]] uint16_t GetMapperID();

	/// <summary>Checks if ROM has battery-backed save RAM.</summary>
	[[nodiscard]] bool HasBattery();

	/// <summary>Checks if ROM has 512-byte trainer at $7000.</summary>
	[[nodiscard]] bool HasTrainer();

	/// <summary>Gets NES-specific game system (NTSC/PAL/Multi).</summary>
	[[nodiscard]] GameSystem GetNesGameSystem();

	/// <summary>Gets full game system including Vs., Famicom, etc.</summary>
	[[nodiscard]] GameSystem GetGameSystem();

	/// <summary>Checks if ROM uses EPSM audio expansion.</summary>
	[[nodiscard]] bool HasEpsm();

	/// <summary>Detects header format version (iNES, NES 2.0, Archaic).</summary>
	[[nodiscard]] RomHeaderVersion GetRomHeaderVersion();

	/// <summary>Calculates size from NES 2.0 exponent/multiplier encoding.</summary>
	/// <param name="exponent">Size exponent value.</param>
	/// <param name="multiplier">Size multiplier value.</param>
	/// <returns>Calculated size in bytes.</returns>
	[[nodiscard]] uint32_t GetSizeValue(uint32_t exponent, uint32_t multiplier);

	/// <summary>Gets PRG-ROM size in bytes.</summary>
	[[nodiscard]] uint32_t GetPrgSize();

	/// <summary>Gets CHR-ROM size in bytes (0 if CHR-RAM).</summary>
	[[nodiscard]] uint32_t GetChrSize();

	/// <summary>Gets work RAM size (-1 for default).</summary>
	[[nodiscard]] int32_t GetWorkRamSize();

	/// <summary>Gets battery-backed save RAM size (-1 for default).</summary>
	[[nodiscard]] int32_t GetSaveRamSize();

	/// <summary>Gets CHR-RAM size (-1 for default).</summary>
	[[nodiscard]] int32_t GetChrRamSize();

	/// <summary>Gets battery-backed CHR-RAM size (-1 for default).</summary>
	[[nodiscard]] int32_t GetSaveChrRamSize();

	/// <summary>Gets submapper ID (NES 2.0 only, 0-15).</summary>
	[[nodiscard]] uint8_t GetSubMapper();

	/// <summary>Gets nametable mirroring type from header.</summary>
	[[nodiscard]] MirroringType GetMirroringType();

	/// <summary>Gets input device type (NES 2.0 extension).</summary>
	[[nodiscard]] GameInputType GetInputType();

	/// <summary>Gets Vs. System type (Unisystem, Dualsystem, etc.).</summary>
	[[nodiscard]] VsSystemType GetVsSystemType();

	/// <summary>Gets Vs. System PPU model variant.</summary>
	[[nodiscard]] PpuModel GetVsSystemPpuModel();
};
