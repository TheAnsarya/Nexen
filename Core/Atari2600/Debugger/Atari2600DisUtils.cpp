#include "pch.h"
#include "Atari2600/Debugger/Atari2600DisUtils.h"
#include "Atari2600/Atari2600Types.h"
#include "Shared/EmuSettings.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/LabelManager.h"
#include "Debugger/MemoryDumper.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/FastString.h"
#include "Shared/MemoryType.h"

// --- Opcode sizes indexed by addressing mode ordinal ---
static constexpr uint8_t _opSize[17] = {
	1, 1, 1, 2, 2,
	2, 3, 2, 2,
	3, 2, 2, 2,
	3, 3, 3, 3
};

// --- Mnemonic table (standard 6502 + unofficial) ---
static constexpr const char* _opName[256] = {
	//	0		1		2		3		4		5		6		7		8		9		A		B		C		D		E		F
	"BRK", "ORA", "STP", "SLO", "NOP", "ORA", "ASL", "SLO", "PHP", "ORA", "ASL", "ANC", "NOP", "ORA", "ASL", "SLO", // 0
	"BPL", "ORA", "STP", "SLO", "NOP", "ORA", "ASL", "SLO", "CLC", "ORA", "NOP", "SLO", "NOP", "ORA", "ASL", "SLO", // 1
	"JSR", "AND", "STP", "RLA", "BIT", "AND", "ROL", "RLA", "PLP", "AND", "ROL", "ANC", "BIT", "AND", "ROL", "RLA", // 2
	"BMI", "AND", "STP", "RLA", "NOP", "AND", "ROL", "RLA", "SEC", "AND", "NOP", "RLA", "NOP", "AND", "ROL", "RLA", // 3
	"RTI", "EOR", "STP", "SRE", "NOP", "EOR", "LSR", "SRE", "PHA", "EOR", "LSR", "ALR", "JMP", "EOR", "LSR", "SRE", // 4
	"BVC", "EOR", "STP", "SRE", "NOP", "EOR", "LSR", "SRE", "CLI", "EOR", "NOP", "SRE", "NOP", "EOR", "LSR", "SRE", // 5
	"RTS", "ADC", "STP", "RRA", "NOP", "ADC", "ROR", "RRA", "PLA", "ADC", "ROR", "ARR", "JMP", "ADC", "ROR", "RRA", // 6
	"BVS", "ADC", "STP", "RRA", "NOP", "ADC", "ROR", "RRA", "SEI", "ADC", "NOP", "RRA", "NOP", "ADC", "ROR", "RRA", // 7
	"NOP", "STA", "NOP", "SAX", "STY", "STA", "STX", "SAX", "DEY", "NOP", "TXA", "ANE", "STY", "STA", "STX", "SAX", // 8
	"BCC", "STA", "STP", "SHA", "STY", "STA", "STX", "SAX", "TYA", "STA", "TXS", "TAS", "SHY", "STA", "SHX", "SHA", // 9
	"LDY", "LDA", "LDX", "LAX", "LDY", "LDA", "LDX", "LAX", "TAY", "LDA", "TAX", "LAX", "LDY", "LDA", "LDX", "LAX", // A
	"BCS", "LDA", "STP", "LAX", "LDY", "LDA", "LDX", "LAX", "CLV", "LDA", "TSX", "LAS", "LDY", "LDA", "LDX", "LAX", // B
	"CPY", "CMP", "NOP", "DCP", "CPY", "CMP", "DEC", "DCP", "INY", "CMP", "DEX", "SBX", "CPY", "CMP", "DEC", "DCP", // C
	"BNE", "CMP", "STP", "DCP", "NOP", "CMP", "DEC", "DCP", "CLD", "CMP", "NOP", "DCP", "NOP", "CMP", "DEC", "DCP", // D
	"CPX", "SBC", "NOP", "ISC", "CPX", "SBC", "INC", "ISC", "INX", "SBC", "NOP", "SBC", "CPX", "SBC", "INC", "ISC", // E
	"BEQ", "SBC", "STP", "ISC", "NOP", "SBC", "INC", "ISC", "SED", "SBC", "NOP", "ISC", "NOP", "SBC", "INC", "ISC"  // F
};

typedef Atari2600AddrMode M;
static constexpr Atari2600AddrMode _opMode[] = {
	//	0		1		2		3		4		5		6		7		8		9		A		B		C		D		E		F
	M::Imp, M::IndX, M::None, M::IndX, M::Zero, M::Zero, M::Zero, M::Zero, M::Imp, M::Imm, M::Acc, M::Imm, M::Abs, M::Abs, M::Abs, M::Abs,                   // 0
	M::Rel, M::IndY, M::None, M::IndYW, M::ZeroX, M::ZeroX, M::ZeroX, M::ZeroX, M::Imp, M::AbsY, M::Imp, M::AbsYW, M::AbsX, M::AbsX, M::AbsXW, M::AbsXW,     // 1
	M::Abs, M::IndX, M::None, M::IndX, M::Zero, M::Zero, M::Zero, M::Zero, M::Imp, M::Imm, M::Acc, M::Imm, M::Abs, M::Abs, M::Abs, M::Abs,                   // 2
	M::Rel, M::IndY, M::None, M::IndYW, M::ZeroX, M::ZeroX, M::ZeroX, M::ZeroX, M::Imp, M::AbsY, M::Imp, M::AbsYW, M::AbsX, M::AbsX, M::AbsXW, M::AbsXW,     // 3
	M::Imp, M::IndX, M::None, M::IndX, M::Zero, M::Zero, M::Zero, M::Zero, M::Imp, M::Imm, M::Acc, M::Imm, M::Abs, M::Abs, M::Abs, M::Abs,                   // 4
	M::Rel, M::IndY, M::None, M::IndYW, M::ZeroX, M::ZeroX, M::ZeroX, M::ZeroX, M::Imp, M::AbsY, M::Imp, M::AbsYW, M::AbsX, M::AbsX, M::AbsXW, M::AbsXW,     // 5
	M::Imp, M::IndX, M::None, M::IndX, M::Zero, M::Zero, M::Zero, M::Zero, M::Imp, M::Imm, M::Acc, M::Imm, M::Ind, M::Abs, M::Abs, M::Abs,                   // 6
	M::Rel, M::IndY, M::None, M::IndYW, M::ZeroX, M::ZeroX, M::ZeroX, M::ZeroX, M::Imp, M::AbsY, M::Imp, M::AbsYW, M::AbsX, M::AbsX, M::AbsXW, M::AbsXW,     // 7
	M::Imm, M::IndX, M::Imm, M::IndX, M::Zero, M::Zero, M::Zero, M::Zero, M::Imp, M::Imm, M::Imp, M::Imm, M::Abs, M::Abs, M::Abs, M::Abs,                    // 8
	M::Rel, M::IndYW, M::None, M::IndYW, M::ZeroX, M::ZeroX, M::ZeroY, M::ZeroY, M::Imp, M::AbsYW, M::Imp, M::AbsYW, M::AbsXW, M::AbsXW, M::AbsYW, M::AbsYW, // 9
	M::Imm, M::IndX, M::Imm, M::IndX, M::Zero, M::Zero, M::Zero, M::Zero, M::Imp, M::Imm, M::Imp, M::Imm, M::Abs, M::Abs, M::Abs, M::Abs,                    // A
	M::Rel, M::IndY, M::None, M::IndY, M::ZeroX, M::ZeroX, M::ZeroY, M::ZeroY, M::Imp, M::AbsY, M::Imp, M::AbsY, M::AbsX, M::AbsX, M::AbsY, M::AbsY,         // B
	M::Imm, M::IndX, M::Imm, M::IndX, M::Zero, M::Zero, M::Zero, M::Zero, M::Imp, M::Imm, M::Imp, M::Imm, M::Abs, M::Abs, M::Abs, M::Abs,                    // C
	M::Rel, M::IndY, M::None, M::IndYW, M::ZeroX, M::ZeroX, M::ZeroX, M::ZeroX, M::Imp, M::AbsY, M::Imp, M::AbsYW, M::AbsX, M::AbsX, M::AbsXW, M::AbsXW,     // D
	M::Imm, M::IndX, M::Imm, M::IndX, M::Zero, M::Zero, M::Zero, M::Zero, M::Imp, M::Imm, M::Imp, M::Imm, M::Abs, M::Abs, M::Abs, M::Abs,                    // E
	M::Rel, M::IndY, M::None, M::IndYW, M::ZeroX, M::ZeroX, M::ZeroX, M::ZeroX, M::Imp, M::AbsY, M::Imp, M::AbsYW, M::AbsX, M::AbsX, M::AbsXW, M::AbsXW,     // F
};

static constexpr bool _isUnofficial[256] = {
	//	0		1		2		3		4		5		6		7		8		9		A		B		C		D		E		F
	false, false, true,  true,  true,  false, false, true,  false, false, false, true,  true,  false, false, true,  // 0
	false, false, true,  true,  true,  false, false, true,  false, false, true,  true,  true,  false, false, true,  // 1
	false, false, true,  true,  false, false, false, true,  false, false, false, true,  false, false, false, true,  // 2
	false, false, true,  true,  true,  false, false, true,  false, false, true,  true,  true,  false, false, true,  // 3
	false, false, true,  true,  true,  false, false, true,  false, false, false, true,  false, false, false, true,  // 4
	false, false, true,  true,  true,  false, false, true,  false, false, true,  true,  true,  false, false, true,  // 5
	false, false, true,  true,  true,  false, false, true,  false, false, false, true,  false, false, false, true,  // 6
	false, false, true,  true,  true,  false, false, true,  false, false, true,  true,  true,  false, false, true,  // 7
	true,  false, true,  true,  false, false, false, true,  false, true,  false, true,  false, false, false, true,  // 8
	false, false, true,  true,  false, false, false, true,  false, false, false, true,  true,  false, true,  true,  // 9
	false, false, false, true,  false, false, false, true,  false, false, false, true,  false, false, false, true,  // A
	false, false, true,  true,  false, false, false, true,  false, false, false, true,  false, false, false, true,  // B
	false, false, true,  true,  false, false, false, true,  false, false, false, true,  false, false, false, true,  // C
	false, false, true,  true,  true,  false, false, true,  false, false, true,  true,  true,  false, false, true,  // D
	false, false, true,  true,  false, false, false, true,  false, false, false, true,  false, false, false, true,  // E
	false, false, true,  true,  true,  false, false, true,  false, false, true,  true,  true,  false, false, true   // F
};

uint8_t Atari2600DisUtils::GetOpSize(Atari2600AddrMode addrMode) {
	return _opSize[(int)addrMode];
}

uint8_t Atari2600DisUtils::GetOpSize(uint8_t opCode) {
	return GetOpSize(_opMode[opCode]);
}

char const* const Atari2600DisUtils::GetOpName(uint8_t opCode) {
	return _opName[opCode];
}

Atari2600AddrMode Atari2600DisUtils::GetOpMode(uint8_t opCode) {
	return _opMode[opCode];
}

bool Atari2600DisUtils::IsOpUnofficial(uint8_t opCode) {
	return _isUnofficial[opCode];
}

uint32_t Atari2600DisUtils::GetOperandAddress(DisassemblyInfo& info, uint32_t memoryAddr) {
	uint8_t* byteCode = info.GetByteCode();
	Atari2600AddrMode addrMode = _opMode[byteCode[0]];

	if (addrMode != Atari2600AddrMode::Imm) {
		uint8_t opSize = info.GetOpSize();
		if (opSize == 3 || addrMode == Atari2600AddrMode::Rel) {
			if (addrMode == Atari2600AddrMode::Rel) {
				return (uint32_t)((int32_t)((int16_t)memoryAddr + (int8_t)byteCode[1] + 2) & 0x1FFF);
			}
			return (byteCode[1] | (byteCode[2] << 8)) & 0x1FFF;
		}
		return byteCode[1];
	}
	return 0;
}

void Atari2600DisUtils::GetDisassembly(DisassemblyInfo& info, string& out, uint32_t memoryAddr, LabelManager* labelManager, EmuSettings* settings) {
	FastString str;
	uint8_t* byteCode = info.GetByteCode();
	uint8_t opCode = byteCode[0];
	Atari2600AddrMode addrMode = _opMode[opCode];

	str.Write(_opName[opCode], 3);
	str.Write(' ');

	uint32_t opAddr = GetOperandAddress(info, memoryAddr);
	string label = labelManager ? labelManager->GetLabel(AddressInfo{(int32_t)opAddr, MemoryType::Atari2600Memory}) : "";

	auto writeAddr = [&](bool useLabel) {
		if (useLabel && !label.empty()) {
			str.Write(label);
		} else if (info.GetOpSize() == 3 || addrMode == Atari2600AddrMode::Rel) {
			str.WriteAll('$', HexUtilities::ToHex((uint16_t)opAddr));
		} else {
			str.WriteAll('$', HexUtilities::ToHex((uint8_t)opAddr));
		}
	};

	switch (addrMode) {
		case Atari2600AddrMode::Acc:
			str.Write('A');
			break;
		case Atari2600AddrMode::Imm:
			str.WriteAll("#$", HexUtilities::ToHex(byteCode[1]));
			break;
		case Atari2600AddrMode::Ind:
			str.Write('(');
			writeAddr(true);
			str.Write(')');
			break;
		case Atari2600AddrMode::IndX:
			str.Write('(');
			writeAddr(true);
			str.Write(",X)");
			break;
		case Atari2600AddrMode::IndY:
		case Atari2600AddrMode::IndYW:
			str.Write('(');
			writeAddr(true);
			str.Write("),Y");
			break;
		case Atari2600AddrMode::Abs:
		case Atari2600AddrMode::Rel:
		case Atari2600AddrMode::Zero:
			writeAddr(true);
			break;
		case Atari2600AddrMode::AbsX:
		case Atari2600AddrMode::AbsXW:
		case Atari2600AddrMode::ZeroX:
			writeAddr(true);
			str.Write(",X");
			break;
		case Atari2600AddrMode::AbsY:
		case Atari2600AddrMode::AbsYW:
		case Atari2600AddrMode::ZeroY:
			writeAddr(true);
			str.Write(",Y");
			break;
		default:
			break;
	}

	out += str.ToString();
}

EffectiveAddressInfo Atari2600DisUtils::GetEffectiveAddress(DisassemblyInfo& info, Atari2600CpuState& state, MemoryDumper* memoryDumper) {
	uint8_t* byteCode = info.GetByteCode();
	Atari2600AddrMode addrMode = _opMode[byteCode[0]];

	switch (addrMode) {
		case Atari2600AddrMode::Zero:
			return {byteCode[1], 1, true};
		case Atari2600AddrMode::ZeroX:
			return {(uint8_t)(byteCode[1] + state.X), 1, true};
		case Atari2600AddrMode::ZeroY:
			return {(uint8_t)(byteCode[1] + state.Y), 1, true};
		case Atari2600AddrMode::Abs:
			return {(int32_t)((byteCode[1] | (byteCode[2] << 8)) & 0x1FFF), 1, true};
		case Atari2600AddrMode::AbsX:
		case Atari2600AddrMode::AbsXW:
			return {(int32_t)(((byteCode[1] | (byteCode[2] << 8)) + state.X) & 0x1FFF), 1, true};
		case Atari2600AddrMode::AbsY:
		case Atari2600AddrMode::AbsYW:
			return {(int32_t)(((byteCode[1] | (byteCode[2] << 8)) + state.Y) & 0x1FFF), 1, true};
		case Atari2600AddrMode::Ind: {
			uint16_t addr = byteCode[1] | (byteCode[2] << 8);
			uint8_t lo = memoryDumper->GetMemoryValue(MemoryType::Atari2600Memory, addr);
			// 6502 indirect JMP bug: page boundary wraps within page
			uint8_t hi = memoryDumper->GetMemoryValue(MemoryType::Atari2600Memory, (addr & 0xff00) | ((addr + 1) & 0xff));
			return {(int32_t)((lo | (hi << 8)) & 0x1FFF), 1, true};
		}
		case Atari2600AddrMode::IndX: {
			uint8_t zp = (uint8_t)(byteCode[1] + state.X);
			uint8_t lo = memoryDumper->GetMemoryValue(MemoryType::Atari2600Memory, zp);
			uint8_t hi = memoryDumper->GetMemoryValue(MemoryType::Atari2600Memory, (uint8_t)(zp + 1));
			return {(int32_t)((lo | (hi << 8)) & 0x1FFF), 1, true};
		}
		case Atari2600AddrMode::IndY:
		case Atari2600AddrMode::IndYW: {
			uint8_t lo = memoryDumper->GetMemoryValue(MemoryType::Atari2600Memory, byteCode[1]);
			uint8_t hi = memoryDumper->GetMemoryValue(MemoryType::Atari2600Memory, (uint8_t)(byteCode[1] + 1));
			return {(int32_t)(((lo | (hi << 8)) + state.Y) & 0x1FFF), 1, true};
		}
		default:
			return {};
	}
}

CdlFlags::CdlFlags Atari2600DisUtils::GetOpFlags(uint8_t opCode, uint16_t pc, uint16_t prevPc) {
	switch (opCode) {
		case 0x20: // JSR
			return CdlFlags::SubEntryPoint;
		case 0x00: // BRK
			return CdlFlags::SubEntryPoint;
		default:
			return CdlFlags::None;
	}
}

bool Atari2600DisUtils::IsJumpToSub(uint8_t opCode) {
	return opCode == 0x20; // JSR
}

bool Atari2600DisUtils::IsReturnInstruction(uint8_t opCode) {
	return opCode == 0x60 || opCode == 0x40; // RTS, RTI
}

bool Atari2600DisUtils::IsUnconditionalJump(uint8_t opCode) {
	switch (opCode) {
		case 0x4C: // JMP abs
		case 0x6C: // JMP ind
		case 0x20: // JSR
		case 0x60: // RTS
		case 0x40: // RTI
			return true;
		default:
			return false;
	}
}

bool Atari2600DisUtils::IsConditionalJump(uint8_t opCode) {
	switch (opCode) {
		case 0x10: // BPL
		case 0x30: // BMI
		case 0x50: // BVC
		case 0x70: // BVS
		case 0x90: // BCC
		case 0xB0: // BCS
		case 0xD0: // BNE
		case 0xF0: // BEQ
			return true;
		default:
			return false;
	}
}
