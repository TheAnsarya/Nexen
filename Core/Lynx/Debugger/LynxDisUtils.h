#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"

class LabelManager;
class MemoryDumper;
class EmuSettings;
class LynxConsole;
struct LynxCpuState;
enum class LynxAddrMode : uint8_t;

/// <summary>
/// Lynx 65C02 disassembly utilities — static methods for disassembling
/// instructions, determining opcode sizes, and classifying instructions.
/// </summary>
class LynxDisUtils {
private:
	static uint8_t GetOpSize(LynxAddrMode addrMode);

public:
	static void GetDisassembly(DisassemblyInfo& info, string& out, uint32_t memoryAddr, LabelManager* labelManager, EmuSettings* settings);
	static EffectiveAddressInfo GetEffectiveAddress(DisassemblyInfo& info, LynxConsole* console, LynxCpuState& state);

	static uint8_t GetOpSize(uint8_t opCode);
	static char const* const GetOpName(uint8_t opCode);
	static LynxAddrMode GetOpMode(uint8_t opCode);
	static bool IsUnconditionalJump(uint8_t opCode);
	static bool IsConditionalJump(uint8_t opCode);
	static bool IsJumpToSub(uint8_t opCode);
	static bool IsReturnInstruction(uint8_t opCode);
	static CdlFlags::CdlFlags GetOpFlags(uint8_t opCode, uint16_t pc, uint16_t prevPc);
};
