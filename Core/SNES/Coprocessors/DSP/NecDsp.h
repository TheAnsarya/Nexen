#pragma once
#include "pch.h"
#include <memory>
#include "SNES/Coprocessors/DSP/NecDspTypes.h"
#include "SNES/Coprocessors/BaseCoprocessor.h"

class SnesConsole;
class Emulator;
class SnesMemoryManager;
class RamHandler;
enum class CoprocessorType;

/// <summary>
/// NEC µPD77C25 / µPD96050 DSP coprocessor emulation.
/// Implements DSP-1, DSP-1A, DSP-1B, DSP-2, DSP-3, DSP-4, and ST010/ST011 chips.
/// </summary>
/// <remarks>
/// **Overview:**
/// The NEC DSP chips are programmable signal processors used for various
/// mathematical operations that would be too slow for the main CPU.
///
/// **DSP Variants:**
/// - **DSP-1/1A/1B**: 3D projection, rotation matrices (Pilotwings, Super Mario Kart)
/// - **DSP-2**: Bitmap scaling/rotation (Dungeon Master)
/// - **DSP-3**: 2D vector math (SD Gundam GX)
/// - **DSP-4**: Top-down racing math (Top Gear 3000)
/// - **ST010**: AI pathfinding (Hayazashi Nidan Morita Shogi)
/// - **ST011**: AI pathfinding (Hayazashi Nidan Morita Shogi 2)
///
/// **Architecture:**
/// - 16-bit data path, 22-bit instructions
/// - 7.6 MHz typical clock (varies by variant)
/// - Hardware multiply/accumulate (ALU)
/// - Program ROM: 2KB-32KB depending on variant
/// - Data ROM: 1KB-4KB for lookup tables
/// - Working RAM: 256 bytes - 2KB
///
/// **Communication Protocol:**
/// - DR (Data Register): Main CPU writes commands/data
/// - SR (Status Register): Read by CPU to check busy/ready
/// - RQM flag: Request to Master - DSP ready for data
/// - DRC flag: Data Register Control - direction (read/write)
///
/// **Operations:** Matrix math, trigonometry, perspective projection,
/// coordinate transformation, pathfinding, bitmap manipulation.
/// </remarks>
class NecDsp final : public BaseCoprocessor {
public:
	/// <summary>Flag indicating a data ROM read operation for debugger.</summary>
	static constexpr uint32_t DataRomReadFlag = 0x80000000;

private:
	/// <summary>SNES console reference.</summary>
	SnesConsole* _console = nullptr;

	/// <summary>Emulator instance reference.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Memory manager reference.</summary>
	SnesMemoryManager* _memoryManager = nullptr;

	/// <summary>DSP processor state (registers, flags, pointers).</summary>
	NecDspState _state = {};

	/// <summary>Handler for DSP RAM access.</summary>
	unique_ptr<RamHandler> _ramHandler;

	/// <summary>Specific DSP type (DSP-1, DSP-2, etc.).</summary>
	CoprocessorType _type;

	/// <summary>DSP clock frequency in Hz (typically 7.6 MHz).</summary>
	double _frequency = 7600000;

	/// <summary>Current opcode being executed.</summary>
	uint32_t _opCode = 0;

	/// <summary>Program ROM buffer (DSP microcode).</summary>
	std::unique_ptr<uint8_t[]> _progRom;

	/// <summary>Pre-decoded program cache for faster execution.</summary>
	std::unique_ptr<uint32_t[]> _prgCache;

	/// <summary>Data ROM buffer (lookup tables, coefficients).</summary>
	std::unique_ptr<uint16_t[]> _dataRom;

	/// <summary>Working RAM buffer.</summary>
	std::unique_ptr<uint16_t[]> _ram;

	/// <summary>Hardware stack for subroutine calls.</summary>
	uint16_t _stack[16];

	/// <summary>Program ROM size in bytes.</summary>
	uint32_t _progSize = 0;

	/// <summary>Data ROM size in words.</summary>
	uint32_t _dataSize = 0;

	/// <summary>RAM size in words.</summary>
	uint32_t _ramSize = 0;

	/// <summary>Stack depth (entries).</summary>
	uint32_t _stackSize = 0;

	/// <summary>Program ROM address mask for wrapping.</summary>
	uint32_t _progMask = 0;

	/// <summary>Data ROM address mask for wrapping.</summary>
	uint32_t _dataMask = 0;

	/// <summary>RAM address mask for wrapping.</summary>
	uint32_t _ramMask = 0;

	/// <summary>Stack pointer mask for wrapping.</summary>
	uint32_t _stackMask = 0;

	/// <summary>Register address mask.</summary>
	uint16_t _registerMask = 0;

	/// <summary>Flag indicating DSP is in RQM wait loop.</summary>
	bool _inRqmLoop = false;

	/// <summary>Fetches the next opcode from program ROM.</summary>
	void ReadOpCode();

	/// <summary>
	/// Executes an ALU operation.
	/// </summary>
	/// <param name="aluOperation">ALU operation code.</param>
	/// <param name="source">Source operand value.</param>
	void RunApuOp(uint8_t aluOperation, uint16_t source);

	/// <summary>Updates the data pointer based on current mode.</summary>
	void UpdateDataPointer();

	/// <summary>Executes a single DSP instruction.</summary>
	void ExecOp();

	/// <summary>Executes instruction and handles return.</summary>
	void ExecAndReturn();

	/// <summary>Performs a jump/branch operation.</summary>
	void Jump();

	/// <summary>
	/// Loads a value into a destination register.
	/// </summary>
	/// <param name="dest">Destination register index.</param>
	/// <param name="value">Value to load.</param>
	void Load(uint8_t dest, uint16_t value);

	/// <summary>
	/// Gets the value from a source operand.
	/// </summary>
	/// <param name="source">Source operand index.</param>
	/// <returns>Source value.</returns>
	uint16_t GetSourceValue(uint8_t source);

	/// <summary>
	/// Reads from program/data ROM.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>ROM value.</returns>
	uint16_t ReadRom(uint32_t addr);

	/// <summary>
	/// Reads from working RAM.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>RAM value.</returns>
	uint16_t ReadRam(uint32_t addr);

	/// <summary>
	/// Writes to working RAM.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Value to write.</param>
	void WriteRam(uint32_t addr, uint16_t value);

	/// <summary>
	/// Private constructor - use InitCoprocessor factory method.
	/// </summary>
	/// <param name="type">DSP variant type.</param>
	/// <param name="console">SNES console reference.</param>
	/// <param name="programRom">Program ROM data.</param>
	/// <param name="dataRom">Data ROM data.</param>
	NecDsp(CoprocessorType type, SnesConsole* console, vector<uint8_t>& programRom, vector<uint8_t>& dataRom);

public:
	/// <summary>Destructor.</summary>
	virtual ~NecDsp();

	/// <summary>
	/// Factory method to create and initialize a DSP coprocessor.
	/// </summary>
	/// <param name="type">DSP variant type.</param>
	/// <param name="console">SNES console reference.</param>
	/// <param name="embeddedFirmware">Firmware data (may be empty for external).</param>
	/// <returns>Initialized DSP instance, or nullptr on failure.</returns>
	static NecDsp* InitCoprocessor(CoprocessorType type, SnesConsole* console, vector<uint8_t>& embeddedFirmware);

	/// <summary>Resets DSP to initial state.</summary>
	void Reset() override;

	/// <summary>Runs DSP until synchronization point.</summary>
	void Run() override;

	/// <summary>Loads battery-backed save data.</summary>
	void LoadBattery() override;

	/// <summary>Saves battery-backed data to file.</summary>
	void SaveBattery() override;

	/// <summary>Pre-decodes program ROM for faster execution.</summary>
	void BuildProgramCache();

	/// <summary>
	/// Reads from DSP I/O registers.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <returns>Register value.</returns>
	uint8_t Read(uint32_t addr) override;

	/// <summary>
	/// Writes to DSP I/O registers.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void Write(uint32_t addr, uint8_t value) override;

	/// <summary>
	/// Gets the opcode at the specified address for debugging.
	/// </summary>
	/// <param name="addr">Program address.</param>
	/// <returns>22-bit opcode value.</returns>
	uint32_t GetOpCode(uint32_t addr);

	/// <summary>Peeks at DSP register without side effects.</summary>
	/// <param name="addr">Register address.</param>
	/// <returns>Register value.</returns>
	uint8_t Peek(uint32_t addr) override;

	/// <summary>Peeks at block of DSP memory.</summary>
	/// <param name="addr">Start address.</param>
	/// <param name="output">Output buffer.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override;

	/// <summary>Gets absolute address info for debugging.</summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address information.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t address) override;

	/// <summary>Gets pointer to program ROM for debugging.</summary>
	uint8_t* DebugGetProgramRom();

	/// <summary>Gets pointer to data ROM for debugging.</summary>
	uint8_t* DebugGetDataRom();

	/// <summary>Gets pointer to working RAM for debugging.</summary>
	uint8_t* DebugGetDataRam();

	/// <summary>Gets program ROM size in bytes.</summary>
	uint32_t DebugGetProgramRomSize();

	/// <summary>Gets data ROM size in bytes.</summary>
	uint32_t DebugGetDataRomSize();

	/// <summary>Gets RAM size in bytes.</summary>
	uint32_t DebugGetDataRamSize();

	/// <summary>Gets reference to DSP state for debugging.</summary>
	NecDspState& GetState();

	/// <summary>Serializes DSP state for save states.</summary>
	/// <param name="s">Serializer for reading/writing state.</param>
	void Serialize(Serializer& s) override;
};