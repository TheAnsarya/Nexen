#pragma once
#include "pch.h"

/// <summary>
/// Interface for CPU-specific assemblers.
/// Implemented by Nes6502Assembler, Snes65816Assembler, GbAssembler, ArmAssembler, etc.
/// </summary>
/// <remarks>
/// Used for:
/// - Debugger code editing (edit and apply)
/// - Patch creation (code modifications)
/// - Runtime code injection (Lua scripts, cheats)
///
/// Assembly process:
/// 1. Parse assembly text line-by-line
/// 2. Resolve labels and addresses
/// 3. Encode instructions to machine code
/// 4. Return assembled bytes or error code
///
/// Error handling:
/// - Positive return: Number of bytes assembled
/// - Negative return: AssemblerSpecialCodes error
/// - Zero return: End of line (normal)
/// </remarks>
class IAssembler {
public:
	virtual ~IAssembler() {}

	/// <summary>
	/// Assemble code text to machine code.
	/// </summary>
	/// <param name="code">Assembly source code (newline-separated instructions)</param>
	/// <param name="startAddress">Starting address for assembly (for relative jumps)</param>
	/// <param name="assembledCode">Output buffer for machine code bytes</param>
	/// <returns>Positive: bytes assembled, Zero: end of line, Negative: error code</returns>
	/// <remarks>
	/// Assembly syntax:
	/// - Labels: MyLabel:
	/// - Instructions: LDA #$40, STA $2000, JMP MyLabel
	/// - Hex values: $40 (lowercase hex preferred)
	/// - Binary values: %10101010
	/// - Decimal values: 64
	/// - Comments: ; This is a comment
	///
	/// Example:
	/// ```asm
	/// Start:
	///   LDA #$ff
	///   STA $2000
	///   JMP Start
	/// ```
	/// </remarks>
	virtual uint32_t AssembleCode(const string& code, uint32_t startAddress, int16_t* assembledCode) = 0;
};

/// <summary>
/// Assembly error codes (negative return values from AssembleCode).
/// </summary>
enum AssemblerSpecialCodes {
	OK = 0,                   ///< Assembly successful
	EndOfLine = -1,           ///< End of line reached (normal)
	ParsingError = -2,        ///< Syntax error
	OutOfRangeJump = -3,      ///< Branch/jump target out of range
	LabelRedefinition = -4,   ///< Label already defined
	MissingOperand = -5,      ///< Instruction requires operand
	OperandOutOfRange = -6,   ///< Operand value too large for instruction
	InvalidHex = -7,          ///< Invalid hexadecimal value
	InvalidSpaces = -8,       ///< Unexpected whitespace
	TrailingText = -9,        ///< Extra text after instruction
	UnknownLabel = -10,       ///< Label not defined
	InvalidInstruction = -11, ///< Unrecognized mnemonic
	InvalidBinaryValue = -12, ///< Invalid binary value
	InvalidOperands = -13,    ///< Operand doesn't match instruction addressing mode
	InvalidLabel = -14,       ///< Invalid label name
};
