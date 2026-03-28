#include "pch.h"
#include "ChannelF/Debugger/ChannelFDisUtils.h"
#include "ChannelF/ChannelFTypes.h"
#include "Shared/EmuSettings.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/LabelManager.h"
#include "Debugger/MemoryDumper.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/FastString.h"
#include "Shared/MemoryType.h"

// === Opcode size table (256 entries) ===
// 1 byte: $00-$1F, $2B-$2F, $30-$7F, $A0-$FF
// 2 bytes: $20-$27, $80-$9F
// 3 bytes: $28-$2A
static constexpr uint8_t _opSize[256] = {
	// $00-$0F: Register transfers (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	// $10-$1F: Shift/memory/misc (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	// $20-$2F: Immediate/port (2), PI/JMP/DCI (3), NOP/XDC/undef (1)
	2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 1, 1, 1, 1, 1,
	// $30-$3F: DS r (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	// $40-$4F: LR A,r (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	// $50-$5F: LR r,A (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	// $60-$6F: LISU/LISL (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	// $70-$7F: LIS n (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	// $80-$8F: BT cond,disp (2 bytes)
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	// $90-$9F: BF cond,disp / BR (2 bytes)
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	// $A0-$AF: INS p (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	// $B0-$BF: OUTS p (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	// $C0-$CF: AS r (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	// $D0-$DF: ASD r (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	// $E0-$EF: XS r (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	// $F0-$FF: NS r (1 byte)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

// === Opcode mnemonic table (base mnemonic only) ===
static constexpr const char* _opName[256] = {
	// $00-$0F
	"LR",  "LR",  "LR",  "LR",  "LR",  "LR",  "LR",  "LR",
	"LR",  "LR",  "LR",  "LR",  "PK",  "LR",  "LR",  "LR",
	// $10-$1F
	"LR",  "LR",  "SR",  "SL",  "SR",  "SL",  "LM",  "ST",
	"COM", "LNK", "DI",  "EI",  "POP", "LR",  "LR",  "INC",
	// $20-$2F
	"LI",  "NI",  "OI",  "XI",  "AI",  "CI",  "IN",  "OUT",
	"PI",  "JMP", "DCI", "NOP", "XDC", "???", "???", "???",
	// $30-$3F
	"DS",  "DS",  "DS",  "DS",  "DS",  "DS",  "DS",  "DS",
	"DS",  "DS",  "DS",  "DS",  "DS",  "DS",  "DS",  "DS",
	// $40-$4F
	"LR",  "LR",  "LR",  "LR",  "LR",  "LR",  "LR",  "LR",
	"LR",  "LR",  "LR",  "LR",  "LR",  "LR",  "LR",  "LR",
	// $50-$5F
	"LR",  "LR",  "LR",  "LR",  "LR",  "LR",  "LR",  "LR",
	"LR",  "LR",  "LR",  "LR",  "LR",  "LR",  "LR",  "LR",
	// $60-$6F
	"LISU","LISU","LISU","LISU","LISU","LISU","LISU","LISU",
	"LISL","LISL","LISL","LISL","LISL","LISL","LISL","LISL",
	// $70-$7F
	"LIS", "LIS", "LIS", "LIS", "LIS", "LIS", "LIS", "LIS",
	"LIS", "LIS", "LIS", "LIS", "LIS", "LIS", "LIS", "LIS",
	// $80-$8F
	"BT",  "BT",  "BT",  "BT",  "BT",  "BT",  "BT",  "BT",
	"BT",  "BT",  "BT",  "BT",  "BT",  "BT",  "BT",  "BT",
	// $90-$9F (BF 0 = BR unconditional)
	"BR",  "BF",  "BF",  "BF",  "BF",  "BF",  "BF",  "BF",
	"BF",  "BF",  "BF",  "BF",  "BF",  "BF",  "BF",  "BF",
	// $A0-$AF
	"INS", "INS", "INS", "INS", "INS", "INS", "INS", "INS",
	"INS", "INS", "INS", "INS", "INS", "INS", "INS", "INS",
	// $B0-$BF
	"OUTS","OUTS","OUTS","OUTS","OUTS","OUTS","OUTS","OUTS",
	"OUTS","OUTS","OUTS","OUTS","OUTS","OUTS","OUTS","OUTS",
	// $C0-$CF
	"AS",  "AS",  "AS",  "AS",  "AS",  "AS",  "AS",  "AS",
	"AS",  "AS",  "AS",  "AS",  "AS",  "AS",  "AS",  "AS",
	// $D0-$DF
	"ASD", "ASD", "ASD", "ASD", "ASD", "ASD", "ASD", "ASD",
	"ASD", "ASD", "ASD", "ASD", "ASD", "ASD", "ASD", "ASD",
	// $E0-$EF
	"XS",  "XS",  "XS",  "XS",  "XS",  "XS",  "XS",  "XS",
	"XS",  "XS",  "XS",  "XS",  "XS",  "XS",  "XS",  "XS",
	// $F0-$FF
	"NS",  "NS",  "NS",  "NS",  "NS",  "NS",  "NS",  "NS",
	"NS",  "NS",  "NS",  "NS",  "NS",  "NS",  "NS",  "NS",
};

// === Addressing mode table ===
typedef ChannelFAddrMode M;
static constexpr ChannelFAddrMode _opMode[256] = {
	// $00-$0F: Implied register transfers
	M::Imp, M::Imp, M::Imp, M::Imp, M::Imp, M::Imp, M::Imp, M::Imp,
	M::Imp, M::Imp, M::Imp, M::Imp, M::Imp, M::Imp, M::Imp, M::Imp,
	// $10-$1F: Implied shift/memory/misc
	M::Imp, M::Imp, M::Imp, M::Imp, M::Imp, M::Imp, M::Imp, M::Imp,
	M::Imp, M::Imp, M::Imp, M::Imp, M::Imp, M::Imp, M::Imp, M::Imp,
	// $20-$2F
	M::Imm, M::Imm, M::Imm, M::Imm, M::Imm, M::Imm, M::Port, M::Port,
	M::Abs, M::Abs, M::Abs, M::Imp, M::Imp, M::None, M::None, M::None,
	// $30-$3F: DS r
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	// $40-$4F: LR A,r
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	// $50-$5F: LR r,A
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	// $60-$67: LISU, $68-$6F: LISL
	M::Isar3, M::Isar3, M::Isar3, M::Isar3, M::Isar3, M::Isar3, M::Isar3, M::Isar3,
	M::Isar3, M::Isar3, M::Isar3, M::Isar3, M::Isar3, M::Isar3, M::Isar3, M::Isar3,
	// $70-$7F: LIS n
	M::Imm4, M::Imm4, M::Imm4, M::Imm4, M::Imm4, M::Imm4, M::Imm4, M::Imm4,
	M::Imm4, M::Imm4, M::Imm4, M::Imm4, M::Imm4, M::Imm4, M::Imm4, M::Imm4,
	// $80-$8F: BT cond,disp
	M::Rel, M::Rel, M::Rel, M::Rel, M::Rel, M::Rel, M::Rel, M::Rel,
	M::Rel, M::Rel, M::Rel, M::Rel, M::Rel, M::Rel, M::Rel, M::Rel,
	// $90-$9F: BF cond,disp / BR
	M::Rel, M::Rel, M::Rel, M::Rel, M::Rel, M::Rel, M::Rel, M::Rel,
	M::Rel, M::Rel, M::Rel, M::Rel, M::Rel, M::Rel, M::Rel, M::Rel,
	// $A0-$AF: INS p
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	// $B0-$BF: OUTS p
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	// $C0-$CF: AS r
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	// $D0-$DF: ASD r
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	// $E0-$EF: XS r
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	// $F0-$FF: NS r
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
	M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg, M::Reg,
};

uint8_t ChannelFDisUtils::GetOpSize(uint8_t opCode) {
	return _opSize[opCode];
}

char const* const ChannelFDisUtils::GetOpName(uint8_t opCode) {
	return _opName[opCode];
}

ChannelFAddrMode ChannelFDisUtils::GetOpMode(uint8_t opCode) {
	return _opMode[opCode];
}

// Implied instructions with specific register operands built into the opcode
static const char* GetImpliedOperands(uint8_t opCode) {
	switch (opCode) {
		case 0x00: return " A,KU";
		case 0x01: return " A,KL";
		case 0x02: return " A,QU";
		case 0x03: return " A,QL";
		case 0x04: return " KU,A";
		case 0x05: return " KL,A";
		case 0x06: return " QU,A";
		case 0x07: return " QL,A";
		case 0x08: return " K,P";
		case 0x09: return " P,K";
		case 0x0a: return " A,IS";
		case 0x0b: return " IS,A";
		case 0x0c: return "";      // PK
		case 0x0d: return " P0,Q";
		case 0x0e: return " Q,DC";
		case 0x0f: return " DC,Q";
		case 0x10: return " DC,H";
		case 0x11: return " H,DC";
		case 0x12: return " 1";    // SR 1
		case 0x13: return " 1";    // SL 1
		case 0x14: return " 4";    // SR 4
		case 0x15: return " 4";    // SL 4
		case 0x16: return "";      // LM
		case 0x17: return "";      // ST
		case 0x18: return "";      // COM
		case 0x19: return "";      // LNK
		case 0x1a: return "";      // DI
		case 0x1b: return "";      // EI
		case 0x1c: return "";      // POP
		case 0x1d: return " W,J";
		case 0x1e: return " J,W";
		case 0x1f: return "";      // INC
		case 0x2b: return "";      // NOP
		case 0x2c: return "";      // XDC
		default:   return "";
	}
}

void ChannelFDisUtils::GetDisassembly(DisassemblyInfo& info, string& out, uint32_t memoryAddr, LabelManager* labelManager, [[maybe_unused]] EmuSettings* settings) {
	FastString str;
	uint8_t* byteCode = info.GetByteCode();
	uint8_t opCode = byteCode[0];
	ChannelFAddrMode mode = _opMode[opCode];
	uint8_t reg = opCode & 0x0f;

	// Write mnemonic
	str.Write(_opName[opCode]);

	switch (mode) {
		case ChannelFAddrMode::Imp: {
			const char* ops = GetImpliedOperands(opCode);
			if (ops[0] != '\0') {
				str.Write(ops);
			}
			break;
		}

		case ChannelFAddrMode::Reg: {
			str.Write(' ');
			string regText = std::to_string(reg);
			if (opCode >= 0x40 && opCode <= 0x4f) {
				// LR A,r
				str.Write("A,");
				str.Write(regText);
			} else if (opCode >= 0x50 && opCode <= 0x5f) {
				// LR r,A
				str.Write(regText);
				str.Write(",A");
			} else if (opCode >= 0xa0 && opCode <= 0xaf) {
				// INS p
				str.Write(regText);
			} else if (opCode >= 0xb0 && opCode <= 0xbf) {
				// OUTS p
				str.Write(regText);
			} else {
				// DS r, AS r, ASD r, XS r, NS r
				str.Write(regText);
			}
			break;
		}

		case ChannelFAddrMode::Isar3: {
			// LISU/LISL - 3-bit value
			str.Write(' ');
			string immText = std::to_string(opCode & 0x07);
			str.Write(immText);
			break;
		}

		case ChannelFAddrMode::Imm4: {
			// LIS - 4-bit immediate in opcode
			str.WriteAll(" $", HexUtilities::ToHex((uint8_t)(opCode & 0x0f)));
			break;
		}

		case ChannelFAddrMode::Imm: {
			// 8-bit immediate
			str.WriteAll(" #$", HexUtilities::ToHex(byteCode[1]));
			break;
		}

		case ChannelFAddrMode::Port: {
			// Port number
			str.WriteAll(" $", HexUtilities::ToHex(byteCode[1]));
			break;
		}

		case ChannelFAddrMode::Rel: {
			// Branch with displacement (BT/BF/BR)
			// Target = PC + 1 (past disp byte) + signed displacement
			// But PC already points past the opcode, so target = memoryAddr + 2 + (int8_t)disp
			// Actually for F8: branch target = (PC after fetching instruction) + displacement
			// PC after fetching 2 bytes = memoryAddr + 2
			int32_t target = (int32_t)(memoryAddr + 1) + (int8_t)byteCode[1];
			target &= 0xffff;

			// For BT, show condition bits
			if (opCode >= 0x80 && opCode <= 0x8f && opCode != 0x80) {
				uint8_t cond = opCode & 0x0f;
				str.WriteAll(' ', std::to_string(cond), ",");
			} else if (opCode >= 0x91 && opCode <= 0x9f) {
				uint8_t cond = opCode & 0x0f;
				str.WriteAll(' ', std::to_string(cond), ",");
			} else {
				str.Write(' ');
			}

			string label = labelManager ? labelManager->GetLabel(AddressInfo{target, MemoryType::ChannelFBiosRom}) : "";
			if (!label.empty()) {
				str.Write(label);
			} else {
				str.WriteAll('$', HexUtilities::ToHex((uint16_t)target));
			}
			break;
		}

		case ChannelFAddrMode::Abs: {
			// 16-bit address (big-endian: high byte first in F8)
			uint16_t addr = ((uint16_t)byteCode[1] << 8) | byteCode[2];
			str.Write(' ');
			string label = labelManager ? labelManager->GetLabel(AddressInfo{(int32_t)addr, MemoryType::ChannelFBiosRom}) : "";
			if (!label.empty()) {
				str.Write(label);
			} else {
				str.WriteAll('$', HexUtilities::ToHex(addr));
			}
			break;
		}

		case ChannelFAddrMode::None:
			break;
	}

	out += str.ToString();
}

EffectiveAddressInfo ChannelFDisUtils::GetEffectiveAddress([[maybe_unused]] DisassemblyInfo& info, [[maybe_unused]] ChannelFCpuState& state, [[maybe_unused]] MemoryDumper* memoryDumper) {
	// F8 doesn't have complex addressing modes like 6502
	// Memory access is through DC0 (data counter), not directly from instruction operand
	return {};
}

CdlFlags::CdlFlags ChannelFDisUtils::GetOpFlags(uint8_t opCode, [[maybe_unused]] uint16_t pc, [[maybe_unused]] uint16_t prevPc) {
	if (opCode == 0x28) { // PI (call)
		return CdlFlags::SubEntryPoint;
	}
	return CdlFlags::None;
}

bool ChannelFDisUtils::IsJumpToSub(uint8_t opCode) {
	return opCode == 0x28; // PI (Push Immediate = call subroutine)
}

bool ChannelFDisUtils::IsReturnInstruction(uint8_t opCode) {
	return opCode == 0x1c // POP (return from subroutine)
		|| opCode == 0x09 // LR P,K (also used as return)
		|| opCode == 0x0c; // PK (push K to PC, sometimes used as call/return)
}

bool ChannelFDisUtils::IsUnconditionalJump(uint8_t opCode) {
	switch (opCode) {
		case 0x28: return true; // PI (call)
		case 0x29: return true; // JMP
		case 0x1c: return true; // POP (return)
		case 0x09: return true; // LR P,K (return)
		case 0x0c: return true; // PK
		case 0x90: return true; // BR (unconditional branch)
		default:   return false;
	}
}

bool ChannelFDisUtils::IsConditionalJump(uint8_t opCode) {
	// BT $81-$8F (conditional branch true, excluding $80 which never branches)
	if (opCode >= 0x81 && opCode <= 0x8f) return true;
	// BF $91-$9F (conditional branch false)
	if (opCode >= 0x91 && opCode <= 0x9f) return true;
	return false;
}
