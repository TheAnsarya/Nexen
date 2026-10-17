#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"

class LabelManager;
class MemoryDumper;
class EmuSettings;
struct ChannelFCpuState;

/// <summary>
/// Fairchild F8 addressing modes for disassembly.
/// </summary>
enum class ChannelFAddrMode : uint8_t {
	Imp,    // Implied (1 byte, no operand)
	Reg,    // Register encoded in low nibble (1 byte)
	Isar3,  // ISAR value in low 3 bits (1 byte)
	Imm4,   // 4-bit immediate in low nibble (1 byte)
	Imm,    // 8-bit immediate operand (2 bytes)
	Port,   // Port number operand (2 bytes)
	Rel,    // Relative branch displacement (2 bytes)
	Abs,    // Absolute 16-bit address (3 bytes)
	None    // Undefined opcode
};

/// <summary>
/// Disassembly utilities for the Fairchild F8 CPU (Channel F).
/// Provides opcode decoding, mnemonic lookup, and effective address
/// computation for the F8 instruction set.
/// </summary>
class ChannelFDisUtils {
public:
	static void GetDisassembly(DisassemblyInfo& info, string& out, uint32_t memoryAddr, LabelManager* labelManager, EmuSettings* settings);
	static EffectiveAddressInfo GetEffectiveAddress(DisassemblyInfo& info, ChannelFCpuState& state, MemoryDumper* memoryDumper);

	static uint8_t GetOpSize(uint8_t opCode);
	static char const* const GetOpName(uint8_t opCode);
	static ChannelFAddrMode GetOpMode(uint8_t opCode);
	[[nodiscard]] static bool IsUnconditionalJump(uint8_t opCode);
	[[nodiscard]] static bool IsConditionalJump(uint8_t opCode);
	static CdlFlags::CdlFlags GetOpFlags(uint8_t opCode, uint16_t pc, uint16_t prevPc);
	[[nodiscard]] static bool IsJumpToSub(uint8_t opCode);
	[[nodiscard]] static bool IsReturnInstruction(uint8_t opCode);
};
