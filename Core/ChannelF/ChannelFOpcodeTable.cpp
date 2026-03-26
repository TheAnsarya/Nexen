#include "pch.h"
#include "ChannelF/ChannelFOpcodeTable.h"

namespace {
	std::array<ChannelFOpcodeInfo, ChannelFOpcodeTable::OpcodeCount> BuildOpcodeTable() {
		std::array<ChannelFOpcodeInfo, ChannelFOpcodeTable::OpcodeCount> table = {};

		for(uint32_t i = 0; i < table.size(); i++) {
			table[i].Opcode = (uint8_t)i;
			table[i].Mnemonic = "ill";
			table[i].Size = 1;
			table[i].Cycles = 1;
			table[i].IsDefined = false;
		}

		// Known baseline entries used by scaffolding/tests. Full matrix lands in #973.
		table[0x00].Mnemonic = "lr a,k";
		table[0x00].Size = 1;
		table[0x00].Cycles = 1;
		table[0x00].IsDefined = true;

		table[0x08].Mnemonic = "lr k,p";
		table[0x08].Size = 1;
		table[0x08].Cycles = 1;
		table[0x08].IsDefined = true;

		table[0x2b].Mnemonic = "dci imm16";
		table[0x2b].Size = 3;
		table[0x2b].Cycles = 2;
		table[0x2b].IsDefined = true;

		table[0x90].Mnemonic = "br7 rel";
		table[0x90].Size = 2;
		table[0x90].Cycles = 2;
		table[0x90].IsDefined = true;

		table[0xf0].Mnemonic = "ins 0";
		table[0xf0].Size = 1;
		table[0xf0].Cycles = 2;
		table[0xf0].IsDefined = true;

		return table;
	}

	const std::array<ChannelFOpcodeInfo, ChannelFOpcodeTable::OpcodeCount> g_opcodeTable = BuildOpcodeTable();
}

const ChannelFOpcodeInfo& ChannelFOpcodeTable::Get(uint8_t opcode) {
	return g_opcodeTable[opcode];
}

bool ChannelFOpcodeTable::IsDefined(uint8_t opcode) {
	return g_opcodeTable[opcode].IsDefined;
}

uint32_t ChannelFOpcodeTable::GetDefinedOpcodeCount() {
	uint32_t count = 0;
	for(const ChannelFOpcodeInfo& info : g_opcodeTable) {
		count += info.IsDefined ? 1u : 0u;
	}
	return count;
}
