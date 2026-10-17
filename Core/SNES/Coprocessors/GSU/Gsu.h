#pragma once
#include "pch.h"
#include <memory>
#include "SNES/Coprocessors/BaseCoprocessor.h"
#include "SNES/Coprocessors/GSU/GsuTypes.h"
#include "SNES/MemoryMappings.h"
#include "SNES/IMemoryHandler.h"

class Emulator;
class SnesConsole;
class SnesCpu;
class SnesMemoryManager;
class EmuSettings;

enum class MemoryOperationType;

/// <summary>
/// GSU (Super FX) coprocessor emulation.
/// The GSU is a custom 16-bit RISC CPU designed by Argonaut Software for Nintendo.
/// Features include:
/// - 10.74 MHz clock (21.47 MHz with /2 divider, or 10.74 MHz direct)
/// - 16 general-purpose 16-bit registers
/// - 512-byte instruction cache for faster execution
/// - Hardware-accelerated plot/pixel operations for 3D rendering
/// - Direct memory access to cartridge ROM and up to 128KB RAM
/// - Bit-depth modes: 2, 4, and 8 bits per pixel
/// Used in: Star Fox, Yoshi's Island, Doom, Winter Gold, Stunt Race FX, Vortex, etc.
/// </summary>
/// <remarks>
/// The GSU operates in parallel with the main SNES CPU but must share bus access.
/// When GSU is running, the main CPU cannot access cartridge ROM/RAM.
/// The GSU uses a GO/STOP mechanism - main CPU writes GO flag to start GSU execution,
/// GSU runs until it executes STOP instruction, then signals completion via IRQ.
/// </remarks>
class Gsu : public BaseCoprocessor {
private:
	/// <summary>Pointer to the main emulator instance.</summary>
	Emulator* _emu;

	/// <summary>Pointer to the SNES console.</summary>
	SnesConsole* _console;

	/// <summary>Pointer to the SNES memory manager for memory access.</summary>
	SnesMemoryManager* _memoryManager;

	/// <summary>Pointer to the main SNES CPU for synchronization.</summary>
	SnesCpu* _cpu;

	/// <summary>Emulator settings reference.</summary>
	EmuSettings* _settings;

	/// <summary>Clock speed multiplier for GSU timing.</summary>
	uint8_t _clockMultiplier;

	/// <summary>Complete GSU processor state.</summary>
	GsuState _state;

	/// <summary>512-byte instruction cache for prefetched code.</summary>
	uint8_t _cache[512];

	/// <summary>Validity flags for each 16-byte cache line (32 lines total).</summary>
	bool _cacheValid[32] = {};

	/// <summary>Flag indicating GSU is waiting for ROM access to complete.</summary>
	bool _waitForRomAccess = false;

	/// <summary>Flag indicating GSU is waiting for RAM access to complete.</summary>
	bool _waitForRamAccess = false;

	/// <summary>Flag indicating GSU has stopped execution.</summary>
	bool _stopped = true;

	/// <summary>Flag indicating R15 (PC) was modified and cache may need refresh.</summary>
	bool _r15Changed = false;

	/// <summary>Address of the last executed opcode (for debugging).</summary>
	uint32_t _lastOpAddr = 0;

	/// <summary>Size of GSU RAM in bytes (up to 128KB).</summary>
	uint32_t _gsuRamSize = 0;

	/// <summary>GSU-dedicated RAM buffer.</summary>
	std::unique_ptr<uint8_t[]> _gsuRam;

	/// <summary>Memory mappings for GSU address space.</summary>
	MemoryMappings _mappings;

	/// <summary>Memory handlers for GSU RAM access.</summary>
	vector<unique_ptr<IMemoryHandler>> _gsuRamHandlers;

	/// <summary>Memory handlers for CPU access to GSU RAM.</summary>
	vector<unique_ptr<IMemoryHandler>> _gsuCpuRamHandlers;

	/// <summary>Memory handlers for CPU access to cartridge ROM.</summary>
	vector<unique_ptr<IMemoryHandler>> _gsuCpuRomHandlers;

	/// <summary>Executes GSU instructions until stopped or interrupted.</summary>
	void Exec();

	/// <summary>
	/// Initializes the program cache at the specified address.
	/// </summary>
	/// <param name="cacheAddr">Base address for cache initialization.</param>
	void InitProgramCache(uint16_t cacheAddr);

	/// <summary>Reads the next operand byte from program memory.</summary>
	/// <returns>The operand byte.</returns>
	uint8_t ReadOperand();

	/// <summary>Reads the next opcode byte from program memory.</summary>
	/// <returns>The opcode byte.</returns>
	uint8_t ReadOpCode();

	/// <summary>
	/// Reads a byte from program memory with specified operation type.
	/// </summary>
	/// <param name="opType">Type of memory operation for debugging.</param>
	/// <returns>The program byte.</returns>
	uint8_t ReadProgramByte(MemoryOperationType opType);

	/// <summary>Reads the value from the current source register.</summary>
	/// <returns>16-bit value from source register.</returns>
	uint16_t ReadSrcReg();

	/// <summary>
	/// Writes a value to the current destination register.
	/// </summary>
	/// <param name="value">16-bit value to write.</param>
	void WriteDestReg(uint16_t value);

	/// <summary>
	/// Writes a value to a specific register.
	/// </summary>
	/// <param name="reg">Register index (0-15).</param>
	/// <param name="value">16-bit value to write.</param>
	void WriteRegister(uint8_t reg, uint16_t value);

	/// <summary>Resets the prefix flags (ALT1, ALT2, B) and source/dest registers.</summary>
	void ResetFlags();

	/// <summary>Invalidates the entire instruction cache.</summary>
	void InvalidateCache();

	/// <summary>Waits for pending ROM operation to complete.</summary>
	void WaitRomOperation();

	/// <summary>Waits for pending RAM operation to complete.</summary>
	void WaitRamOperation();

	/// <summary>Blocks until ROM bus is available for access.</summary>
	void WaitForRomAccess();

	/// <summary>Blocks until RAM bus is available for access.</summary>
	void WaitForRamAccess();

	/// <summary>Updates the running state based on GO flag and pending operations.</summary>
	void UpdateRunningState();

	/// <summary>Reads the ROM buffer register value.</summary>
	/// <returns>Buffered ROM data byte.</returns>
	uint8_t ReadRomBuffer();

	/// <summary>
	/// Reads a byte from RAM buffer at specified address.
	/// </summary>
	/// <param name="addr">RAM address to read.</param>
	/// <returns>RAM data byte.</returns>
	uint8_t ReadRamBuffer(uint16_t addr);

	/// <summary>
	/// Writes a byte to RAM at specified address.
	/// </summary>
	/// <param name="addr">RAM address to write.</param>
	/// <param name="value">Data byte to write.</param>
	void WriteRam(uint16_t addr, uint8_t value);

	/// <summary>
	/// Advances the GSU emulation by specified cycles.
	/// </summary>
	/// <param name="cycles">Number of master clock cycles to execute.</param>
	void Step(uint64_t cycles);

	// ===== GSU Instructions =====
	// Control instructions

	/// <summary>STOP - Halts GSU execution and optionally triggers IRQ.</summary>
	void STOP();

	/// <summary>NOP - No operation, just advances PC.</summary>
	void NOP();

	/// <summary>CACHE - Initializes instruction cache at current PC.</summary>
	void CACHE();

	/// <summary>
	/// Generic branch instruction handler.
	/// </summary>
	/// <param name="branch">True if branch should be taken.</param>
	void Branch(bool branch);

	// Branch instructions
	/// <summary>BRA - Branch always (unconditional).</summary>
	void BRA();

	/// <summary>BLT - Branch if less than (Sign != Overflow).</summary>
	void BLT();

	/// <summary>BGE - Branch if greater or equal (Sign == Overflow).</summary>
	void BGE();

	/// <summary>BNE - Branch if not equal (Zero == 0).</summary>
	void BNE();

	/// <summary>BEQ - Branch if equal (Zero == 1).</summary>
	void BEQ();

	/// <summary>BPL - Branch if plus (Sign == 0).</summary>
	void BPL();

	/// <summary>BMI - Branch if minus (Sign == 1).</summary>
	void BMI();

	/// <summary>BCC - Branch if carry clear (Carry == 0).</summary>
	void BCC();

	/// <summary>BCS - Branch if carry set (Carry == 1).</summary>
	void BCS();

	/// <summary>BVC - Branch if overflow clear (Overflow == 0).</summary>
	void BVC();

	/// <summary>BVS - Branch if overflow set (Overflow == 1).</summary>
	void BVS();

	/// <summary>
	/// JMP - Jump to address in register.
	/// </summary>
	/// <param name="reg">Register containing jump target.</param>
	void JMP(uint8_t reg);

	// Register prefix instructions
	/// <summary>
	/// TO - Sets destination register for next instruction.
	/// </summary>
	/// <param name="reg">Destination register index.</param>
	void TO(uint8_t reg);

	/// <summary>
	/// FROM - Sets source register for next instruction.
	/// </summary>
	/// <param name="reg">Source register index.</param>
	void FROM(uint8_t reg);

	/// <summary>
	/// WITH - Sets both source and destination register for next instruction.
	/// </summary>
	/// <param name="reg">Register index for source and destination.</param>
	void WITH(uint8_t reg);

	// Memory instructions
	/// <summary>
	/// STORE - Stores register value to RAM.
	/// </summary>
	/// <param name="reg">Register containing address.</param>
	void STORE(uint8_t reg);

	/// <summary>
	/// LOAD - Loads value from RAM into register.
	/// </summary>
	/// <param name="reg">Register containing address.</param>
	void LOAD(uint8_t reg);

	// ALT mode instructions
	/// <summary>LOOP - Decrements R12 and branches if non-zero.</summary>
	void LOOP();

	/// <summary>ALT1 - Sets ALT1 prefix flag.</summary>
	void ALT1();

	/// <summary>ALT2 - Sets ALT2 prefix flag.</summary>
	void ALT2();

	/// <summary>ALT3 - Sets both ALT1 and ALT2 prefix flags.</summary>
	void ALT3();

	// Data manipulation instructions
	/// <summary>MERGE - Merges high bytes of R7 and R8 into destination.</summary>
	void MERGE();

	/// <summary>SWAP - Swaps high and low bytes of source register.</summary>
	void SWAP();

	// Graphics instructions
	/// <summary>PLOT/RPIX - Plots pixel or reads pixel depending on ALT mode.</summary>
	void PlotRpix();

	/// <summary>COLOR/CMODE - Sets color register or color mode.</summary>
	void ColorCMode();

	/// <summary>
	/// Gets tile index for given screen coordinates.
	/// </summary>
	/// <param name="x">X screen coordinate.</param>
	/// <param name="y">Y screen coordinate.</param>
	/// <returns>Tile index in tilemap.</returns>
	uint16_t GetTileIndex(uint8_t x, uint8_t y);

	/// <summary>
	/// Gets RAM address for tile at given coordinates.
	/// </summary>
	/// <param name="x">X screen coordinate.</param>
	/// <param name="y">Y screen coordinate.</param>
	/// <returns>RAM address of tile data.</returns>
	uint32_t GetTileAddress(uint8_t x, uint8_t y);

	/// <summary>
	/// Reads pixel value at given coordinates.
	/// </summary>
	/// <param name="x">X screen coordinate.</param>
	/// <param name="y">Y screen coordinate.</param>
	/// <returns>Pixel color value.</returns>
	uint8_t ReadPixel(uint8_t x, uint8_t y);

	/// <summary>Checks if current pixel is transparent (color 0).</summary>
	/// <returns>True if pixel is transparent.</returns>
	bool IsTransparentPixel();

	/// <summary>
	/// Draws a pixel at given coordinates using current color.
	/// </summary>
	/// <param name="x">X screen coordinate.</param>
	/// <param name="y">Y screen coordinate.</param>
	void DrawPixel(uint8_t x, uint8_t y);

	/// <summary>
	/// Flushes primary pixel cache to RAM for coordinates.
	/// </summary>
	/// <param name="x">X coordinate of cache.</param>
	/// <param name="y">Y coordinate of cache.</param>
	void FlushPrimaryCache(uint8_t x, uint8_t y);

	/// <summary>
	/// Writes pixel cache contents to RAM.
	/// </summary>
	/// <param name="cache">Pixel cache to write.</param>
	void WritePixelCache(GsuPixelCache& cache);

	/// <summary>
	/// Gets color value with gradient/dither applied.
	/// </summary>
	/// <param name="source">Base color value.</param>
	/// <returns>Final color with effects applied.</returns>
	uint8_t GetColor(uint8_t source);

	// Arithmetic instructions
	/// <summary>
	/// ADD/ADC/ADI - Adds register value to destination.
	/// </summary>
	/// <param name="reg">Register to add or immediate value.</param>
	void Add(uint8_t reg);

	/// <summary>
	/// SUB/SBC/CMP - Subtracts or compares register values.
	/// </summary>
	/// <param name="reg">Register for subtraction/comparison.</param>
	void SubCompare(uint8_t reg);

	/// <summary>
	/// MULT/UMULT - Multiplies register values (signed or unsigned).
	/// </summary>
	/// <param name="reg">Register to multiply with.</param>
	void MULT(uint8_t reg);

	/// <summary>FMULT/LMULT - Fractional or long multiplication.</summary>
	void FMultLMult();

	// Logical instructions
	/// <summary>
	/// AND/BIC - Logical AND or bit clear operation.
	/// </summary>
	/// <param name="reg">Register for AND/BIC operation.</param>
	void AndBitClear(uint8_t reg);

	/// <summary>SBK - Store low byte to last RAM address.</summary>
	void SBK();

	/// <summary>
	/// LINK - Sets register to return address (PC + parameter).
	/// </summary>
	/// <param name="reg">Offset to add to PC for link address.</param>
	void LINK(uint8_t reg);

	/// <summary>SEX - Sign extends low byte to 16 bits.</summary>
	void SignExtend();

	/// <summary>NOT - Bitwise NOT of source register.</summary>
	void NOT();

	/// <summary>LSR - Logical shift right by one bit.</summary>
	void LSR();

	/// <summary>ROL - Rotate left through carry by one bit.</summary>
	void ROL();

	/// <summary>ASR - Arithmetic shift right by one bit (preserves sign).</summary>
	void ASR();

	/// <summary>ROR - Rotate right through carry by one bit.</summary>
	void ROR();

	/// <summary>LOB - Extracts low byte of source register.</summary>
	void LOB();

	/// <summary>HIB - Extracts high byte of source register.</summary>
	void HIB();

	/// <summary>
	/// IBT/SMS/LMS - Immediate byte transfer or short memory access.
	/// </summary>
	/// <param name="reg">Target register.</param>
	void IbtSmsLms(uint8_t reg);

	/// <summary>
	/// IWT/LM/SM - Immediate word transfer or memory access.
	/// </summary>
	/// <param name="reg">Target register.</param>
	void IwtLmSm(uint8_t reg);

	/// <summary>
	/// OR/XOR - Logical OR or XOR operation.
	/// </summary>
	/// <param name="reg">Register for operation.</param>
	void OrXor(uint8_t reg);

	/// <summary>
	/// INC - Increments register value.
	/// </summary>
	/// <param name="reg">Register to increment.</param>
	void INC(uint8_t reg);

	/// <summary>
	/// DEC - Decrements register value.
	/// </summary>
	/// <param name="reg">Register to decrement.</param>
	void DEC(uint8_t reg);

	/// <summary>GETC/RAMB/ROMB - Gets color from ROM or sets bank register.</summary>
	void GetCRamBRomB();

	/// <summary>GETB/GETBH/GETBL - Gets byte from ROM buffer.</summary>
	void GETB();

public:
	/// <summary>
	/// Creates a new GSU (Super FX) coprocessor instance.
	/// </summary>
	/// <param name="console">SNES console instance.</param>
	/// <param name="gsuRamSize">Size of GSU RAM in bytes (typically 32KB or 64KB).</param>
	Gsu(SnesConsole* console, uint32_t gsuRamSize);

	/// <summary>Destructor - cleans up GSU resources.</summary>
	virtual ~Gsu();

	/// <summary>Called at end of each frame for frame-based processing.</summary>
	void ProcessEndOfFrame() override;

	/// <summary>
	/// Reads from GSU register space.
	/// </summary>
	/// <param name="addr">Address to read from.</param>
	/// <param name="opType">Type of memory operation.</param>
	/// <returns>Register value.</returns>
	uint8_t ReadGsu(uint32_t addr, MemoryOperationType opType);

	/// <summary>
	/// Writes to GSU register space.
	/// </summary>
	/// <param name="addr">Address to write to.</param>
	/// <param name="value">Value to write.</param>
	/// <param name="opType">Type of memory operation.</param>
	void WriteGsu(uint32_t addr, uint8_t value, MemoryOperationType opType);

	/// <summary>Loads battery-backed SRAM from file.</summary>
	void LoadBattery() override;

	/// <summary>Saves battery-backed SRAM to file.</summary>
	void SaveBattery() override;

	/// <summary>Main execution loop - runs GSU until stopped.</summary>
	void Run() override;

	/// <summary>Resets the GSU to initial power-on state.</summary>
	void Reset() override;

	/// <summary>
	/// Reads a byte from GSU memory (CPU side access).
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Data byte.</returns>
	uint8_t Read(uint32_t addr) override;

	/// <summary>
	/// Peeks a byte from GSU memory without side effects.
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Data byte.</returns>
	uint8_t Peek(uint32_t addr) override;

	/// <summary>
	/// Peeks a block of memory from GSU address space.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer to receive data.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override;

	/// <summary>
	/// Writes a byte to GSU memory (CPU side access).
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Data byte to write.</param>
	void Write(uint32_t addr, uint8_t value) override;

	/// <summary>
	/// Gets absolute address info for debugging.
	/// </summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address information.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t address) override;

	/// <summary>
	/// Serializes GSU state for save states.
	/// </summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override;

	/// <summary>Gets reference to current GSU state.</summary>
	/// <returns>GSU state structure.</returns>
	GsuState& GetState();

	/// <summary>Gets the GSU memory mappings.</summary>
	/// <returns>Pointer to memory mappings.</returns>
	MemoryMappings* GetMemoryMappings();

	/// <summary>Gets current program counter for debugging.</summary>
	/// <returns>24-bit address (bank:offset).</returns>
	uint32_t DebugGetProgramCounter();

	/// <summary>
	/// Sets program counter for debugging.
	/// </summary>
	/// <param name="addr">New program counter value.</param>
	void DebugSetProgramCounter(uint32_t addr);
};