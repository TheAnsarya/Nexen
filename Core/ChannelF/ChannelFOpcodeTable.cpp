#include "pch.h"
#include "ChannelF/ChannelFOpcodeTable.h"

namespace {
	// Helper to define an opcode entry
	void DefineOp(std::array<ChannelFOpcodeInfo, 256>& t, uint8_t op, const char* mnemonic, uint8_t size, uint8_t cycles) {
		t[op].Mnemonic = mnemonic;
		t[op].Size = size;
		t[op].Cycles = cycles;
		t[op].IsDefined = true;
	}

	std::array<ChannelFOpcodeInfo, ChannelFOpcodeTable::OpcodeCount> BuildOpcodeTable() {
		std::array<ChannelFOpcodeInfo, ChannelFOpcodeTable::OpcodeCount> table = {};

		for (uint32_t i = 0; i < table.size(); i++) {
			table[i].Opcode = (uint8_t)i;
			table[i].Mnemonic = "ill";
			table[i].Size = 1;
			table[i].Cycles = 1;
			table[i].IsDefined = false;
		}

		// $00-$07: Register transfers (LR A,KU / KL / QU / QL / KU,A / KL,A / QU,A / QL,A)
		DefineOp(table, 0x00, "lr a,ku", 1, 1);
		DefineOp(table, 0x01, "lr a,kl", 1, 1);
		DefineOp(table, 0x02, "lr a,qu", 1, 1);
		DefineOp(table, 0x03, "lr a,ql", 1, 1);
		DefineOp(table, 0x04, "lr ku,a", 1, 1);
		DefineOp(table, 0x05, "lr kl,a", 1, 1);
		DefineOp(table, 0x06, "lr qu,a", 1, 1);
		DefineOp(table, 0x07, "lr ql,a", 1, 1);

		// $08-$0B: K/P/IS transfers
		DefineOp(table, 0x08, "lr k,p", 1, 1);
		DefineOp(table, 0x09, "lr p,k", 1, 1);
		DefineOp(table, 0x0a, "lr a,is", 1, 1);
		DefineOp(table, 0x0b, "lr is,a", 1, 1);

		// $0C-$0F: PK, LR P0,Q, LR Q,DC, LR DC,Q
		DefineOp(table, 0x0c, "pk", 1, 1);
		DefineOp(table, 0x0d, "lr p0,q", 1, 1);
		DefineOp(table, 0x0e, "lr q,dc", 1, 1);
		DefineOp(table, 0x0f, "lr dc,q", 1, 1);

		// $10-$11: DC/H transfers
		DefineOp(table, 0x10, "lr dc,h", 1, 1);
		DefineOp(table, 0x11, "lr h,dc", 1, 1);

		// $12-$15: Shifts
		DefineOp(table, 0x12, "sr 1", 1, 1);
		DefineOp(table, 0x13, "sl 1", 1, 1);
		DefineOp(table, 0x14, "sr 4", 1, 1);
		DefineOp(table, 0x15, "sl 4", 1, 1);

		// $16-$17: Memory via DC0
		DefineOp(table, 0x16, "lm", 1, 1);
		DefineOp(table, 0x17, "st", 1, 1);

		// $18-$1F: Misc implied
		DefineOp(table, 0x18, "com", 1, 1);
		DefineOp(table, 0x19, "lnk", 1, 1);
		DefineOp(table, 0x1a, "di", 1, 1);
		DefineOp(table, 0x1b, "ei", 1, 1);
		DefineOp(table, 0x1c, "pop", 1, 1);
		DefineOp(table, 0x1d, "lr w,j", 1, 1);
		DefineOp(table, 0x1e, "lr j,w", 1, 1);
		DefineOp(table, 0x1f, "inc", 1, 1);

		// $20-$27: 2-byte immediate
		DefineOp(table, 0x20, "li", 2, 2);
		DefineOp(table, 0x21, "ni", 2, 2);
		DefineOp(table, 0x22, "oi", 2, 2);
		DefineOp(table, 0x23, "xi", 2, 2);
		DefineOp(table, 0x24, "ai", 2, 2);
		DefineOp(table, 0x25, "ci", 2, 2);
		DefineOp(table, 0x26, "in", 2, 2);
		DefineOp(table, 0x27, "out", 2, 2);

		// $28-$2C: 3-byte absolute / misc
		DefineOp(table, 0x28, "pi", 3, 3);
		DefineOp(table, 0x29, "jmp", 3, 3);
		DefineOp(table, 0x2a, "dci", 3, 3);
		DefineOp(table, 0x2b, "nop", 1, 1);
		DefineOp(table, 0x2c, "xdc", 1, 1);

		// $30-$3F: DS r (decrement scratchpad)
		for (uint8_t r = 0; r < 16; r++) {
			char buf[16];
			snprintf(buf, sizeof(buf), "ds %d", r);
			// Copy mnemonic to static storage
			static const char* dsMnemonics[16] = {
				"ds 0", "ds 1", "ds 2", "ds 3", "ds 4", "ds 5", "ds 6", "ds 7",
				"ds 8", "ds 9", "ds 10", "ds 11", "ds (is)", "ds (is+)", "ds (is-)", "ds 15"
			};
			DefineOp(table, (uint8_t)(0x30 + r), dsMnemonics[r], 1, 2);
		}

		// $40-$4F: LR A,r
		{
			static const char* lrMnemonics[16] = {
				"lr a,0", "lr a,1", "lr a,2", "lr a,3", "lr a,4", "lr a,5", "lr a,6", "lr a,7",
				"lr a,8", "lr a,9", "lr a,10", "lr a,11", "lr a,(is)", "lr a,(is+)", "lr a,(is-)", "lr a,15"
			};
			for (uint8_t r = 0; r < 16; r++) {
				DefineOp(table, (uint8_t)(0x40 + r), lrMnemonics[r], 1, 1);
			}
		}

		// $50-$5F: LR r,A
		{
			static const char* lraMnemonics[16] = {
				"lr 0,a", "lr 1,a", "lr 2,a", "lr 3,a", "lr 4,a", "lr 5,a", "lr 6,a", "lr 7,a",
				"lr 8,a", "lr 9,a", "lr 10,a", "lr 11,a", "lr (is),a", "lr (is+),a", "lr (is-),a", "lr 15,a"
			};
			for (uint8_t r = 0; r < 16; r++) {
				DefineOp(table, (uint8_t)(0x50 + r), lraMnemonics[r], 1, 1);
			}
		}

		// $60-$67: LISU
		for (uint8_t n = 0; n < 8; n++) {
			static const char* lisuMnemonics[8] = {
				"lisu 0", "lisu 1", "lisu 2", "lisu 3", "lisu 4", "lisu 5", "lisu 6", "lisu 7"
			};
			DefineOp(table, (uint8_t)(0x60 + n), lisuMnemonics[n], 1, 1);
		}

		// $68-$6F: LISL
		for (uint8_t n = 0; n < 8; n++) {
			static const char* lislMnemonics[8] = {
				"lisl 0", "lisl 1", "lisl 2", "lisl 3", "lisl 4", "lisl 5", "lisl 6", "lisl 7"
			};
			DefineOp(table, (uint8_t)(0x68 + n), lislMnemonics[n], 1, 1);
		}

		// $70: CLR, $71-$7F: LIS
		DefineOp(table, 0x70, "clr", 1, 1);
		{
			static const char* lisMnemonics[15] = {
				"lis 1", "lis 2", "lis 3", "lis 4", "lis 5", "lis 6", "lis 7", "lis 8",
				"lis 9", "lis 10", "lis 11", "lis 12", "lis 13", "lis 14", "lis 15"
			};
			for (uint8_t n = 1; n <= 15; n++) {
				DefineOp(table, (uint8_t)(0x70 + n), lisMnemonics[n - 1], 1, 1);
			}
		}

		// $80-$87: BT (branch on test)
		{
			static const char* btMnemonics[8] = {
				"bt 0", "bp", "bc", "bt 3", "bz", "bt 5", "bt 6", "bt 7"
			};
			for (uint8_t m = 0; m < 8; m++) {
				DefineOp(table, (uint8_t)(0x80 + m), btMnemonics[m], 2, 2);
			}
		}

		// $88-$8E: Memory ALU
		DefineOp(table, 0x88, "am", 1, 2);
		DefineOp(table, 0x89, "amd", 1, 2);
		DefineOp(table, 0x8a, "nm", 1, 2);
		DefineOp(table, 0x8b, "om", 1, 2);
		DefineOp(table, 0x8c, "xm", 1, 2);
		DefineOp(table, 0x8d, "cm", 1, 2);
		DefineOp(table, 0x8e, "adc", 1, 2);

		// $8F: BR7
		DefineOp(table, 0x8f, "br7", 2, 2);

		// $90: BR, $91-$9F: BF
		DefineOp(table, 0x90, "br", 2, 2);
		{
			static const char* bfMnemonics[15] = {
				"bf 1", "bf 2", "bf 3", "bf 4", "bf 5", "bf 6", "bf 7",
				"bf 8", "bf 9", "bf 10", "bf 11", "bf 12", "bf 13", "bf 14", "bf 15"
			};
			for (uint8_t m = 1; m <= 15; m++) {
				DefineOp(table, (uint8_t)(0x90 + m), bfMnemonics[m - 1], 2, 2);
			}
		}

		// $A0-$AF: INS (input from short port)
		{
			static const char* insMnemonics[16] = {
				"ins 0", "ins 1", "ins 2", "ins 3", "ins 4", "ins 5", "ins 6", "ins 7",
				"ins 8", "ins 9", "ins 10", "ins 11", "ins 12", "ins 13", "ins 14", "ins 15"
			};
			for (uint8_t p = 0; p < 16; p++) {
				DefineOp(table, (uint8_t)(0xa0 + p), insMnemonics[p], 1, 1);
			}
		}

		// $B0-$BF: OUTS (output to short port)
		{
			static const char* outsMnemonics[16] = {
				"outs 0", "outs 1", "outs 2", "outs 3", "outs 4", "outs 5", "outs 6", "outs 7",
				"outs 8", "outs 9", "outs 10", "outs 11", "outs 12", "outs 13", "outs 14", "outs 15"
			};
			for (uint8_t p = 0; p < 16; p++) {
				DefineOp(table, (uint8_t)(0xb0 + p), outsMnemonics[p], 1, 1);
			}
		}

		// $C0-$CF: AS r (add scratchpad)
		{
			static const char* asMnemonics[16] = {
				"as 0", "as 1", "as 2", "as 3", "as 4", "as 5", "as 6", "as 7",
				"as 8", "as 9", "as 10", "as 11", "as (is)", "as (is+)", "as (is-)", "as 15"
			};
			for (uint8_t r = 0; r < 16; r++) {
				DefineOp(table, (uint8_t)(0xc0 + r), asMnemonics[r], 1, 1);
			}
		}

		// $D0-$DF: ASD r (add scratchpad decimal)
		{
			static const char* asdMnemonics[16] = {
				"asd 0", "asd 1", "asd 2", "asd 3", "asd 4", "asd 5", "asd 6", "asd 7",
				"asd 8", "asd 9", "asd 10", "asd 11", "asd (is)", "asd (is+)", "asd (is-)", "asd 15"
			};
			for (uint8_t r = 0; r < 16; r++) {
				DefineOp(table, (uint8_t)(0xd0 + r), asdMnemonics[r], 1, 2);
			}
		}

		// $E0-$EF: XS r (xor scratchpad)
		{
			static const char* xsMnemonics[16] = {
				"xs 0", "xs 1", "xs 2", "xs 3", "xs 4", "xs 5", "xs 6", "xs 7",
				"xs 8", "xs 9", "xs 10", "xs 11", "xs (is)", "xs (is+)", "xs (is-)", "xs 15"
			};
			for (uint8_t r = 0; r < 16; r++) {
				DefineOp(table, (uint8_t)(0xe0 + r), xsMnemonics[r], 1, 1);
			}
		}

		// $F0-$FF: NS r (and scratchpad)
		{
			static const char* nsMnemonics[16] = {
				"ns 0", "ns 1", "ns 2", "ns 3", "ns 4", "ns 5", "ns 6", "ns 7",
				"ns 8", "ns 9", "ns 10", "ns 11", "ns (is)", "ns (is+)", "ns (is-)", "ns 15"
			};
			for (uint8_t r = 0; r < 16; r++) {
				DefineOp(table, (uint8_t)(0xf0 + r), nsMnemonics[r], 1, 1);
			}
		}

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
	for (const ChannelFOpcodeInfo& info : g_opcodeTable) {
		count += info.IsDefined ? 1u : 0u;
	}
	return count;
}
