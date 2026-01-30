#pragma once
#include "pch.h"

/// <summary>
/// Identifies CPU and coprocessor types across all supported emulation platforms.
/// Used for debugger context, trace logging, and performance profiling.
/// </summary>
/// <remarks>
/// Stored as uint8_t for minimal memory footprint.
/// Order is significant for array indexing in multi-core systems.
/// Coprocessors (DSP, SA-1, Super FX) are treated as separate CPU types.
/// </remarks>
enum class CpuType : uint8_t {
	Snes,    ///< SNES 65816 main CPU (16-bit with 24-bit addressing)
	Spc,     ///< SNES SPC700 audio CPU (8-bit, 16-bit addressing)
	NecDsp,  ///< SNES NEC DSP coprocessor (used in games like Pilotwings)
	Sa1,     ///< SNES SA-1 coprocessor (65816-compatible with enhancements)
	Gsu,     ///< SNES Super FX (GSU) coprocessor (RISC CPU for 3D graphics)
	Cx4,     ///< SNES Cx4 coprocessor (used in Mega Man X2/X3)
	St018,   ///< SNES ST018 coprocessor (used in Hayazashi Nidan Morita Shougi)
	Gameboy, ///< Game Boy CPU (Sharp LR35902 - Z80-like with differences)
	Nes,     ///< NES 6502 CPU (8-bit, 16-bit addressing)
	Pce,     ///< PC Engine HuC6280 CPU (65C02-based with enhancements)
	Sms,     ///< Sega Master System Z80 CPU (8-bit, 16-bit addressing)
	Gba,     ///< Game Boy Advance ARM7TDMI CPU (32-bit RISC)
	Ws       ///< WonderSwan NEC V30MZ CPU (80186-compatible)
};

/// <summary>
/// Utility functions for CpuType enum operations.
/// Uses constexpr for compile-time evaluation and [[nodiscard]] for safety.
/// </summary>
class CpuTypeUtilities {
public:
	/// <summary>
	/// Get total number of CPU types (for array sizing and iteration).
	/// </summary>
	/// <returns>Number of CPU types defined in CpuType enum</returns>
	/// <remarks>
	/// constexpr allows compile-time array sizing: std::array<T, GetCpuTypeCount()>
	/// Value is (int)CpuType::Ws + 1 assuming contiguous enum values starting at 0.
	/// [[nodiscard]] prevents accidentally discarding the count value.
	/// </remarks>
	[[nodiscard]] static constexpr int GetCpuTypeCount() {
		return (int)CpuType::Ws + 1;
	}
};
