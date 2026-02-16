#include "pch.h"
#include <regex>
#include <unordered_map>
#include "Utilities/HexUtilities.h"
#include "Utilities/StringUtilities.h"
#include "Lynx/Debugger/LynxAssembler.h"
#include "Lynx/Debugger/LynxDisUtils.h"
#include "Debugger/LabelManager.h"

LynxAssembler::LynxAssembler(LabelManager* labelManager) : Base6502Assembler<LynxAddrMode>(labelManager, CpuType::Lynx) {
}

string LynxAssembler::GetOpName(uint8_t opcode) {
	return LynxDisUtils::GetOpName(opcode);
}

LynxAddrMode LynxAssembler::GetOpMode(uint8_t opcode) {
	return LynxDisUtils::GetOpMode(opcode);
}

bool LynxAssembler::IsOfficialOp(uint8_t opcode) {
	// All 65C02 opcodes are official (no unofficial NMOS opcodes)
	return true;
}

AssemblerSpecialCodes LynxAssembler::ResolveOpMode(AssemblerLineData& op, uint32_t instructionAddress, bool firstPass) {
	AssemblerOperand& operand = op.Operands[0];
	AssemblerOperand& operand2 = op.Operands[1];

	if (operand.ByteCount > 2 || operand2.ByteCount > 2) {
		return AssemblerSpecialCodes::InvalidOperands;
	}

	// 65C02 has no 3-operand instructions (unlike PCE's block transfer)
	if (op.Operands[2].Type != OperandType::None) {
		return AssemblerSpecialCodes::InvalidOperands;
	}

	// Two-operand forms are not used on the Lynx 65C02
	if (operand2.Type == OperandType::Custom) {
		return AssemblerSpecialCodes::InvalidOperands;
	}

	if (operand.IsImmediate) {
		if (operand.HasOpeningParenthesis || operand.ByteCount == 0 || op.OperandCount > 1) {
			return AssemblerSpecialCodes::InvalidOperands;
		} else if (operand.ByteCount > 1) {
			return AssemblerSpecialCodes::OperandOutOfRange;
		}
		op.AddrMode = IsOpModeAvailable(op.OpCode, LynxAddrMode::Rel) ? LynxAddrMode::Rel : LynxAddrMode::Imm;
	} else if (operand.HasOpeningParenthesis) {
		// Indirect modes: (nn,X), (nn),Y, (nnnn), (nn), (nnnn,X)
		if (operand2.Type == OperandType::X && operand2.HasClosingParenthesis) {
			if (operand.ByteCount == 2) {
				op.AddrMode = LynxAddrMode::AbsIndX; // JMP ($nnnn,X)
			} else if (operand.ByteCount == 1) {
				op.AddrMode = LynxAddrMode::IndX; // ($nn,X)
			} else {
				return AssemblerSpecialCodes::InvalidOperands;
			}
		} else if (operand.HasClosingParenthesis && operand2.Type == OperandType::Y) {
			op.AddrMode = LynxAddrMode::IndY; // ($nn),Y
		} else if (operand.HasClosingParenthesis) {
			if (operand.ByteCount == 2) {
				op.AddrMode = LynxAddrMode::Ind; // JMP ($nnnn)
			} else if (operand.ByteCount == 1) {
				op.AddrMode = LynxAddrMode::ZpgInd; // ($nn) — 65C02 zero page indirect
			} else {
				return AssemblerSpecialCodes::InvalidOperands;
			}
		} else {
			return AssemblerSpecialCodes::InvalidOperands;
		}
	} else if (operand.HasParenOrBracket() || operand2.HasParenOrBracket()) {
		return AssemblerSpecialCodes::ParsingError;
	} else {
		// Non-indirect, non-immediate modes
		if (operand2.Type == OperandType::X) {
			if (operand.ByteCount == 2) {
				op.AddrMode = LynxAddrMode::AbsX;
			} else if (operand.ByteCount == 1) {
				AdjustOperandSize(op, operand, LynxAddrMode::ZpgX, LynxAddrMode::AbsX);
			} else {
				return AssemblerSpecialCodes::InvalidOperands;
			}
		} else if (operand2.Type == OperandType::Y) {
			if (operand.ByteCount == 2) {
				op.AddrMode = LynxAddrMode::AbsY;
			} else if (operand.ByteCount == 1) {
				AdjustOperandSize(op, operand, LynxAddrMode::ZpgY, LynxAddrMode::AbsY);
			} else {
				return AssemblerSpecialCodes::InvalidOperands;
			}
		} else if (operand.Type == OperandType::A) {
			op.AddrMode = LynxAddrMode::Acc;
		} else if (op.OperandCount == 0) {
			op.AddrMode = IsOpModeAvailable(op.OpCode, LynxAddrMode::Acc) ? LynxAddrMode::Acc : LynxAddrMode::Imp;
		} else if (op.OperandCount == 1) {
			bool allowRelMode = IsOpModeAvailable(op.OpCode, LynxAddrMode::Rel);
			if (allowRelMode) {
				op.AddrMode = LynxAddrMode::Rel;

				// Convert absolute address to relative offset
				int16_t addressGap = (int16_t)(operand.Value - (instructionAddress + 2));
				if (addressGap > 127 || addressGap < -128) {
					if (!firstPass) {
						return AssemblerSpecialCodes::OutOfRangeJump;
					}
				}

				operand.ByteCount = 1;
				operand.Value = (uint8_t)addressGap;
			} else if (operand.ByteCount == 2) {
				op.AddrMode = LynxAddrMode::Abs;
				operand.ByteCount = 2;
			} else if (operand.ByteCount == 1) {
				AdjustOperandSize(op, operand, LynxAddrMode::Zpg, LynxAddrMode::Abs);
			} else {
				return AssemblerSpecialCodes::InvalidOperands;
			}
		} else {
			return AssemblerSpecialCodes::InvalidOperands;
		}
	}

	return AssemblerSpecialCodes::OK;
}
