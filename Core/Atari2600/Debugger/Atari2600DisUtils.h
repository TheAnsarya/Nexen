#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"

class LabelManager;
class MemoryDumper;
class EmuSettings;
struct Atari2600CpuState;
enum class Atari2600AddrMode : uint8_t;

/// <summary>
/// Disassembly utilities for the Atari 2600 6507 CPU.
/// Provides opcode decoding, mnemonic lookup, and effective address
/// computation for the 6502-compatible instruction set.
/// </summary>
class Atari2600DisUtils {
private:
	static uint32_t GetOperandAddress(DisassemblyInfo& info, uint32_t memoryAddr);
	static uint8_t GetOpSize(Atari2600AddrMode addrMode);

public:
	static void GetDisassembly(DisassemblyInfo& info, string& out, uint32_t memoryAddr, LabelManager* labelManager, EmuSettings* settings);
	static EffectiveAddressInfo GetEffectiveAddress(DisassemblyInfo& info, Atari2600CpuState& state, MemoryDumper* memoryDumper);

	static uint8_t GetOpSize(uint8_t opCode);
	static char const* const GetOpName(uint8_t opCode);
	static Atari2600AddrMode GetOpMode(uint8_t opCode);
	[[nodiscard]] static bool IsOpUnofficial(uint8_t opCode);
	[[nodiscard]] static bool IsUnconditionalJump(uint8_t opCode);
	[[nodiscard]] static bool IsConditionalJump(uint8_t opCode);
	static CdlFlags::CdlFlags GetOpFlags(uint8_t opCode, uint16_t pc, uint16_t prevPc);
	[[nodiscard]] static bool IsJumpToSub(uint8_t opCode);
	[[nodiscard]] static bool IsReturnInstruction(uint8_t opCode);
};
