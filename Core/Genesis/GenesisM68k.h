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
	uint8_t _pendingInterruptLevel = 0;
	bool _instructionTraceEnabled = false;
	uint32_t _instructionTraceCapacity = 16384;
	vector<GenesisInstructionTraceEntry> _instructionTraceEntries = {};
	uint32_t _instructionTraceWriteIndex = 0;
	bool _instructionTraceWrapped = false;
	uint64_t _instructionTraceSequence = 0;
	uint64_t _forcedCycleFloorCount = 0;
	uint64_t _forcedClockAdvanceCount = 0;
	uint32_t _samePcRunLength = 0;
	uint32_t _lastRunPc = 0xffffffff;
	uint16_t _lastRunOpcode = 0;
	bool _debugForceNoCycleProgress = false;
	uint64_t _resetProbeCount = 0;
	uint32_t _lastResetVectorSp = 0;
	uint32_t _lastResetVectorPc = 0;
	bool _firstDispatchCaptured = false;
	GenesisInstructionTraceEntry _firstDispatchEntry = {};
	uint64_t _dispatchFaultCount = 0;
	string _lastDispatchFaultSummary = {};
	uint64_t _dispatchGuardHitCount = 0;
	uint64_t _decodeFaultCount = 0;
	uint64_t _execCallCount = 0;
	uint32_t _lastFetchProgramCounter = 0;
	uint16_t _lastFetchOpcode = 0;
	uint16_t _lastFetchPreviewWordA = 0;
	uint16_t _lastFetchPreviewWordB = 0;
	string _lastDispatchBoundarySummary = {};
	uint8_t _lastDecodedGroup = 0;
	uint8_t _lastDecodedSubOp = 0;
	uint8_t _lastDecodedMode = 0;
	uint8_t _lastDecodedReg = 0;
	uint64_t _decodeGroupHitCount[16] = {};
	string _lastDecodeRouteSummary = {};
	bool _instructionFlowConfigLoaded = false;
	bool _instructionFlowLogEnabled = false;
	uint32_t _instructionFlowLogLimit = 20000;
	uint32_t _instructionFlowLogStride = 1;
	uint32_t _instructionFlowLogCount = 0;
	uint32_t _instructionFlowLogSkipped = 0;
	string _lastInstructionFlowLogLine = {};
	vector<string> _recentInstructionFlowLogs = {};
	uint32_t _recentInstructionFlowCapacity = 64;
	bool _strictAddressErrorChecks = true;
	uint64_t _addressErrorCount = 0;
	uint32_t _lastAddressErrorAddr = 0;
	uint32_t _lastAddressErrorPc = 0;
	uint8_t _lastAddressErrorSize = 0;
	bool _lastAddressErrorWrite = false;
	string _lastAddressErrorSource = {};

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
		addr &= 0xffffff;
		if (CheckAddressError(addr, 2, false, "read16")) {
			return 0;
		}
		addr &= 0xfffffe;
		AddCycles(4);
		return _memoryManager->Read16(addr);
	}
	__forceinline uint32_t Read32(uint32_t addr) {
		addr &= 0xffffff;
		if (CheckAddressError(addr, 4, false, "read32")) {
			return 0;
		}
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
		addr &= 0xffffff;
		if (CheckAddressError(addr, 2, true, "write16")) {
			return;
		}
		addr &= 0xfffffe;
		AddCycles(4);
		_memoryManager->Write16(addr, value);
	}
	__forceinline void Write32(uint32_t addr, uint32_t value) {
		addr &= 0xffffff;
		if (CheckAddressError(addr, 4, true, "write32")) {
			return;
		}
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
	bool CheckAddressError(uint32_t addr, uint8_t size, bool isWrite, const char* sourceTag);
	void RecordInstructionTrace(uint32_t programCounterBefore, uint32_t programCounterAfter, uint16_t opcode, uint16_t operandWordA, uint16_t operandWordB, uint16_t statusRegisterBefore, uint16_t statusRegisterAfter, uint64_t cycleCountBefore, uint64_t cycleCountAfter, uint32_t d0Before, uint32_t d0After, uint32_t a0Before, uint32_t a0After, uint32_t a7Before, uint32_t a7After, bool forcedCycleFloor, bool stoppedBefore, bool stoppedAfter);
	uint16_t PeekWord(uint32_t addr) const;
	void LoadInstructionFlowLogConfig();
	void MaybeLogInstructionFlow(uint32_t prePc, uint16_t opcode, uint16_t operandWordA, uint16_t operandWordB, uint64_t cyclesBefore, uint64_t cyclesAfter, uint16_t srBefore, uint16_t srAfter);
	__forceinline void AddCycles(uint32_t cycles) {
		if (_debugForceNoCycleProgress) {
			return;
		}
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
	void Op_TAS(uint16_t opcode);

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

	void SetInstructionTraceEnabled(bool enabled) { _instructionTraceEnabled = enabled; }
	bool GetInstructionTraceEnabled() const { return _instructionTraceEnabled; }
	void SetInstructionTraceCapacity(uint32_t capacity);
	uint32_t GetInstructionTraceCapacity() const { return _instructionTraceCapacity; }
	void ClearInstructionTrace();
	vector<GenesisInstructionTraceEntry> GetInstructionTraceSnapshot() const;
	string BuildInstructionTraceDigest() const;
	string BuildInstructionTraceWindow(uint32_t maxLines) const;
	string BuildExecutionStallSummary() const;
	string BuildAddressErrorSummary() const;
	uint64_t GetForcedCycleFloorCount() const { return _forcedCycleFloorCount; }
	uint64_t GetForcedClockAdvanceCount() const { return _forcedClockAdvanceCount; }
	void ForceClockAdvance(uint32_t cycles);
	void SetDebugForceNoCycleProgress(bool forceNoProgress) { _debugForceNoCycleProgress = forceNoProgress; }
	uint64_t GetResetProbeCount() const { return _resetProbeCount; }
	uint32_t GetLastResetVectorSp() const { return _lastResetVectorSp; }
	uint32_t GetLastResetVectorPc() const { return _lastResetVectorPc; }
	bool HasFirstDispatchEntry() const { return _firstDispatchCaptured; }
	const GenesisInstructionTraceEntry& GetFirstDispatchEntry() const { return _firstDispatchEntry; }
	uint64_t GetDispatchFaultCount() const { return _dispatchFaultCount; }
	const string& GetLastDispatchFaultSummary() const { return _lastDispatchFaultSummary; }
	uint64_t GetDispatchGuardHitCount() const { return _dispatchGuardHitCount; }
	uint64_t GetDecodeFaultCount() const { return _decodeFaultCount; }
	uint32_t GetLastFetchProgramCounter() const { return _lastFetchProgramCounter; }
	uint16_t GetLastFetchOpcode() const { return _lastFetchOpcode; }
	const string& GetLastDispatchBoundarySummary() const { return _lastDispatchBoundarySummary; }
	uint8_t GetLastDecodedGroup() const { return _lastDecodedGroup; }
	uint8_t GetLastDecodedSubOp() const { return _lastDecodedSubOp; }
	uint8_t GetLastDecodedMode() const { return _lastDecodedMode; }
	uint8_t GetLastDecodedReg() const { return _lastDecodedReg; }
	const string& GetLastDecodeRouteSummary() const { return _lastDecodeRouteSummary; }
	const string& GetLastInstructionFlowLogLine() const { return _lastInstructionFlowLogLine; }
	uint32_t GetInstructionFlowLogCount() const { return _instructionFlowLogCount; }
	uint32_t GetInstructionFlowLogSkippedCount() const { return _instructionFlowLogSkipped; }
	vector<string> GetRecentInstructionFlowLogs() const { return _recentInstructionFlowLogs; }
	void ArmAggressiveFlowTrace(uint32_t limit, uint32_t stride = 1, uint32_t ringCapacity = 128);
	string BuildCrashProbeSummary() const;
	string BuildDispatchBoundaryProbeSummary() const;
	string BuildInstructionFlowSummary() const;

	void Serialize(Serializer& s) override;
};
