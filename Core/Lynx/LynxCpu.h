#if (defined(DUMMYCPU) && !defined(__DUMMYLYNXCPU__H)) || (!defined(DUMMYCPU) && !defined(__LYNXCPU__H))
#ifdef DUMMYCPU
#define __DUMMYLYNXCPU__H
#else
#define __LYNXCPU__H
#endif

#include "pch.h"
#include "Lynx/LynxTypes.h"
#include "Shared/MemoryOperationType.h"
#include "Utilities/ISerializable.h"

class Emulator;
class LynxConsole;
class LynxMemoryManager;

/// <summary>
/// WDC 65C02 CPU core for the Atari Lynx.
///
/// Key differences from NMOS 6502 (as used in NES):
/// - New instructions: BRA, PHX, PHY, PLX, PLY, STZ, TRB, TSB, WAI, STP
/// - New addressing mode: (zp) zero page indirect
/// - JMP ($xxFF) bug fixed — wraps page correctly
/// - RMW instructions don't write-back old value before new
/// - ADC/SBC in decimal mode set flags correctly + extra cycle
/// - All undefined opcodes are NOPs (not illegal ops)
/// - No NMI on Lynx hardware (vector exists but unused)
/// </summary>
class LynxCpu final : public ISerializable {
private:
	typedef void (LynxCpu::*OpFunc)();

	Emulator* _emu = nullptr;
	LynxConsole* _console = nullptr;
	LynxMemoryManager* _memoryManager = nullptr;

	LynxCpuState _state = {};

	OpFunc _opTable[256] = {};
	LynxAddrMode _addrMode[256] = {};
	LynxAddrMode _instAddrMode = LynxAddrMode::None;
	uint16_t _operand = 0;

	bool _irqPending = false;

	// Precomputed NZ flag table: for each byte value 0-255, stores the correct
	// Zero and Negative flag bits. This replaces two conditional branches per
	// instruction in SetZeroNeg() with a single indexed lookup.
	// Entry[0] = Zero flag (0x02); Entry[128-255] = Negative flag (0x80); else 0.
	static constexpr auto _nzTable = [] {
		std::array<uint8_t, 256> table{};
		for (int i = 0; i < 256; i++) {
			table[i] = (i == 0 ? LynxPSFlags::Zero : 0)
			         | ((i & 0x80) ? LynxPSFlags::Negative : 0);
		}
		return table;
	}();

	// =========================================================================
	// Memory Access — each access counts as one CPU cycle
	// =========================================================================

	uint8_t MemoryRead(uint16_t addr, MemoryOperationType opType = MemoryOperationType::Read);
	void MemoryWrite(uint16_t addr, uint8_t value, MemoryOperationType opType = MemoryOperationType::Write);

	/// <summary>Read and increment PC</summary>
	__forceinline uint8_t ReadByte() {
		uint8_t val = MemoryRead(_state.PC, MemoryOperationType::ExecOperand);
		_state.PC++;
		return val;
	}

	/// <summary>Read 16-bit word at addr (little-endian)</summary>
	__forceinline uint16_t MemoryReadWord(uint16_t addr) {
		uint8_t lo = MemoryRead(addr);
		uint8_t hi = MemoryRead(addr + 1);
		return lo | (hi << 8);
	}

	/// <summary>Dummy read for cycle accuracy</summary>
	__forceinline void DummyRead() {
		MemoryRead(_state.PC, MemoryOperationType::DummyRead);
	}

	// =========================================================================
	// Stack Operations
	// =========================================================================

	__forceinline void Push(uint8_t value) {
		MemoryWrite(0x0100 | _state.SP, value);
		_state.SP--;
	}

	__forceinline void PushWord(uint16_t value) {
		Push(static_cast<uint8_t>(value >> 8));
		Push(static_cast<uint8_t>(value));
	}

	__forceinline uint8_t Pop() {
		_state.SP++;
		return MemoryRead(0x0100 | _state.SP);
	}

	__forceinline uint16_t PopWord() {
		uint8_t lo = Pop();
		uint8_t hi = Pop();
		return lo | (hi << 8);
	}

	// =========================================================================
	// Flag Helpers
	// =========================================================================

	[[nodiscard]] __forceinline bool CheckFlag(uint8_t flag) const { return (_state.PS & flag) != 0; }
	__forceinline void SetFlag(uint8_t flag) { _state.PS |= flag; }
	__forceinline void ClearFlag(uint8_t flag) { _state.PS &= ~flag; }

	__forceinline void SetFlagValue(uint8_t flag, bool set) {
		// Branchless: clear the flag bit, then OR in (flag & -set).
		// -true == 0xFF (all bits set), -false == 0x00 (no bits).
		// Avoids a branch on every ADC, SBC, CMP, shift, and rotate.
		_state.PS = (_state.PS & ~flag) | (flag & static_cast<uint8_t>(-static_cast<int8_t>(set)));
	}

	__forceinline void SetZeroNeg(uint8_t value) {
		// Single table lookup replaces two conditional branches.
		// Mask out old Z+N, OR in precomputed flags for this value.
		_state.PS = (_state.PS & ~(LynxPSFlags::Zero | LynxPSFlags::Negative)) | _nzTable[value];
	}

	// =========================================================================
	// Register Accessors
	// =========================================================================

	[[nodiscard]] __forceinline uint8_t A() const { return _state.A; }
	[[nodiscard]] __forceinline uint8_t X() const { return _state.X; }
	[[nodiscard]] __forceinline uint8_t Y() const { return _state.Y; }
	[[nodiscard]] __forceinline uint16_t PC() const { return _state.PC; }
	[[nodiscard]] __forceinline uint8_t SP() const { return _state.SP; }
	[[nodiscard]] __forceinline uint8_t PS() const { return _state.PS; }

	__forceinline void SetA(uint8_t v) { _state.A = v; SetZeroNeg(v); }
	__forceinline void SetX(uint8_t v) { _state.X = v; SetZeroNeg(v); }
	__forceinline void SetY(uint8_t v) { _state.Y = v; SetZeroNeg(v); }
	__forceinline void SetPC(uint16_t v) { _state.PC = v; }
	__forceinline void SetSP(uint8_t v) { _state.SP = v; }
	__forceinline void SetPS(uint8_t v) { _state.PS = (v & 0xcf) | LynxPSFlags::Reserved; }

	// =========================================================================
	// Operand Access
	// =========================================================================

	[[nodiscard]] __forceinline uint16_t GetOperand() { return _operand; }

	[[nodiscard]] __forceinline uint8_t GetOperandValue() {
		if (_instAddrMode >= LynxAddrMode::Zpg) {
			return MemoryRead(GetOperand());
		} else {
			return static_cast<uint8_t>(GetOperand());
		}
	}

	// =========================================================================
	// Addressing Mode Implementations
	// =========================================================================

	uint16_t GetImmediate() { return ReadByte(); }
	uint16_t GetZpgAddr() { return ReadByte(); }
	uint16_t GetZpgXAddr() { uint8_t addr = ReadByte(); DummyRead(); return static_cast<uint8_t>(addr + X()); }
	uint16_t GetZpgYAddr() { uint8_t addr = ReadByte(); DummyRead(); return static_cast<uint8_t>(addr + Y()); }
	uint16_t GetAbsAddr() { uint8_t lo = ReadByte(); return lo | (ReadByte() << 8); }

	uint16_t GetAbsXAddr(bool forWrite) {
		uint8_t lo = ReadByte();
		uint8_t hi = ReadByte();
		uint16_t addr = (lo | (hi << 8)) + X();
		if (forWrite || (lo + X()) > 0xff) {
			DummyRead(); // Page cross penalty
		}
		return addr;
	}

	uint16_t GetAbsYAddr(bool forWrite) {
		uint8_t lo = ReadByte();
		uint8_t hi = ReadByte();
		uint16_t addr = (lo | (hi << 8)) + Y();
		if (forWrite || (lo + Y()) > 0xff) {
			DummyRead();
		}
		return addr;
	}

	uint16_t GetIndXAddr() {
		uint8_t zpAddr = ReadByte();
		DummyRead(); // Add X cycle
		zpAddr += X();
		uint8_t lo = MemoryRead(zpAddr);
		uint8_t hi = MemoryRead(static_cast<uint8_t>(zpAddr + 1));
		return lo | (hi << 8);
	}

	uint16_t GetIndYAddr(bool forWrite) {
		uint8_t zpAddr = ReadByte();
		uint8_t lo = MemoryRead(zpAddr);
		uint8_t hi = MemoryRead(static_cast<uint8_t>(zpAddr + 1));
		uint16_t addr = (lo | (hi << 8)) + Y();
		if (forWrite || (lo + Y()) > 0xff) {
			DummyRead();
		}
		return addr;
	}

	/// <summary>65C02: ($nn) — zero page indirect without index</summary>
	uint16_t GetZpgIndAddr() {
		uint8_t zpAddr = ReadByte();
		uint8_t lo = MemoryRead(zpAddr);
		uint8_t hi = MemoryRead(static_cast<uint8_t>(zpAddr + 1));
		return lo | (hi << 8);
	}

	/// <summary>65C02: ($nnnn) — JMP indirect, NO page-crossing bug</summary>
	uint16_t GetIndAddr() {
		uint16_t addr = GetAbsAddr();
		// 65C02 fix: reads correctly across page boundary
		uint8_t lo = MemoryRead(addr);
		uint8_t hi = MemoryRead(addr + 1);
		return lo | (hi << 8);
	}

	/// <summary>65C02: ($nnnn,X) — JMP (abs,X)</summary>
	uint16_t GetAbsIndXAddr() {
		uint16_t addr = GetAbsAddr();
		DummyRead(); // Add X cycle
		addr += X();
		uint8_t lo = MemoryRead(addr);
		uint8_t hi = MemoryRead(addr + 1);
		return lo | (hi << 8);
	}

	uint16_t FetchOperand();

	// =========================================================================
	// Page-crossing check
	// =========================================================================

	[[nodiscard]] __forceinline bool CheckPageCrossed(uint16_t base, int8_t offset) {
		return ((base + offset) & 0xff00) != (base & 0xff00);
	}

	// =========================================================================
	// Instruction Implementations
	// =========================================================================

	// --- Load/Store ---
	void LDA() { SetA(GetOperandValue()); }
	void LDX() { SetX(GetOperandValue()); }
	void LDY() { SetY(GetOperandValue()); }
	void STA() { MemoryWrite(GetOperand(), A()); }
	void STX() { MemoryWrite(GetOperand(), X()); }
	void STY() { MemoryWrite(GetOperand(), Y()); }
	void STZ() { MemoryWrite(GetOperand(), 0); }  // 65C02

	// --- Transfer ---
	void TAX() { SetX(A()); }
	void TAY() { SetY(A()); }
	void TXA() { SetA(X()); }
	void TYA() { SetA(Y()); }
	void TSX() { SetX(SP()); }
	void TXS() { SetSP(X()); }

	// --- Stack ---
	void PHA() { Push(A()); }
	void PLA() { DummyRead(); SetA(Pop()); }
	void PHP() { Push(PS() | LynxPSFlags::Break | LynxPSFlags::Reserved); }
	void PLP() { DummyRead(); SetPS(Pop()); }
	void PHX() { Push(X()); }      // 65C02
	void PLX() { DummyRead(); SetX(Pop()); }  // 65C02
	void PHY() { Push(Y()); }      // 65C02
	void PLY() { DummyRead(); SetY(Pop()); }  // 65C02

	// --- Arithmetic ---
	void ADC();
	void SBC();

	void CMP() { CmpRegister(A(), GetOperandValue()); }
	void CPX() { CmpRegister(X(), GetOperandValue()); }
	void CPY() { CmpRegister(Y(), GetOperandValue()); }

	__forceinline void CmpRegister(uint8_t reg, uint8_t value) {
		uint16_t result = reg - value;
		SetFlagValue(LynxPSFlags::Carry, reg >= value);
		SetZeroNeg(static_cast<uint8_t>(result));
	}

	void INC_A() { SetA(A() + 1); }  // 65C02 INC A
	void DEC_A() { SetA(A() - 1); }  // 65C02 DEC A

	void INC() {
		uint16_t addr = GetOperand();
		uint8_t val = MemoryRead(addr);
		DummyRead(); // 65C02: no double write-back, just a dummy read
		val++;
		SetZeroNeg(val);
		MemoryWrite(addr, val);
	}

	void DEC() {
		uint16_t addr = GetOperand();
		uint8_t val = MemoryRead(addr);
		DummyRead();
		val--;
		SetZeroNeg(val);
		MemoryWrite(addr, val);
	}

	void INX() { SetX(X() + 1); }
	void INY() { SetY(Y() + 1); }
	void DEX() { SetX(X() - 1); }
	void DEY() { SetY(Y() - 1); }

	// --- Logic ---
	void AND() { SetA(A() & GetOperandValue()); }
	void ORA() { SetA(A() | GetOperandValue()); }
	void EOR() { SetA(A() ^ GetOperandValue()); }

	void BIT();
	void BIT_Imm();  // 65C02: BIT immediate doesn't affect N/V

	// --- Shifts/Rotates ---
	void ASL_A() {
		SetFlagValue(LynxPSFlags::Carry, (A() & 0x80) != 0);
		SetA(A() << 1);
	}

	void ASL() {
		uint16_t addr = GetOperand();
		uint8_t val = MemoryRead(addr);
		DummyRead();
		SetFlagValue(LynxPSFlags::Carry, (val & 0x80) != 0);
		val <<= 1;
		SetZeroNeg(val);
		MemoryWrite(addr, val);
	}

	void LSR_A() {
		SetFlagValue(LynxPSFlags::Carry, (A() & 0x01) != 0);
		SetA(A() >> 1);
	}

	void LSR() {
		uint16_t addr = GetOperand();
		uint8_t val = MemoryRead(addr);
		DummyRead();
		SetFlagValue(LynxPSFlags::Carry, (val & 0x01) != 0);
		val >>= 1;
		SetZeroNeg(val);
		MemoryWrite(addr, val);
	}

	void ROL_A() {
		uint8_t carry = CheckFlag(LynxPSFlags::Carry) ? 1 : 0;
		SetFlagValue(LynxPSFlags::Carry, (A() & 0x80) != 0);
		SetA((A() << 1) | carry);
	}

	void ROL() {
		uint16_t addr = GetOperand();
		uint8_t val = MemoryRead(addr);
		DummyRead();
		uint8_t carry = CheckFlag(LynxPSFlags::Carry) ? 1 : 0;
		SetFlagValue(LynxPSFlags::Carry, (val & 0x80) != 0);
		val = (val << 1) | carry;
		SetZeroNeg(val);
		MemoryWrite(addr, val);
	}

	void ROR_A() {
		uint8_t carry = CheckFlag(LynxPSFlags::Carry) ? 0x80 : 0;
		SetFlagValue(LynxPSFlags::Carry, (A() & 0x01) != 0);
		SetA((A() >> 1) | carry);
	}

	void ROR() {
		uint16_t addr = GetOperand();
		uint8_t val = MemoryRead(addr);
		DummyRead();
		uint8_t carry = CheckFlag(LynxPSFlags::Carry) ? 0x80 : 0;
		SetFlagValue(LynxPSFlags::Carry, (val & 0x01) != 0);
		val = (val >> 1) | carry;
		SetZeroNeg(val);
		MemoryWrite(addr, val);
	}

	// --- 65C02: TRB/TSB ---
	void TRB() {
		uint16_t addr = GetOperand();
		uint8_t val = MemoryRead(addr);
		DummyRead();
		SetFlagValue(LynxPSFlags::Zero, (A() & val) == 0);
		MemoryWrite(addr, val & ~A());
	}

	void TSB() {
		uint16_t addr = GetOperand();
		uint8_t val = MemoryRead(addr);
		DummyRead();
		SetFlagValue(LynxPSFlags::Zero, (A() & val) == 0);
		MemoryWrite(addr, val | A());
	}

	// --- Branches ---
	void BranchRelative(bool branch);
	void BCC() { BranchRelative(!CheckFlag(LynxPSFlags::Carry)); }
	void BCS() { BranchRelative(CheckFlag(LynxPSFlags::Carry)); }
	void BEQ() { BranchRelative(CheckFlag(LynxPSFlags::Zero)); }
	void BNE() { BranchRelative(!CheckFlag(LynxPSFlags::Zero)); }
	void BMI() { BranchRelative(CheckFlag(LynxPSFlags::Negative)); }
	void BPL() { BranchRelative(!CheckFlag(LynxPSFlags::Negative)); }
	void BVS() { BranchRelative(CheckFlag(LynxPSFlags::Overflow)); }
	void BVC() { BranchRelative(!CheckFlag(LynxPSFlags::Overflow)); }
	void BRA() { BranchRelative(true); }  // 65C02: unconditional branch

	// --- Jumps/Calls ---
	void JMP() { SetPC(GetOperand()); }
	void JSR();
	void RTS();
	void RTI();
	void BRK();

	// --- Flag Set/Clear ---
	void CLC() { ClearFlag(LynxPSFlags::Carry); }
	void SEC() { SetFlag(LynxPSFlags::Carry); }
	void CLD() { ClearFlag(LynxPSFlags::Decimal); }
	void SED() { SetFlag(LynxPSFlags::Decimal); }
	void CLI() { ClearFlag(LynxPSFlags::Interrupt); }
	void SEI() { SetFlag(LynxPSFlags::Interrupt); }
	void CLV() { ClearFlag(LynxPSFlags::Overflow); }

	// --- 65C02: WAI/STP ---
	void WAI();
	void STP();

	// --- NOP (various sizes for undefined opcodes) ---
	void NOP() {}
	void NOP_Imm() { (void)GetOperandValue(); }  // 2-byte NOP

	// WDC 65C02 undefined 3-byte NOPs with extra cycles
	void NOP_5C() { DummyRead(); DummyRead(); DummyRead(); DummyRead(); DummyRead(); } // $5C: 8 cycles total (3 addr + 5 dummy)
	void NOP_DC() { DummyRead(); } // $DC/$FC: 4 cycles total (3 addr + 1 dummy)

	// --- IRQ handling ---
	void HandleIrq();

	void InitOpTable();

public:
#ifndef DUMMYCPU
	LynxCpu(Emulator* emu, LynxConsole* console, LynxMemoryManager* memoryManager);
#else
	LynxCpu(Emulator* emu, LynxMemoryManager* memoryManager);
#endif
	~LynxCpu() = default;

	/// <summary>Execute one instruction</summary>
	void Exec();

	/// <summary>Set IRQ line state</summary>
	void SetIrqLine(bool active) { _irqPending = active; }

	/// <summary>Get current CPU state for debugger</summary>
	[[nodiscard]] LynxCpuState& GetState() { return _state; }

	/// <summary>Get cycle count (master clock)</summary>
	[[nodiscard]] uint64_t GetCycleCount() const { return _state.CycleCount; }

	/// <summary>Increment cycle count (called by memory manager)</summary>
	void IncCycleCount() { _state.CycleCount++; }

	/// <summary>Add stall cycles (e.g., sprite engine bus contention)</summary>
	void AddCycles(uint32_t cycles) { _state.CycleCount += cycles; }

	void Serialize(Serializer& s) override;

#ifdef DUMMYCPU
private:
	uint32_t _memOpCounter = 0;
	MemoryOperationInfo _memOperations[10] = {};

	void LogMemoryOperation(uint32_t addr, uint8_t value, MemoryOperationType type);

public:
	void SetDummyState(LynxCpuState& state);

	uint32_t GetOperationCount();
	MemoryOperationInfo GetOperationInfo(uint32_t index);
#endif
};

#endif
