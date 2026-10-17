#include "pch.h"
#include <regex>
#include <unordered_map>
#include "Utilities/HexUtilities.h"
#include "Utilities/StringUtilities.h"
#include "Atari2600/Debugger/Atari2600Assembler.h"
#include "Atari2600/Debugger/Atari2600DisUtils.h"
#include "Debugger/LabelManager.h"

Atari2600Assembler::Atari2600Assembler(LabelManager* labelManager) : Base6502Assembler<Atari2600AddrMode>(labelManager, CpuType::Atari2600) {
}

string Atari2600Assembler::GetOpName(uint8_t opcode) {
	return Atari2600DisUtils::GetOpName(opcode);
}

bool Atari2600Assembler::IsOfficialOp(uint8_t opcode) {
	return !Atari2600DisUtils::IsOpUnofficial(opcode);
}

Atari2600AddrMode Atari2600Assembler::GetOpMode(uint8_t opcode) {
	Atari2600AddrMode mode = Atari2600DisUtils::GetOpMode(opcode);
	switch (mode) {
		case Atari2600AddrMode::AbsXW:
			return Atari2600AddrMode::AbsX;
		case Atari2600AddrMode::AbsYW:
			return Atari2600AddrMode::AbsY;
		case Atari2600AddrMode::IndYW:
			return Atari2600AddrMode::IndY;
		default:
			return mode;
	}
}

AssemblerSpecialCodes Atari2600Assembler::ResolveOpMode(AssemblerLineData& op, uint32_t instructionAddress, bool firstPass) {
	if (op.OperandCount > 2) {
		return AssemblerSpecialCodes::InvalidOperands;
	} else if (op.OperandCount == 2 && op.Operands[1].Type != OperandType::X && op.Operands[1].Type != OperandType::Y) {
		return AssemblerSpecialCodes::InvalidOperands;
	}

	AssemblerOperand& operand = op.Operands[0];
	AssemblerOperand& operand2 = op.Operands[1];
	if (operand.ByteCount > 2 || operand2.ByteCount > 0) {
		return AssemblerSpecialCodes::InvalidOperands;
	}

	if (operand.IsImmediate) {
		if (operand.HasParenOrBracket() || operand.ByteCount == 0 || op.OperandCount > 1) {
			return AssemblerSpecialCodes::ParsingError;
		} else if (operand.ByteCount > 1) {
			return AssemblerSpecialCodes::OperandOutOfRange;
		}
		op.AddrMode = IsOpModeAvailable(op.OpCode, Atari2600AddrMode::Rel) ? Atari2600AddrMode::Rel : Atari2600AddrMode::Imm;
	} else if (operand.HasOpeningParenthesis) {
		if (operand2.Type == OperandType::X && operand2.HasClosingParenthesis && operand.ByteCount == 1) {
			op.AddrMode = Atari2600AddrMode::IndX;
		} else if (operand.HasClosingParenthesis && operand2.Type == OperandType::Y && operand.ByteCount == 1) {
			op.AddrMode = Atari2600AddrMode::IndY;
		} else if (operand.HasClosingParenthesis && operand.ByteCount > 0) {
			op.AddrMode = Atari2600AddrMode::Ind;
			operand.ByteCount = 2;
		} else {
			return AssemblerSpecialCodes::ParsingError;
		}
	} else if (operand.HasParenOrBracket() || operand2.HasParenOrBracket()) {
		return AssemblerSpecialCodes::ParsingError;
	} else {
		if (operand2.Type == OperandType::X) {
			if (operand.ByteCount == 2) {
				op.AddrMode = Atari2600AddrMode::AbsX;
			} else if (operand.ByteCount == 1) {
				AdjustOperandSize(op, operand, Atari2600AddrMode::ZeroX, Atari2600AddrMode::AbsX);
			} else {
				return AssemblerSpecialCodes::ParsingError;
			}
		} else if (operand2.Type == OperandType::Y) {
			if (operand.ByteCount == 2) {
				op.AddrMode = Atari2600AddrMode::AbsY;
			} else if (operand.ByteCount == 1) {
				AdjustOperandSize(op, operand, Atari2600AddrMode::ZeroY, Atari2600AddrMode::AbsY);
			} else {
				return AssemblerSpecialCodes::ParsingError;
			}
		} else if (operand.Type == OperandType::A) {
			op.AddrMode = Atari2600AddrMode::Acc;
		} else if (op.OperandCount == 0) {
			op.AddrMode = IsOpModeAvailable(op.OpCode, Atari2600AddrMode::Acc) ? Atari2600AddrMode::Acc : Atari2600AddrMode::Imp;
		} else if (op.OperandCount == 1) {
			if (IsOpModeAvailable(op.OpCode, Atari2600AddrMode::Rel)) {
				op.AddrMode = Atari2600AddrMode::Rel;

				int16_t addressGap = operand.Value - (instructionAddress + 2);
				if (addressGap > 127 || addressGap < -128) {
					if (!firstPass) {
						return AssemblerSpecialCodes::OutOfRangeJump;
					}
				}

				operand.ByteCount = 1;
				operand.Value = (uint8_t)addressGap;
			} else if (operand.ByteCount == 2) {
				op.AddrMode = Atari2600AddrMode::Abs;
			} else if (operand.ByteCount == 1) {
				AdjustOperandSize(op, operand, Atari2600AddrMode::Zero, Atari2600AddrMode::Abs);
			} else {
				return AssemblerSpecialCodes::ParsingError;
			}
		} else {
			return AssemblerSpecialCodes::ParsingError;
		}
	}

	return AssemblerSpecialCodes::OK;
}
