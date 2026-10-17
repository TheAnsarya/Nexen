#pragma once
#include "pch.h"

/// <summary>
/// Lightweight F8 opcode metadata scaffold used for early bring-up and debugger integration.
///
/// This table is intentionally conservative at this stage:
/// - Every opcode has a deterministic metadata entry.
/// - Unknown/undocumented opcodes are represented as "ill".
/// - Semantics and timing values are refined in issue #973.
/// </summary>
struct ChannelFOpcodeInfo {
	uint8_t Opcode = 0;
	const char* Mnemonic = "ill";
	uint8_t Size = 1;
	uint8_t Cycles = 1;
	bool IsDefined = false;
};

class ChannelFOpcodeTable {
public:
	static constexpr uint32_t OpcodeCount = 256;

	[[nodiscard]] static const ChannelFOpcodeInfo& Get(uint8_t opcode);
	[[nodiscard]] static bool IsDefined(uint8_t opcode);
	[[nodiscard]] static uint32_t GetDefinedOpcodeCount();
};
