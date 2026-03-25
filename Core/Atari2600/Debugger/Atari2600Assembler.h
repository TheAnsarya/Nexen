#pragma once
#include "pch.h"
#include <regex>
#include "Debugger/Base6502Assembler.h"
#include "Atari2600/Atari2600Types.h"

class LabelManager;

class Atari2600Assembler final : public Base6502Assembler<Atari2600AddrMode> {
private:
	string GetOpName(uint8_t opcode) override;
	bool IsOfficialOp(uint8_t opcode) override;
	Atari2600AddrMode GetOpMode(uint8_t opcode) override;
	AssemblerSpecialCodes ResolveOpMode(AssemblerLineData& op, uint32_t instructionAddress, bool firstPass) override;

public:
	Atari2600Assembler(LabelManager* labelManager);
	virtual ~Atari2600Assembler() = default;
};
