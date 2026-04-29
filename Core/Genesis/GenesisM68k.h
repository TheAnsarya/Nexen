#pragma once
#include "pch.h"
#include "Genesis/GenesisTypes.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Utilities/ISerializable.h"

class Emulator;
class GenesisConsole;

class GenesisM68k final : public ISerializable {
private:
	Emulator* _emu = nullptr;
	GenesisConsole* _console = nullptr;
	GenesisMemoryManager* _memoryManager = nullptr;

	GenesisM68kState _state = {};

	// Prefetch
	uint16_t _prefetch[2] = {};
	uint32_t _prefetchAddr = 0;

	// ===== Memory access (inline for hot-path inlining) =====
	__forceinline uint8_t Read8(uint32_t addr) {
		addr &= 0xffffff; // 24-bit address bus
		AddCycles(4);
		return _memoryManager->Read8(addr);
	}
	__forceinline uint16_t Read16(uint32_t addr) {
		addr &= 0xfffffe; // Word-aligned
		AddCycles(4);
		return _memoryManager->Read16(addr);
	}
	__forceinline uint32_t Read32(uint32_t addr) {
		uint16_t hi = Read16(addr);
		uint16_t lo = Read16(addr + 2);
		return ((uint32_t)hi << 16) | lo;
	}
	__forceinline void Write8(uint32_t addr, uint8_t value) {
		addr &= 0xffffff;
		AddCycles(4);
		_memoryManager->Write8(addr, value);
	}
	__forceinline void Write16(uint32_t addr, uint16_t value) {
		addr &= 0xfffffe;
		AddCycles(4);
		_memoryManager->Write16(addr, value);
	}
	__forceinline void Write32(uint32_t addr, uint32_t value) {
		Write16(addr, (uint16_t)(value >> 16));
		Write16(addr + 2, (uint16_t)(value & 0xffff));
	}

	// ===== Stack =====
	__forceinline void Push16(uint16_t value) {
		SP() -= 2;
		Write16(SP(), value);
	}
	__forceinline void Push32(uint32_t value) {
		SP() -= 4;
		Write32(SP(), value);
	}
	__forceinline uint16_t Pop16() {
		uint16_t val = Read16(SP());
		SP() += 2;
		return val;
	}
	__forceinline uint32_t Pop32() {
		uint32_t val = Read32(SP());
		SP() += 4;
		return val;
	}

	// ===== Instruction fetch =====
	__forceinline uint16_t FetchOpcode() {
		uint16_t op = Read16(_state.PC);
		_state.PC += 2;
		return op;
	}
	__forceinline uint16_t FetchWord() {
		uint16_t w = Read16(_state.PC);
		_state.PC += 2;
		return w;
	}
	__forceinline uint32_t FetchLong() {
		uint32_t l = Read32(_state.PC);
		_state.PC += 4;
		return l;
	}

	// ===== Effective address resolution =====
	// Returns the address for a given mode/register pair
	uint32_t GetEffectiveAddress(uint8_t mode, uint8_t reg, uint8_t size);
	uint32_t ReadEa(uint8_t mode, uint8_t reg, uint8_t size);
	void WriteEa(uint8_t mode, uint8_t reg, uint8_t size, uint32_t value);

	// ===== Flag operations =====
	void SetFlags_Logical(uint32_t result, uint8_t size);
	void SetFlags_Add(uint32_t src, uint32_t dst, uint32_t result, uint8_t size);
	void SetFlags_Sub(uint32_t src, uint32_t dst, uint32_t result, uint8_t size);
	void SetFlags_Cmp(uint32_t src, uint32_t dst, uint32_t result, uint8_t size);

	bool GetCcr(uint16_t flag) { return (_state.SR & flag) != 0; }
	void SetCcr(uint16_t flag) { _state.SR |= flag; }
	void ClearCcr(uint16_t flag) { _state.SR &= ~flag; }
	void SetCcrIf(uint16_t flag, bool condition) { if (condition) SetCcr(flag); else ClearCcr(flag); }

	uint8_t GetIntMask() { return (_state.SR >> 8) & 7; }
	bool IsSupervisor() { return (_state.SR & M68kFlags::Supervisor) != 0; }

	// ===== Active stack pointer =====
	uint32_t& SP() { return _state.A[7]; }

	// ===== Size helpers (constexpr lookup tables — no branches) =====
	static __forceinline constexpr uint32_t SizeMask(uint8_t size) {
		constexpr uint32_t masks[] = { 0xff, 0xffff, 0xffffffff };
		return masks[size];
	}
	static __forceinline constexpr uint32_t SignBit(uint8_t size) {
		constexpr uint32_t bits[] = { 0x80, 0x8000, 0x80000000 };
		return bits[size];
	}
	static __forceinline bool IsNeg(uint32_t value, uint8_t size) {
		return (value & SignBit(size)) != 0;
	}

	// ===== Exception handling =====
	void RaiseException(uint8_t vector);
	void ProcessInterrupt(uint8_t level);

	// ===== Condition code testing =====
	bool TestCondition(uint8_t cc);

	// ===== Instruction execution =====
	void ExecuteInstruction(uint16_t opcode);
	__forceinline void AddCycles(uint32_t cycles) {
		_state.CycleCount += cycles;
		if (_memoryManager) {
			_memoryManager->Exec(cycles);
		}
	}

	// --- Data Movement ---
	void Op_MOVE(uint16_t opcode);
	void Op_MOVEA(uint16_t opcode);
	void Op_MOVEQ(uint16_t opcode);
	void Op_MOVEM(uint16_t opcode);
	void Op_LEA(uint16_t opcode);
	void Op_PEA(uint16_t opcode);
	void Op_CLR(uint16_t opcode);
	void Op_EXG(uint16_t opcode);
	void Op_SWAP(uint16_t opcode);
	void Op_LINK(uint16_t opcode);
	void Op_UNLK(uint16_t opcode);

	// --- Arithmetic ---
	void Op_ADD(uint16_t opcode);
	void Op_ADDA(uint16_t opcode);
	void Op_ADDI(uint16_t opcode);
	void Op_ADDQ(uint16_t opcode);
	void Op_SUB(uint16_t opcode);
	void Op_SUBA(uint16_t opcode);
	void Op_SUBI(uint16_t opcode);
	void Op_SUBQ(uint16_t opcode);
	void Op_CMP(uint16_t opcode);
	void Op_CMPA(uint16_t opcode);
	void Op_CMPI(uint16_t opcode);
	void Op_MULU(uint16_t opcode);
	void Op_MULS(uint16_t opcode);
	void Op_DIVU(uint16_t opcode);
	void Op_DIVS(uint16_t opcode);
	void Op_NEG(uint16_t opcode);
	void Op_EXT(uint16_t opcode);
	void Op_TST(uint16_t opcode);

	// --- Logic ---
	void Op_AND(uint16_t opcode);
	void Op_ANDI(uint16_t opcode);
	void Op_OR(uint16_t opcode);
	void Op_ORI(uint16_t opcode);
	void Op_EOR(uint16_t opcode);
	void Op_EORI(uint16_t opcode);
	void Op_NOT(uint16_t opcode);

	// --- Shifts/Rotates ---
	void Op_ASd(uint16_t opcode);   // ASL/ASR
	void Op_LSd(uint16_t opcode);   // LSL/LSR
	void Op_ROd(uint16_t opcode);   // ROL/ROR
	void Op_ROXd(uint16_t opcode);  // ROXL/ROXR

	// --- Bit operations ---
	void Op_BTST(uint16_t opcode);
	void Op_BSET(uint16_t opcode);
	void Op_BCLR(uint16_t opcode);
	void Op_BCHG(uint16_t opcode);

	// --- Branch ---
	void Op_BRA(uint16_t opcode);
	void Op_BSR(uint16_t opcode);
	void Op_Bcc(uint16_t opcode);
	void Op_DBcc(uint16_t opcode);
	void Op_Scc(uint16_t opcode);
	void Op_JMP(uint16_t opcode);
	void Op_JSR(uint16_t opcode);
	void Op_RTS(uint16_t opcode);
	void Op_RTE(uint16_t opcode);
	void Op_RTR(uint16_t opcode);

	// --- System ---
	void Op_NOP(uint16_t opcode);
	void Op_TRAP(uint16_t opcode);
	void Op_RESET(uint16_t opcode);
	void Op_STOP(uint16_t opcode);
	void Op_MOVE_SR(uint16_t opcode);
	void Op_MOVE_USP(uint16_t opcode);
	void Op_ANDI_SR(uint16_t opcode);
	void Op_ORI_SR(uint16_t opcode);
	void Op_EORI_SR(uint16_t opcode);

	// --- Special ---
	void Op_ILLEGAL(uint16_t opcode);

public:
	GenesisM68k() = default;
	void Init(Emulator* emu, GenesisConsole* console, GenesisMemoryManager* memoryManager);

	void Exec();
	void Reset(bool softReset);

	GenesisM68kState& GetState() { return _state; }
	void SetState(GenesisM68kState& state) { _state = state; }

	void SetInterrupt(uint8_t level);

	void Serialize(Serializer& s) override;
};
