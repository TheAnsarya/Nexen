#if (defined(DUMMYCPU) && !defined(__DUMMYCPU__H)) || (!defined(DUMMYCPU) && !defined(__CPU__H))
#ifdef DUMMYCPU
#define __DUMMYCPU__H
#else
#define __CPU__H
#endif

#include "pch.h"
#include "SNES/SnesCpuTypes.h"
#include "Utilities/ISerializable.h"
#include "Shared/MemoryOperationType.h"

class MemoryMappings;
class SnesMemoryManager;
class SnesDmaController;
class SnesConsole;
class Emulator;

/// <summary>
/// SNES 65816 CPU emulator - Ricoh 5A22 implementation.
/// Cycle-accurate emulation of the 16-bit CPU with 24-bit address space.
/// </summary>
/// <remarks>
/// The 65816 is a 16-bit extension of the 6502 with:
/// - 24-bit address bus (16MB address space)
/// - 16-bit accumulator and index registers (switchable to 8-bit)
/// - Direct page relocatable anywhere in bank 0
/// - Program bank (PBR) and data bank (DBR) registers
/// - Stack relocatable anywhere in bank 0
/// - Native and emulation modes (6502 compatibility)
///
/// **Key Features:**
/// - **Memory Modes**: 8/16-bit accumulator (M flag), 8/16-bit index (X flag)
/// - **Address Modes**: 24 addressing modes including long addressing
/// - **Block Moves**: MVN/MVP for efficient memory copy
/// - **Coprocessor Support**: Interface for SA-1, Super FX, DSP chips
///
/// **Interrupt Vectors (Native Mode):**
/// - NMI: $00FFEA (triggered by PPU V-blank)
/// - IRQ: $00FFEE (hardware/software interrupts)
/// - BRK: $00FFE6 (software breakpoint)
/// - COP: $00FFE4 (coprocessor instruction)
///
/// **DMA Integration:**
/// - CPU halts during DMA transfers
/// - HDMA runs during H-blank
///
/// **Performance Notes:**
/// - Uses DUMMYCPU define for trace/debug builds with additional hooks
/// - __forceinline on critical paths (ProcessCpuCycle, memory access)
/// </remarks>
class SnesCpu : public ISerializable {
private:
	// Interrupt vectors (native mode - 24-bit addresses)
	static constexpr uint32_t NmiVector = 0x00FFEA;       ///< Non-maskable interrupt (V-blank)
	static constexpr uint32_t ResetVector = 0x00FFFC;     ///< Reset vector (startup)
	static constexpr uint32_t IrqVector = 0x00FFEE;       ///< Interrupt request
	static constexpr uint32_t AbortVector = 0x00FFE8;     ///< Abort (not used on SNES)
	static constexpr uint32_t BreakVector = 0x00FFE6;     ///< BRK instruction
	static constexpr uint32_t CoprocessorVector = 0x00FFE4; ///< COP instruction

	// Legacy vectors (emulation mode - 16-bit addresses in bank 0)
	static constexpr uint16_t LegacyNmiVector = 0xFFFA;
	static constexpr uint32_t LegacyIrqVector = 0xFFFE;
	static constexpr uint32_t LegacyCoprocessorVector = 0x00FFF4;

	typedef void (SnesCpu::*Func)();  ///< Instruction handler function pointer

	SnesMemoryManager* _memoryManager = nullptr;  ///< Memory mapping and bus access
	SnesDmaController* _dmaController = nullptr;  ///< DMA/HDMA controller
	Emulator* _emu = nullptr;                     ///< Emulator for debugger hooks
	SnesConsole* _console = nullptr;              ///< Parent console

	bool _immediateMode = false;      ///< Current instruction uses immediate operand
	uint32_t _readWriteMask = 0xFFFFFF; ///< Address mask (24-bit)

	SnesCpuState _state = {};  ///< CPU registers and flags
	uint32_t _operand = 0;     ///< Current instruction operand
	bool _waiOver = true;      ///< WAI instruction complete (interrupt received)

	/// Get full 24-bit address for program access (PBR:addr)
	uint32_t GetProgramAddress(uint16_t addr);
	/// Get full 24-bit address for data access (DBR:addr)
	uint32_t GetDataAddress(uint16_t addr);

	/// Calculate direct page address with optional emulation mode wrap
	uint16_t GetDirectAddress(uint16_t offset, bool allowEmulationMode = true);

	/// Indirect addressing helpers
	uint16_t GetDirectAddressIndirectWord(uint16_t offset);
	uint16_t GetDirectAddressIndirectWordWithPageWrap(uint16_t offset);
	uint32_t GetDirectAddressIndirectLong(uint16_t offset);

	[[nodiscard]] uint8_t GetOpCode();       ///< Fetch opcode at PC
	uint16_t GetResetVector(); ///< Get reset vector based on mode

	void ProcessCpuCycle();  ///< Called each master clock cycle

	// Cycle timing helpers
	void Idle();                                    ///< Consume a cycle doing nothing
	void IdleOrDummyWrite(uint32_t addr, uint8_t value); ///< Idle or dummy write (RMW)
	void IdleOrRead();                              ///< Idle or dummy read
	void IdleEndJump();                             ///< Extra cycle at end of jump
	void IdleTakeBranch();                          ///< Extra cycle when branch taken

	// Operand fetch
	uint8_t ReadOperandByte();   ///< Read 8-bit immediate operand
	uint16_t ReadOperandWord();  ///< Read 16-bit immediate operand
	uint32_t ReadOperandLong();  ///< Read 24-bit immediate operand

	uint16_t ReadVector(uint16_t vector);  ///< Read interrupt vector

	uint8_t Read(uint32_t addr, MemoryOperationType type);  ///< Memory read with type

	void SetSP(uint16_t sp, bool allowEmulationMode = true);  ///< Set stack pointer
	__forceinline void RestrictStackPointerValue();           ///< Enforce SP constraints
	void SetPS(uint8_t ps);  ///< Set processor status register

	void SetRegister(uint8_t& reg, uint8_t value);                    ///< Set 8-bit register
	void SetRegister(uint16_t& reg, uint16_t value, bool eightBitMode); ///< Set 16-bit register

	__forceinline void SetZeroNegativeFlags(uint16_t value);
	__forceinline void SetZeroNegativeFlags(uint8_t value);

	__forceinline void ClearFlags(uint8_t flags);
	__forceinline void SetFlags(uint8_t flags);
	[[nodiscard]] __forceinline bool CheckFlag(uint8_t flag);

	uint8_t ReadCode(uint16_t addr, MemoryOperationType type = MemoryOperationType::Read);
	uint16_t ReadCodeWord(uint16_t addr, MemoryOperationType type = MemoryOperationType::Read);

	uint8_t ReadData(uint32_t addr, MemoryOperationType type = MemoryOperationType::Read);
	uint16_t ReadDataWord(uint32_t addr, MemoryOperationType type = MemoryOperationType::Read);
	uint32_t ReadDataLong(uint32_t addr, MemoryOperationType type = MemoryOperationType::Read);

	void Write(uint32_t addr, uint8_t value, MemoryOperationType type = MemoryOperationType::Write);
	void WriteWord(uint32_t addr, uint16_t value, MemoryOperationType type = MemoryOperationType::Write);
	void WriteWordRmw(uint32_t addr, uint16_t value, MemoryOperationType type = MemoryOperationType::Write);

	[[nodiscard]] uint8_t GetByteValue();

	[[nodiscard]] uint16_t GetWordValue();

	void PushByte(uint8_t value, bool allowEmulationMode = true);
	uint8_t PopByte(bool allowEmulationMode = true);

	void PushWord(uint16_t value, bool allowEmulationMode = true);
	uint16_t PopWord(bool allowEmulationMode = true);

	// Add/subtract instructions
	void Add8(uint8_t value);
	void Add16(uint16_t value);
	void ADC();

	void Sub8(uint8_t value);
	void Sub16(uint16_t value);
	void SBC();

	// Branch instructions
	void BCC();
	void BCS();
	void BEQ();
	void BMI();
	void BNE();
	void BPL();
	void BRA();
	void BRL();
	void BVC();
	void BVS();
	void BranchRelative(bool branch);

	// Set/clear flag instructions
	void CLC();
	void CLD();
	void CLI();
	void CLV();
	void SEC();
	void SED();
	void SEI();

	void REP();
	void SEP();

	// Increment/decrement instructions
	void DEX();
	void DEY();
	void INX();
	void INY();
	void DEC();
	void INC();

	void DEC_Acc();
	void INC_Acc();

	void IncDecReg(uint16_t& reg, int8_t offset);
	void IncDec(int8_t offset);

	// Compare instructions
	void Compare(uint16_t reg, bool eightBitMode);
	void CMP();
	void CPX();
	void CPY();

	// Jump instructions
	void JML();
	void JMP();
	void JSL();
	void JSR();
	void RTI();
	void RTL();
	void RTS();

	void JMP_AbsIdxXInd();
	void JSR_AbsIdxXInd();

	// Interrupts
	void ProcessInterrupt(uint16_t vector, bool forHardwareInterrupt);
	void BRK();
	void COP();

	// Bitwise operations
	void AND();
	void EOR();
	void ORA();

	template <typename T>
	T ShiftLeft(T value);
	template <typename T>
	T RollLeft(T value);
	template <typename T>
	T ShiftRight(T value);
	template <typename T>
	T RollRight(T value);

	// Shift operations
	void ASL_Acc();
	void ASL();
	void LSR_Acc();
	void LSR();
	void ROL_Acc();
	void ROL();
	void ROR_Acc();
	void ROR();

	// Move operations
	void MVN();
	void MVP();

	// Push/pull instructions
	void PEA();
	void PEI();
	void PER();
	void PHB();
	void PHD();
	void PHK();
	void PHP();
	void PLB();
	void PLD();
	void PLP();

	void PHA();
	void PHX();
	void PHY();
	void PLA();
	void PLX();
	void PLY();

	void PushRegister(uint16_t reg, bool eightBitMode);
	void PullRegister(uint16_t& reg, bool eightBitMode);

	// Store/load instructions
	void LoadRegister(uint16_t& reg, bool eightBitMode);
	void StoreRegister(uint16_t val, bool eightBitMode);

	void LDA();
	void LDX();
	void LDY();

	void STA();
	void STX();
	void STY();
	void STZ();

	// Test bits
	template <typename T>
	void TestBits(T value, bool alterZeroFlagOnly);
	void BIT();

	void TRB();
	void TSB();

	// Transfer registers
	void TAX();
	void TAY();
	void TCD();
	void TCS();
	void TDC();
	void TSC();
	void TSX();
	void TXA();
	void TXS();
	void TXY();
	void TYA();
	void TYX();
	void XBA();
	void XCE();

	// No operation
	void NOP();
	void WDM();

	// Misc.
	void STP();
	void WAI();

	// Addressing modes
	// Absolute: a
	void AddrMode_Abs();
	// Absolute Indexed: a,x
	void AddrMode_AbsIdxX(bool isWrite);
	// Absolute Indexed: a,y
	void AddrMode_AbsIdxY(bool isWrite);
	// Absolute Long: al
	void AddrMode_AbsLng();
	// Absolute Long Indexed: al,x
	void AddrMode_AbsLngIdxX();

	void AddrMode_AbsJmp();
	void AddrMode_AbsLngJmp();
	void AddrMode_AbsInd();    // JMP only
	void AddrMode_AbsIndLng(); // JML only

	void AddrMode_Acc();

	void AddrMode_BlkMov();

	uint8_t ReadDirectOperandByte();

	// Direct: d
	void AddrMode_Dir();
	// Direct Indexed: d,x
	void AddrMode_DirIdxX();
	// Direct Indexed: d,y
	void AddrMode_DirIdxY();
	// Direct Indirect: (d)
	void AddrMode_DirInd();

	// Direct Indexed Indirect: (d,x)
	void AddrMode_DirIdxIndX();
	// Direct Indirect Indexed: (d),y
	void AddrMode_DirIndIdxY(bool isWrite);
	// Direct Indirect Long: [d]
	void AddrMode_DirIndLng();
	// Direct Indirect Indexed Long: [d],y
	void AddrMode_DirIndLngIdxY();

	void AddrMode_Imm8();
	void AddrMode_Imm16();
	void AddrMode_ImmX();
	void AddrMode_ImmM();

	void AddrMode_Imp();

	void AddrMode_RelLng();
	void AddrMode_Rel();

	void AddrMode_StkRel();
	void AddrMode_StkRelIndIdxY();

	__forceinline void RunOp();
	__noinline void ProcessHaltedState();
	__forceinline void CheckForInterrupts();

public:
#ifndef DUMMYCPU
	SnesCpu(SnesConsole* console);
#else
	DummySnesCpu(SnesConsole* console, CpuType type);
#endif

	virtual ~SnesCpu();

	void PowerOn();

	void Reset();
	void Exec();

	SnesCpuState& GetState();
	uint64_t GetCycleCount();

	template <uint64_t value>
	void IncreaseCycleCount();

	void SetNmiFlag(uint8_t delay);
	void DetectNmiSignalEdge();

	void SetIrqSource(SnesIrqSource source);
	bool CheckIrqSource(SnesIrqSource source);
	void ClearIrqSource(SnesIrqSource source);

	// Inherited via ISerializable
	void Serialize(Serializer& s) override;

#ifdef DUMMYCPU
private:
	MemoryMappings* _memoryMappings = nullptr;

	uint32_t _memOpCounter = 0;
	MemoryOperationInfo _memOperations[10] = {};

	void LogMemoryOperation(uint32_t addr, uint8_t value, MemoryOperationType type);

public:
	void SetDummyState(SnesCpuState& state);
	[[nodiscard]] int32_t GetLastOperand();

	[[nodiscard]] uint32_t GetOperationCount();
	MemoryOperationInfo GetOperationInfo(uint32_t index);
#endif
};

template <uint64_t count>
void SnesCpu::IncreaseCycleCount() {
	_state.CycleCount += count;
}

#endif
