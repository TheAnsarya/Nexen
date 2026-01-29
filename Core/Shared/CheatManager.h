#pragma once
#include "pch.h"
#include <optional>
#include "Shared/CpuType.h"

using std::optional;

class Emulator;
enum class MemoryType;

/// <summary>Cheat code format types for various consoles and devices</summary>
enum class CheatType : uint8_t {
	NesGameGenie = 0,        ///< NES Game Genie (6 or 8 character codes)
	NesProActionRocky,       ///< NES Pro Action Rocky / Replay
	NesCustom,               ///< NES custom format (address:value or address?compare:value)
	GbGameGenie,             ///< Game Boy Game Genie
	GbGameShark,             ///< Game Boy Game Shark
	SnesGameGenie,           ///< SNES Game Genie
	SnesProActionReplay,     ///< SNES Pro Action Replay
	PceRaw,                  ///< PC Engine raw format
	PceAddress,              ///< PC Engine address format
	SmsProActionReplay,      ///< Sega Master System Pro Action Replay
	SmsGameGenie             ///< Sega Master System Game Genie
};

/// <summary>
/// Internal representation of decoded cheat code.
/// Converted from external format (Game Genie, etc.) to memory operation.
/// </summary>
struct InternalCheatCode {
	MemoryType MemType = {};       ///< Memory region to patch
	uint32_t Address = 0;          ///< Address to patch
	int16_t Compare = -1;          ///< Compare value (-1 = no compare)
	uint8_t Value = 0;             ///< Value to write
	CheatType Type = {};           ///< Original cheat format type
	CpuType Cpu = {};              ///< CPU this cheat applies to
	bool IsRamCode = false;        ///< RAM code (applied every frame)
	bool IsAbsoluteAddress = false;///< Absolute address vs bank-relative
};

/// <summary>External cheat code structure (user-provided format)</summary>
struct CheatCode {
	CheatType Type;     ///< Cheat format type
	char Code[16];      ///< Cheat code string (e.g., "SLXPLOVS")
};

/// <summary>
/// Multi-console cheat code manager supporting Game Genie, Action Replay, and custom formats.
/// Decodes external cheat codes and applies memory patches during emulation.
/// </summary>
/// <remarks>
/// Supported formats:
/// - NES: Game Genie (6/8 char), Pro Action Rocky, Custom (addr:val or addr?cmp:val)
/// - SNES: Game Genie, Pro Action Replay
/// - Game Boy: Game Genie, Game Shark
/// - PC Engine: Raw, Address format
/// - SMS: Game Genie, Pro Action Replay
/// 
/// Two cheat types:
/// 1. ROM codes: Applied once when address accessed
/// 2. RAM codes: Applied every frame (for changing values)
/// 
/// Cheat application:
/// - Template method ApplyCheat<cpuType>() called during memory reads
/// - HasCheats<cpuType>() checks if any cheats active for CPU
/// - Bank-aware caching (_bankHasCheats) for fast lookup
/// 
/// Thread safety: Not thread-safe - modify cheats only when emulation paused.
/// </remarks>
class CheatManager {
private:
	Emulator* _emu;                          ///< Emulator instance
	bool _hasCheats[CpuTypeUtilities::GetCpuTypeCount()] = {};                   ///< Per-CPU cheat flags
	bool _bankHasCheats[CpuTypeUtilities::GetCpuTypeCount()][0x100] = {};        ///< Per-bank cheat flags
	vector<CheatCode> _cheats;               ///< Active external cheats

	/// <summary>RAM cheats to refresh each frame (per CPU)</summary>
	vector<InternalCheatCode> _ramRefreshCheats[CpuTypeUtilities::GetCpuTypeCount()];
	
	/// <summary>Address-indexed cheat lookup (per CPU)</summary>
	unordered_map<uint32_t, InternalCheatCode> _cheatsByAddress[CpuTypeUtilities::GetCpuTypeCount()];

	/// <summary>Try to convert external cheat code to internal format</summary>
	optional<InternalCheatCode> TryConvertCode(CheatCode code);

	/// <summary>Convert SNES Game Genie code</summary>
	optional<InternalCheatCode> ConvertFromSnesGameGenie(string code);
	
	/// <summary>Convert SNES Pro Action Replay code</summary>
	optional<InternalCheatCode> ConvertFromSnesProActionReplay(string code);

	/// <summary>Convert Game Boy Game Genie code</summary>
	optional<InternalCheatCode> ConvertFromGbGameGenie(string code);
	
	/// <summary>Convert Game Boy Game Shark code</summary>
	optional<InternalCheatCode> ConvertFromGbGameShark(string code);

	/// <summary>Convert PC Engine raw format code</summary>
	optional<InternalCheatCode> ConvertFromPceRaw(string code);
	
	/// <summary>Convert PC Engine address format code</summary>
	optional<InternalCheatCode> ConvertFromPceAddress(string code);

	/// <summary>Convert NES Game Genie code (6 or 8 characters)</summary>
	optional<InternalCheatCode> ConvertFromNesGameGenie(string code);
	
	/// <summary>Convert NES Pro Action Rocky code</summary>
	optional<InternalCheatCode> ConvertFromNesProActionRocky(string code);
	
	/// <summary>Convert NES custom format (addr:val or addr?cmp:val)</summary>
	optional<InternalCheatCode> ConvertFromNesCustomCode(string code);

	/// <summary>Convert SMS Game Genie code</summary>
	optional<InternalCheatCode> ConvertFromSmsGameGenie(string code);
	
	/// <summary>Convert SMS Pro Action Replay code</summary>
	optional<InternalCheatCode> ConvertFromSmsProActionReplay(string code);

	/// <summary>
	/// Get bank shift amount for CPU address space.
	/// </summary>
	/// <param name="cpuType">CPU type</param>
	/// <returns>Number of bits to shift for bank number</returns>
	/// <remarks>
	/// Bank sizes:
	/// - SNES: 64KB banks (shift 16)
	/// - GB/NES/SMS: 256-byte banks (shift 8)
	/// - PCE: 8KB banks (shift 13)
	/// </remarks>
	__forceinline constexpr int GetBankShift(CpuType cpuType) {
		switch (cpuType) {
			case CpuType::Snes:
				return 16;
			case CpuType::Gameboy:
				return 8;
			case CpuType::Nes:
				return 8;
			case CpuType::Pce:
				return 13;
			case CpuType::Sms:
				return 8;
			default: [[unlikely]]
				throw std::runtime_error("unsupported cpu type");
		}
	}

public:
	/// <summary>Construct cheat manager for emulator</summary>
	CheatManager(Emulator* emu);

	/// <summary>Add single cheat code (decoded and validated)</summary>
	/// <returns>True if cheat added successfully</returns>
	bool AddCheat(CheatCode code);
	
	/// <summary>Internal clear (no notification)</summary>
	void InternalClearCheats();

	/// <summary>Replace all cheats with new set (vector)</summary>
	void SetCheats(vector<CheatCode>& codes);
	
	/// <summary>Replace all cheats with new set (array)</summary>
	void SetCheats(CheatCode codes[], uint32_t length);
	
	/// <summary>
	/// Clear all active cheats.
	/// </summary>
	/// <param name="showMessage">Show notification to user if true</param>
	void ClearCheats(bool showMessage = true);

	/// <summary>Get list of active external cheats</summary>
	vector<CheatCode> GetCheats();

	/// <summary>
	/// Convert external cheat to internal format (for testing/validation).
	/// </summary>
	/// <param name="input">External cheat code</param>
	/// <param name="output">Decoded internal cheat (output)</param>
	/// <returns>True if conversion succeeded</returns>
	bool GetConvertedCheat(CheatCode input, InternalCheatCode& output);

	/// <summary>Get list of RAM refresh cheats for CPU (applied every frame)</summary>
	vector<InternalCheatCode>& GetRamRefreshCheats(CpuType cpuType);
	
	/// <summary>Apply all RAM cheats for CPU (called once per frame)</summary>
	void RefreshRamCheats(CpuType cpuType);

	/// <summary>
	/// Check if any cheats active for CPU (fast check).
	/// </summary>
	template <CpuType cpuType>
	__forceinline bool HasCheats() {
		return _hasCheats[(int)cpuType];
	}

	/// <summary>
	/// Apply cheat if address has active code (called during memory reads).
	/// </summary>
	/// <param name="addr">Memory address being read</param>
	/// <param name="value">Value to potentially patch (modified if cheat active)</param>
	/// <remarks>
	/// Fast path: Checks bank flag first (cache optimization)
	/// Slow path: Looks up cheat in address map, applies if found
	/// Compare value checked if cheat has compare condition
	/// </remarks>
	template <CpuType cpuType>
	__noinline void ApplyCheat(uint32_t addr, uint8_t& value);
};
