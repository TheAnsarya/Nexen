#pragma once
#include "pch.h"
#include "SNES/Coprocessors/BaseCoprocessor.h"
#include "SNES/Coprocessors/CX4/Cx4Types.h"
#include "SNES/MemoryMappings.h"

class SnesConsole;
class Emulator;
class SnesMemoryManager;
class SnesCpu;

/// <summary>
/// CX4 (Hitachi HG51B169) coprocessor emulation.
/// A specialized microcontroller used in Mega Man X2 and Mega Man X3 for
/// real-time wireframe 3D graphics rendering and sprite manipulation.
/// Features:
/// - 20 MHz clock (divided from SNES master clock)
/// - 24-bit accumulator with hardware multiplication
/// - 16 general purpose registers (24-bit)
/// - 2-page program cache for fast code execution
/// - Built-in DMA controller
/// - Programmable micro-code stored in cartridge ROM
/// </summary>
/// <remarks>
/// The CX4 runs custom micro-code programs that perform 3D transformations,
/// line drawing, and sprite generation. The main CPU triggers CX4 operations
/// by writing to control registers and waiting for completion.
/// </remarks>
class Cx4 : public BaseCoprocessor {
private:
	/// <summary>Size of CX4 data RAM (3KB).</summary>
	static constexpr int DataRamSize = 0xC00;

	/// <summary>Pointer to main emulator instance.</summary>
	Emulator* _emu;

	/// <summary>Pointer to SNES console.</summary>
	SnesConsole* _console;

	/// <summary>Pointer to memory manager.</summary>
	SnesMemoryManager* _memoryManager;

	/// <summary>Pointer to main SNES CPU.</summary>
	SnesCpu* _cpu;

	/// <summary>Memory mappings for CX4 address space.</summary>
	MemoryMappings _mappings;

	/// <summary>Clock ratio between CX4 and master clock.</summary>
	double _clockRatio;

	/// <summary>Complete CX4 processor state.</summary>
	Cx4State _state;

	/// <summary>Program RAM - 2 pages of 256 instructions each.</summary>
	uint16_t _prgRam[2][256];

	/// <summary>Data RAM - 3KB of working memory.</summary>
	uint8_t _dataRam[Cx4::DataRamSize];

	/// <summary>
	/// Executes a single CX4 instruction.
	/// </summary>
	/// <param name="opCode">16-bit instruction opcode.</param>
	void Exec(uint16_t opCode);

	/// <summary>Switches the active cache page.</summary>
	void SwitchCachePage();

	/// <summary>
	/// Processes cache loading until target cycle.
	/// </summary>
	/// <param name="targetCycle">Target cycle count to run until.</param>
	/// <returns>True if cache operation completed.</returns>
	bool ProcessCache(uint64_t targetCycle);

	/// <summary>
	/// Processes DMA transfer until target cycle.
	/// </summary>
	/// <param name="targetCycle">Target cycle count to run until.</param>
	void ProcessDma(uint64_t targetCycle);

	/// <summary>
	/// Advances CX4 emulation by specified cycles.
	/// </summary>
	/// <param name="cycles">Number of cycles to execute.</param>
	void Step(uint64_t cycles);

	/// <summary>Checks if CX4 is currently executing code.</summary>
	/// <returns>True if CX4 is running.</returns>
	bool IsRunning();

	/// <summary>Checks if CX4 is busy (running, DMA, or cache loading).</summary>
	/// <returns>True if CX4 is busy.</returns>
	bool IsBusy();

	/// <summary>
	/// Gets memory access delay for address.
	/// </summary>
	/// <param name="addr">Address to check.</param>
	/// <returns>Delay in cycles.</returns>
	uint8_t GetAccessDelay(uint32_t addr);

	/// <summary>
	/// Reads from CX4 internal registers.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <returns>Register value.</returns>
	uint8_t ReadCx4(uint32_t addr);

	/// <summary>
	/// Writes to CX4 internal registers.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteCx4(uint32_t addr, uint8_t value);

	/// <summary>
	/// Gets source operand value for instruction.
	/// </summary>
	/// <param name="src">Source register specifier.</param>
	/// <returns>24-bit source value.</returns>
	uint32_t GetSourceValue(uint8_t src);

	/// <summary>
	/// Writes value to specified register.
	/// </summary>
	/// <param name="reg">Register index.</param>
	/// <param name="value">Value to write.</param>
	void WriteRegister(uint8_t reg, uint32_t value);

	/// <summary>
	/// Sets accumulator and updates flags.
	/// </summary>
	/// <param name="value">New accumulator value.</param>
	void SetA(uint32_t value);

	// ===== CX4 Instructions =====

	/// <summary>NOP - No operation.</summary>
	void NOP();

	/// <summary>WAIT - Suspends execution for delay cycles.</summary>
	void WAIT();

	/// <summary>
	/// Conditional branch instruction.
	/// </summary>
	/// <param name="branch">Branch condition result.</param>
	/// <param name="unused">Unused parameter.</param>
	/// <param name="dest">Branch destination.</param>
	void Branch(bool branch, uint8_t, uint8_t dest);

	/// <summary>
	/// Conditional skip instruction.
	/// </summary>
	/// <param name="flagToCheck">Flag to test.</param>
	/// <param name="skipIfSet">Skip if flag is this value.</param>
	void Skip(uint8_t flagToCheck, uint8_t skipIfSet);

	/// <summary>
	/// Jump to subroutine with condition.
	/// </summary>
	/// <param name="branch">Branch condition result.</param>
	/// <param name="unused">Unused parameter.</param>
	/// <param name="dest">Subroutine address.</param>
	void JSR(bool branch, uint8_t, uint8_t dest);

	/// <summary>RTS - Return from subroutine.</summary>
	void RTS();

	/// <summary>Pushes PC to call stack.</summary>
	void PushPC();

	/// <summary>Pulls PC from call stack.</summary>
	void PullPC();

	/// <summary>Updates Zero and Negative flags based on accumulator.</summary>
	void SetZeroNegativeFlags();

	/// <summary>
	/// Adds two values with carry handling.
	/// </summary>
	/// <param name="a">First operand.</param>
	/// <param name="b">Second operand.</param>
	/// <returns>Sum with carry in bit 24.</returns>
	uint32_t AddValues(uint32_t a, uint32_t b);

	/// <summary>
	/// Subtracts two values with borrow handling.
	/// </summary>
	/// <param name="a">Minuend.</param>
	/// <param name="b">Subtrahend.</param>
	/// <returns>Difference.</returns>
	uint32_t Subtract(uint32_t a, uint32_t b);

	// Comparison instructions
	/// <summary>Compare register (reversed operands) with shift.</summary>
	void CMPR(uint8_t shift, uint8_t src);
	/// <summary>Compare register (reversed) with immediate.</summary>
	void CMPR_Imm(uint8_t shift, uint8_t imm);
	/// <summary>Compare register with shift.</summary>
	void CMP(uint8_t shift, uint8_t src);
	/// <summary>Compare with immediate.</summary>
	void CMP_Imm(uint8_t shift, uint8_t imm);

	/// <summary>Sign extends accumulator based on mode.</summary>
	void SignExtend(uint8_t mode);

	// Load/store instructions
	/// <summary>Load from source to destination register.</summary>
	void Load(uint8_t dest, uint8_t src);
	/// <summary>Load immediate value to register.</summary>
	void Load_Imm(uint8_t dest, uint8_t imm);

	// Arithmetic instructions
	/// <summary>Add register with optional shift.</summary>
	void ADD(uint8_t shift, uint8_t src);
	/// <summary>Add immediate with optional shift.</summary>
	void ADD_Imm(uint8_t shift, uint8_t imm);
	/// <summary>Subtract register with optional shift.</summary>
	void SUB(uint8_t shift, uint8_t src);
	/// <summary>Subtract immediate with optional shift.</summary>
	void SUB_Imm(uint8_t shift, uint8_t imm);
	/// <summary>Subtract reversed (src - A) with optional shift.</summary>
	void SUBR(uint8_t shift, uint8_t src);
	/// <summary>Subtract reversed with immediate.</summary>
	void SUBR_Imm(uint8_t shift, uint8_t imm);

	/// <summary>Signed multiply with register.</summary>
	void SMUL(uint8_t src);
	/// <summary>Signed multiply with immediate.</summary>
	void SMUL_Imm(uint8_t imm);

	// Logical instructions
	/// <summary>Logical AND with optional shift.</summary>
	void AND(uint8_t shift, uint8_t src);
	/// <summary>Logical AND with immediate.</summary>
	void AND_Imm(uint8_t shift, uint8_t imm);
	/// <summary>Logical OR with optional shift.</summary>
	void OR(uint8_t shift, uint8_t src);
	/// <summary>Logical OR with immediate.</summary>
	void OR_Imm(uint8_t shift, uint8_t imm);
	/// <summary>Logical XOR with optional shift.</summary>
	void XOR(uint8_t shift, uint8_t src);
	/// <summary>Logical XOR with immediate.</summary>
	void XOR_Imm(uint8_t shift, uint8_t imm);
	/// <summary>Logical XNOR with optional shift.</summary>
	void XNOR(uint8_t shift, uint8_t src);
	/// <summary>Logical XNOR with immediate.</summary>
	void XNOR_Imm(uint8_t shift, uint8_t imm);

	// Shift/rotate instructions
	/// <summary>Logical shift right by register amount.</summary>
	void SHR(uint8_t src);
	/// <summary>Logical shift right by immediate.</summary>
	void SHR_Imm(uint8_t imm);
	/// <summary>Arithmetic shift right by register amount.</summary>
	void ASR(uint8_t src);
	/// <summary>Arithmetic shift right by immediate.</summary>
	void ASR_Imm(uint8_t imm);
	/// <summary>Shift left by register amount.</summary>
	void SHL(uint8_t src);
	/// <summary>Shift left by immediate.</summary>
	void SHL_Imm(uint8_t imm);
	/// <summary>Rotate right by register amount.</summary>
	void ROR(uint8_t src);
	/// <summary>Rotate right by immediate.</summary>
	void ROR_Imm(uint8_t imm);

	// Memory access instructions
	/// <summary>Read from ROM using address in register.</summary>
	void ReadRom();
	/// <summary>Read from ROM at immediate address.</summary>
	void ReadRom_Imm(uint16_t imm);
	/// <summary>Read RAM byte at specified index.</summary>
	void ReadRam(uint8_t byteIndex);
	/// <summary>Read RAM byte with immediate offset.</summary>
	void ReadRam_Imm(uint8_t byteIndex, uint8_t imm);
	/// <summary>Write RAM byte at specified index.</summary>
	void WriteRam(uint8_t byteIndex);
	/// <summary>Write RAM byte with immediate offset.</summary>
	void WriteRam_Imm(uint8_t byteIndex, uint8_t imm);

	/// <summary>Load byte into page register.</summary>
	void LoadP(uint8_t byteIndex, uint8_t imm);

	// Register manipulation
	/// <summary>Swap bytes in register.</summary>
	void Swap(uint8_t reg);
	/// <summary>Increment memory address register.</summary>
	void IncMar();
	/// <summary>Store source register to destination.</summary>
	void Store(uint8_t src, uint8_t dst);
	/// <summary>Stop CX4 execution.</summary>
	void Stop();

public:
	/// <summary>
	/// Creates a new CX4 coprocessor instance.
	/// </summary>
	/// <param name="console">SNES console instance.</param>
	Cx4(SnesConsole* console);

	/// <summary>Resets CX4 to initial power-on state.</summary>
	void Reset() override;

	/// <summary>Main execution loop - runs CX4 until stopped.</summary>
	void Run() override;

	/// <summary>
	/// Reads from CX4 address space.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Data byte.</returns>
	uint8_t Read(uint32_t addr) override;

	/// <summary>
	/// Writes to CX4 address space.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Data byte.</param>
	void Write(uint32_t addr, uint8_t value) override;

	/// <summary>
	/// Serializes CX4 state for save states.
	/// </summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override;

	/// <summary>
	/// Peeks memory without side effects.
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Data byte.</returns>
	uint8_t Peek(uint32_t addr) override;

	/// <summary>
	/// Peeks a block of memory.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer for data.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override;

	/// <summary>
	/// Gets absolute address for debugging.
	/// </summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address info.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t address) override;

	/// <summary>Gets pointer to CX4 memory mappings.</summary>
	MemoryMappings* GetMemoryMappings();

	/// <summary>Gets reference to current CX4 state.</summary>
	Cx4State& GetState();
};