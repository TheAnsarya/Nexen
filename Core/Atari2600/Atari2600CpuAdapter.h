#pragma once
#include "pch.h"
#include <functional>

class Atari2600CpuAdapter {
private:
	std::function<uint8_t(uint16_t)> _read;
	std::function<void(uint16_t, uint8_t)> _write;
	uint16_t _pc = 0x1000;
	uint64_t _cycleCount = 0;
	uint8_t _a = 0;
	uint8_t _x = 0;
	uint8_t _y = 0;
	uint8_t _sp = 0xFD;
	uint8_t _status = 0x24;
	uint8_t _instructionCyclesRemaining = 0;

	static constexpr uint8_t FlagCarry = 0x01;
	static constexpr uint8_t FlagZero = 0x02;
	static constexpr uint8_t FlagInterruptDisable = 0x04;
	static constexpr uint8_t FlagDecimal = 0x08;
	static constexpr uint8_t FlagBreak = 0x10;
	static constexpr uint8_t FlagUnused = 0x20;
	static constexpr uint8_t FlagOverflow = 0x40;
	static constexpr uint8_t FlagNegative = 0x80;

	[[nodiscard]] uint8_t Read(uint16_t addr) const {
		return _read ? _read(addr & 0x1FFF) : 0xFF;
	}

	void Write(uint16_t addr, uint8_t value) const {
		if (_write) {
			_write(addr & 0x1FFF, value);
		}
	}

	[[nodiscard]] uint8_t FetchByte() {
		uint8_t value = Read(_pc);
		_pc = (_pc + 1) & 0x1FFF;
		return value;
	}

	[[nodiscard]] uint16_t FetchWord() {
		uint16_t low = FetchByte();
		uint16_t high = FetchByte();
		return (uint16_t)((high << 8) | low);
	}

	void SetFlag(uint8_t flag, bool enabled) {
		if (enabled) {
			_status |= flag;
		} else {
			_status &= (uint8_t)~flag;
		}
	}

	[[nodiscard]] bool GetFlag(uint8_t flag) const {
		return (_status & flag) != 0;
	}

	void UpdateZeroNegative(uint8_t value) {
		SetFlag(FlagZero, value == 0);
		SetFlag(FlagNegative, (value & 0x80) != 0);
	}

	[[nodiscard]] uint8_t AddWithCarry(uint8_t left, uint8_t right, bool subtract) {
		uint16_t operand = subtract ? (uint16_t)(right ^ 0xFF) : right;
		uint16_t carryIn = GetFlag(FlagCarry) ? 1 : 0;
		uint16_t sum = (uint16_t)left + operand + carryIn;
		uint8_t result = (uint8_t)sum;

		SetFlag(FlagCarry, sum > 0xFF);
		SetFlag(FlagOverflow, ((~(left ^ operand) & (left ^ result)) & 0x80) != 0);
		UpdateZeroNegative(result);
		return result;
	}

	void Compare(uint8_t left, uint8_t right) {
		uint16_t diff = (uint16_t)left - right;
		SetFlag(FlagCarry, left >= right);
		SetFlag(FlagZero, left == right);
		SetFlag(FlagNegative, (diff & 0x80) != 0);
	}

	[[nodiscard]] uint8_t BranchIf(bool condition) {
		int8_t offset = (int8_t)FetchByte();
		if (!condition) {
			return 2;
		}

		uint16_t sourcePc = _pc;
		uint16_t targetPc = (uint16_t)(_pc + offset);
		_pc = targetPc & 0x1FFF;

		uint8_t cycles = 3;
		if (((sourcePc ^ targetPc) & 0xFF00) != 0) {
			cycles++;
		}
		return cycles;
	}

	void PushByte(uint8_t value) {
		Write(0x0100 | _sp, value);
		_sp--;
	}

	[[nodiscard]] uint8_t PullByte() {
		_sp++;
		return Read(0x0100 | _sp);
	}

	void PushWord(uint16_t value) {
		PushByte((uint8_t)(value >> 8));
		PushByte((uint8_t)(value & 0xff));
	}

	[[nodiscard]] uint16_t PullWord() {
		uint16_t low = PullByte();
		uint16_t high = PullByte();
		return (uint16_t)((high << 8) | low);
	}

	[[nodiscard]] uint16_t AddrZP() { return FetchByte(); }
	[[nodiscard]] uint16_t AddrZPX() { return (uint8_t)(FetchByte() + _x); }
	[[nodiscard]] uint16_t AddrZPY() { return (uint8_t)(FetchByte() + _y); }
	[[nodiscard]] uint16_t AddrAbs() { return FetchWord(); }

	[[nodiscard]] uint16_t AddrAbsX(bool& pageCrossed) {
		uint16_t base = FetchWord();
		uint16_t addr = (uint16_t)(base + _x);
		pageCrossed = ((base ^ addr) & 0xff00) != 0;
		return addr;
	}

	[[nodiscard]] uint16_t AddrAbsY(bool& pageCrossed) {
		uint16_t base = FetchWord();
		uint16_t addr = (uint16_t)(base + _y);
		pageCrossed = ((base ^ addr) & 0xff00) != 0;
		return addr;
	}

	[[nodiscard]] uint16_t AddrIndX() {
		uint8_t zp = (uint8_t)(FetchByte() + _x);
		uint16_t lo = Read(zp);
		uint16_t hi = Read((uint8_t)(zp + 1));
		return (uint16_t)((hi << 8) | lo);
	}

	[[nodiscard]] uint16_t AddrIndY(bool& pageCrossed) {
		uint8_t zp = FetchByte();
		uint16_t lo = Read(zp);
		uint16_t hi = Read((uint8_t)(zp + 1));
		uint16_t base = (uint16_t)((hi << 8) | lo);
		uint16_t addr = (uint16_t)(base + _y);
		pageCrossed = ((base ^ addr) & 0xff00) != 0;
		return addr;
	}

	[[nodiscard]] uint8_t DoASL(uint8_t v) {
		SetFlag(FlagCarry, (v & 0x80) != 0);
		v = (uint8_t)(v << 1);
		UpdateZeroNegative(v);
		return v;
	}

	[[nodiscard]] uint8_t DoLSR(uint8_t v) {
		SetFlag(FlagCarry, (v & 0x01) != 0);
		v = (uint8_t)(v >> 1);
		UpdateZeroNegative(v);
		return v;
	}

	[[nodiscard]] uint8_t DoROL(uint8_t v) {
		bool oldCarry = GetFlag(FlagCarry);
		SetFlag(FlagCarry, (v & 0x80) != 0);
		v = (uint8_t)((v << 1) | (oldCarry ? 1 : 0));
		UpdateZeroNegative(v);
		return v;
	}

	[[nodiscard]] uint8_t DoROR(uint8_t v) {
		bool oldCarry = GetFlag(FlagCarry);
		SetFlag(FlagCarry, (v & 0x01) != 0);
		v = (uint8_t)((v >> 1) | (oldCarry ? 0x80 : 0));
		UpdateZeroNegative(v);
		return v;
	}

	void DoBIT(uint8_t v) {
		SetFlag(FlagZero, (_a & v) == 0);
		SetFlag(FlagOverflow, (v & 0x40) != 0);
		SetFlag(FlagNegative, (v & 0x80) != 0);
	}

	[[nodiscard]] uint8_t ExecuteInstruction() {
		uint8_t opcode = FetchByte();
		bool pageCrossed = false;

		switch (opcode) {
			// BRK
			case 0x00: {
				_pc = (_pc + 1) & 0x1fff;
				PushWord(_pc);
				PushByte(_status | FlagBreak | FlagUnused);
				SetFlag(FlagInterruptDisable, true);
				uint16_t lo = Read(0x1ffe);
				uint16_t hi = Read(0x1fff);
				_pc = ((hi << 8) | lo) & 0x1fff;
				return 7;
			}

			// ORA
			case 0x01: _a |= Read(AddrIndX()); UpdateZeroNegative(_a); return 6;
			case 0x05: _a |= Read(AddrZP()); UpdateZeroNegative(_a); return 3;
			case 0x09: _a |= FetchByte(); UpdateZeroNegative(_a); return 2;
			case 0x0d: _a |= Read(AddrAbs()); UpdateZeroNegative(_a); return 4;
			case 0x11: _a |= Read(AddrIndY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 6 : 5;
			case 0x15: _a |= Read(AddrZPX()); UpdateZeroNegative(_a); return 4;
			case 0x19: _a |= Read(AddrAbsY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;
			case 0x1d: _a |= Read(AddrAbsX(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;

			// ASL
			case 0x06: { uint16_t a = AddrZP(); Write(a, DoASL(Read(a))); return 5; }
			case 0x0a: _a = DoASL(_a); return 2;
			case 0x0e: { uint16_t a = AddrAbs(); Write(a, DoASL(Read(a))); return 6; }
			case 0x16: { uint16_t a = AddrZPX(); Write(a, DoASL(Read(a))); return 6; }
			case 0x1e: { uint16_t a = AddrAbsX(pageCrossed); Write(a, DoASL(Read(a))); return 7; }

			// PHP
			case 0x08: PushByte(_status | FlagBreak | FlagUnused); return 3;

			// BPL
			case 0x10: return BranchIf(!GetFlag(FlagNegative));

			// CLC
			case 0x18: SetFlag(FlagCarry, false); return 2;

			// JSR
			case 0x20: {
				uint16_t target = FetchWord();
				PushWord((_pc - 1) & 0x1fff);
				_pc = target & 0x1fff;
				return 6;
			}

			// AND
			case 0x21: _a &= Read(AddrIndX()); UpdateZeroNegative(_a); return 6;
			case 0x25: _a &= Read(AddrZP()); UpdateZeroNegative(_a); return 3;
			case 0x29: _a &= FetchByte(); UpdateZeroNegative(_a); return 2;
			case 0x2d: _a &= Read(AddrAbs()); UpdateZeroNegative(_a); return 4;
			case 0x31: _a &= Read(AddrIndY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 6 : 5;
			case 0x35: _a &= Read(AddrZPX()); UpdateZeroNegative(_a); return 4;
			case 0x39: _a &= Read(AddrAbsY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;
			case 0x3d: _a &= Read(AddrAbsX(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;

			// BIT
			case 0x24: DoBIT(Read(AddrZP())); return 3;
			case 0x2c: DoBIT(Read(AddrAbs())); return 4;

			// ROL
			case 0x26: { uint16_t a = AddrZP(); Write(a, DoROL(Read(a))); return 5; }
			case 0x2a: _a = DoROL(_a); return 2;
			case 0x2e: { uint16_t a = AddrAbs(); Write(a, DoROL(Read(a))); return 6; }
			case 0x36: { uint16_t a = AddrZPX(); Write(a, DoROL(Read(a))); return 6; }
			case 0x3e: { uint16_t a = AddrAbsX(pageCrossed); Write(a, DoROL(Read(a))); return 7; }

			// PLP
			case 0x28: _status = PullByte(); SetFlag(FlagUnused, true); return 4;

			// BMI
			case 0x30: return BranchIf(GetFlag(FlagNegative));

			// SEC
			case 0x38: SetFlag(FlagCarry, true); return 2;

			// RTI
			case 0x40: {
				_status = PullByte();
				SetFlag(FlagUnused, true);
				_pc = PullWord() & 0x1fff;
				return 6;
			}

			// EOR
			case 0x41: _a ^= Read(AddrIndX()); UpdateZeroNegative(_a); return 6;
			case 0x45: _a ^= Read(AddrZP()); UpdateZeroNegative(_a); return 3;
			case 0x49: _a ^= FetchByte(); UpdateZeroNegative(_a); return 2;
			case 0x4d: _a ^= Read(AddrAbs()); UpdateZeroNegative(_a); return 4;
			case 0x51: _a ^= Read(AddrIndY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 6 : 5;
			case 0x55: _a ^= Read(AddrZPX()); UpdateZeroNegative(_a); return 4;
			case 0x59: _a ^= Read(AddrAbsY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;
			case 0x5d: _a ^= Read(AddrAbsX(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;

			// LSR
			case 0x46: { uint16_t a = AddrZP(); Write(a, DoLSR(Read(a))); return 5; }
			case 0x4a: _a = DoLSR(_a); return 2;
			case 0x4e: { uint16_t a = AddrAbs(); Write(a, DoLSR(Read(a))); return 6; }
			case 0x56: { uint16_t a = AddrZPX(); Write(a, DoLSR(Read(a))); return 6; }
			case 0x5e: { uint16_t a = AddrAbsX(pageCrossed); Write(a, DoLSR(Read(a))); return 7; }

			// PHA
			case 0x48: PushByte(_a); return 3;

			// JMP abs
			case 0x4c: _pc = FetchWord() & 0x1fff; return 3;

			// BVC
			case 0x50: return BranchIf(!GetFlag(FlagOverflow));

			// CLI
			case 0x58: SetFlag(FlagInterruptDisable, false); return 2;

			// RTS
			case 0x60: _pc = (PullWord() + 1) & 0x1fff; return 6;

			// ADC
			case 0x61: _a = AddWithCarry(_a, Read(AddrIndX()), false); return 6;
			case 0x65: _a = AddWithCarry(_a, Read(AddrZP()), false); return 3;
			case 0x69: _a = AddWithCarry(_a, FetchByte(), false); return 2;
			case 0x6d: _a = AddWithCarry(_a, Read(AddrAbs()), false); return 4;
			case 0x71: _a = AddWithCarry(_a, Read(AddrIndY(pageCrossed)), false); return pageCrossed ? 6 : 5;
			case 0x75: _a = AddWithCarry(_a, Read(AddrZPX()), false); return 4;
			case 0x79: _a = AddWithCarry(_a, Read(AddrAbsY(pageCrossed)), false); return pageCrossed ? 5 : 4;
			case 0x7d: _a = AddWithCarry(_a, Read(AddrAbsX(pageCrossed)), false); return pageCrossed ? 5 : 4;

			// ROR
			case 0x66: { uint16_t a = AddrZP(); Write(a, DoROR(Read(a))); return 5; }
			case 0x6a: _a = DoROR(_a); return 2;
			case 0x6e: { uint16_t a = AddrAbs(); Write(a, DoROR(Read(a))); return 6; }
			case 0x76: { uint16_t a = AddrZPX(); Write(a, DoROR(Read(a))); return 6; }
			case 0x7e: { uint16_t a = AddrAbsX(pageCrossed); Write(a, DoROR(Read(a))); return 7; }

			// PLA
			case 0x68: _a = PullByte(); UpdateZeroNegative(_a); return 4;

			// JMP indirect (with 6502 page-crossing bug)
			case 0x6c: {
				uint16_t ptr = FetchWord();
				uint16_t lo = Read(ptr);
				uint16_t hiAddr = (ptr & 0xff00) | ((ptr + 1) & 0x00ff);
				uint16_t hi = Read(hiAddr);
				_pc = ((hi << 8) | lo) & 0x1fff;
				return 5;
			}

			// BVS
			case 0x70: return BranchIf(GetFlag(FlagOverflow));

			// SEI
			case 0x78: SetFlag(FlagInterruptDisable, true); return 2;

			// STA
			case 0x81: Write(AddrIndX(), _a); return 6;
			case 0x85: Write(AddrZP(), _a); return 3;
			case 0x8d: Write(AddrAbs(), _a); return 4;
			case 0x91: Write(AddrIndY(pageCrossed), _a); return 6;
			case 0x95: Write(AddrZPX(), _a); return 4;
			case 0x99: Write(AddrAbsY(pageCrossed), _a); return 5;
			case 0x9d: Write(AddrAbsX(pageCrossed), _a); return 5;

			// STY
			case 0x84: Write(AddrZP(), _y); return 3;
			case 0x8c: Write(AddrAbs(), _y); return 4;
			case 0x94: Write(AddrZPX(), _y); return 4;

			// STX
			case 0x86: Write(AddrZP(), _x); return 3;
			case 0x8e: Write(AddrAbs(), _x); return 4;
			case 0x96: Write(AddrZPY(), _x); return 4;

			// DEY
			case 0x88: _y--; UpdateZeroNegative(_y); return 2;

			// TXA
			case 0x8a: _a = _x; UpdateZeroNegative(_a); return 2;

			// BCC
			case 0x90: return BranchIf(!GetFlag(FlagCarry));

			// TYA
			case 0x98: _a = _y; UpdateZeroNegative(_a); return 2;

			// TXS (does NOT set flags)
			case 0x9a: _sp = _x; return 2;

			// LDY
			case 0xa0: _y = FetchByte(); UpdateZeroNegative(_y); return 2;
			case 0xa4: _y = Read(AddrZP()); UpdateZeroNegative(_y); return 3;
			case 0xac: _y = Read(AddrAbs()); UpdateZeroNegative(_y); return 4;
			case 0xb4: _y = Read(AddrZPX()); UpdateZeroNegative(_y); return 4;
			case 0xbc: _y = Read(AddrAbsX(pageCrossed)); UpdateZeroNegative(_y); return pageCrossed ? 5 : 4;

			// LDA
			case 0xa1: _a = Read(AddrIndX()); UpdateZeroNegative(_a); return 6;
			case 0xa5: _a = Read(AddrZP()); UpdateZeroNegative(_a); return 3;
			case 0xa9: _a = FetchByte(); UpdateZeroNegative(_a); return 2;
			case 0xad: _a = Read(AddrAbs()); UpdateZeroNegative(_a); return 4;
			case 0xb1: _a = Read(AddrIndY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 6 : 5;
			case 0xb5: _a = Read(AddrZPX()); UpdateZeroNegative(_a); return 4;
			case 0xb9: _a = Read(AddrAbsY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;
			case 0xbd: _a = Read(AddrAbsX(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;

			// LDX
			case 0xa2: _x = FetchByte(); UpdateZeroNegative(_x); return 2;
			case 0xa6: _x = Read(AddrZP()); UpdateZeroNegative(_x); return 3;
			case 0xae: _x = Read(AddrAbs()); UpdateZeroNegative(_x); return 4;
			case 0xb6: _x = Read(AddrZPY()); UpdateZeroNegative(_x); return 4;
			case 0xbe: _x = Read(AddrAbsY(pageCrossed)); UpdateZeroNegative(_x); return pageCrossed ? 5 : 4;

			// TAY
			case 0xa8: _y = _a; UpdateZeroNegative(_y); return 2;

			// TAX
			case 0xaa: _x = _a; UpdateZeroNegative(_x); return 2;

			// BCS
			case 0xb0: return BranchIf(GetFlag(FlagCarry));

			// CLV
			case 0xb8: SetFlag(FlagOverflow, false); return 2;

			// TSX
			case 0xba: _x = _sp; UpdateZeroNegative(_x); return 2;

			// CPY
			case 0xc0: Compare(_y, FetchByte()); return 2;
			case 0xc4: Compare(_y, Read(AddrZP())); return 3;
			case 0xcc: Compare(_y, Read(AddrAbs())); return 4;

			// CMP
			case 0xc1: Compare(_a, Read(AddrIndX())); return 6;
			case 0xc5: Compare(_a, Read(AddrZP())); return 3;
			case 0xc9: Compare(_a, FetchByte()); return 2;
			case 0xcd: Compare(_a, Read(AddrAbs())); return 4;
			case 0xd1: Compare(_a, Read(AddrIndY(pageCrossed))); return pageCrossed ? 6 : 5;
			case 0xd5: Compare(_a, Read(AddrZPX())); return 4;
			case 0xd9: Compare(_a, Read(AddrAbsY(pageCrossed))); return pageCrossed ? 5 : 4;
			case 0xdd: Compare(_a, Read(AddrAbsX(pageCrossed))); return pageCrossed ? 5 : 4;

			// DEC
			case 0xc6: { uint16_t a = AddrZP(); uint8_t v = Read(a) - 1; Write(a, v); UpdateZeroNegative(v); return 5; }
			case 0xce: { uint16_t a = AddrAbs(); uint8_t v = Read(a) - 1; Write(a, v); UpdateZeroNegative(v); return 6; }
			case 0xd6: { uint16_t a = AddrZPX(); uint8_t v = Read(a) - 1; Write(a, v); UpdateZeroNegative(v); return 6; }
			case 0xde: { uint16_t a = AddrAbsX(pageCrossed); uint8_t v = Read(a) - 1; Write(a, v); UpdateZeroNegative(v); return 7; }

			// INY
			case 0xc8: _y++; UpdateZeroNegative(_y); return 2;

			// DEX
			case 0xca: _x--; UpdateZeroNegative(_x); return 2;

			// BNE
			case 0xd0: return BranchIf(!GetFlag(FlagZero));

			// CLD
			case 0xd8: SetFlag(FlagDecimal, false); return 2;

			// CPX
			case 0xe0: Compare(_x, FetchByte()); return 2;
			case 0xe4: Compare(_x, Read(AddrZP())); return 3;
			case 0xec: Compare(_x, Read(AddrAbs())); return 4;

			// SBC
			case 0xe1: _a = AddWithCarry(_a, Read(AddrIndX()), true); return 6;
			case 0xe5: _a = AddWithCarry(_a, Read(AddrZP()), true); return 3;
			case 0xe9: _a = AddWithCarry(_a, FetchByte(), true); return 2;
			case 0xed: _a = AddWithCarry(_a, Read(AddrAbs()), true); return 4;
			case 0xf1: _a = AddWithCarry(_a, Read(AddrIndY(pageCrossed)), true); return pageCrossed ? 6 : 5;
			case 0xf5: _a = AddWithCarry(_a, Read(AddrZPX()), true); return 4;
			case 0xf9: _a = AddWithCarry(_a, Read(AddrAbsY(pageCrossed)), true); return pageCrossed ? 5 : 4;
			case 0xfd: _a = AddWithCarry(_a, Read(AddrAbsX(pageCrossed)), true); return pageCrossed ? 5 : 4;

			// INC
			case 0xe6: { uint16_t a = AddrZP(); uint8_t v = Read(a) + 1; Write(a, v); UpdateZeroNegative(v); return 5; }
			case 0xee: { uint16_t a = AddrAbs(); uint8_t v = Read(a) + 1; Write(a, v); UpdateZeroNegative(v); return 6; }
			case 0xf6: { uint16_t a = AddrZPX(); uint8_t v = Read(a) + 1; Write(a, v); UpdateZeroNegative(v); return 6; }
			case 0xfe: { uint16_t a = AddrAbsX(pageCrossed); uint8_t v = Read(a) + 1; Write(a, v); UpdateZeroNegative(v); return 7; }

			// INX
			case 0xe8: _x++; UpdateZeroNegative(_x); return 2;

			// NOP
			case 0xea: return 2;

			// BEQ
			case 0xf0: return BranchIf(GetFlag(FlagZero));

			// SED
			case 0xf8: SetFlag(FlagDecimal, true); return 2;

			default:
				return 2;
		}
	}

public:
	void Reset() {
		_pc = 0x1000;
		_cycleCount = 0;
		_a = 0;
		_x = 0;
		_y = 0;
		_sp = 0xFD;
		_status = FlagUnused | FlagInterruptDisable;
		_instructionCyclesRemaining = 0;
	}

	void SetReadCallback(std::function<uint8_t(uint16_t)> cb) {
		_read = std::move(cb);
	}

	void SetWriteCallback(std::function<void(uint16_t, uint8_t)> cb) {
		_write = std::move(cb);
	}

	void StepCycles(uint32_t cycles) {
		for (uint32_t i = 0; i < cycles; i++) {
			if (_instructionCyclesRemaining == 0) {
				_instructionCyclesRemaining = ExecuteInstruction();
				if (_instructionCyclesRemaining == 0) {
					_instructionCyclesRemaining = 1;
				}
			}

			_instructionCyclesRemaining--;
			_cycleCount++;
		}
	}

	uint64_t GetCycleCount() const {
		return _cycleCount;
	}

	uint16_t GetProgramCounter() const {
		return _pc;
	}

	void ExportState(uint16_t& programCounter, uint64_t& cycleCount, uint8_t& a, uint8_t& x, uint8_t& y, uint8_t& sp, uint8_t& status, uint8_t& remainingCycles) const {
		programCounter = _pc;
		cycleCount = _cycleCount;
		a = _a;
		x = _x;
		y = _y;
		sp = _sp;
		status = _status;
		remainingCycles = _instructionCyclesRemaining;
	}

	void ImportState(uint16_t programCounter, uint64_t cycleCount, uint8_t a, uint8_t x, uint8_t y, uint8_t sp, uint8_t status, uint8_t remainingCycles) {
		_pc = programCounter & 0x1FFF;
		_cycleCount = cycleCount;
		_a = a;
		_x = x;
		_y = y;
		_sp = sp;
		_status = status;
		_instructionCyclesRemaining = remainingCycles;
	}
};
