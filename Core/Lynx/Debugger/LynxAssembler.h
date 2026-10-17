#pragma once
#include "pch.h"
#include <regex>
#include "Debugger/Base6502Assembler.h"
#include "Lynx/LynxCpu.h"

class LabelManager;

/// <summary>
/// Atari Lynx 65C02 assembler for the debugger's code editing.
/// Delegates opcode tables to LynxDisUtils (WDC 65C02 instruction set).
/// </summary>
class LynxAssembler final : public Base6502Assembler<LynxAddrMode> {
private:
	string GetOpName(uint8_t opcode) override;
	LynxAddrMode GetOpMode(uint8_t opcode) override;
	bool IsOfficialOp(uint8_t opcode) override;
	AssemblerSpecialCodes ResolveOpMode(AssemblerLineData& op, uint32_t instructionAddress, bool firstPass) override;

public:
	LynxAssembler(LabelManager* labelManager);
	virtual ~LynxAssembler() = default;
};
