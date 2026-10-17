#pragma once
#include "pch.h"
#include "SNES/SnesCpuTypes.h"
#include "Utilities/ISerializable.h"
#include "Shared/MemoryOperationType.h"

class Sa1;
class Emulator;

/// <summary>
/// SA-1 CPU emulation (65C816 variant running at 10.74MHz).
/// The SA-1 is a coprocessor that contains a second 65C816 CPU core
/// running at approximately 4x the speed of the main S-CPU.
/// </summary>
/// <remarks>
/// The SA-1 CPU is nearly identical to the main SNES 65C816 with some key differences:
/// - Runs at 10.74MHz (vs ~3.58MHz for S-CPU, so approximately 3x faster)
/// - Has direct access to SA-1 internal RAM (2KB I-RAM) and BW-RAM
/// - Can perform hardware-accelerated arithmetic (multiply/divide)
/// - Has its own interrupt system (NMI, IRQ from multiple sources)
/// - Supports character conversion DMA for graphics format conversion
/// 
/// Memory map differences:
/// - ROM access can be slower due to bus arbitration with S-CPU
/// - Has dedicated fast RAM regions not accessible to S-CPU directly
/// - Vector addresses can be remapped by the SA-1 control registers
/// 
/// The SA-1 and S-CPU communicate via shared registers and interrupt signals.
/// </remarks>
class Sa1Cpu : public ISerializable {
public:
	/// <summary>NMI vector address ($FFEA in native mode).</summary>
	static constexpr uint32_t NmiVector = 0x00FFEA;

	/// <summary>Reset vector address ($FFFC).</summary>
	static constexpr uint32_t ResetVector = 0x00FFFC;

	/// <summary>IRQ vector address ($FFEE in native mode).</summary>
	static constexpr uint32_t IrqVector = 0x00FFEE;

private:
	/// <summary>Abort vector address ($FFE8 in native mode).</summary>
	static constexpr uint32_t AbortVector = 0x00FFE8;

	/// <summary>BRK instruction vector address ($FFE6 in native mode).</summary>
	static constexpr uint32_t BreakVector = 0x00FFE6;

	/// <summary>COP instruction vector address ($FFE4 in native mode).</summary>
	static constexpr uint32_t CoprocessorVector = 0x00FFE4;

	/// <summary>Legacy (6502 emulation mode) NMI vector ($FFFA).</summary>
	static constexpr uint16_t LegacyNmiVector = 0xFFFA;

	/// <summary>Legacy (6502 emulation mode) IRQ vector ($FFFE).</summary>
	static constexpr uint32_t LegacyIrqVector = 0xFFFE;

	/// <summary>Legacy (6502 emulation mode) COP vector ($FFF4).</summary>
	static constexpr uint32_t LegacyCoprocessorVector = 0x00FFF4;

	/// <summary>Reference to parent SA-1 coprocessor.</summary>
	Sa1* _sa1 = nullptr;

	/// <summary>Reference to emulator for debugging.</summary>
	Emulator* _emu = nullptr;

	/// <summary>True when processing an immediate mode instruction.</summary>
	bool _immediateMode = false;

	/// <summary>Address mask for read/write operations (24-bit address space).</summary>
	uint32_t _readWriteMask = 0xFFFFFF;

	/// <summary>CPU register state (A, X, Y, SP, PC, P, etc.).</summary>
	SnesCpuState _state = {};

	/// <summary>Current instruction operand value.</summary>
	uint32_t _operand = 0;

	/// <summary>True when WAI instruction has completed (interrupt received).</summary>
	bool _waiOver = true;

	/// <summary>Gets the full 24-bit program address from 16-bit PC.</summary>
	uint32_t GetProgramAddress(uint16_t addr);

	/// <summary>Gets the full 24-bit data address with data bank.</summary>
	uint32_t GetDataAddress(uint16_t addr);

	/// <summary>Calculates direct page address with offset.</summary>
	uint16_t GetDirectAddress(uint16_t offset, bool allowEmulationMode = true);

	/// <summary>Reads indirect word from direct page.</summary>
	uint16_t GetDirectAddressIndirectWord(uint16_t offset);

	/// <summary>Reads indirect word from direct page with page wrap (emulation mode).</summary>
	uint16_t GetDirectAddressIndirectWordWithPageWrap(uint16_t offset);

	/// <summary>Reads indirect long (24-bit address) from direct page.</summary>
	uint32_t GetDirectAddressIndirectLong(uint16_t offset);

	/// <summary>Fetches the next opcode byte from the instruction stream.</summary>
	uint8_t GetOpCode();

	/// <summary>Reads a 16-bit vector address from the vector table.</summary>
	uint16_t ReadVector(uint16_t vector);

	/// <summary>Gets the reset vector address (potentially remapped by SA-1).</summary>
	uint16_t GetResetVector();

	/// <summary>Processes a single CPU cycle for timing.</summary>
	void ProcessCpuCycle(uint32_t addr);

	/// <summary>Executes an idle cycle (no memory access).</summary>
	void Idle();

	/// <summary>Executes idle or performs a dummy read (opcode-dependent).</summary>
	void IdleOrRead();

	/// <summary>Executes idle or performs a dummy write (for read-modify-write).</summary>
	void IdleOrDummyWrite(uint32_t addr, uint8_t value);

	/// <summary>Idle cycle at end of jump instruction.</summary>
	void IdleEndJump();

	/// <summary>Idle cycle when branch is taken.</summary>
	void IdleTakeBranch();

	/// <summary>Detects rising edge of NMI signal for interrupt processing.</summary>
	void DetectNmiSignalEdge();

	/// <summary>Checks if there's a bus access conflict with S-CPU.</summary>
	bool IsAccessConflict();

	/// <summary>Reads a single byte operand from instruction stream.</summary>
	uint8_t ReadOperandByte();

	/// <summary>Reads a 16-bit word operand from instruction stream.</summary>
	uint16_t ReadOperandWord();

	/// <summary>Reads a 24-bit long operand from instruction stream.</summary>
	uint32_t ReadOperandLong();

	/// <summary>Reads a byte from the specified address.</summary>
	uint8_t Read(uint32_t addr, MemoryOperationType type);

	/// <summary>Sets the stack pointer with optional emulation mode handling.</summary>
	void SetSP(uint16_t sp, bool allowEmulationMode = true);

	/// <summary>Restricts stack pointer to valid range (emulation mode: page 1).</summary>
	__forceinline void RestrictStackPointerValue();

	/// <summary>Sets the processor status register and updates mode flags.</summary>
	void SetPS(uint8_t ps);

	/// <summary>Sets an 8-bit register and updates N/Z flags.</summary>
	void SetRegister(uint8_t& reg, uint8_t value);

	/// <summary>Sets a 16-bit register with optional 8-bit mode handling.</summary>
	void SetRegister(uint16_t& reg, uint16_t value, bool eightBitMode);

	/// <summary>Sets Zero and Negative flags based on 16-bit value.</summary>
	void SetZeroNegativeFlags(uint16_t value);

	/// <summary>Sets Zero and Negative flags based on 8-bit value.</summary>
	void SetZeroNegativeFlags(uint8_t value);

	/// <summary>Clears the specified processor flags.</summary>
	void ClearFlags(uint8_t flags);

	/// <summary>Sets the specified processor flags.</summary>
	void SetFlags(uint8_t flags);

	/// <summary>Checks if a specific processor flag is set.</summary>
	bool CheckFlag(uint8_t flag);

	/// <summary>Reads code from program bank (PBR:addr).</summary>
	uint8_t ReadCode(uint16_t addr, MemoryOperationType type = MemoryOperationType::Read);

	/// <summary>Reads a 16-bit word from program bank.</summary>
	uint16_t ReadCodeWord(uint16_t addr, MemoryOperationType type = MemoryOperationType::Read);

	/// <summary>Reads data from the specified 24-bit address.</summary>
	uint8_t ReadData(uint32_t addr, MemoryOperationType type = MemoryOperationType::Read);

	/// <summary>Reads a 16-bit word from the specified 24-bit address.</summary>
	uint16_t ReadDataWord(uint32_t addr, MemoryOperationType type = MemoryOperationType::Read);

	/// <summary>Reads a 24-bit long from the specified address.</summary>
	uint32_t ReadDataLong(uint32_t addr, MemoryOperationType type = MemoryOperationType::Read);

	/// <summary>Writes a byte to the specified 24-bit address.</summary>
	void Write(uint32_t addr, uint8_t value, MemoryOperationType type = MemoryOperationType::Write);

	/// <summary>Writes a 16-bit word to the specified address (low byte first).</summary>
	void WriteWord(uint32_t addr, uint16_t value, MemoryOperationType type = MemoryOperationType::Write);

	/// <summary>Writes a word for read-modify-write operations (high byte first).</summary>
	void WriteWordRmw(uint32_t addr, uint16_t value, MemoryOperationType type = MemoryOperationType::Write);

	/// <summary>Gets the accumulator value in 8-bit mode.</summary>
	uint8_t GetByteValue();

	/// <summary>Gets the accumulator value in 16-bit mode.</summary>
	uint16_t GetWordValue();

	/// <summary>Pushes a byte onto the stack.</summary>
	void PushByte(uint8_t value, bool allowEmulationMode = true);

	/// <summary>Pops a byte from the stack.</summary>
	uint8_t PopByte(bool allowEmulationMode = true);

	/// <summary>Pushes a 16-bit word onto the stack (high byte first).</summary>
	void PushWord(uint16_t value, bool allowEmulationMode = true);

	/// <summary>Pops a 16-bit word from the stack (low byte first).</summary>
	uint16_t PopWord(bool allowEmulationMode = true);

	// ==================== Arithmetic Instructions ====================

	/// <summary>8-bit addition with carry (ADC in 8-bit mode).</summary>
	void Add8(uint8_t value);

	/// <summary>16-bit addition with carry (ADC in 16-bit mode).</summary>
	void Add16(uint16_t value);

	/// <summary>ADC - Add with Carry instruction.</summary>
	void ADC();

	/// <summary>8-bit subtraction with borrow (SBC in 8-bit mode).</summary>
	void Sub8(uint8_t value);

	/// <summary>16-bit subtraction with borrow (SBC in 16-bit mode).</summary>
	void Sub16(uint16_t value);

	/// <summary>SBC - Subtract with Carry instruction.</summary>
	void SBC();

	// ==================== Branch Instructions ====================

	/// <summary>BCC - Branch if Carry Clear.</summary>
	void BCC();

	/// <summary>BCS - Branch if Carry Set.</summary>
	void BCS();

	/// <summary>BEQ - Branch if Equal (Zero flag set).</summary>
	void BEQ();

	/// <summary>BMI - Branch if Minus (Negative flag set).</summary>
	void BMI();

	/// <summary>BNE - Branch if Not Equal (Zero flag clear).</summary>
	void BNE();

	/// <summary>BPL - Branch if Plus (Negative flag clear).</summary>
	void BPL();

	/// <summary>BRA - Branch Always (unconditional relative branch).</summary>
	void BRA();

	/// <summary>BRL - Branch Long Always (16-bit relative offset).</summary>
	void BRL();

	/// <summary>BVC - Branch if Overflow Clear.</summary>
	void BVC();

	/// <summary>BVS - Branch if Overflow Set.</summary>
	void BVS();

	/// <summary>Helper for relative branch instructions.</summary>
	void BranchRelative(bool branch);

	// ==================== Flag Instructions ====================

	/// <summary>CLC - Clear Carry Flag.</summary>
	void CLC();

	/// <summary>CLD - Clear Decimal Mode Flag.</summary>
	void CLD();

	/// <summary>CLI - Clear Interrupt Disable Flag.</summary>
	void CLI();

	/// <summary>CLV - Clear Overflow Flag.</summary>
	void CLV();

	/// <summary>SEC - Set Carry Flag.</summary>
	void SEC();

	/// <summary>SED - Set Decimal Mode Flag.</summary>
	void SED();

	/// <summary>SEI - Set Interrupt Disable Flag.</summary>
	void SEI();

	/// <summary>REP - Reset Processor Status Bits.</summary>
	void REP();

	/// <summary>SEP - Set Processor Status Bits.</summary>
	void SEP();

	// ==================== Increment/Decrement Instructions ====================

	/// <summary>DEX - Decrement Index Register X.</summary>
	void DEX();

	/// <summary>DEY - Decrement Index Register Y.</summary>
	void DEY();

	/// <summary>INX - Increment Index Register X.</summary>
	void INX();

	/// <summary>INY - Increment Index Register Y.</summary>
	void INY();

	/// <summary>DEC - Decrement Memory.</summary>
	void DEC();

	/// <summary>INC - Increment Memory.</summary>
	void INC();

	/// <summary>DEC A - Decrement Accumulator.</summary>
	void DEC_Acc();

	/// <summary>INC A - Increment Accumulator.</summary>
	void INC_Acc();

	/// <summary>Helper for increment/decrement register operations.</summary>
	void IncDecReg(uint16_t& reg, int8_t offset);

	/// <summary>Helper for increment/decrement memory operations.</summary>
	void IncDec(int8_t offset);

	// ==================== Compare Instructions ====================

	/// <summary>Helper for compare instructions.</summary>
	void Compare(uint16_t reg, bool eightBitMode);

	/// <summary>CMP - Compare Accumulator with Memory.</summary>
	void CMP();

	/// <summary>CPX - Compare Index Register X with Memory.</summary>
	void CPX();

	/// <summary>CPY - Compare Index Register Y with Memory.</summary>
	void CPY();

	// ==================== Jump/Call Instructions ====================

	/// <summary>JML - Jump Long (24-bit absolute address).</summary>
	void JML();

	/// <summary>JMP - Jump (16-bit address within current bank).</summary>
	void JMP();

	/// <summary>JSL - Jump to Subroutine Long (24-bit, pushes PBR and PC).</summary>
	void JSL();

	/// <summary>JSR - Jump to Subroutine (16-bit, pushes PC).</summary>
	void JSR();

	/// <summary>RTI - Return from Interrupt (pops P, PC, and optionally PBR).</summary>
	void RTI();

	/// <summary>RTL - Return from Subroutine Long (pops PC and PBR).</summary>
	void RTL();

	/// <summary>RTS - Return from Subroutine (pops PC).</summary>
	void RTS();

	/// <summary>JMP (a,x) - Jump Absolute Indexed Indirect.</summary>
	void JMP_AbsIdxXInd();

	/// <summary>JSR (a,x) - Jump to Subroutine Absolute Indexed Indirect.</summary>
	void JSR_AbsIdxXInd();

	// ==================== Interrupt Instructions ====================

	/// <summary>Processes an interrupt (hardware or software).</summary>
	void ProcessInterrupt(uint16_t vector, bool forHardwareInterrupt);

	/// <summary>BRK - Software Break instruction.</summary>
	void BRK();

	/// <summary>COP - Coprocessor Enable instruction.</summary>
	void COP();

	// ==================== Bitwise Logic Instructions ====================

	/// <summary>AND - Logical AND with Accumulator.</summary>
	void AND();

	/// <summary>EOR - Exclusive OR with Accumulator.</summary>
	void EOR();

	/// <summary>ORA - Logical OR with Accumulator.</summary>
	void ORA();

	// ==================== Shift/Rotate Templates ====================

	/// <summary>Shifts value left, carry receives bit 7.</summary>
	template <typename T>
	T ShiftLeft(T value);

	/// <summary>Rotates value left through carry.</summary>
	template <typename T>
	T RollLeft(T value);

	/// <summary>Shifts value right, carry receives bit 0.</summary>
	template <typename T>
	T ShiftRight(T value);

	/// <summary>Rotates value right through carry.</summary>
	template <typename T>
	T RollRight(T value);

	// ==================== Shift/Rotate Instructions ====================

	/// <summary>ASL A - Arithmetic Shift Left Accumulator.</summary>
	void ASL_Acc();

	/// <summary>ASL - Arithmetic Shift Left Memory.</summary>
	void ASL();

	/// <summary>LSR A - Logical Shift Right Accumulator.</summary>
	void LSR_Acc();

	/// <summary>LSR - Logical Shift Right Memory.</summary>
	void LSR();

	/// <summary>ROL A - Rotate Left Accumulator.</summary>
	void ROL_Acc();

	/// <summary>ROL - Rotate Left Memory.</summary>
	void ROL();

	/// <summary>ROR A - Rotate Right Accumulator.</summary>
	void ROR_Acc();

	/// <summary>ROR - Rotate Right Memory.</summary>
	void ROR();

	// ==================== Block Move Instructions ====================

	/// <summary>MVN - Block Move Negative (decrementing addresses).</summary>
	void MVN();

	/// <summary>MVP - Block Move Positive (incrementing addresses).</summary>
	void MVP();

	// ==================== Push/Pull Instructions ====================

	/// <summary>PEA - Push Effective Absolute Address.</summary>
	void PEA();

	/// <summary>PEI - Push Effective Indirect Address.</summary>
	void PEI();

	/// <summary>PER - Push Effective PC Relative Address.</summary>
	void PER();

	/// <summary>PHB - Push Data Bank Register.</summary>
	void PHB();

	/// <summary>PHD - Push Direct Page Register.</summary>
	void PHD();

	/// <summary>PHK - Push Program Bank Register.</summary>
	void PHK();

	/// <summary>PHP - Push Processor Status.</summary>
	void PHP();

	/// <summary>PLB - Pull Data Bank Register.</summary>
	void PLB();

	/// <summary>PLD - Pull Direct Page Register.</summary>
	void PLD();

	/// <summary>PLP - Pull Processor Status.</summary>
	void PLP();

	/// <summary>PHA - Push Accumulator.</summary>
	void PHA();

	/// <summary>PHX - Push Index Register X.</summary>
	void PHX();

	/// <summary>PHY - Push Index Register Y.</summary>
	void PHY();

	/// <summary>PLA - Pull Accumulator.</summary>
	void PLA();

	/// <summary>PLX - Pull Index Register X.</summary>
	void PLX();

	/// <summary>PLY - Pull Index Register Y.</summary>
	void PLY();

	/// <summary>Helper for pushing registers with width mode handling.</summary>
	void PushRegister(uint16_t reg, bool eightBitMode);

	/// <summary>Helper for pulling registers with width mode handling.</summary>
	void PullRegister(uint16_t& reg, bool eightBitMode);

	// ==================== Load/Store Instructions ====================

	/// <summary>Helper for load register instructions.</summary>
	void LoadRegister(uint16_t& reg, bool eightBitMode);

	/// <summary>Helper for store register instructions.</summary>
	void StoreRegister(uint16_t val, bool eightBitMode);

	/// <summary>LDA - Load Accumulator from Memory.</summary>
	void LDA();

	/// <summary>LDX - Load Index Register X from Memory.</summary>
	void LDX();

	/// <summary>LDY - Load Index Register Y from Memory.</summary>
	void LDY();

	/// <summary>STA - Store Accumulator to Memory.</summary>
	void STA();

	/// <summary>STX - Store Index Register X to Memory.</summary>
	void STX();

	/// <summary>STY - Store Index Register Y to Memory.</summary>
	void STY();

	/// <summary>STZ - Store Zero to Memory.</summary>
	void STZ();

	// ==================== Bit Test Instructions ====================

	/// <summary>Helper template for bit test operations.</summary>
	template <typename T>
	void TestBits(T value, bool alterZeroFlagOnly);

	/// <summary>BIT - Bit Test (sets N, V, Z based on memory AND accumulator).</summary>
	void BIT();

	/// <summary>TRB - Test and Reset Bits (clears bits in memory that are set in A).</summary>
	void TRB();

	/// <summary>TSB - Test and Set Bits (sets bits in memory that are set in A).</summary>
	void TSB();

	// ==================== Transfer Instructions ====================

	/// <summary>TAX - Transfer Accumulator to Index X.</summary>
	void TAX();

	/// <summary>TAY - Transfer Accumulator to Index Y.</summary>
	void TAY();

	/// <summary>TCD - Transfer 16-bit Accumulator to Direct Page Register.</summary>
	void TCD();

	/// <summary>TCS - Transfer 16-bit Accumulator to Stack Pointer.</summary>
	void TCS();

	/// <summary>TDC - Transfer Direct Page Register to 16-bit Accumulator.</summary>
	void TDC();

	/// <summary>TSC - Transfer Stack Pointer to 16-bit Accumulator.</summary>
	void TSC();

	/// <summary>TSX - Transfer Stack Pointer to Index X.</summary>
	void TSX();

	/// <summary>TXA - Transfer Index X to Accumulator.</summary>
	void TXA();

	/// <summary>TXS - Transfer Index X to Stack Pointer.</summary>
	void TXS();

	/// <summary>TXY - Transfer Index X to Index Y.</summary>
	void TXY();

	/// <summary>TYA - Transfer Index Y to Accumulator.</summary>
	void TYA();

	/// <summary>TYX - Transfer Index Y to Index X.</summary>
	void TYX();

	/// <summary>XBA - Exchange B and A (swap high and low bytes of 16-bit A).</summary>
	void XBA();

	/// <summary>XCE - Exchange Carry and Emulation flags (switches native/emulation mode).</summary>
	void XCE();

	// ==================== Miscellaneous Instructions ====================

	/// <summary>NOP - No Operation.</summary>
	void NOP();

	/// <summary>WDM - Reserved for future expansion (2-byte NOP).</summary>
	void WDM();

	/// <summary>STP - Stop the Processor (until reset).</summary>
	void STP();

	/// <summary>WAI - Wait for Interrupt (halts CPU until interrupt).</summary>
	void WAI();

	// ==================== Addressing Modes ====================

	/// <summary>Absolute addressing: a (16-bit address in data bank).</summary>
	void AddrMode_Abs();

	/// <summary>Absolute Indexed X: a,x (16-bit + X in data bank).</summary>
	void AddrMode_AbsIdxX(bool isWrite);

	/// <summary>Absolute Indexed Y: a,y (16-bit + Y in data bank).</summary>
	void AddrMode_AbsIdxY(bool isWrite);

	/// <summary>Absolute Long: al (24-bit address).</summary>
	void AddrMode_AbsLng();

	/// <summary>Absolute Long Indexed: al,x (24-bit + X).</summary>
	void AddrMode_AbsLngIdxX();

	/// <summary>Absolute for JMP instruction.</summary>
	void AddrMode_AbsJmp();

	/// <summary>Absolute Long for JML instruction.</summary>
	void AddrMode_AbsLngJmp();

	/// <summary>Absolute Indirect: (a) - JMP only, reads 16-bit pointer.</summary>
	void AddrMode_AbsInd();

	/// <summary>Absolute Indirect Long: [a] - JML only, reads 24-bit pointer.</summary>
	void AddrMode_AbsIndLng();

	/// <summary>Accumulator addressing (implied A register).</summary>
	void AddrMode_Acc();

	/// <summary>Block move addressing (source/dest banks in operand).</summary>
	void AddrMode_BlkMov();

	/// <summary>Reads a byte operand for direct page addressing.</summary>
	uint8_t ReadDirectOperandByte();

	/// <summary>Direct Page: d (8-bit offset from DP register).</summary>
	void AddrMode_Dir();

	/// <summary>Direct Page Indexed X: d,x (DP + offset + X).</summary>
	void AddrMode_DirIdxX();

	/// <summary>Direct Page Indexed Y: d,y (DP + offset + Y).</summary>
	void AddrMode_DirIdxY();

	/// <summary>Direct Page Indirect: (d) - reads 16-bit pointer from DP.</summary>
	void AddrMode_DirInd();

	/// <summary>Direct Page Indexed Indirect: (d,x) - DP + X points to 16-bit address.</summary>
	void AddrMode_DirIdxIndX();

	/// <summary>Direct Page Indirect Indexed: (d),y - DP points to address, then add Y.</summary>
	void AddrMode_DirIndIdxY(bool isWrite);

	/// <summary>Direct Page Indirect Long: [d] - reads 24-bit pointer from DP.</summary>
	void AddrMode_DirIndLng();

	/// <summary>Direct Page Indirect Long Indexed: [d],y - 24-bit pointer + Y.</summary>
	void AddrMode_DirIndLngIdxY();

	/// <summary>Immediate 8-bit: #const (8-bit constant).</summary>
	void AddrMode_Imm8();

	/// <summary>Immediate 16-bit: #const (16-bit constant).</summary>
	void AddrMode_Imm16();

	/// <summary>Immediate X/Y width: uses 8 or 16-bit based on X flag.</summary>
	void AddrMode_ImmX();

	/// <summary>Immediate Memory width: uses 8 or 16-bit based on M flag.</summary>
	void AddrMode_ImmM();

	/// <summary>Implied addressing (no operand).</summary>
	void AddrMode_Imp();

	/// <summary>Relative Long: rl (16-bit signed offset for BRL).</summary>
	void AddrMode_RelLng();

	/// <summary>Relative: r (8-bit signed offset for branches).</summary>
	void AddrMode_Rel();

	/// <summary>Stack Relative: d,s (stack pointer + 8-bit offset).</summary>
	void AddrMode_StkRel();

	/// <summary>Stack Relative Indirect Indexed: (d,s),y - SR points to address, add Y.</summary>
	void AddrMode_StkRelIndIdxY();

	/// <summary>Executes the current opcode.</summary>
	void RunOp();

	/// <summary>Checks for pending NMI/IRQ and processes if needed.</summary>
	__forceinline void CheckForInterrupts();

	/// <summary>Processes the halted state (WAI/STP).</summary>
	__noinline void ProcessHaltedState();

public:
	/// <summary>
	/// Creates a new SA-1 CPU instance.
	/// </summary>
	/// <param name="sa1">Reference to parent SA-1 coprocessor.</param>
	/// <param name="emu">Reference to emulator for debugging.</param>
	Sa1Cpu(Sa1* sa1, Emulator* emu);

	/// <summary>Destructor.</summary>
	virtual ~Sa1Cpu();

	/// <summary>Initializes CPU state to power-on defaults.</summary>
	void PowerOn();

	/// <summary>Resets the CPU (reads reset vector, initializes state).</summary>
	void Reset();

	/// <summary>Executes a single instruction.</summary>
	void Exec();

	/// <summary>Gets the current CPU state for debugging.</summary>
	SnesCpuState& GetState();

	/// <summary>Gets the total cycle count since power-on.</summary>
	uint64_t GetCycleCount();

	/// <summary>Increases cycle count by a compile-time constant.</summary>
	template <uint64_t value>
	void IncreaseCycleCount();

	/// <summary>Sets the NMI signal with optional delay.</summary>
	void SetNmiFlag(uint8_t delay);

	/// <summary>Sets an IRQ source (multiple sources can be active).</summary>
	void SetIrqSource(SnesIrqSource source);

	/// <summary>Checks if a specific IRQ source is active.</summary>
	bool CheckIrqSource(SnesIrqSource source);

	/// <summary>Clears an IRQ source.</summary>
	void ClearIrqSource(SnesIrqSource source);

	/// <summary>Increases cycle count by a runtime value.</summary>
	void IncreaseCycleCount(uint64_t cycleCount);

	/// <summary>Serializes CPU state for save states.</summary>
	void Serialize(Serializer& s) override;
};

/// <summary>
/// Template implementation for increasing cycle count by a compile-time constant.
/// </summary>
template <uint64_t count>
void Sa1Cpu::IncreaseCycleCount() {
	_state.CycleCount += count;
}
