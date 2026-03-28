#include "pch.h"
#include "Lynx/Debugger/LynxDisUtils.h"
#include "Lynx/LynxCpu.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxTypes.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/LabelManager.h"
#include "Shared/EmuSettings.h"
#include "Utilities/FastString.h"
#include "Utilities/HexUtilities.h"
#include "Lynx/Debugger/DummyLynxCpu.h"
#include "Lynx/LynxMemoryManager.h"

/// <summary>
/// Operand byte size for each LynxAddrMode.
/// Index matches LynxAddrMode enum order.
/// </summary>
constexpr uint8_t _opSize[] = {
	1, // None     - BRK (implied, but 1-byte encoding)
	1, // Acc      - Accumulator
	1, // Imp      - Implied
	2, // Imm      - #nn
	2, // Rel      - Relative (branches)
	2, // Zpg      - $nn
	2, // ZpgX     - $nn,X
	2, // ZpgY     - $nn,Y
	3, // Abs      - $nnnn
	3, // AbsX     - $nnnn,X
	3, // AbsXW    - $nnnn,X (write)
	3, // AbsY     - $nnnn,Y
	3, // AbsYW    - $nnnn,Y (write)
	3, // Ind      - ($nnnn)
	2, // IndX     - ($nn,X)
	2, // IndY     - ($nn),Y
	2, // IndYW    - ($nn),Y (write)
	2, // ZpgInd   - ($nn) — 65C02
	3, // AbsIndX  - ($nnnn,X) — 65C02
};

/// <summary>
/// Full 256-entry opcode mnemonic table for the WDC 65C02.
/// Undefined opcodes are NOP (65C02 treats all unused as NOP).
/// </summary>
constexpr const char* _opName[256] = {
	// 0x00-0x0F
	"BRK", "ORA", "NOP", "NOP", "TSB", "ORA", "ASL", "NOP",
	"PHP", "ORA", "ASL", "NOP", "TSB", "ORA", "ASL", "NOP",
	// 0x10-0x1F
	"BPL", "ORA", "ORA", "NOP", "TRB", "ORA", "ASL", "NOP",
	"CLC", "ORA", "INC", "NOP", "TRB", "ORA", "ASL", "NOP",
	// 0x20-0x2F
	"JSR", "AND", "NOP", "NOP", "BIT", "AND", "ROL", "NOP",
	"PLP", "AND", "ROL", "NOP", "BIT", "AND", "ROL", "NOP",
	// 0x30-0x3F
	"BMI", "AND", "AND", "NOP", "BIT", "AND", "ROL", "NOP",
	"SEC", "AND", "DEC", "NOP", "BIT", "AND", "ROL", "NOP",
	// 0x40-0x4F
	"RTI", "EOR", "NOP", "NOP", "NOP", "EOR", "LSR", "NOP",
	"PHA", "EOR", "LSR", "NOP", "JMP", "EOR", "LSR", "NOP",
	// 0x50-0x5F
	"BVC", "EOR", "EOR", "NOP", "NOP", "EOR", "LSR", "NOP",
	"CLI", "EOR", "PHY", "NOP", "NOP", "EOR", "LSR", "NOP",
	// 0x60-0x6F
	"RTS", "ADC", "NOP", "NOP", "STZ", "ADC", "ROR", "NOP",
	"PLA", "ADC", "ROR", "NOP", "JMP", "ADC", "ROR", "NOP",
	// 0x70-0x7F
	"BVS", "ADC", "ADC", "NOP", "STZ", "ADC", "ROR", "NOP",
	"SEI", "ADC", "PLY", "NOP", "JMP", "ADC", "ROR", "NOP",
	// 0x80-0x8F
	"BRA", "STA", "NOP", "NOP", "STY", "STA", "STX", "NOP",
	"DEY", "BIT", "TXA", "NOP", "STY", "STA", "STX", "NOP",
	// 0x90-0x9F
	"BCC", "STA", "STA", "NOP", "STY", "STA", "STX", "NOP",
	"TYA", "STA", "TXS", "NOP", "STZ", "STA", "STZ", "NOP",
	// 0xA0-0xAF
	"LDY", "LDA", "LDX", "NOP", "LDY", "LDA", "LDX", "NOP",
	"TAY", "LDA", "TAX", "NOP", "LDY", "LDA", "LDX", "NOP",
	// 0xB0-0xBF
	"BCS", "LDA", "LDA", "NOP", "LDY", "LDA", "LDX", "NOP",
	"CLV", "LDA", "TSX", "NOP", "LDY", "LDA", "LDX", "NOP",
	// 0xC0-0xCF
	"CPY", "CMP", "NOP", "NOP", "CPY", "CMP", "DEC", "NOP",
	"INY", "CMP", "DEX", "WAI", "CPY", "CMP", "DEC", "NOP",
	// 0xD0-0xDF
	"BNE", "CMP", "CMP", "NOP", "NOP", "CMP", "DEC", "NOP",
	"CLD", "CMP", "PHX", "STP", "NOP", "CMP", "DEC", "NOP",
	// 0xE0-0xEF
	"CPX", "SBC", "NOP", "NOP", "CPX", "SBC", "INC", "NOP",
	"INX", "SBC", "NOP", "NOP", "CPX", "SBC", "INC", "NOP",
	// 0xF0-0xFF
	"BEQ", "SBC", "SBC", "NOP", "NOP", "SBC", "INC", "NOP",
	"SED", "SBC", "PLX", "NOP", "NOP", "SBC", "INC", "NOP",
};

/// <summary>
/// Full 256-entry addressing mode table for the WDC 65C02.
/// Must match the LynxCpu::InitOpTable() registration.
/// </summary>
constexpr LynxAddrMode _opMode[256] = {
	// 0x00-0x0F
	LynxAddrMode::None,  LynxAddrMode::IndX,  LynxAddrMode::Imm,   LynxAddrMode::Imp,
	LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Imp,
	LynxAddrMode::Imp,   LynxAddrMode::Imm,   LynxAddrMode::Acc,   LynxAddrMode::Imp,
	LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Imp,
	// 0x10-0x1F
	LynxAddrMode::Rel,   LynxAddrMode::IndY,  LynxAddrMode::ZpgInd, LynxAddrMode::Imp,
	LynxAddrMode::Zpg,   LynxAddrMode::ZpgX,  LynxAddrMode::ZpgX,  LynxAddrMode::Imp,
	LynxAddrMode::Imp,   LynxAddrMode::AbsY,  LynxAddrMode::Acc,   LynxAddrMode::Imp,
	LynxAddrMode::Abs,   LynxAddrMode::AbsX,  LynxAddrMode::AbsX,  LynxAddrMode::Imp,
	// 0x20-0x2F
	LynxAddrMode::Abs,   LynxAddrMode::IndX,  LynxAddrMode::Imm,   LynxAddrMode::Imp,
	LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Imp,
	LynxAddrMode::Imp,   LynxAddrMode::Imm,   LynxAddrMode::Acc,   LynxAddrMode::Imp,
	LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Imp,
	// 0x30-0x3F
	LynxAddrMode::Rel,   LynxAddrMode::IndY,  LynxAddrMode::ZpgInd, LynxAddrMode::Imp,
	LynxAddrMode::ZpgX,  LynxAddrMode::ZpgX,  LynxAddrMode::ZpgX,  LynxAddrMode::Imp,
	LynxAddrMode::Imp,   LynxAddrMode::AbsY,  LynxAddrMode::Acc,   LynxAddrMode::Imp,
	LynxAddrMode::AbsX,  LynxAddrMode::AbsX,  LynxAddrMode::AbsX,  LynxAddrMode::Imp,
	// 0x40-0x4F
	LynxAddrMode::Imp,   LynxAddrMode::IndX,  LynxAddrMode::Imm,   LynxAddrMode::Imp,
	LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Imp,  // $44=2-byte NOP
	LynxAddrMode::Imp,   LynxAddrMode::Imm,   LynxAddrMode::Acc,   LynxAddrMode::Imp,
	LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Imp,
	// 0x50-0x5F
	LynxAddrMode::Rel,   LynxAddrMode::IndY,  LynxAddrMode::ZpgInd, LynxAddrMode::Imp,
	LynxAddrMode::Zpg,   LynxAddrMode::ZpgX,  LynxAddrMode::ZpgX,  LynxAddrMode::Imp,  // $54=2-byte NOP
	LynxAddrMode::Imp,   LynxAddrMode::AbsY,  LynxAddrMode::Imp,   LynxAddrMode::Imp,
	LynxAddrMode::Abs,   LynxAddrMode::AbsX,  LynxAddrMode::AbsX,  LynxAddrMode::Imp,
	// 0x60-0x6F
	LynxAddrMode::Imp,   LynxAddrMode::IndX,  LynxAddrMode::Imm,   LynxAddrMode::Imp,
	LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Imp,
	LynxAddrMode::Imp,   LynxAddrMode::Imm,   LynxAddrMode::Acc,   LynxAddrMode::Imp,
	LynxAddrMode::Ind,   LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Imp,
	// 0x70-0x7F
	LynxAddrMode::Rel,   LynxAddrMode::IndY,  LynxAddrMode::ZpgInd, LynxAddrMode::Imp,
	LynxAddrMode::ZpgX,  LynxAddrMode::ZpgX,  LynxAddrMode::ZpgX,  LynxAddrMode::Imp,
	LynxAddrMode::Imp,   LynxAddrMode::AbsY,  LynxAddrMode::Imp,   LynxAddrMode::Imp,
	LynxAddrMode::AbsIndX, LynxAddrMode::AbsX, LynxAddrMode::AbsX, LynxAddrMode::Imp,
	// 0x80-0x8F
	LynxAddrMode::Rel,   LynxAddrMode::IndX,  LynxAddrMode::Imm,   LynxAddrMode::Imp,
	LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Imp,
	LynxAddrMode::Imp,   LynxAddrMode::Imm,   LynxAddrMode::Imp,   LynxAddrMode::Imp,
	LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Imp,
	// 0x90-0x9F
	LynxAddrMode::Rel,   LynxAddrMode::IndY,  LynxAddrMode::ZpgInd, LynxAddrMode::Imp,
	LynxAddrMode::ZpgX,  LynxAddrMode::ZpgX,  LynxAddrMode::ZpgY,  LynxAddrMode::Imp,
	LynxAddrMode::Imp,   LynxAddrMode::AbsY,  LynxAddrMode::Imp,   LynxAddrMode::Imp,
	LynxAddrMode::Abs,   LynxAddrMode::AbsX,  LynxAddrMode::AbsX,  LynxAddrMode::Imp,
	// 0xA0-0xAF
	LynxAddrMode::Imm,   LynxAddrMode::IndX,  LynxAddrMode::Imm,   LynxAddrMode::Imp,
	LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Imp,
	LynxAddrMode::Imp,   LynxAddrMode::Imm,   LynxAddrMode::Imp,   LynxAddrMode::Imp,
	LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Imp,
	// 0xB0-0xBF
	LynxAddrMode::Rel,   LynxAddrMode::IndY,  LynxAddrMode::ZpgInd, LynxAddrMode::Imp,
	LynxAddrMode::ZpgX,  LynxAddrMode::ZpgX,  LynxAddrMode::ZpgY,  LynxAddrMode::Imp,
	LynxAddrMode::Imp,   LynxAddrMode::AbsY,  LynxAddrMode::Imp,   LynxAddrMode::Imp,
	LynxAddrMode::AbsX,  LynxAddrMode::AbsX,  LynxAddrMode::AbsY,  LynxAddrMode::Imp,
	// 0xC0-0xCF
	LynxAddrMode::Imm,   LynxAddrMode::IndX,  LynxAddrMode::Imm,   LynxAddrMode::Imp,
	LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Imp,
	LynxAddrMode::Imp,   LynxAddrMode::Imm,   LynxAddrMode::Imp,   LynxAddrMode::Imp,
	LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Imp,
	// 0xD0-0xDF
	LynxAddrMode::Rel,   LynxAddrMode::IndY,  LynxAddrMode::ZpgInd, LynxAddrMode::Imp,
	LynxAddrMode::Zpg,   LynxAddrMode::ZpgX,  LynxAddrMode::ZpgX,  LynxAddrMode::Imp,  // $D4=2-byte NOP
	LynxAddrMode::Imp,   LynxAddrMode::AbsY,  LynxAddrMode::Imp,   LynxAddrMode::Imp,
	LynxAddrMode::Abs,   LynxAddrMode::AbsX,  LynxAddrMode::AbsX,  LynxAddrMode::Imp,
	// 0xE0-0xEF
	LynxAddrMode::Imm,   LynxAddrMode::IndX,  LynxAddrMode::Imm,   LynxAddrMode::Imp,
	LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Zpg,   LynxAddrMode::Imp,
	LynxAddrMode::Imp,   LynxAddrMode::Imm,   LynxAddrMode::Imp,   LynxAddrMode::Imp,
	LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Abs,   LynxAddrMode::Imp,
	// 0xF0-0xFF
	LynxAddrMode::Rel,   LynxAddrMode::IndY,  LynxAddrMode::ZpgInd, LynxAddrMode::Imp,
	LynxAddrMode::Zpg,   LynxAddrMode::ZpgX,  LynxAddrMode::ZpgX,  LynxAddrMode::Imp,  // $F4=2-byte NOP
	LynxAddrMode::Imp,   LynxAddrMode::AbsY,  LynxAddrMode::Imp,   LynxAddrMode::Imp,
	LynxAddrMode::Abs,   LynxAddrMode::AbsX,  LynxAddrMode::AbsX,  LynxAddrMode::Imp,
};

/// <summary>
/// Pre-computed direct opcode-to-size table, avoiding the double indirection
/// of _opMode[opCode] then _opSize[mode]. Generated at compile time.
/// </summary>
constexpr auto MakeOpSizeDirect() -> std::array<uint8_t, 256> {
	std::array<uint8_t, 256> result{};
	for (int i = 0; i < 256; i++) {
		result[i] = _opSize[static_cast<int>(_opMode[i])];
	}
	return result;
}

constexpr auto _opSizeDirect = MakeOpSizeDirect();

/// <summary>
/// Combined opcode classification flags for efficient single-lookup queries.
/// Replaces four separate switch statements with one indexed table read + bit test.
/// </summary>
enum OpClassifyFlags : uint8_t {
	CLASSIFY_UNCONDITIONAL = 1 << 0,
	CLASSIFY_CONDITIONAL   = 1 << 1,
	CLASSIFY_JUMP_TO_SUB   = 1 << 2,
	CLASSIFY_RETURN        = 1 << 3,
};

constexpr auto MakeOpClassify() -> std::array<uint8_t, 256> {
	std::array<uint8_t, 256> result{};

	// Unconditional jumps
	result[0x20] |= CLASSIFY_UNCONDITIONAL; // JSR abs
	result[0x40] |= CLASSIFY_UNCONDITIONAL; // RTI
	result[0x4c] |= CLASSIFY_UNCONDITIONAL; // JMP abs
	result[0x60] |= CLASSIFY_UNCONDITIONAL; // RTS
	result[0x6c] |= CLASSIFY_UNCONDITIONAL; // JMP (abs)
	result[0x7c] |= CLASSIFY_UNCONDITIONAL; // JMP (abs,X) — 65C02
	result[0x80] |= CLASSIFY_UNCONDITIONAL; // BRA — 65C02

	// Conditional jumps
	result[0x10] |= CLASSIFY_CONDITIONAL; // BPL
	result[0x30] |= CLASSIFY_CONDITIONAL; // BMI
	result[0x50] |= CLASSIFY_CONDITIONAL; // BVC
	result[0x70] |= CLASSIFY_CONDITIONAL; // BVS
	result[0x90] |= CLASSIFY_CONDITIONAL; // BCC
	result[0xb0] |= CLASSIFY_CONDITIONAL; // BCS
	result[0xd0] |= CLASSIFY_CONDITIONAL; // BNE
	result[0xf0] |= CLASSIFY_CONDITIONAL; // BEQ

	// Jump to sub (interrupt-like calls)
	result[0x00] |= CLASSIFY_JUMP_TO_SUB; // BRK
	result[0x20] |= CLASSIFY_JUMP_TO_SUB; // JSR abs

	// Return instructions
	result[0x60] |= CLASSIFY_RETURN; // RTS
	result[0x40] |= CLASSIFY_RETURN; // RTI

	return result;
}

constexpr auto _opClassify = MakeOpClassify();

void LynxDisUtils::GetDisassembly(DisassemblyInfo& info, string& out, uint32_t memoryAddr, LabelManager* labelManager, EmuSettings* settings) {
	FastString str;
	uint8_t* byteCode = info.GetByteCode();
	uint8_t opCode = byteCode[0];
	LynxAddrMode mode = _opMode[opCode];

	str.Write(_opName[opCode]);
	str.Write(' ');

	auto writeLabelOrAddr = [&](uint16_t addr, bool zp) {
		AddressInfo absAddr = { addr, MemoryType::LynxMemory };
		string label = labelManager ? labelManager->GetLabel(absAddr) : "";
		if (!label.empty()) {
			str.Write(label, true);
		} else if (zp) {
			str.WriteAll('$', HexUtilities::ToHex(static_cast<uint8_t>(addr)));
		} else {
			str.WriteAll('$', HexUtilities::ToHex(addr));
		}
	};

	switch (mode) {
		case LynxAddrMode::Acc:
			str.Write('A');
			break;

		case LynxAddrMode::Imm:
			str.WriteAll("#$", HexUtilities::ToHex(byteCode[1]));
			break;

		case LynxAddrMode::Rel: {
			int8_t offset = static_cast<int8_t>(byteCode[1]);
			uint16_t target = static_cast<uint16_t>(memoryAddr + 2 + offset);
			writeLabelOrAddr(target, false);
			break;
		}

		case LynxAddrMode::Zpg:
			writeLabelOrAddr(byteCode[1], true);
			break;

		case LynxAddrMode::ZpgX:
			writeLabelOrAddr(byteCode[1], true);
			str.Write(",X", 2);
			break;

		case LynxAddrMode::ZpgY:
			writeLabelOrAddr(byteCode[1], true);
			str.Write(",Y", 2);
			break;

		case LynxAddrMode::Abs:
			writeLabelOrAddr(byteCode[1] | (byteCode[2] << 8), false);
			break;

		case LynxAddrMode::AbsX:
		case LynxAddrMode::AbsXW:
			writeLabelOrAddr(byteCode[1] | (byteCode[2] << 8), false);
			str.Write(",X", 2);
			break;

		case LynxAddrMode::AbsY:
		case LynxAddrMode::AbsYW:
			writeLabelOrAddr(byteCode[1] | (byteCode[2] << 8), false);
			str.Write(",Y", 2);
			break;

		case LynxAddrMode::Ind:
			str.Write('(');
			writeLabelOrAddr(byteCode[1] | (byteCode[2] << 8), false);
			str.Write(')');
			break;

		case LynxAddrMode::IndX:
			str.Write('(');
			writeLabelOrAddr(byteCode[1], true);
			str.Write(",X)", 3);
			break;

		case LynxAddrMode::IndY:
		case LynxAddrMode::IndYW:
			str.Write('(');
			writeLabelOrAddr(byteCode[1], true);
			str.Write("),Y", 3);
			break;

		case LynxAddrMode::ZpgInd:
			str.Write('(');
			writeLabelOrAddr(byteCode[1], true);
			str.Write(')');
			break;

		case LynxAddrMode::AbsIndX:
			str.Write('(');
			writeLabelOrAddr(byteCode[1] | (byteCode[2] << 8), false);
			str.Write(",X)", 3);
			break;

		default:
			break;
	}

	out += str.ToString();
}

EffectiveAddressInfo LynxDisUtils::GetEffectiveAddress(DisassemblyInfo& info, LynxConsole* console, LynxCpuState& state) {
	bool isJump = (_opClassify[info.GetOpCode()] & (CLASSIFY_UNCONDITIONAL | CLASSIFY_CONDITIONAL)) != 0;
	if (isJump) {
		return {};
	}

	bool showEffectiveAddress = false;
	switch (_opMode[info.GetOpCode()]) {
		case LynxAddrMode::Imp:
		case LynxAddrMode::Acc:
			return {};

		case LynxAddrMode::Zpg:
		case LynxAddrMode::ZpgX:
		case LynxAddrMode::ZpgY:
		case LynxAddrMode::IndX:
		case LynxAddrMode::IndY:
		case LynxAddrMode::ZpgInd:
		case LynxAddrMode::AbsX:
		case LynxAddrMode::AbsY:
		case LynxAddrMode::AbsIndX:
			showEffectiveAddress = true;
			break;

		default:
			showEffectiveAddress = false;
			break;
	}

	// Cache DummyCpu to avoid rebuilding 256-entry op table on every call
	// thread_local for thread safety; recreated only when memory manager changes (ROM load)
	thread_local static std::unique_ptr<DummyLynxCpu> cachedCpu;
	thread_local static LynxMemoryManager* cachedMemMgr = nullptr;

	LynxMemoryManager* memMgr = console->GetMemoryManager();
	if (!cachedCpu || cachedMemMgr != memMgr) {
		cachedCpu = std::make_unique<DummyLynxCpu>(nullptr, memMgr);
		cachedMemMgr = memMgr;
	}

	cachedCpu->SetDummyState(state);
	cachedCpu->Exec();

	uint32_t count = cachedCpu->GetOperationCount();
	for (int i = count - 1; i > 0; i--) {
		MemoryOperationInfo opInfo = cachedCpu->GetOperationInfo(i);
		if (opInfo.Type != MemoryOperationType::ExecOperand && opInfo.Type != MemoryOperationType::DummyRead) {
			return {(int32_t)opInfo.Address, 1, showEffectiveAddress};
		}
	}

	return {};
}

uint8_t LynxDisUtils::GetOpSize(LynxAddrMode addrMode) {
	return _opSize[static_cast<int>(addrMode)];
}

uint8_t LynxDisUtils::GetOpSize(uint8_t opCode) {
	return _opSizeDirect[opCode];
}

char const* const LynxDisUtils::GetOpName(uint8_t opCode) {
	return _opName[opCode];
}

LynxAddrMode LynxDisUtils::GetOpMode(uint8_t opCode) {
	return _opMode[opCode];
}

bool LynxDisUtils::IsUnconditionalJump(uint8_t opCode) {
	return (_opClassify[opCode] & CLASSIFY_UNCONDITIONAL) != 0;
}

bool LynxDisUtils::IsConditionalJump(uint8_t opCode) {
	return (_opClassify[opCode] & CLASSIFY_CONDITIONAL) != 0;
}

bool LynxDisUtils::IsJumpToSub(uint8_t opCode) {
	return (_opClassify[opCode] & CLASSIFY_JUMP_TO_SUB) != 0;
}

bool LynxDisUtils::IsReturnInstruction(uint8_t opCode) {
	return (_opClassify[opCode] & CLASSIFY_RETURN) != 0;
}

CdlFlags::CdlFlags LynxDisUtils::GetOpFlags(uint8_t opCode, uint16_t pc, uint16_t prevPc) {
	uint8_t flags = _opClassify[opCode];
	if (flags & CLASSIFY_JUMP_TO_SUB) {
		return CdlFlags::SubEntryPoint;
	} else if (flags & (CLASSIFY_UNCONDITIONAL | CLASSIFY_CONDITIONAL)) {
		if (prevPc + _opSizeDirect[opCode] != pc) {
			return CdlFlags::JumpTarget;
		}
	}
	return CdlFlags::None;
}
