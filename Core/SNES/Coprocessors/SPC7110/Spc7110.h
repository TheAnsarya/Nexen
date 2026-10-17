#pragma once
#include "pch.h"
#include "SNES/Coprocessors/BaseCoprocessor.h"
#include "SNES/Coprocessors/SPC7110/Spc7110Decomp.h"
#include "SNES/Coprocessors/SPC7110/Rtc4513.h"

class SnesConsole;
class Emulator;
class Spc7110Decomp;
class IMemoryHandler;
class BaseCartridge;

/// <summary>
/// SPC7110 coprocessor emulation (data compression and memory mapping chip).
/// Used by Far East of Eden Zero, Momotarou Dentetsu Happy, Super Power League 4.
/// Provides data decompression, ALU operations, and enhanced memory banking.
/// </summary>
/// <remarks>
/// The SPC7110 is a powerful coprocessor that provides:
/// - Real-time data decompression (context-based probabilistic encoding)
/// - Hardware multiply/divide operations (32x16, 32/16)
/// - Large ROM addressing (up to 32Mbit data ROM + 8Mbit program ROM)
/// - Optional RTC (Real-Time Clock) via Epson RTC-4513 chip
/// 
/// Memory regions:
/// - Program ROM: Standard SNES mapping
/// - Data ROM: Accessed via $4800 after configuring pointers
/// - SRAM: Battery-backed save RAM
/// 
/// Key registers:
/// - $4800: Data port (reads decompressed or raw data)
/// - $4801-$4807: Data ROM pointer and control
/// - $4808-$480B: Decompression setup
/// - $4820-$482F: ALU registers
/// - $4830-$4834: Memory mapping
/// </remarks>
class Spc7110 : public BaseCoprocessor {
private:
	/// <summary>Hardware decompression engine.</summary>
	unique_ptr<Spc7110Decomp> _decomp;

	/// <summary>Real-time clock (optional).</summary>
	unique_ptr<Rtc4513> _rtc;

	/// <summary>Handler for CPU register access ($2180-$2183).</summary>
	IMemoryHandler* _cpuRegisterHandler = nullptr;

	/// <summary>Reference to emulator.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Reference to SNES console.</summary>
	SnesConsole* _console = nullptr;

	/// <summary>Reference to cartridge.</summary>
	BaseCartridge* _cart = nullptr;

	/// <summary>True if cartridge has RTC chip.</summary>
	bool _useRtc = false;

	/// <summary>Actual data ROM size (may differ from declared size).</summary>
	uint32_t _realDataRomSize = 0;

	// ==================== Decompression State ====================

	/// <summary>Base address for decompression directory.</summary>
	uint32_t _directoryBase = 0;

	/// <summary>Current directory entry index.</summary>
	uint8_t _directoryIndex = 0;

	/// <summary>Target offset within compressed data.</summary>
	uint16_t _targetOffset = 0;

	/// <summary>Remaining bytes to output.</summary>
	uint16_t _dataLengthCounter = 0;

	/// <summary>Bytes to skip in decompressed output.</summary>
	uint8_t _skipBytes = 0;

	/// <summary>Decompression mode flags.</summary>
	uint8_t _decompFlags = 0;

	/// <summary>Current decompression mode (1/2/4 bpp).</summary>
	uint8_t _decompMode = 0;

	/// <summary>Source address in data ROM.</summary>
	uint32_t _srcAddress = 0;

	/// <summary>Current offset in decompression stream.</summary>
	uint32_t _decompOffset = 0;

	/// <summary>Decompression status flags.</summary>
	uint8_t _decompStatus = 0;

	/// <summary>Output buffer for decompressed data.</summary>
	uint8_t _decompBuffer[32];

	// ==================== ALU State ====================

	/// <summary>32-bit dividend/multiplicand for ALU operations.</summary>
	uint32_t _dividend = 0;

	/// <summary>16-bit multiplier.</summary>
	uint16_t _multiplier = 0;

	/// <summary>16-bit divisor.</summary>
	uint16_t _divisor = 0;

	/// <summary>32-bit multiplication/division result.</summary>
	uint32_t _multDivResult = 0;

	/// <summary>16-bit division remainder.</summary>
	uint16_t _remainder = 0;

	/// <summary>ALU operation state machine position.</summary>
	uint8_t _aluState = 0;

	/// <summary>ALU control flags (signed mode, etc.).</summary>
	uint8_t _aluFlags = 0;

	// ==================== Memory Mapping State ====================

	/// <summary>SRAM enable flags.</summary>
	uint8_t _sramEnabled = 0;

	/// <summary>Data ROM bank mapping registers.</summary>
	uint8_t _dataRomBanks[3] = {0, 1, 2};

	/// <summary>Data ROM size configuration.</summary>
	uint8_t _dataRomSize = 0;

	// ==================== Data ROM Read State ====================

	/// <summary>Base address for sequential reads.</summary>
	uint32_t _readBase = 0;

	/// <summary>Current read offset from base.</summary>
	uint16_t _readOffset = 0;

	/// <summary>Address increment per read.</summary>
	uint16_t _readStep = 0;

	/// <summary>Read mode (auto-increment, etc.).</summary>
	uint8_t _readMode = 0;

	/// <summary>Prefetch buffer for data reads.</summary>
	uint8_t _readBuffer = 0;

	/// <summary>Executes pending multiplication operation.</summary>
	void ProcessMultiplication();

	/// <summary>Executes pending division operation.</summary>
	void ProcessDivision();

	/// <summary>Fills read buffer from data ROM.</summary>
	void FillReadBuffer();

	/// <summary>Increments read position by step value.</summary>
	void IncrementPosition();

	/// <summary>Increments position for $4810 reads.</summary>
	void IncrementPosition4810();

	/// <summary>Loads header from compression directory.</summary>
	void LoadEntryHeader();

	/// <summary>Begins decompression of selected entry.</summary>
	void BeginDecompression();

	/// <summary>
	/// Reads next decompressed byte from stream.
	/// </summary>
	/// <returns>Decompressed data byte.</returns>
	uint8_t ReadDecompressedByte();

public:
	/// <summary>
	/// Creates a new SPC7110 coprocessor instance.
	/// </summary>
	/// <param name="console">Reference to SNES console.</param>
	/// <param name="useRtc">True if cartridge has RTC chip.</param>
	Spc7110(SnesConsole* console, bool useRtc);

	/// <summary>
	/// Reads directly from data ROM (for decompressor).
	/// </summary>
	/// <param name="addr">Data ROM address.</param>
	/// <returns>Data byte.</returns>
	uint8_t ReadDataRom(uint32_t addr);

	/// <summary>Serializes SPC7110 state for save states.</summary>
	void Serialize(Serializer& s) override;

	/// <summary>
	/// Reads from SPC7110 registers or mapped memory.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Data byte.</returns>
	uint8_t Read(uint32_t addr) override;

	/// <summary>
	/// Peeks SPC7110 without side effects.
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Data byte.</returns>
	uint8_t Peek(uint32_t addr) override;

	/// <summary>
	/// Peeks a block of data for debugging.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer for data.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override;

	/// <summary>
	/// Writes to SPC7110 registers.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Data byte to write.</param>
	void Write(uint32_t addr, uint8_t value) override;

	/// <summary>Updates memory mappings after register changes.</summary>
	void UpdateMappings();

	/// <summary>
	/// Gets absolute address for debugging.
	/// </summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address info.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t address) override;

	/// <summary>Resets SPC7110 to power-on state.</summary>
	void Reset() override;

	/// <summary>Loads battery-backed data (SRAM and RTC).</summary>
	void LoadBattery() override;

	/// <summary>Saves battery-backed data (SRAM and RTC).</summary>
	void SaveBattery() override;
};