#pragma once
#include "pch.h"

/// <summary>
/// ARM instruction categories for GBA ARM7TDMI CPU disassembly and execution.
/// Used by disassembler to classify opcode families.
/// </summary>
enum class ArmOpCategory {
	BranchExchangeRegister, ///< BX - Branch and exchange (switch ARM/Thumb mode)
	Branch,                 ///< B, BL - Unconditional/conditional branches
	Msr,                    ///< MSR - Move to status register (CPSR/SPSR)
	Mrs,                    ///< MRS - Move from status register
	DataProcessing,         ///< ALU operations (ADD, SUB, AND, ORR, etc.)
	Multiply,               ///< MUL, MLA - 32-bit multiply
	MultiplyLong,           ///< MULL, MLAL - 64-bit multiply
	SingleDataTransfer,     ///< LDR, STR - Load/store word/byte
	SignedHalfDataTransfer, ///< LDRH, STRH, LDRSB, LDRSH - Halfword/signed loads
	BlockDataTransfer,      ///< LDM, STM - Load/store multiple registers
	SingleDataSwap,         ///< SWP - Atomic swap (semaphore support)
	SoftwareInterrupt,      ///< SWI - Software interrupt (BIOS call)
	InvalidOp,              ///< Invalid/undefined opcode
};

/// <summary>
/// ARM ALU operations for data processing instructions.
/// Maps to opcode bits [24:21] in ARM data processing format.
/// </summary>
/// <remarks>
/// Encoded as 4-bit value in ARM instruction word.
/// Stored as uint8_t for memory efficiency.
/// Used by ARM7TDMI emulation core (Game Boy Advance).
/// </remarks>
enum class ArmAluOperation : uint8_t {
	And, ///< Logical AND
	Eor, ///< Logical exclusive OR
	Sub, ///< Subtract
	Rsb, ///< Reverse subtract (operand2 - operand1)

	Add, ///< Add
	Adc, ///< Add with carry
	Sbc, ///< Subtract with carry
	Rsc, ///< Reverse subtract with carry

	Tst, ///< Test bits (AND, discards result, updates flags)
	Teq, ///< Test equality (EOR, discards result, updates flags)
	Cmp, ///< Compare (SUB, discards result, updates flags)
	Cmn, ///< Compare negative (ADD, discards result, updates flags)

	Orr, ///< Logical OR
	Mov, ///< Move (copy operand2 to dest)
	Bic, ///< Bit clear (AND NOT)
	Mvn  ///< Move NOT (copy inverted operand2 to dest)
};
