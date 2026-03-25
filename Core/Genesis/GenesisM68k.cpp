#include "pch.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Shared/Emulator.h"
#include "Utilities/Serializer.h"

void GenesisM68k::Init(Emulator* emu, GenesisConsole* console, GenesisMemoryManager* memoryManager) {
	_emu = emu;
	_console = console;
	_memoryManager = memoryManager;
}

// ===== Size helpers =====

uint32_t GenesisM68k::SizeMask(uint8_t size) {
	switch (size) {
		case 0: return 0xFF;
		case 1: return 0xFFFF;
		default: return 0xFFFFFFFF;
	}
}

uint32_t GenesisM68k::SignBit(uint8_t size) {
	switch (size) {
		case 0: return 0x80;
		case 1: return 0x8000;
		default: return 0x80000000;
	}
}

bool GenesisM68k::IsNeg(uint32_t value, uint8_t size) {
	return (value & SignBit(size)) != 0;
}

// ===== Memory access (delegates to memory manager) =====

uint8_t GenesisM68k::Read8(uint32_t addr) {
	addr &= 0xFFFFFF; // 24-bit address bus
	AddCycles(4);
	return _memoryManager->Read8(addr);
}

uint16_t GenesisM68k::Read16(uint32_t addr) {
	addr &= 0xFFFFFE; // Word-aligned
	AddCycles(4);
	return _memoryManager->Read16(addr);
}

uint32_t GenesisM68k::Read32(uint32_t addr) {
	uint16_t hi = Read16(addr);
	uint16_t lo = Read16(addr + 2);
	return ((uint32_t)hi << 16) | lo;
}

void GenesisM68k::Write8(uint32_t addr, uint8_t value) {
	addr &= 0xFFFFFF;
	AddCycles(4);
	_memoryManager->Write8(addr, value);
}

void GenesisM68k::Write16(uint32_t addr, uint16_t value) {
	addr &= 0xFFFFFE;
	AddCycles(4);
	_memoryManager->Write16(addr, value);
}

void GenesisM68k::Write32(uint32_t addr, uint32_t value) {
	Write16(addr, (uint16_t)(value >> 16));
	Write16(addr + 2, (uint16_t)(value & 0xFFFF));
}

// ===== Stack =====

void GenesisM68k::Push16(uint16_t value) {
	SP() -= 2;
	Write16(SP(), value);
}

void GenesisM68k::Push32(uint32_t value) {
	SP() -= 4;
	Write32(SP(), value);
}

uint16_t GenesisM68k::Pop16() {
	uint16_t val = Read16(SP());
	SP() += 2;
	return val;
}

uint32_t GenesisM68k::Pop32() {
	uint32_t val = Read32(SP());
	SP() += 4;
	return val;
}

// ===== Instruction fetch =====

uint16_t GenesisM68k::FetchOpcode() {
	uint16_t op = Read16(_state.PC);
	_state.PC += 2;
	return op;
}

uint16_t GenesisM68k::FetchWord() {
	uint16_t w = Read16(_state.PC);
	_state.PC += 2;
	return w;
}

uint32_t GenesisM68k::FetchLong() {
	uint32_t l = Read32(_state.PC);
	_state.PC += 4;
	return l;
}

// ===== Flag operations =====

void GenesisM68k::SetFlags_Logical(uint32_t result, uint8_t size) {
	uint32_t mask = SizeMask(size);
	result &= mask;
	ClearCcr(M68kFlags::Carry | M68kFlags::Overflow);
	SetCcrIf(M68kFlags::Zero, result == 0);
	SetCcrIf(M68kFlags::Negative, IsNeg(result, size));
}

void GenesisM68k::SetFlags_Add(uint32_t src, uint32_t dst, uint32_t result, uint8_t size) {
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);
	src &= mask; dst &= mask; uint32_t res = result & mask;

	bool sm = (src & msb) != 0;
	bool dm = (dst & msb) != 0;
	bool rm = (res & msb) != 0;

	SetCcrIf(M68kFlags::Carry, (result >> (size == 0 ? 8 : (size == 1 ? 16 : 32))) & 1);
	SetCcrIf(M68kFlags::Overflow, (sm && dm && !rm) || (!sm && !dm && rm));
	SetCcrIf(M68kFlags::Zero, res == 0);
	SetCcrIf(M68kFlags::Negative, rm);
	SetCcrIf(M68kFlags::Extend, (result >> (size == 0 ? 8 : (size == 1 ? 16 : 32))) & 1);
}

void GenesisM68k::SetFlags_Sub(uint32_t src, uint32_t dst, uint32_t result, uint8_t size) {
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);
	src &= mask; dst &= mask; uint32_t res = result & mask;

	bool sm = (src & msb) != 0;
	bool dm = (dst & msb) != 0;
	bool rm = (res & msb) != 0;

	bool carry = src > dst;
	SetCcrIf(M68kFlags::Carry, carry);
	SetCcrIf(M68kFlags::Overflow, (!sm && dm && !rm) || (sm && !dm && rm));
	SetCcrIf(M68kFlags::Zero, res == 0);
	SetCcrIf(M68kFlags::Negative, rm);
	SetCcrIf(M68kFlags::Extend, carry);
}

void GenesisM68k::SetFlags_Cmp(uint32_t src, uint32_t dst, uint32_t result, uint8_t size) {
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);
	src &= mask; dst &= mask; uint32_t res = result & mask;

	bool sm = (src & msb) != 0;
	bool dm = (dst & msb) != 0;
	bool rm = (res & msb) != 0;

	SetCcrIf(M68kFlags::Carry, src > dst);
	SetCcrIf(M68kFlags::Overflow, (!sm && dm && !rm) || (sm && !dm && rm));
	SetCcrIf(M68kFlags::Zero, res == 0);
	SetCcrIf(M68kFlags::Negative, rm);
}

// ===== Condition testing =====

bool GenesisM68k::TestCondition(uint8_t cc) {
	bool C = GetCcr(M68kFlags::Carry);
	bool V = GetCcr(M68kFlags::Overflow);
	bool Z = GetCcr(M68kFlags::Zero);
	bool N = GetCcr(M68kFlags::Negative);

	switch (cc) {
		case 0x0: return true;           // T (true)
		case 0x1: return false;          // F (false)
		case 0x2: return !C && !Z;       // HI (higher)
		case 0x3: return C || Z;         // LS (lower or same)
		case 0x4: return !C;             // CC (carry clear)
		case 0x5: return C;              // CS (carry set)
		case 0x6: return !Z;             // NE (not equal)
		case 0x7: return Z;              // EQ (equal)
		case 0x8: return !V;             // VC (overflow clear)
		case 0x9: return V;              // VS (overflow set)
		case 0xA: return !N;             // PL (plus)
		case 0xB: return N;              // MI (minus)
		case 0xC: return N == V;         // GE (greater or equal)
		case 0xD: return N != V;         // LT (less than)
		case 0xE: return !Z && (N == V); // GT (greater than)
		case 0xF: return Z || (N != V);  // LE (less or equal)
		default: return false;
	}
}

// ===== Effective address resolution =====

uint32_t GenesisM68k::GetEffectiveAddress(uint8_t mode, uint8_t reg, uint8_t size) {
	switch (mode) {
		case 0: // Dn - data register direct (no address)
			return 0;
		case 1: // An - address register direct (no address)
			return 0;
		case 2: // (An) - address register indirect
			return _state.A[reg];
		case 3: // (An)+ - postincrement
		{
			uint32_t addr = _state.A[reg];
			uint32_t inc = (size == 0) ? 1 : (size == 1 ? 2 : 4);
			if (size == 0 && reg == 7) inc = 2; // SP always word-aligned
			_state.A[reg] += inc;
			return addr;
		}
		case 4: // -(An) - predecrement
		{
			uint32_t dec = (size == 0) ? 1 : (size == 1 ? 2 : 4);
			if (size == 0 && reg == 7) dec = 2;
			_state.A[reg] -= dec;
			return _state.A[reg];
		}
		case 5: // (d16, An) - displacement
		{
			int16_t disp = (int16_t)FetchWord();
			return _state.A[reg] + disp;
		}
		case 6: // (d8, An, Xn) - index
		{
			uint16_t ext = FetchWord();
			uint8_t xreg = (ext >> 12) & 0x0F;
			bool isLong = (ext >> 11) & 1;
			int8_t disp = (int8_t)(ext & 0xFF);
			int32_t xval;
			if (xreg < 8) {
				xval = isLong ? (int32_t)_state.D[xreg] : (int16_t)(uint16_t)_state.D[xreg];
			} else {
				xval = isLong ? (int32_t)_state.A[xreg - 8] : (int16_t)(uint16_t)_state.A[xreg - 8];
			}
			return _state.A[reg] + disp + xval;
		}
		case 7: // Absolute/PC modes
			switch (reg) {
				case 0: // (xxx).W - absolute short
					return (int16_t)FetchWord();
				case 1: // (xxx).L - absolute long
					return FetchLong();
				case 2: // (d16, PC) - PC displacement
				{
					uint32_t pc = _state.PC;
					int16_t disp = (int16_t)FetchWord();
					return pc + disp;
				}
				case 3: // (d8, PC, Xn) - PC index
				{
					uint32_t pc = _state.PC;
					uint16_t ext = FetchWord();
					uint8_t xreg = (ext >> 12) & 0x0F;
					bool isLong = (ext >> 11) & 1;
					int8_t disp = (int8_t)(ext & 0xFF);
					int32_t xval;
					if (xreg < 8) {
						xval = isLong ? (int32_t)_state.D[xreg] : (int16_t)(uint16_t)_state.D[xreg];
					} else {
						xval = isLong ? (int32_t)_state.A[xreg - 8] : (int16_t)(uint16_t)_state.A[xreg - 8];
					}
					return pc + disp + xval;
				}
				case 4: // #imm - immediate
					return 0; // Handled specially
				default:
					return 0;
			}
		default:
			return 0;
	}
}

uint32_t GenesisM68k::ReadEa(uint8_t mode, uint8_t reg, uint8_t size) {
	uint32_t mask = SizeMask(size);
	switch (mode) {
		case 0: // Dn
			return _state.D[reg] & mask;
		case 1: // An
			return _state.A[reg] & mask;
		case 7:
			if (reg == 4) { // #imm
				if (size == 2) return FetchLong();
				return FetchWord() & mask;
			}
			[[fallthrough]];
		default:
		{
			uint32_t addr = GetEffectiveAddress(mode, reg, size);
			if (size == 0) return Read8(addr);
			if (size == 1) return Read16(addr);
			return Read32(addr);
		}
	}
}

void GenesisM68k::WriteEa(uint8_t mode, uint8_t reg, uint8_t size, uint32_t value) {
	uint32_t mask = SizeMask(size);
	switch (mode) {
		case 0: // Dn
			_state.D[reg] = (_state.D[reg] & ~mask) | (value & mask);
			return;
		case 1: // An
			if (size == 1) {
				_state.A[reg] = (int16_t)(uint16_t)(value & 0xFFFF); // Sign-extend word to long
			} else {
				_state.A[reg] = value;
			}
			return;
		default:
		{
			uint32_t addr = GetEffectiveAddress(mode, reg, size);
			if (size == 0) Write8(addr, (uint8_t)value);
			else if (size == 1) Write16(addr, (uint16_t)value);
			else Write32(addr, value);
			return;
		}
	}
}

// ===== Exception handling =====

void GenesisM68k::RaiseException(uint8_t vector) {
	// Switch to supervisor mode, push PC and SR
	uint16_t oldSR = _state.SR;
	_state.SR |= M68kFlags::Supervisor;
	_state.SR &= ~M68kFlags::Trace;

	// Swap stacks if needed
	if (!(oldSR & M68kFlags::Supervisor)) {
		_state.USP = _state.A[7];
		_state.A[7] = _state.SSP;
	}

	Push32(_state.PC);
	Push16(oldSR);

	// Read vector
	uint32_t vectorAddr = Read32(vector * 4);
	_state.PC = vectorAddr;
	_state.Stopped = false;
	AddCycles(34);
}

void GenesisM68k::ProcessInterrupt(uint8_t level) {
	uint16_t oldSR = _state.SR;
	_state.SR = (_state.SR & ~M68kFlags::IntMask) | ((uint16_t)level << 8);
	_state.SR |= M68kFlags::Supervisor;
	_state.SR &= ~M68kFlags::Trace;

	if (!(oldSR & M68kFlags::Supervisor)) {
		_state.USP = _state.A[7];
		_state.A[7] = _state.SSP;
	}

	Push32(_state.PC);
	Push16(oldSR);

	// Genesis interrupt vectors: level 2=external, 4=HBlank, 6=VBlank
	uint8_t vector = 24 + level;
	_state.PC = Read32(vector * 4);
	_state.Stopped = false;
	AddCycles(44);
}

void GenesisM68k::SetInterrupt(uint8_t level) {
	// Will be processed at start of next instruction
	if (level > GetIntMask() || level == 7) {
		ProcessInterrupt(level);
	}
}

// ===== Cycle counting =====

void GenesisM68k::AddCycles(uint32_t cycles) {
	_state.CycleCount += cycles;
}

// ===== Main execution =====

void GenesisM68k::Exec() {
	if (_state.Stopped) {
		AddCycles(4);
		return;
	}

	uint16_t opcode = FetchOpcode();
	ExecuteInstruction(opcode);
}

void GenesisM68k::Reset(bool softReset) {
	if (!softReset) {
		memset(&_state, 0, sizeof(_state));
	}

	_state.SR = 0x2700; // Supervisor mode, all interrupts masked
	_state.Stopped = false;

	// Read initial SSP and PC from vector table
	_state.SSP = Read32(0x000000);
	_state.A[7] = _state.SSP;
	_state.PC = Read32(0x000004);
}

// ===== Instruction decoder =====
// Decodes the 68000 instruction set by examining opcode bit fields

void GenesisM68k::ExecuteInstruction(uint16_t opcode) {
	uint8_t group = (opcode >> 12) & 0x0F;

	switch (group) {
		case 0x0: // Bit manipulation / MOVEP / Immediate
		{
			if ((opcode & 0x0100) && !(opcode & 0x0038)) {
				// Dynamic bit operations on Dn
				uint8_t bitOp = (opcode >> 6) & 3;
				switch (bitOp) {
					case 0: Op_BTST(opcode); break;
					case 1: Op_BCHG(opcode); break;
					case 2: Op_BCLR(opcode); break;
					case 3: Op_BSET(opcode); break;
				}
			} else if (opcode & 0x0100) {
				// Dynamic bit ops on memory
				uint8_t bitOp = (opcode >> 6) & 3;
				switch (bitOp) {
					case 0: Op_BTST(opcode); break;
					case 1: Op_BCHG(opcode); break;
					case 2: Op_BCLR(opcode); break;
					case 3: Op_BSET(opcode); break;
				}
			} else {
				uint8_t subOp = (opcode >> 9) & 7;
				switch (subOp) {
					case 0: // ORI
						if ((opcode & 0x3F) == 0x3C) Op_ORI_SR(opcode);
						else Op_ORI(opcode);
						break;
					case 1: // ANDI
						if ((opcode & 0x3F) == 0x3C) Op_ANDI_SR(opcode);
						else Op_ANDI(opcode);
						break;
					case 2: Op_SUBI(opcode); break;
					case 3: Op_ADDI(opcode); break;
					case 4: // Static bit operations
					{
						uint8_t bitOp = (opcode >> 6) & 3;
						switch (bitOp) {
							case 0: Op_BTST(opcode); break;
							case 1: Op_BCHG(opcode); break;
							case 2: Op_BCLR(opcode); break;
							case 3: Op_BSET(opcode); break;
						}
						break;
					}
					case 5: // EORI
						if ((opcode & 0x3F) == 0x3C) Op_EORI_SR(opcode);
						else Op_EORI(opcode);
						break;
					case 6: Op_CMPI(opcode); break;
					default: Op_ILLEGAL(opcode); break;
				}
			}
			break;
		}

		case 0x1: // MOVE.B
		case 0x2: // MOVE.L
		case 0x3: // MOVE.W
		{
			uint8_t dstReg = (opcode >> 9) & 7;
			uint8_t dstMode = (opcode >> 6) & 7;
			if (dstMode == 1 && group != 1) {
				Op_MOVEA(opcode);
			} else {
				Op_MOVE(opcode);
			}
			break;
		}

		case 0x4: // Miscellaneous
		{
			if ((opcode & 0xFFC0) == 0x46C0) { Op_MOVE_SR(opcode); break; } // MOVE to SR
			if ((opcode & 0xFFC0) == 0x44C0) { Op_MOVE_SR(opcode); break; } // MOVE to CCR
			if ((opcode & 0xFFC0) == 0x40C0) { Op_MOVE_SR(opcode); break; } // MOVE from SR
			if ((opcode & 0xFFF0) == 0x4E60) { Op_MOVE_USP(opcode); break; }
			if ((opcode & 0xFFF8) == 0x4E50) { Op_LINK(opcode); break; }
			if ((opcode & 0xFFF8) == 0x4E58) { Op_UNLK(opcode); break; }
			if ((opcode & 0xFFF8) == 0x4840) { Op_SWAP(opcode); break; }
			if ((opcode & 0xFFF8) == 0x4880) { Op_EXT(opcode); break; }  // EXT.W
			if ((opcode & 0xFFF8) == 0x48C0) { Op_EXT(opcode); break; }  // EXT.L
			if (opcode == 0x4E75) { Op_RTS(opcode); break; }
			if (opcode == 0x4E73) { Op_RTE(opcode); break; }
			if (opcode == 0x4E77) { Op_RTR(opcode); break; }
			if (opcode == 0x4E71) { Op_NOP(opcode); break; }
			if (opcode == 0x4E70) { Op_RESET(opcode); break; }
			if (opcode == 0x4E72) { Op_STOP(opcode); break; }
			if ((opcode & 0xFFF0) == 0x4E40) { Op_TRAP(opcode); break; }
			if ((opcode & 0xFFC0) == 0x4EC0) { Op_JMP(opcode); break; }
			if ((opcode & 0xFFC0) == 0x4E80) { Op_JSR(opcode); break; }
			if ((opcode & 0xFFC0) == 0x4800) { // NBCD - not implementing now
				Op_ILLEGAL(opcode); break;
			}
			if ((opcode & 0xFB80) == 0x4880) { Op_MOVEM(opcode); break; }
			if ((opcode & 0xFFC0) == 0x4AC0) { Op_TST(opcode); break; } // TAS
			if ((opcode & 0xFF00) == 0x4A00) { Op_TST(opcode); break; }

			uint8_t subOp = (opcode >> 6) & 3;
			switch (subOp) {
				case 0: case 1: case 2:
				{
					uint8_t op2 = (opcode >> 8) & 0x0F;
					if (op2 == 0x02 || op2 == 0x06 || op2 == 0x0A) {
						Op_CLR(opcode);
					} else if (op2 == 0x04 || op2 == 0x08 || op2 == 0x0C) {
						Op_NEG(opcode);
					} else if ((opcode & 0xFFC0) == 0x4AC0) {
						Op_TST(opcode);
					} else {
						Op_ILLEGAL(opcode);
					}
					break;
				}
				default:
					if ((opcode & 0xFFC0) == 0x4EC0) Op_JMP(opcode);
					else if ((opcode & 0xFFC0) == 0x4E80) Op_JSR(opcode);
					else if ((opcode & 0xFFC0) == 0x41C0 || (opcode & 0xF1C0) == 0x41C0) Op_LEA(opcode);
					else Op_ILLEGAL(opcode);
					break;
			}
			break;
		}

		case 0x5: // ADDQ/SUBQ/Scc/DBcc
		{
			uint8_t size = (opcode >> 6) & 3;
			if (size == 3) {
				// Scc / DBcc
				uint8_t mode = (opcode >> 3) & 7;
				if (mode == 1) Op_DBcc(opcode);
				else Op_Scc(opcode);
			} else {
				if (opcode & 0x0100) Op_SUBQ(opcode);
				else Op_ADDQ(opcode);
			}
			break;
		}

		case 0x6: // Bcc/BSR/BRA
		{
			uint8_t cc = (opcode >> 8) & 0x0F;
			if (cc == 0) Op_BRA(opcode);
			else if (cc == 1) Op_BSR(opcode);
			else Op_Bcc(opcode);
			break;
		}

		case 0x7: // MOVEQ
			Op_MOVEQ(opcode);
			break;

		case 0x8: // OR/DIV
		{
			uint8_t opMode = (opcode >> 6) & 7;
			if (opMode == 3) Op_DIVU(opcode);
			else if (opMode == 7) Op_DIVS(opcode);
			else Op_OR(opcode);
			break;
		}

		case 0x9: // SUB/SUBA
		{
			uint8_t opMode = (opcode >> 6) & 7;
			if (opMode == 3 || opMode == 7) Op_SUBA(opcode);
			else Op_SUB(opcode);
			break;
		}

		case 0xB: // CMP/CMPA/EOR
		{
			uint8_t opMode = (opcode >> 6) & 7;
			if (opMode == 3 || opMode == 7) Op_CMPA(opcode);
			else if (opMode >= 4 && opMode <= 6) Op_EOR(opcode);
			else Op_CMP(opcode);
			break;
		}

		case 0xC: // AND/MUL/EXG
		{
			uint8_t opMode = (opcode >> 6) & 7;
			if (opMode == 3) Op_MULU(opcode);
			else if (opMode == 7) Op_MULS(opcode);
			else if ((opcode & 0xF130) == 0xC100) Op_EXG(opcode);
			else Op_AND(opcode);
			break;
		}

		case 0xD: // ADD/ADDA
		{
			uint8_t opMode = (opcode >> 6) & 7;
			if (opMode == 3 || opMode == 7) Op_ADDA(opcode);
			else Op_ADD(opcode);
			break;
		}

		case 0xE: // Shift/Rotate
		{
			uint8_t type = (opcode >> 3) & 3;
			switch (type & 3) {
				case 0: Op_ASd(opcode); break;
				case 1: Op_LSd(opcode); break;
				case 2: Op_ROXd(opcode); break;
				case 3: Op_ROd(opcode); break;
			}
			break;
		}

		case 0xA: // Line-A (unimplemented)
		case 0xF: // Line-F (unimplemented)
		default:
			Op_ILLEGAL(opcode);
			break;
	}
}

// ===== Data Movement Instructions =====

void GenesisM68k::Op_MOVE(uint16_t opcode) {
	uint8_t size;
	switch ((opcode >> 12) & 3) {
		case 1: size = 0; break; // .B
		case 3: size = 1; break; // .W
		case 2: size = 2; break; // .L
		default: size = 1; break;
	}

	uint8_t srcMode = (opcode >> 3) & 7;
	uint8_t srcReg = opcode & 7;
	uint8_t dstMode = (opcode >> 6) & 7;
	uint8_t dstReg = (opcode >> 9) & 7;

	uint32_t data = ReadEa(srcMode, srcReg, size);
	SetFlags_Logical(data, size);
	WriteEa(dstMode, dstReg, size, data);
	AddCycles(4);
}

void GenesisM68k::Op_MOVEA(uint16_t opcode) {
	uint8_t size = ((opcode >> 12) & 3) == 3 ? 1 : 2;
	uint8_t srcMode = (opcode >> 3) & 7;
	uint8_t srcReg = opcode & 7;
	uint8_t dstReg = (opcode >> 9) & 7;

	uint32_t data = ReadEa(srcMode, srcReg, size);
	if (size == 1) data = (int16_t)(uint16_t)data; // Sign extend word
	_state.A[dstReg] = data;
	AddCycles(4);
}

void GenesisM68k::Op_MOVEQ(uint16_t opcode) {
	uint8_t dstReg = (opcode >> 9) & 7;
	int8_t data = (int8_t)(opcode & 0xFF);
	_state.D[dstReg] = (int32_t)data; // Sign extend
	SetFlags_Logical(_state.D[dstReg], 2);
	AddCycles(4);
}

void GenesisM68k::Op_MOVEM(uint16_t opcode) {
	uint8_t size = (opcode & 0x0040) ? 2 : 1;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint16_t regList = FetchWord();
	bool toMemory = !(opcode & 0x0400);

	if (toMemory) {
		if (mode == 4) { // Predecrement: register order is reversed
			for (int i = 15; i >= 0; i--) {
				if (regList & (1 << (15 - i))) {
					uint32_t val = (i < 8) ? _state.D[i] : _state.A[i - 8];
					if (size == 1) {
						_state.A[reg] -= 2;
						Write16(_state.A[reg], (uint16_t)val);
					} else {
						_state.A[reg] -= 4;
						Write32(_state.A[reg], val);
					}
				}
			}
		} else {
			uint32_t addr = GetEffectiveAddress(mode, reg, size);
			for (int i = 0; i < 16; i++) {
				if (regList & (1 << i)) {
					uint32_t val = (i < 8) ? _state.D[i] : _state.A[i - 8];
					if (size == 1) {
						Write16(addr, (uint16_t)val);
						addr += 2;
					} else {
						Write32(addr, val);
						addr += 4;
					}
				}
			}
		}
	} else { // To registers
		uint32_t addr;
		if (mode == 3) { // Postincrement
			addr = _state.A[reg];
		} else {
			addr = GetEffectiveAddress(mode, reg, size);
		}
		for (int i = 0; i < 16; i++) {
			if (regList & (1 << i)) {
				if (i < 8) {
					_state.D[i] = (size == 1) ? (int16_t)(int32_t)Read16(addr) : Read32(addr);
				} else {
					_state.A[i - 8] = (size == 1) ? (int16_t)(int32_t)Read16(addr) : Read32(addr);
				}
				addr += (size == 1) ? 2 : 4;
			}
		}
		if (mode == 3) {
			_state.A[reg] = addr;
		}
	}
	AddCycles(4);
}

void GenesisM68k::Op_LEA(uint16_t opcode) {
	uint8_t dstReg = (opcode >> 9) & 7;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	_state.A[dstReg] = GetEffectiveAddress(mode, reg, 2);
	AddCycles(4);
}

void GenesisM68k::Op_PEA(uint16_t opcode) {
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint32_t addr = GetEffectiveAddress(mode, reg, 2);
	Push32(addr);
	AddCycles(4);
}

void GenesisM68k::Op_CLR(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	WriteEa(mode, reg, size, 0);
	_state.SR &= ~(M68kFlags::Carry | M68kFlags::Overflow | M68kFlags::Negative);
	_state.SR |= M68kFlags::Zero;
	AddCycles(4);
}

void GenesisM68k::Op_EXG(uint16_t opcode) {
	uint8_t rx = (opcode >> 9) & 7;
	uint8_t ry = opcode & 7;
	uint8_t opMode = (opcode >> 3) & 0x1F;

	if (opMode == 0x08) { // Data <-> Data
		std::swap(_state.D[rx], _state.D[ry]);
	} else if (opMode == 0x09) { // Address <-> Address
		std::swap(_state.A[rx], _state.A[ry]);
	} else if (opMode == 0x11) { // Data <-> Address
		std::swap(_state.D[rx], _state.A[ry]);
	}
	AddCycles(6);
}

void GenesisM68k::Op_SWAP(uint16_t opcode) {
	uint8_t reg = opcode & 7;
	uint32_t val = _state.D[reg];
	_state.D[reg] = (val >> 16) | (val << 16);
	SetFlags_Logical(_state.D[reg], 2);
	AddCycles(4);
}

void GenesisM68k::Op_LINK(uint16_t opcode) {
	uint8_t reg = opcode & 7;
	int16_t disp = (int16_t)FetchWord();
	Push32(_state.A[reg]);
	_state.A[reg] = SP();
	SP() += disp;
	AddCycles(16);
}

void GenesisM68k::Op_UNLK(uint16_t opcode) {
	uint8_t reg = opcode & 7;
	SP() = _state.A[reg];
	_state.A[reg] = Pop32();
	AddCycles(12);
}

// ===== Arithmetic =====

void GenesisM68k::Op_ADD(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;
	bool toReg = !((opcode >> 8) & 1);

	uint32_t mask = SizeMask(size);
	if (toReg) {
		uint32_t src = ReadEa(mode, eaReg, size);
		uint32_t dst = _state.D[reg] & mask;
		uint64_t result = (uint64_t)src + (uint64_t)dst;
		SetFlags_Add(src, dst, (uint32_t)result, size);
		_state.D[reg] = (_state.D[reg] & ~mask) | ((uint32_t)result & mask);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, eaReg, size);
		uint32_t src = _state.D[reg] & mask;
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		uint64_t result = (uint64_t)src + (uint64_t)dst;
		SetFlags_Add(src, dst, (uint32_t)result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, (uint32_t)result);
	}
	AddCycles(4);
}

void GenesisM68k::Op_ADDA(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode & 0x0100) ? 2 : 1;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	int32_t src = (size == 1) ? (int16_t)(uint16_t)ReadEa(mode, eaReg, size) : (int32_t)ReadEa(mode, eaReg, size);
	_state.A[reg] += src;
	AddCycles(size == 2 ? 6 : 8);
}

void GenesisM68k::Op_ADDI(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t imm = (size == 2) ? FetchLong() : (FetchWord() & mask);
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	uint32_t dst = ReadEa(mode, reg, size);
	uint64_t result = (uint64_t)(dst & mask) + (uint64_t)(imm & mask);
	SetFlags_Add(imm, dst, (uint32_t)result, size);
	WriteEa(mode, reg, size, (uint32_t)result);
	AddCycles(size == 2 ? 16 : 8);
}

void GenesisM68k::Op_ADDQ(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint8_t data = (opcode >> 9) & 7;
	if (data == 0) data = 8;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode == 1) { // Address register — no flags affected
		_state.A[reg] += data;
	} else {
		uint32_t mask = SizeMask(size);
		uint32_t dst = ReadEa(mode, reg, size) & mask;
		uint64_t result = (uint64_t)dst + data;
		SetFlags_Add(data, dst, (uint32_t)result, size);
		WriteEa(mode, reg, size, (uint32_t)result);
	}
	AddCycles(4);
}

void GenesisM68k::Op_SUB(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;
	bool toReg = !((opcode >> 8) & 1);
	uint32_t mask = SizeMask(size);

	if (toReg) {
		uint32_t src = ReadEa(mode, eaReg, size) & mask;
		uint32_t dst = _state.D[reg] & mask;
		uint32_t result = dst - src;
		SetFlags_Sub(src, dst, result, size);
		_state.D[reg] = (_state.D[reg] & ~mask) | (result & mask);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, eaReg, size);
		uint32_t src = _state.D[reg] & mask;
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		uint32_t result = dst - src;
		SetFlags_Sub(src, dst, result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(4);
}

void GenesisM68k::Op_SUBA(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode & 0x0100) ? 2 : 1;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	int32_t src = (size == 1) ? (int16_t)(uint16_t)ReadEa(mode, eaReg, size) : (int32_t)ReadEa(mode, eaReg, size);
	_state.A[reg] -= src;
	AddCycles(size == 2 ? 6 : 8);
}

void GenesisM68k::Op_SUBI(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t imm = (size == 2) ? FetchLong() : (FetchWord() & mask);
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	uint32_t dst = ReadEa(mode, reg, size) & mask;
	uint32_t result = dst - imm;
	SetFlags_Sub(imm, dst, result, size);
	WriteEa(mode, reg, size, result);
	AddCycles(size == 2 ? 16 : 8);
}

void GenesisM68k::Op_SUBQ(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint8_t data = (opcode >> 9) & 7;
	if (data == 0) data = 8;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode == 1) {
		_state.A[reg] -= data;
	} else {
		uint32_t mask = SizeMask(size);
		uint32_t dst = ReadEa(mode, reg, size) & mask;
		uint32_t result = dst - data;
		SetFlags_Sub(data, dst, result, size);
		WriteEa(mode, reg, size, result);
	}
	AddCycles(4);
}

void GenesisM68k::Op_CMP(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;
	uint32_t mask = SizeMask(size);

	uint32_t src = ReadEa(mode, eaReg, size) & mask;
	uint32_t dst = _state.D[reg] & mask;
	uint32_t result = dst - src;
	SetFlags_Cmp(src, dst, result, size);
	AddCycles(4);
}

void GenesisM68k::Op_CMPA(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode & 0x0100) ? 2 : 1;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	int32_t src = (size == 1) ? (int16_t)(uint16_t)ReadEa(mode, eaReg, size) : (int32_t)ReadEa(mode, eaReg, size);
	int32_t dst = (int32_t)_state.A[reg];
	uint32_t result = (uint32_t)(dst - src);
	SetFlags_Cmp((uint32_t)src, (uint32_t)dst, result, 2);
	AddCycles(6);
}

void GenesisM68k::Op_CMPI(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t imm = (size == 2) ? FetchLong() : (FetchWord() & mask);
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	uint32_t dst = ReadEa(mode, reg, size) & mask;
	uint32_t result = dst - imm;
	SetFlags_Cmp(imm, dst, result, size);
	AddCycles(size == 2 ? 12 : 8);
}

void GenesisM68k::Op_MULU(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	uint16_t src = (uint16_t)ReadEa(mode, eaReg, 1);
	uint16_t dst = (uint16_t)_state.D[reg];
	uint32_t result = (uint32_t)src * (uint32_t)dst;
	_state.D[reg] = result;
	SetFlags_Logical(result, 2);
	AddCycles(70); // Worst case
}

void GenesisM68k::Op_MULS(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	int16_t src = (int16_t)(uint16_t)ReadEa(mode, eaReg, 1);
	int16_t dst = (int16_t)(uint16_t)_state.D[reg];
	int32_t result = (int32_t)src * (int32_t)dst;
	_state.D[reg] = (uint32_t)result;
	SetFlags_Logical((uint32_t)result, 2);
	AddCycles(70);
}

void GenesisM68k::Op_DIVU(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	uint16_t divisor = (uint16_t)ReadEa(mode, eaReg, 1);
	if (divisor == 0) {
		RaiseException(5); // Division by zero
		return;
	}
	uint32_t dividend = _state.D[reg];
	uint32_t quotient = dividend / divisor;
	uint16_t remainder = (uint16_t)(dividend % divisor);

	if (quotient > 0xFFFF) {
		SetCcr(M68kFlags::Overflow);
		ClearCcr(M68kFlags::Carry);
	} else {
		_state.D[reg] = ((uint32_t)remainder << 16) | (quotient & 0xFFFF);
		ClearCcr(M68kFlags::Carry | M68kFlags::Overflow);
		SetCcrIf(M68kFlags::Zero, (quotient & 0xFFFF) == 0);
		SetCcrIf(M68kFlags::Negative, (quotient & 0x8000) != 0);
	}
	AddCycles(140); // Worst case
}

void GenesisM68k::Op_DIVS(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	int16_t divisor = (int16_t)(uint16_t)ReadEa(mode, eaReg, 1);
	if (divisor == 0) {
		RaiseException(5);
		return;
	}
	int32_t dividend = (int32_t)_state.D[reg];
	int32_t quotient = dividend / divisor;
	int16_t remainder = (int16_t)(dividend % divisor);

	if (quotient < -32768 || quotient > 32767) {
		SetCcr(M68kFlags::Overflow);
		ClearCcr(M68kFlags::Carry);
	} else {
		_state.D[reg] = ((uint32_t)(uint16_t)remainder << 16) | ((uint16_t)quotient & 0xFFFF);
		ClearCcr(M68kFlags::Carry | M68kFlags::Overflow);
		SetCcrIf(M68kFlags::Zero, (quotient & 0xFFFF) == 0);
		SetCcrIf(M68kFlags::Negative, (quotient & 0x8000) != 0);
	}
	AddCycles(158);
}

void GenesisM68k::Op_NEG(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint32_t mask = SizeMask(size);

	uint32_t dst = ReadEa(mode, reg, size) & mask;
	uint32_t result = (uint32_t)(-(int32_t)dst) & mask;
	SetFlags_Sub(dst, 0, result, size);
	WriteEa(mode, reg, size, result);
	AddCycles(4);
}

void GenesisM68k::Op_EXT(uint16_t opcode) {
	uint8_t reg = opcode & 7;
	if (opcode & 0x0040) { // EXT.L
		_state.D[reg] = (int16_t)(uint16_t)_state.D[reg];
		SetFlags_Logical(_state.D[reg], 2);
	} else { // EXT.W
		_state.D[reg] = (_state.D[reg] & 0xFFFF0000) | ((uint16_t)(int8_t)_state.D[reg]);
		SetFlags_Logical(_state.D[reg], 1);
	}
	AddCycles(4);
}

void GenesisM68k::Op_TST(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint32_t data = ReadEa(mode, reg, size);
	SetFlags_Logical(data, size);
	AddCycles(4);
}

// ===== Logic =====

void GenesisM68k::Op_AND(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;
	bool toReg = !((opcode >> 8) & 1);
	uint32_t mask = SizeMask(size);

	if (toReg) {
		uint32_t src = ReadEa(mode, eaReg, size) & mask;
		uint32_t result = (_state.D[reg] & mask) & src;
		_state.D[reg] = (_state.D[reg] & ~mask) | result;
		SetFlags_Logical(result, size);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, eaReg, size);
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		uint32_t result = dst & (_state.D[reg] & mask);
		SetFlags_Logical(result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(4);
}

void GenesisM68k::Op_ANDI(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t imm = (size == 2) ? FetchLong() : (FetchWord() & mask);
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	uint32_t dst = ReadEa(mode, reg, size) & mask;
	uint32_t result = dst & imm;
	SetFlags_Logical(result, size);
	WriteEa(mode, reg, size, result);
	AddCycles(size == 2 ? 14 : 8);
}

void GenesisM68k::Op_OR(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;
	bool toReg = !((opcode >> 8) & 1);
	uint32_t mask = SizeMask(size);

	if (toReg) {
		uint32_t src = ReadEa(mode, eaReg, size) & mask;
		uint32_t result = (_state.D[reg] & mask) | src;
		_state.D[reg] = (_state.D[reg] & ~mask) | result;
		SetFlags_Logical(result, size);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, eaReg, size);
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		uint32_t result = dst | (_state.D[reg] & mask);
		SetFlags_Logical(result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(4);
}

void GenesisM68k::Op_ORI(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t imm = (size == 2) ? FetchLong() : (FetchWord() & mask);
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	uint32_t dst = ReadEa(mode, reg, size) & mask;
	uint32_t result = dst | imm;
	SetFlags_Logical(result, size);
	WriteEa(mode, reg, size, result);
	AddCycles(size == 2 ? 16 : 8);
}

void GenesisM68k::Op_EOR(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;
	uint32_t mask = SizeMask(size);

	uint32_t src = _state.D[reg] & mask;
	if (mode == 0) {
		uint32_t result = (_state.D[eaReg] & mask) ^ src;
		_state.D[eaReg] = (_state.D[eaReg] & ~mask) | result;
		SetFlags_Logical(result, size);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, eaReg, size);
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		uint32_t result = dst ^ src;
		SetFlags_Logical(result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(4);
}

void GenesisM68k::Op_EORI(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t imm = (size == 2) ? FetchLong() : (FetchWord() & mask);
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	uint32_t dst = ReadEa(mode, reg, size) & mask;
	uint32_t result = dst ^ imm;
	SetFlags_Logical(result, size);
	WriteEa(mode, reg, size, result);
	AddCycles(size == 2 ? 16 : 8);
}

void GenesisM68k::Op_NOT(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint32_t mask = SizeMask(size);

	uint32_t dst = ReadEa(mode, reg, size) & mask;
	uint32_t result = (~dst) & mask;
	SetFlags_Logical(result, size);
	WriteEa(mode, reg, size, result);
	AddCycles(4);
}

// ===== Shifts / Rotates =====

void GenesisM68k::Op_ASd(uint16_t opcode) {
	bool left = (opcode >> 8) & 1;
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);

	uint8_t reg = opcode & 7;
	uint8_t count;
	if (opcode & 0x20) {
		count = _state.D[(opcode >> 9) & 7] & 0x3F;
	} else {
		count = (opcode >> 9) & 7;
		if (count == 0) count = 8;
	}

	uint32_t data = _state.D[reg] & mask;
	ClearCcr(M68kFlags::Carry | M68kFlags::Overflow);

	for (uint8_t i = 0; i < count; i++) {
		if (left) {
			SetCcrIf(M68kFlags::Carry | M68kFlags::Extend, (data & msb) != 0);
			uint32_t old = data;
			data = (data << 1) & mask;
			if ((old ^ data) & msb) SetCcr(M68kFlags::Overflow);
		} else {
			SetCcrIf(M68kFlags::Carry | M68kFlags::Extend, data & 1);
			uint32_t sign = data & msb;
			data = (data >> 1) | sign;
		}
	}

	_state.D[reg] = (_state.D[reg] & ~mask) | (data & mask);
	SetCcrIf(M68kFlags::Zero, (data & mask) == 0);
	SetCcrIf(M68kFlags::Negative, (data & msb) != 0);
	if (count == 0) ClearCcr(M68kFlags::Carry);
	AddCycles(6 + 2 * count);
}

void GenesisM68k::Op_LSd(uint16_t opcode) {
	bool left = (opcode >> 8) & 1;
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);

	uint8_t reg = opcode & 7;
	uint8_t count;
	if (opcode & 0x20) {
		count = _state.D[(opcode >> 9) & 7] & 0x3F;
	} else {
		count = (opcode >> 9) & 7;
		if (count == 0) count = 8;
	}

	uint32_t data = _state.D[reg] & mask;
	ClearCcr(M68kFlags::Carry | M68kFlags::Overflow);

	for (uint8_t i = 0; i < count; i++) {
		if (left) {
			SetCcrIf(M68kFlags::Carry | M68kFlags::Extend, (data & msb) != 0);
			data = (data << 1) & mask;
		} else {
			SetCcrIf(M68kFlags::Carry | M68kFlags::Extend, data & 1);
			data >>= 1;
		}
	}

	_state.D[reg] = (_state.D[reg] & ~mask) | (data & mask);
	SetCcrIf(M68kFlags::Zero, (data & mask) == 0);
	SetCcrIf(M68kFlags::Negative, (data & msb) != 0);
	if (count == 0) ClearCcr(M68kFlags::Carry);
	AddCycles(6 + 2 * count);
}

void GenesisM68k::Op_ROd(uint16_t opcode) {
	bool left = (opcode >> 8) & 1;
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);
	uint8_t bits = (size == 0) ? 8 : ((size == 1) ? 16 : 32);

	uint8_t reg = opcode & 7;
	uint8_t count;
	if (opcode & 0x20) {
		count = _state.D[(opcode >> 9) & 7] & 0x3F;
	} else {
		count = (opcode >> 9) & 7;
		if (count == 0) count = 8;
	}

	uint32_t data = _state.D[reg] & mask;
	ClearCcr(M68kFlags::Carry | M68kFlags::Overflow);

	for (uint8_t i = 0; i < count; i++) {
		if (left) {
			bool bit = (data & msb) != 0;
			data = ((data << 1) | (bit ? 1 : 0)) & mask;
			SetCcrIf(M68kFlags::Carry, bit);
		} else {
			bool bit = (data & 1) != 0;
			data = (data >> 1) | (bit ? msb : 0);
			SetCcrIf(M68kFlags::Carry, bit);
		}
	}

	_state.D[reg] = (_state.D[reg] & ~mask) | (data & mask);
	SetCcrIf(M68kFlags::Zero, (data & mask) == 0);
	SetCcrIf(M68kFlags::Negative, (data & msb) != 0);
	if (count == 0) ClearCcr(M68kFlags::Carry);
	AddCycles(6 + 2 * count);
}

void GenesisM68k::Op_ROXd(uint16_t opcode) {
	bool left = (opcode >> 8) & 1;
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);

	uint8_t reg = opcode & 7;
	uint8_t count;
	if (opcode & 0x20) {
		count = _state.D[(opcode >> 9) & 7] & 0x3F;
	} else {
		count = (opcode >> 9) & 7;
		if (count == 0) count = 8;
	}

	uint32_t data = _state.D[reg] & mask;
	ClearCcr(M68kFlags::Overflow);

	for (uint8_t i = 0; i < count; i++) {
		bool x = GetCcr(M68kFlags::Extend);
		if (left) {
			bool bit = (data & msb) != 0;
			data = ((data << 1) | (x ? 1 : 0)) & mask;
			SetCcrIf(M68kFlags::Carry | M68kFlags::Extend, bit);
		} else {
			bool bit = (data & 1) != 0;
			data = (data >> 1) | (x ? msb : 0);
			SetCcrIf(M68kFlags::Carry | M68kFlags::Extend, bit);
		}
	}

	_state.D[reg] = (_state.D[reg] & ~mask) | (data & mask);
	SetCcrIf(M68kFlags::Zero, (data & mask) == 0);
	SetCcrIf(M68kFlags::Negative, (data & msb) != 0);
	AddCycles(6 + 2 * count);
}

// ===== Bit operations =====

void GenesisM68k::Op_BTST(uint16_t opcode) {
	uint8_t bitNum;
	if (opcode & 0x0100) {
		bitNum = _state.D[(opcode >> 9) & 7];
	} else {
		bitNum = (uint8_t)FetchWord();
	}

	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode == 0) {
		bitNum &= 31;
		SetCcrIf(M68kFlags::Zero, !(_state.D[reg] & (1u << bitNum)));
	} else {
		bitNum &= 7;
		uint8_t data = (uint8_t)ReadEa(mode, reg, 0);
		SetCcrIf(M68kFlags::Zero, !(data & (1 << bitNum)));
	}
	AddCycles(4);
}

void GenesisM68k::Op_BSET(uint16_t opcode) {
	uint8_t bitNum;
	if (opcode & 0x0100) {
		bitNum = _state.D[(opcode >> 9) & 7];
	} else {
		bitNum = (uint8_t)FetchWord();
	}

	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode == 0) {
		bitNum &= 31;
		SetCcrIf(M68kFlags::Zero, !(_state.D[reg] & (1u << bitNum)));
		_state.D[reg] |= (1u << bitNum);
	} else {
		bitNum &= 7;
		uint32_t addr = GetEffectiveAddress(mode, reg, 0);
		uint8_t data = Read8(addr);
		SetCcrIf(M68kFlags::Zero, !(data & (1 << bitNum)));
		Write8(addr, data | (1 << bitNum));
	}
	AddCycles(8);
}

void GenesisM68k::Op_BCLR(uint16_t opcode) {
	uint8_t bitNum;
	if (opcode & 0x0100) {
		bitNum = _state.D[(opcode >> 9) & 7];
	} else {
		bitNum = (uint8_t)FetchWord();
	}

	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode == 0) {
		bitNum &= 31;
		SetCcrIf(M68kFlags::Zero, !(_state.D[reg] & (1u << bitNum)));
		_state.D[reg] &= ~(1u << bitNum);
	} else {
		bitNum &= 7;
		uint32_t addr = GetEffectiveAddress(mode, reg, 0);
		uint8_t data = Read8(addr);
		SetCcrIf(M68kFlags::Zero, !(data & (1 << bitNum)));
		Write8(addr, data & ~(1 << bitNum));
	}
	AddCycles(8);
}

void GenesisM68k::Op_BCHG(uint16_t opcode) {
	uint8_t bitNum;
	if (opcode & 0x0100) {
		bitNum = _state.D[(opcode >> 9) & 7];
	} else {
		bitNum = (uint8_t)FetchWord();
	}

	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode == 0) {
		bitNum &= 31;
		SetCcrIf(M68kFlags::Zero, !(_state.D[reg] & (1u << bitNum)));
		_state.D[reg] ^= (1u << bitNum);
	} else {
		bitNum &= 7;
		uint32_t addr = GetEffectiveAddress(mode, reg, 0);
		uint8_t data = Read8(addr);
		SetCcrIf(M68kFlags::Zero, !(data & (1 << bitNum)));
		Write8(addr, data ^ (1 << bitNum));
	}
	AddCycles(8);
}

// ===== Branch =====

void GenesisM68k::Op_BRA(uint16_t opcode) {
	int8_t disp8 = (int8_t)(opcode & 0xFF);
	int32_t offset;
	if (disp8 == 0) {
		offset = (int16_t)FetchWord();
	} else {
		offset = disp8;
	}
	_state.PC = (_state.PC - 2) + 2 + offset; // PC was already advanced past opcode
	if (disp8 != 0) _state.PC += 2; // For short branch, PC points after opcode word
	AddCycles(10);
}

void GenesisM68k::Op_BSR(uint16_t opcode) {
	int8_t disp8 = (int8_t)(opcode & 0xFF);
	int32_t offset;
	uint32_t returnPC;
	if (disp8 == 0) {
		offset = (int16_t)FetchWord();
		returnPC = _state.PC;
	} else {
		offset = disp8;
		returnPC = _state.PC;
	}
	Push32(returnPC);
	_state.PC = (_state.PC - (disp8 == 0 ? 4 : 2)) + 2 + offset;
	AddCycles(18);
}

void GenesisM68k::Op_Bcc(uint16_t opcode) {
	uint8_t cc = (opcode >> 8) & 0x0F;
	int8_t disp8 = (int8_t)(opcode & 0xFF);
	int32_t offset;

	if (disp8 == 0) {
		offset = (int16_t)FetchWord();
	} else {
		offset = disp8;
	}

	if (TestCondition(cc)) {
		uint32_t base = _state.PC - (disp8 == 0 ? 4 : 2);
		_state.PC = base + 2 + offset;
		AddCycles(10);
	} else {
		AddCycles(disp8 == 0 ? 12 : 8);
	}
}

void GenesisM68k::Op_DBcc(uint16_t opcode) {
	uint8_t cc = (opcode >> 8) & 0x0F;
	uint8_t reg = opcode & 7;
	int16_t disp = (int16_t)FetchWord();

	if (!TestCondition(cc)) {
		int16_t counter = (int16_t)(uint16_t)_state.D[reg];
		counter--;
		_state.D[reg] = (_state.D[reg] & 0xFFFF0000) | (uint16_t)counter;
		if (counter != -1) {
			_state.PC = (_state.PC - 4) + 2 + disp;
			AddCycles(10);
		} else {
			AddCycles(14);
		}
	} else {
		AddCycles(12);
	}
}

void GenesisM68k::Op_Scc(uint16_t opcode) {
	uint8_t cc = (opcode >> 8) & 0x0F;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	uint8_t val = TestCondition(cc) ? 0xFF : 0x00;
	WriteEa(mode, reg, 0, val);
	AddCycles(mode == 0 ? (TestCondition(cc) ? 6 : 4) : 8);
}

void GenesisM68k::Op_JMP(uint16_t opcode) {
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	_state.PC = GetEffectiveAddress(mode, reg, 2);
	AddCycles(8);
}

void GenesisM68k::Op_JSR(uint16_t opcode) {
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint32_t addr = GetEffectiveAddress(mode, reg, 2);
	Push32(_state.PC);
	_state.PC = addr;
	AddCycles(16);
}

void GenesisM68k::Op_RTS(uint16_t opcode) {
	_state.PC = Pop32();
	AddCycles(16);
}

void GenesisM68k::Op_RTE(uint16_t opcode) {
	if (!IsSupervisor()) {
		RaiseException(8); // Privilege violation
		return;
	}
	uint16_t newSR = Pop16();
	_state.PC = Pop32();

	bool wasSupervisor = IsSupervisor();
	_state.SR = newSR;
	if (wasSupervisor && !IsSupervisor()) {
		_state.SSP = _state.A[7];
		_state.A[7] = _state.USP;
	}
	AddCycles(20);
}

void GenesisM68k::Op_RTR(uint16_t opcode) {
	uint16_t ccr = Pop16();
	_state.SR = (_state.SR & 0xFF00) | (ccr & 0x001F);
	_state.PC = Pop32();
	AddCycles(20);
}

// ===== System =====

void GenesisM68k::Op_NOP(uint16_t opcode) {
	AddCycles(4);
}

void GenesisM68k::Op_TRAP(uint16_t opcode) {
	uint8_t vector = 32 + (opcode & 0x0F);
	RaiseException(vector);
}

void GenesisM68k::Op_RESET(uint16_t opcode) {
	if (!IsSupervisor()) {
		RaiseException(8);
		return;
	}
	// Reset external devices only (not CPU itself)
	AddCycles(132);
}

void GenesisM68k::Op_STOP(uint16_t opcode) {
	if (!IsSupervisor()) {
		RaiseException(8);
		return;
	}
	uint16_t newSR = FetchWord();
	_state.SR = newSR;
	_state.Stopped = true;
	AddCycles(4);
}

void GenesisM68k::Op_MOVE_SR(uint16_t opcode) {
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if ((opcode & 0xFFC0) == 0x40C0) {
		// MOVE from SR
		WriteEa(mode, reg, 1, _state.SR);
		AddCycles(6);
	} else if ((opcode & 0xFFC0) == 0x44C0) {
		// MOVE to CCR
		uint16_t data = (uint16_t)ReadEa(mode, reg, 1);
		_state.SR = (_state.SR & 0xFF00) | (data & 0x001F);
		AddCycles(12);
	} else if ((opcode & 0xFFC0) == 0x46C0) {
		// MOVE to SR
		if (!IsSupervisor()) {
			RaiseException(8);
			return;
		}
		uint16_t data = (uint16_t)ReadEa(mode, reg, 1);
		bool wasSupervisor = IsSupervisor();
		_state.SR = data;
		if (wasSupervisor && !IsSupervisor()) {
			_state.SSP = _state.A[7];
			_state.A[7] = _state.USP;
		}
		AddCycles(12);
	}
}

void GenesisM68k::Op_MOVE_USP(uint16_t opcode) {
	if (!IsSupervisor()) {
		RaiseException(8);
		return;
	}
	uint8_t reg = opcode & 7;
	if (opcode & 0x08) {
		// MOVE USP, An
		_state.A[reg] = _state.USP;
	} else {
		// MOVE An, USP
		_state.USP = _state.A[reg];
	}
	AddCycles(4);
}

void GenesisM68k::Op_ANDI_SR(uint16_t opcode) {
	if ((opcode & 0xFF) == 0x3C) {
		// ANDI to CCR
		uint16_t imm = FetchWord();
		_state.SR &= (0xFF00 | (imm & 0x001F));
	} else {
		if (!IsSupervisor()) { RaiseException(8); return; }
		uint16_t imm = FetchWord();
		_state.SR &= imm;
	}
	AddCycles(20);
}

void GenesisM68k::Op_ORI_SR(uint16_t opcode) {
	if ((opcode & 0xFF) == 0x3C) {
		uint16_t imm = FetchWord();
		_state.SR |= (imm & 0x001F);
	} else {
		if (!IsSupervisor()) { RaiseException(8); return; }
		uint16_t imm = FetchWord();
		_state.SR |= imm;
	}
	AddCycles(20);
}

void GenesisM68k::Op_EORI_SR(uint16_t opcode) {
	if ((opcode & 0xFF) == 0x3C) {
		uint16_t imm = FetchWord();
		_state.SR ^= (imm & 0x001F);
	} else {
		if (!IsSupervisor()) { RaiseException(8); return; }
		uint16_t imm = FetchWord();
		_state.SR ^= imm;
	}
	AddCycles(20);
}

void GenesisM68k::Op_ILLEGAL(uint16_t opcode) {
	_state.PC -= 2; // Back up to illegal opcode
	RaiseException(4); // Illegal instruction
}

// ===== Serialization =====

void GenesisM68k::Serialize(Serializer& s) {
	for (int i = 0; i < 8; i++) {
		SVI(_state.D[i]);
		SVI(_state.A[i]);
	}
	SV(_state.PC);
	SV(_state.USP);
	SV(_state.SSP);
	SV(_state.SR);
	SV(_state.CycleCount);
	SV(_state.Stopped);
}
