#if (defined(DUMMYCPU) && !defined(__DUMMYWsCpu__H)) || (!defined(DUMMYCPU) && !defined(__WsCpu__H))
#ifdef DUMMYCPU
#define __DUMMYWsCpu__H
#else
#define __WsCpu__H
#endif

#include "pch.h"
#include "WS/WsCpuPrefetch.h"
#include "WS/WsTypes.h"
#include "Shared/MemoryOperationType.h"
#include "Utilities/ISerializable.h"

class Emulator;
class WsConsole;
class WsMemoryManager;

/// <summary>
/// WonderSwan CPU emulator - NEC V30MZ (80186-compatible) implementation.
/// 16-bit x86-compatible CPU with power-saving features.
/// </summary>
/// <remarks>
/// The V30MZ is a CMOS implementation of Intel 80186 with NEC extensions:
/// - 16-bit data bus, 20-bit address space (1MB)
/// - 8 general registers: AX, BX, CX, DX, SP, BP, SI, DI
/// - 4 segment registers: CS, DS, ES, SS (16-bit segments)
/// - 16-bit instruction pointer (IP) and flags register
/// - x86 segmented memory model (segment:offset → 20-bit address)
///
/// **Clock Speed:**
/// - WonderSwan: 3.072 MHz
/// - WonderSwan Color: 3.072 MHz (higher speed mode available)
/// - SwanCrystal: Same as WSC
///
/// **Registers:**
/// - AX (AH:AL): Accumulator
/// - BX (BH:BL): Base register
/// - CX (CH:CL): Count register
/// - DX (DH:DL): Data register
/// - SP: Stack pointer
/// - BP: Base pointer
/// - SI: Source index
/// - DI: Destination index
///
/// **Segment Registers:**
/// - CS: Code segment
/// - DS: Data segment (default for data access)
/// - ES: Extra segment (string destinations)
/// - SS: Stack segment
///
/// **Flags:**
/// - CF: Carry flag
/// - PF: Parity flag (set if low byte has even parity)
/// - AF: Auxiliary carry (BCD operations)
/// - ZF: Zero flag
/// - SF: Sign flag
/// - TF: Trap flag (single-step debug)
/// - IF: Interrupt flag
/// - DF: Direction flag (string operations)
/// - OF: Overflow flag
///
/// **Instruction Prefixes:**
/// - REP/REPE/REPNE: Repeat string operations
/// - Segment override: CS:, DS:, ES:, SS:
/// - LOCK: Bus lock for atomic operations
///
/// **Interrupts:**
/// - INT n: Software interrupt (n = vector number)
/// - Hardware interrupts via interrupt controller
/// - NMI: Non-maskable interrupt
///
/// **Power Features:**
/// - HALT: Low-power wait for interrupt
/// - Low-power CMOS design for battery operation
/// </remarks>
class WsCpu final : public ISerializable {
private:
	/// <summary>Decoded ModR/M byte state for addressing.</summary>
	struct ModRmState {
		uint16_t Segment;   ///< Segment base for memory access
		uint16_t Offset;    ///< Offset within segment
		uint8_t Mode;       ///< Addressing mode (0-3)
		uint8_t Register;   ///< Register operand field
		uint8_t Rm;         ///< R/M field (register or memory)
	};

	/// <summary>REP prefix mode for string instructions.</summary>
	enum class WsRepMode : uint8_t {
		None,      ///< No repeat prefix
		Zero,      ///< REPE/REPZ - repeat while zero flag set
		NotZero    ///< REPNE/REPNZ - repeat while zero flag clear
	};

	/// <summary>ALU operation type for Group 1 instructions.</summary>
	enum class AluOp : uint8_t {
		Add = 0,  ///< Addition
		Or,       ///< Logical OR
		Adc,      ///< Add with carry
		Sbb,      ///< Subtract with borrow
		And,      ///< Logical AND
		Sub,      ///< Subtraction
		Xor,      ///< Exclusive OR
		Cmp       ///< Compare (subtract without storing)
	};

	/// <summary>Shift/rotate count source mode.</summary>
	enum class Grp2Mode {
		One,       ///< Shift by 1
		CL,        ///< Shift by CL register value
		Immediate  ///< Shift by immediate byte
	};

	/// <summary>Instruction prefix state (segment override, REP, LOCK).</summary>
	struct PrefixState {
		uint16_t PrefixCount;  ///< Number of prefixes decoded
		WsSegment Segment;     ///< Segment override (if any)
		WsRepMode Rep;         ///< REP prefix mode
		bool Lock;             ///< LOCK prefix active
		bool Preserve;         ///< Preserve prefixes across instruction
	};

	static WsCpuParityTable _parity;  ///< Parity flag lookup table

	Emulator* _emu = nullptr;                ///< Emulator for debugger hooks
	WsMemoryManager* _memoryManager = nullptr; ///< Memory/IO access
	WsConsole* _console = nullptr;           ///< Parent console

	WsCpuState _state = {};    ///< CPU register state
	ModRmState _modRm = {};    ///< Current instruction's ModR/M decoding
	PrefixState _prefix = {};  ///< Active instruction prefixes

	uint64_t _suppressIrqClock = 0;   ///< IRQ suppression until this clock (MOV to SS)
	uint64_t _suppressTrapClock = 0;  ///< Trap suppression until this clock

#ifndef DUMMYCPU
	WsCpuPrefetch _prefetch;  ///< Instruction prefetch queue (4 bytes)
#endif

	// Lookup tables for fast register decoding
	uint16_t* _modRegLut8[4] = {&_state.AX, &_state.CX, &_state.DX, &_state.BX};  ///< 8-bit reg decode
	uint16_t* _modSegLut16[4] = {&_state.ES, &_state.CS, &_state.SS, &_state.DS}; ///< Segment reg decode
	uint16_t* _modRegLut16[8] = {  ///< 16-bit register decode lookup
	    &_state.AX, &_state.CX, &_state.DX, &_state.BX,
	    &_state.SP, &_state.BP, &_state.SI, &_state.DI};

	uint8_t _opCode = 0;  ///< Current opcode (saved for REP refetch)

	// MUL sets carry/overflow which divisions/AAM inherit
	bool _mulOverflow = false;  ///< Last MUL overflow for division ops

	template <typename T>
	__forceinline T ReadPort(uint16_t port);
	template <typename T>
	__forceinline void WritePort(uint16_t port, T value);

	__forceinline void ProcessMemoryAccess(uint32_t addr);
	__forceinline void ProcessPortAccess(uint16_t port);

	__forceinline uint8_t ReadCodeByte(bool forOpCode = false);
	__forceinline uint16_t ReadCodeWord();
	template <typename T>
	__forceinline T ReadImmediate();

	template <typename T>
	__forceinline T ReadMemory(uint16_t seg, uint16_t offset);
	template <typename T>
	__forceinline void WriteMemory(uint16_t seg, uint16_t offset, T value);

	template <uint8_t cycles = 1>
	__forceinline void Idle();

	template <typename T>
	constexpr uint32_t GetMaxValue();
	template <typename T>
	constexpr uint32_t GetBitCount();
	template <typename T>
	constexpr uint32_t GetSign();

	void Move(uint16_t& dst, uint16_t src);
	void MoveLo(uint16_t& dst, uint8_t src);
	void MoveHi(uint16_t& dst, uint8_t src);

	void Push(uint16_t value);
	void PushSP();
	void Pop(uint16_t& dst);
	void PushSegment(uint16_t value);
	void PopSegment(uint16_t& dst);
	void PushFlags();
	void PopFlags();
	void SetFlags(uint16_t flags);
	void PopMemory();
	void PopAll();
	void PushAll();

	uint16_t GetSegment(WsSegment defaultSegment);

	__forceinline void ReadModRmByte();

	template <typename T>
	T GetModRegister(uint8_t reg) = delete;
	template <typename T>
	void SetModRegister(uint8_t reg, T value) = delete;

	uint16_t GetModSegRegister(uint8_t reg);
	void SetModSegRegister(uint8_t reg, uint16_t value);

	template <typename T>
	T GetModRm();
	template <typename T>
	void SetModRm(T value);

	template <bool sign, typename T>
	void Grp1ModRm();
	template <typename T, Grp2Mode mode>
	void Grp2ModRm();
	template <typename T>
	void Grp3ModRm();
	template <typename T>
	void Grp45ModRm();

	template <typename T>
	void TestModRm();
	template <typename T>
	void TestImmediate();

	template <typename T>
	void ExchangeModRm();
	void Exchange(uint16_t& x, uint16_t& y);

	template <bool direction, typename T>
	void MoveModRm();
	template <bool direction>
	void MoveSegment();
	template <bool direction, typename T>
	void MoveAccumulator();
	template <typename T>
	void MoveImmediate();

	template <AluOp op, bool direction, typename T>
	void ProcessAluModRm();
	template <AluOp op, typename T>
	void ProcessAluImm();
	template <typename T>
	T GetAluResult(AluOp op, T param1, T param2);

	template <typename T>
	void Inc(T& dst);
	template <typename T>
	void Dec(T& dst);

	template <typename T>
	T Add(T x, T y, uint8_t carry);
	template <typename T>
	T Sub(T x, T y, uint8_t borrow);

	template <typename T>
	void MultSignedModRm();
	template <typename T>
	void MulSigned(T x, T y);
	template <typename T>
	int32_t GetMultiplyResult(T x, T y);
	template <typename T>
	void MulUnsigned(T x, T y);

	template <typename T>
	void DivSigned(T y);
	template <typename T>
	void DivUnsigned(T y);
	void ProcessInvalidDiv();

	template <typename T>
	void UpdateFlags(T result);
	template <typename T>
	T And(T x, T y);
	template <typename T>
	T Or(T x, T y);
	template <typename T>
	T Xor(T x, T y);
	template <typename T>
	T ROL(T x, uint8_t shift);
	template <typename T>
	T ROR(T x, uint8_t shift);
	template <typename T>
	T RCL(T x, uint8_t shift);
	template <typename T>
	T RCR(T x, uint8_t shift);
	template <typename T>
	T SHL(T x, uint8_t shift);
	template <typename T>
	T SHR(T x, uint8_t shift);
	template <typename T>
	T SAR(T x, uint8_t shift);

	void JumpFar();
	void Jump(bool condition);
	void JumpNearWord();
	void Call(uint16_t offset);
	void CallNearWord();
	void CallFar();
	void CallFar(uint16_t segment, uint16_t offset);
	void Loop();
	void LoopIf(bool condition);
	void Ret();
	void RetImm();
	void RetFar();
	void RetFarImm();
	void RetInterrupt();

	void Interrupt(uint8_t vector, bool pushFirstPrefix = false);
	void InterruptOverflow();

	void Enter();
	void Leave();

	void NOP();
	void FP01();

	void BOUND();

	void SALC();

	void CBW();
	void CWD();

	void LAHF();
	void SAHF();

	void LdsLesLeaModRm();

	uint16_t LoadSegment();
	void LDS();
	void LES();
	void LEA();

	void XLAT();

	void AdjustAscii(bool forSub);
	void AAA();
	void AAS();

	void AAD();
	void AAM();

	void AdjustDecimal(bool forSub);
	void DAA();
	void DAS();

	template <typename T, uint8_t delay>
	void Out(uint16_t port, T data);
	template <typename T, bool isDxPort>
	void InStoreAx(uint16_t port);

	template <typename T>
	void INS();
	template <typename T>
	void OUTS();

	template <typename T>
	void ProcessStringOperation(bool incSi, bool incDi);
	template <typename T>
	void ProcessStringCmpOperation(bool incSi);
	template <typename T>
	void MOVS();
	template <typename T>
	void STOS();
	template <typename T>
	void LODS();
	template <typename T>
	void CMPS();
	template <typename T>
	void SCAS();

	void Undefined();
	void Wait();
	void Halt();

	void SuppressIrq(bool suppressTrap);

	void SetFlagValue(bool& flag, bool value);
	void SetIrqFlag();

public:
	WsCpu(Emulator* emu, WsConsole* console, WsMemoryManager* memoryManager);

	WsCpuState& GetState() { return _state; }
	[[nodiscard]] uint64_t GetCycleCount() { return _state.CycleCount; }
	[[nodiscard]] bool IsPowerOff() { return _state.PowerOff; }

	void ProcessCpuCycle();

	__forceinline void IncCycleCount() { _state.CycleCount++; }
	void ClearPrefetch();

	[[nodiscard]] uint32_t GetProgramCounter(bool adjustForRepLoop = false);

	void Exec();
	void ExecOpCode();

	void Serialize(Serializer& s) override;

#ifdef DUMMYCPU
private:
	uint32_t _memOpCounter = 0;
	MemoryOperationInfo _memOperations[32] = {};
	bool _memWordAccess[32] = {};

public:
	void SetDummyState(WsCpuState& state);
	[[nodiscard]] uint32_t GetOperationCount();
	void LogMemoryOperation(uint32_t addr, uint16_t value, MemoryOperationType type, MemoryType memType, bool isWordAccess);
	[[nodiscard]] MemoryOperationInfo GetOperationInfo(uint32_t index);
	[[nodiscard]] bool IsWordAccess(uint32_t index);
#endif
};

template <>
uint8_t WsCpu::GetModRegister<uint8_t>(uint8_t reg);
template <>
uint16_t WsCpu::GetModRegister<uint16_t>(uint8_t reg);
template <>
void WsCpu::SetModRegister<uint8_t>(uint8_t reg, uint8_t value);
template <>
void WsCpu::SetModRegister<uint16_t>(uint8_t reg, uint16_t value);

#endif
