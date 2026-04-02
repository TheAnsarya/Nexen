#include "pch.h"
#include "ChannelF/ChannelFCpu.h"

void ChannelFCpu::Reset() {
	_a = 0;
	_w = 0;
	_isar = 0;
	_pc0 = 0; // BIOS entry point
	_pc1 = 0;
	_dc0 = 0;
	_dc1 = 0;
	_interruptsEnabled = false;
	_irqLine = false;
	_interruptVector = 0;
	_lastOpcode = 0;
	_cycleCount = 0;
	memset(_scratchpad, 0, sizeof(_scratchpad));
}

void ChannelFCpu::StepCycles(uint32_t targetCycles) {
	uint64_t endCycle = _cycleCount + targetCycles;
	while (_cycleCount < endCycle) {
		uint8_t cycles = ExecuteInstruction();
		_cycleCount += cycles;
	}
}

uint8_t ChannelFCpu::StepOne() {
	uint8_t cycles = ExecuteInstruction();
	_cycleCount += cycles;

	// Check for interrupt after non-privileged instructions
	if ((_w & FlagICB) && _irqLine && !IsPrivilegedOpcode(_lastOpcode)) {
		uint8_t irqCycles = DeliverInterrupt();
		_cycleCount += irqCycles;
		cycles += irqCycles;
	}

	return cycles;
}

void ChannelFCpu::ExportState(
	uint8_t& a, uint8_t& w, uint8_t& isar,
	uint16_t& pc0, uint16_t& pc1, uint16_t& dc0, uint16_t& dc1,
	uint64_t& cycleCount, bool& interruptsEnabled,
	uint8_t* scratchpadOut
) const {
	a = _a;
	w = _w;
	isar = _isar;
	pc0 = _pc0;
	pc1 = _pc1;
	dc0 = _dc0;
	dc1 = _dc1;
	cycleCount = _cycleCount;
	interruptsEnabled = _interruptsEnabled;
	memcpy(scratchpadOut, _scratchpad, 64);
}

void ChannelFCpu::ImportState(
	uint8_t a, uint8_t w, uint8_t isar,
	uint16_t pc0, uint16_t pc1, uint16_t dc0, uint16_t dc1,
	uint64_t cycleCount, bool interruptsEnabled,
	const uint8_t* scratchpadIn
) {
	_a = a;
	_w = w;
	_isar = isar;
	_pc0 = pc0;
	_pc1 = pc1;
	_dc0 = dc0;
	_dc1 = dc1;
	_cycleCount = cycleCount;
	_interruptsEnabled = interruptsEnabled;
	memcpy(_scratchpad, scratchpadIn, 64);
}

uint8_t ChannelFCpu::ExecuteInstruction() {
	uint8_t opcode = FetchByte();
	_lastOpcode = opcode;

	switch (opcode) {
		// ===== $00-$07: LR register transfers =====
		case 0x00: // LR A,KU
			_a = GetScratchpad(RegKU);
			return 4;
		case 0x01: // LR A,KL
			_a = GetScratchpad(RegKL);
			return 4;
		case 0x02: // LR A,QU
			_a = GetScratchpad(RegQU);
			return 4;
		case 0x03: // LR A,QL
			_a = GetScratchpad(RegQL);
			return 4;
		case 0x04: // LR KU,A
			SetScratchpad(RegKU, _a);
			return 4;
		case 0x05: // LR KL,A
			SetScratchpad(RegKL, _a);
			return 4;
		case 0x06: // LR QU,A
			SetScratchpad(RegQU, _a);
			return 4;
		case 0x07: // LR QL,A
			SetScratchpad(RegQL, _a);
			return 4;

		// ===== $08-$0B: LR K/P/IS transfers =====
		case 0x08: // LR K,P — Save PC1 to K (r12:r13)
			SetScratchpad(RegKU, (uint8_t)(_pc1 >> 8));
			SetScratchpad(RegKL, (uint8_t)(_pc1 & 0xff));
			return 16;
		case 0x09: // LR P,K — Load PC1 from K (r12:r13)
			_pc1 = (uint16_t)((GetScratchpad(RegKU) << 8) | GetScratchpad(RegKL));
			return 16;
		case 0x0a: // LR A,IS
			_a = _isar;
			return 4;
		case 0x0b: // LR IS,A
			_isar = _a & 0x3f;
			return 4;

		// ===== $0C-$0D: PK, LR P0,Q =====
		case 0x0c: { // PK — PC1←PC0, PC0←K
			_pc1 = _pc0;
			_pc0 = (uint16_t)((GetScratchpad(RegKU) << 8) | GetScratchpad(RegKL));
			return 16;
		}
		case 0x0d: // LR P0,Q — PC0←Q (r14:r15)
			_pc0 = (uint16_t)((GetScratchpad(RegQU) << 8) | GetScratchpad(RegQL));
			return 16;

		// ===== $0E-$0F: LR Q,DC / LR DC,Q =====
		case 0x0e: // LR Q,DC — Q←DC0
			SetScratchpad(RegQU, (uint8_t)(_dc0 >> 8));
			SetScratchpad(RegQL, (uint8_t)(_dc0 & 0xff));
			return 16;
		case 0x0f: // LR DC,Q — DC0←Q
			_dc0 = (uint16_t)((GetScratchpad(RegQU) << 8) | GetScratchpad(RegQL));
			return 16;

		// ===== $10-$11: LR DC,H / LR H,DC =====
		case 0x10: // LR DC,H — DC0←H (r10:r11)
			_dc0 = (uint16_t)((GetScratchpad(RegHU) << 8) | GetScratchpad(RegHL));
			return 16;
		case 0x11: // LR H,DC — H←DC0
			SetScratchpad(RegHU, (uint8_t)(_dc0 >> 8));
			SetScratchpad(RegHL, (uint8_t)(_dc0 & 0xff));
			return 16;

		// ===== $12-$15: Shift operations =====
		case 0x12: // SR 1 — Shift right 1
			_a >>= 1;
			return 4;
		case 0x13: // SL 1 — Shift left 1
			_a <<= 1;
			return 4;
		case 0x14: // SR 4 — Shift right 4
			_a >>= 4;
			return 4;
		case 0x15: // SL 4 — Shift left 4
			_a <<= 4;
			return 4;

		// ===== $16-$17: LM, ST (memory via DC0) =====
		case 0x16: // LM — A←mem[DC0], DC0++
			_a = Read(_dc0);
			_dc0++;
			return 10;
		case 0x17: // ST — mem[DC0]←A, DC0++
			Write(_dc0, _a);
			_dc0++;
			return 10;

		// ===== $18-$1F: Misc implied =====
		case 0x18: // COM — Complement A
			_a = (uint8_t)(~_a);
			SetFlags(_a);
			return 4;
		case 0x19: { // LNK — A += carry
			uint16_t result = (uint16_t)(_a + ((_w & FlagCarry) ? 1 : 0));
			SetFlagsWithCarryOverflow(result, _a, (_w & FlagCarry) ? 1 : 0);
			_a = (uint8_t)result;
			return 4;
		}
		case 0x1a: // DI — Disable interrupts
			_interruptsEnabled = false;
			_w &= ~FlagICB;
			return 8;
		case 0x1b: // EI — Enable interrupts
			_interruptsEnabled = true;
			_w |= FlagICB;
			return 8;
		case 0x1c: // POP — PC0←PC1 (return)
			_pc0 = _pc1;
			return 8;
		case 0x1d: // LR W,J — W←r9
			_w = GetScratchpad(RegJ);
			_interruptsEnabled = (_w & FlagICB) != 0;
			return 8;
		case 0x1e: // LR J,W — r9←W
			SetScratchpad(RegJ, _w);
			return 4;
		case 0x1f: { // INC — Increment accumulator
			uint16_t result = (uint16_t)(_a + 1);
			SetFlagsWithCarryOverflow(result, _a, 1);
			_a = (uint8_t)result;
			return 4;
		}

		// ===== $20-$27: 2-byte immediate operations =====
		case 0x20: { // LI imm8 — A←immediate
			_a = FetchByte();
			return 10;
		}
		case 0x21: { // NI imm8 — A &= immediate
			_a &= FetchByte();
			SetFlags(_a);
			return 10;
		}
		case 0x22: { // OI imm8 — A |= immediate
			_a |= FetchByte();
			SetFlags(_a);
			return 10;
		}
		case 0x23: { // XI imm8 — A ^= immediate
			_a ^= FetchByte();
			SetFlags(_a);
			return 10;
		}
		case 0x24: { // AI imm8 — A += immediate
			uint8_t imm = FetchByte();
			uint16_t result = (uint16_t)(_a + imm);
			SetFlagsWithCarryOverflow(result, _a, imm);
			_a = (uint8_t)result;
			return 10;
		}
		case 0x25: { // CI imm8 — Compare A with immediate
			uint8_t imm = FetchByte();
			// CI: flags from (~A + imm + 1), matching F8 hardware
			uint16_t result = (uint16_t)((uint8_t)(~_a) + imm + 1);
			uint8_t r = (uint8_t)result;
			_w &= ~FlagsMask;
			if (r == 0) _w |= FlagZero;
			if (~r & 0x80) _w |= FlagSign;
			if (result > 0xff) _w |= FlagCarry;
			if ((((uint8_t)(~_a) ^ r) & (imm ^ r) & 0x80) != 0) _w |= FlagOverflow;
			return 10;
		}
		case 0x26: { // IN imm8 — A←port[immediate]
			uint8_t port = FetchByte();
			_a = ReadPort(port);
			SetFlags(_a);
			return 16;
		}
		case 0x27: { // OUT imm8 — port[immediate]←A
			uint8_t port = FetchByte();
			WritePort(port, _a);
			return 16;
		}

		// ===== $28-$2C: 3-byte absolute / misc =====
		case 0x28: { // PI imm16 — Push & Jump (PC1←PC0, PC0←imm16)
			uint8_t high = FetchByte();
			uint8_t low = FetchByte();
			_pc1 = _pc0;
			_pc0 = (uint16_t)((high << 8) | low);
			return 26;
		}
		case 0x29: { // JMP imm16
			uint8_t high = FetchByte();
			uint8_t low = FetchByte();
			_pc0 = (uint16_t)((high << 8) | low);
			return 22;
		}
		case 0x2a: { // DCI imm16 — DC0←imm16
			uint8_t high = FetchByte();
			uint8_t low = FetchByte();
			_dc0 = (uint16_t)((high << 8) | low);
			return 24;
		}
		case 0x2b: // NOP
			return 4;
		case 0x2c: { // XDC — Swap DC0 and DC1
			uint16_t tmp = _dc0;
			_dc0 = _dc1;
			_dc1 = tmp;
			return 8;
		}
		// $2D-$2F: Undefined
		case 0x2d: case 0x2e: case 0x2f:
			return 4;

		// ===== $30-$3F: DS r — Decrement scratchpad, skip if zero =====
		case 0x30: case 0x31: case 0x32: case 0x33:
		case 0x34: case 0x35: case 0x36: case 0x37:
		case 0x38: case 0x39: case 0x3a: case 0x3b:
		case 0x3c: case 0x3d: case 0x3e: case 0x3f: {
			uint8_t reg = opcode & 0x0f;
			uint8_t val = (uint8_t)(GetScratchpad(reg) - 1);
			SetScratchpad(reg, val);
			SetFlags(val);
			return 6; // DS uses long fetch cycle (cL=6)
		}

		// ===== $40-$4F: LR A,r — Load A from scratchpad =====
		case 0x40: case 0x41: case 0x42: case 0x43:
		case 0x44: case 0x45: case 0x46: case 0x47:
		case 0x48: case 0x49: case 0x4a: case 0x4b: {
			_a = GetScratchpad(opcode & 0x0f);
			return 4;
		}
		case 0x4c: // LR A,(IS)
			_a = GetIsarReg();
			return 4;
		case 0x4d: // LR A,(IS++)
			_a = GetIsarReg();
			_isar = (_isar & 0x38) | ((_isar + 1) & 0x07);
			return 4;
		case 0x4e: // LR A,(IS--)
			_a = GetIsarReg();
			_isar = (_isar & 0x38) | ((_isar - 1) & 0x07);
			return 4;
		case 0x4f:
			_a = GetScratchpad(0x0f);
			return 4;

		// ===== $50-$5F: LR r,A — Store A to scratchpad =====
		case 0x50: case 0x51: case 0x52: case 0x53:
		case 0x54: case 0x55: case 0x56: case 0x57:
		case 0x58: case 0x59: case 0x5a: case 0x5b: {
			SetScratchpad(opcode & 0x0f, _a);
			return 4;
		}
		case 0x5c: // LR (IS),A
			SetIsarReg(_a);
			return 4;
		case 0x5d: // LR (IS++),A
			SetIsarReg(_a);
			_isar = (_isar & 0x38) | ((_isar + 1) & 0x07);
			return 4;
		case 0x5e: // LR (IS--),A
			SetIsarReg(_a);
			_isar = (_isar & 0x38) | ((_isar - 1) & 0x07);
			return 4;
		case 0x5f:
			SetScratchpad(0x0f, _a);
			return 4;

		// ===== $60-$67: LISU — Set upper 3 bits of ISAR =====
		case 0x60: case 0x61: case 0x62: case 0x63:
		case 0x64: case 0x65: case 0x66: case 0x67:
			_isar = (_isar & 0x07) | ((opcode & 0x07) << 3);
			return 4;

		// ===== $68-$6F: LISL — Set lower 3 bits of ISAR =====
		case 0x68: case 0x69: case 0x6a: case 0x6b:
		case 0x6c: case 0x6d: case 0x6e: case 0x6f:
			_isar = (_isar & 0x38) | (opcode & 0x07);
			return 4;

		// ===== $70: CLR, $71-$7F: LIS =====
		case 0x70: // CLR
			_a = 0;
			return 4;
		case 0x71: case 0x72: case 0x73: case 0x74:
		case 0x75: case 0x76: case 0x77: case 0x78:
		case 0x79: case 0x7a: case 0x7b: case 0x7c:
		case 0x7d: case 0x7e: case 0x7f: // LIS — Load immediate short (4-bit)
			_a = opcode & 0x0f;
			return 4;

		// ===== $80-$87: BT — Branch on test =====
		// Branch if (W & mask) != 0 where mask = opcode & 0x07
		case 0x80: case 0x81: case 0x82: case 0x83:
		case 0x84: case 0x85: case 0x86: case 0x87: {
			int8_t offset = (int8_t)FetchByte();
			uint8_t mask = opcode & 0x07;
			if (mask != 0 && (_w & mask) != 0) {
				_pc0 = (uint16_t)(_pc0 + offset);
				return 14; // taken: ROMC_1C(4)+ROMC_01(6)+fetch(4)
			}
			return 12; // not taken: ROMC_1C(4)+ROMC_03(4)+fetch(4)
		}

		// ===== $88-$8E: Memory ALU operations via DC0 =====
		case 0x88: { // AM — Add memory to A
			uint8_t mem = Read(_dc0);
			uint16_t result = (uint16_t)(_a + mem);
			SetFlagsWithCarryOverflow(result, _a, mem);
			_a = (uint8_t)result;
			_dc0++;
			return 10; // ROMC_02(6)+fetch(4)
		}
		case 0x89: { // AMD — Add memory decimal (BCD)
			uint8_t mem = Read(_dc0);
			uint16_t result = (uint16_t)(_a + mem);

			// BCD correction
			if ((_a & 0x0f) + (mem & 0x0f) > 9) {
				result += 6;
			}
			if (result > 0x99) {
				result += 0x60;
			}

			_w &= ~FlagsMask;
			uint8_t r = (uint8_t)result;
			if (r == 0) _w |= FlagZero;
			if (~r & 0x80) _w |= FlagSign; // complementary
			if (result > 0xff) _w |= FlagCarry;
			_a = r;
			_dc0++;
			return 10;
		}
		case 0x8a: { // NM — AND memory
			_a &= Read(_dc0);
			SetFlags(_a);
			_dc0++;
			return 10;
		}
		case 0x8b: { // OM — OR memory
			_a |= Read(_dc0);
			SetFlags(_a);
			_dc0++;
			return 10;
		}
		case 0x8c: { // XM — XOR memory
			_a ^= Read(_dc0);
			SetFlags(_a);
			_dc0++;
			return 10;
		}
		case 0x8d: { // CM — Compare memory
			uint8_t mem = Read(_dc0);
			// CM: flags from (~A + mem + 1), matching F8 hardware
			uint16_t result = (uint16_t)((uint8_t)(~_a) + mem + 1);
			uint8_t r = (uint8_t)result;
			_w &= ~FlagsMask;
			if (r == 0) _w |= FlagZero;
			if (~r & 0x80) _w |= FlagSign;
			if (result > 0xff) _w |= FlagCarry;
			if ((((uint8_t)(~_a) ^ r) & (mem ^ r) & 0x80) != 0) _w |= FlagOverflow;
			_dc0++;
			return 10;
		}
		case 0x8e: { // ADC — Add with carry (memory)
			uint8_t mem = Read(_dc0);
			uint16_t result = (uint16_t)(_a + mem + ((_w & FlagCarry) ? 1 : 0));
			SetFlagsWithCarryOverflow(result, _a, mem);
			_a = (uint8_t)result;
			_dc0++;
			return 10;
		}

		// ===== $8F: BR7 =====
		case 0x8f: { // BR7 — Branch if ISAR lower 3 bits != 7
			int8_t offset = (int8_t)FetchByte();
			if ((_isar & 0x07) != 0x07) {
				_pc0 = (uint16_t)(_pc0 + offset);
				return 10; // taken: ROMC_01(6)+fetch(4)
			}
			return 8; // not taken: ROMC_03(4)+fetch(4)
		}

		// ===== $90: BR unconditional, $91-$9F: BF — Branch on false =====
		case 0x90: { // BR — Unconditional branch
			int8_t offset = (int8_t)FetchByte();
			_pc0 = (uint16_t)(_pc0 + offset);
			return 14; // always taken: ROMC_1C(4)+ROMC_01(6)+fetch(4)
		}
		case 0x91: case 0x92: case 0x93: case 0x94:
		case 0x95: case 0x96: case 0x97: case 0x98:
		case 0x99: case 0x9a: case 0x9b: case 0x9c:
		case 0x9d: case 0x9e: case 0x9f: { // BF — Branch if (W & mask) == 0
			int8_t offset = (int8_t)FetchByte();
			uint8_t mask = opcode & 0x0f;
			if ((_w & mask) == 0) {
				_pc0 = (uint16_t)(_pc0 + offset);
				return 14; // taken: ROMC_1C(4)+ROMC_01(6)+fetch(4)
			}
			return 12; // not taken: ROMC_1C(4)+ROMC_03(4)+fetch(4)
		}

		// ===== $A0-$AF: INS — Input from port (short) =====
		// $A0-$A3: Local ports (8 cycles: ROMC_1C(4)+fetch(4))
		case 0xa0: case 0xa1: case 0xa2: case 0xa3: {
			uint8_t port = opcode & 0x0f;
			_a = ReadPort(port);
			SetFlags(_a);
			return 8;
		}
		// $A4-$AF: External ports (16 cycles: ROMC_1C(6)+ROMC_1B(6)+fetch(4))
		case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab:
		case 0xac: case 0xad: case 0xae: case 0xaf: {
			uint8_t port = opcode & 0x0f;
			_a = ReadPort(port);
			SetFlags(_a);
			return 16;
		}

		// ===== $B0-$BF: OUTS — Output to port (short) =====
		// $B0-$B3: Local ports (8 cycles: ROMC_1C(4)+fetch(4))
		case 0xb0: case 0xb1: case 0xb2: case 0xb3: {
			uint8_t port = opcode & 0x0f;
			WritePort(port, _a);
			return 8;
		}
		// $B4-$BF: External ports (16 cycles: ROMC_1C(6)+ROMC_1A(6)+fetch(4))
		case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb:
		case 0xbc: case 0xbd: case 0xbe: case 0xbf: {
			uint8_t port = opcode & 0x0f;
			WritePort(port, _a);
			return 16;
		}

		// ===== $C0-$CF: AS r — Add scratchpad to A =====
		case 0xc0: case 0xc1: case 0xc2: case 0xc3:
		case 0xc4: case 0xc5: case 0xc6: case 0xc7:
		case 0xc8: case 0xc9: case 0xca: case 0xcb: {
			uint8_t reg = opcode & 0x0f;
			uint8_t val = GetScratchpad(reg);
			uint16_t result = (uint16_t)(_a + val);
			SetFlagsWithCarryOverflow(result, _a, val);
			_a = (uint8_t)result;
			return 4;
		}
		case 0xcc: { // AS (IS)
			uint8_t val = GetIsarReg();
			uint16_t result = (uint16_t)(_a + val);
			SetFlagsWithCarryOverflow(result, _a, val);
			_a = (uint8_t)result;
			return 4;
		}
		case 0xcd: { // AS (IS++)
			uint8_t val = GetIsarReg();
			uint16_t result = (uint16_t)(_a + val);
			SetFlagsWithCarryOverflow(result, _a, val);
			_a = (uint8_t)result;
			_isar = (_isar & 0x38) | ((_isar + 1) & 0x07);
			return 4;
		}
		case 0xce: { // AS (IS--)
			uint8_t val = GetIsarReg();
			uint16_t result = (uint16_t)(_a + val);
			SetFlagsWithCarryOverflow(result, _a, val);
			_a = (uint8_t)result;
			_isar = (_isar & 0x38) | ((_isar - 1) & 0x07);
			return 4;
		}
		case 0xcf: {
			uint8_t val = GetScratchpad(0x0f);
			uint16_t result = (uint16_t)(_a + val);
			SetFlagsWithCarryOverflow(result, _a, val);
			_a = (uint8_t)result;
			return 4;
		}

		// ===== $D0-$DF: ASD r — Add scratchpad decimal (BCD) =====
		case 0xd0: case 0xd1: case 0xd2: case 0xd3:
		case 0xd4: case 0xd5: case 0xd6: case 0xd7:
		case 0xd8: case 0xd9: case 0xda: case 0xdb:
		case 0xdc: case 0xdd: case 0xde: case 0xdf: {
			uint8_t reg;
			if ((opcode & 0x0f) == 0x0c) {
				reg = _isar & 0x3f;
			} else if ((opcode & 0x0f) == 0x0d) {
				reg = _isar & 0x3f;
			} else if ((opcode & 0x0f) == 0x0e) {
				reg = _isar & 0x3f;
			} else {
				reg = opcode & 0x0f;
			}
			uint8_t val = GetScratchpad(reg);
			uint16_t result = (uint16_t)(_a + val);
			if ((_a & 0x0f) + (val & 0x0f) > 9) {
				result += 6;
			}
			if (result > 0x99) {
				result += 0x60;
			}
			_w &= ~FlagsMask;
			uint8_t r = (uint8_t)result;
			if (r == 0) _w |= FlagZero;
			if (~r & 0x80) _w |= FlagSign; // complementary
			if (result > 0xff) _w |= FlagCarry;
			_a = r;
			if ((opcode & 0x0f) == 0x0d) {
				_isar = (_isar & 0x38) | ((_isar + 1) & 0x07);
			} else if ((opcode & 0x0f) == 0x0e) {
				_isar = (_isar & 0x38) | ((_isar - 1) & 0x07);
			}
			return 8; // ROMC_1C(4)+fetch(4)
		}

		// ===== $E0-$EF: XS r — XOR scratchpad =====
		case 0xe0: case 0xe1: case 0xe2: case 0xe3:
		case 0xe4: case 0xe5: case 0xe6: case 0xe7:
		case 0xe8: case 0xe9: case 0xea: case 0xeb: {
			_a ^= GetScratchpad(opcode & 0x0f);
			SetFlags(_a);
			return 4;
		}
		case 0xec: // XS (IS)
			_a ^= GetIsarReg();
			SetFlags(_a);
			return 4;
		case 0xed: // XS (IS++)
			_a ^= GetIsarReg();
			SetFlags(_a);
			_isar = (_isar & 0x38) | ((_isar + 1) & 0x07);
			return 4;
		case 0xee: // XS (IS--)
			_a ^= GetIsarReg();
			SetFlags(_a);
			_isar = (_isar & 0x38) | ((_isar - 1) & 0x07);
			return 4;
		case 0xef:
			_a ^= GetScratchpad(0x0f);
			SetFlags(_a);
			return 4;

		// ===== $F0-$FF: NS r — AND scratchpad =====
		case 0xf0: case 0xf1: case 0xf2: case 0xf3:
		case 0xf4: case 0xf5: case 0xf6: case 0xf7:
		case 0xf8: case 0xf9: case 0xfa: case 0xfb: {
			_a &= GetScratchpad(opcode & 0x0f);
			SetFlags(_a);
			return 4;
		}
		case 0xfc: // NS (IS)
			_a &= GetIsarReg();
			SetFlags(_a);
			return 4;
		case 0xfd: // NS (IS++)
			_a &= GetIsarReg();
			SetFlags(_a);
			_isar = (_isar & 0x38) | ((_isar + 1) & 0x07);
			return 4;
		case 0xfe: // NS (IS--)
			_a &= GetIsarReg();
			SetFlags(_a);
			_isar = (_isar & 0x38) | ((_isar - 1) & 0x07);
			return 4;
		case 0xff:
			_a &= GetScratchpad(0x0f);
			SetFlags(_a);
			return 4;

		default:
			return 4;
	}
}

bool ChannelFCpu::IsPrivilegedOpcode(uint8_t opcode) {
	// Privileged instructions skip the post-instruction interrupt check.
	// From F8 specification: PK, EI, POP, LR W,J, OUT, PI, JMP, OUTS
	switch (opcode) {
		case 0x0c: // PK — Push K (PC1←PC0, PC0←(KU,KL))
		case 0x1b: // EI — Enable Interrupts
		case 0x1c: // POP — PC0←PC1
		case 0x1d: // LR W,J — Restore W from J
		case 0x27: // OUT — Output to port
		case 0x28: // PI — Push and Jump immediate
		case 0x29: // JMP — Jump immediate
			return true;
		default:
			// $B4-$BF: OUTS — Output scratchpad to port (ISAR variants)
			return (opcode >= 0xb4 && opcode <= 0xbf);
	}
}

uint8_t ChannelFCpu::DeliverInterrupt() {
	// F8 interrupt delivery sequence (MAME: ROMC_10 + ROMC_1C + ROMC_0F + ROMC_13)
	// Total: 24 cycles (4 ROMC cycles × 6 clocks each)
	// 1. Push: PC1 ← PC0 (save return address)
	_pc1 = _pc0;
	// 2. Jump: PC0 ← interrupt vector (from 3853 SMI)
	_pc0 = _interruptVector;
	// 3. Clear ICB (disable further interrupts until re-enabled)
	_w &= ~FlagICB;
	_interruptsEnabled = false;
	// 4. Acknowledge: clear IRQ line
	_irqLine = false;
	return 24;
}
