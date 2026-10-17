#pragma once
#include "pch.h"
#include <memory>
#include "SNES/Coprocessors/ST018/St018Types.h"
#include "SNES/Coprocessors/ST018/ArmV3Types.h"
#include "SNES/Coprocessors/ST018/ArmV3Cpu.h"
#include "SNES/Coprocessors/BaseCoprocessor.h"

class SnesConsole;
class Emulator;
class ArmV3Cpu;
class SnesMemoryManager;

/// <summary>
/// ST018 coprocessor emulation.
/// The ST018 is a Seta DSP chip containing an ARM v3 CPU core used for
/// AI calculations in the Japanese shogi (chess) game Hayazashi Nidan Morita Shogi 2.
/// Features:
/// - ARM v3 RISC CPU running at 21.47 MHz
/// - 128KB program ROM, 32KB data ROM
/// - 16KB work RAM (battery-backed for save data)
/// - Bidirectional communication via data registers
/// </summary>
/// <remarks>
/// The ST018 is unique among SNES coprocessors as it uses a 32-bit ARM CPU
/// rather than a custom DSP or 8/16-bit processor. The ARM executes shogi
/// AI algorithms while the SNES handles graphics and user interface.
/// </remarks>
class St018 final : public BaseCoprocessor {
private:
	/// <summary>Size of ARM program ROM (128KB).</summary>
	static constexpr int PrgRomSize = 0x20000;

	/// <summary>Size of ARM data ROM (32KB).</summary>
	static constexpr int DataRomSize = 0x8000;

	/// <summary>Size of ARM work RAM (16KB).</summary>
	static constexpr int WorkRamSize = 0x4000;

	/// <summary>Pointer to SNES console.</summary>
	SnesConsole* _console = nullptr;

	/// <summary>Pointer to main emulator instance.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Pointer to SNES memory manager.</summary>
	SnesMemoryManager* _memoryManager = nullptr;

	/// <summary>ST018 communication state.</summary>
	St018State _state = {};

	/// <summary>ARM v3 CPU instance.</summary>
	unique_ptr<ArmV3Cpu> _cpu;

	/// <summary>ARM program ROM buffer.</summary>
	std::unique_ptr<uint8_t[]> _prgRom;

	/// <summary>ARM data ROM buffer.</summary>
	std::unique_ptr<uint8_t[]> _dataRom;

	/// <summary>ARM work RAM buffer (battery-backed).</summary>
	std::unique_ptr<uint8_t[]> _workRam;

	/// <summary>
	/// Reads a byte from ARM address space.
	/// </summary>
	/// <param name="addr">ARM address.</param>
	/// <returns>Data byte.</returns>
	__forceinline uint8_t ReadCpuByte(uint32_t addr);

	/// <summary>
	/// Writes a byte to ARM address space.
	/// </summary>
	/// <param name="addr">ARM address.</param>
	/// <param name="value">Data byte.</param>
	__forceinline void WriteCpuByte(uint32_t addr, uint8_t value);

	/// <summary>Gets the current ST018 status register value.</summary>
	/// <returns>Status byte indicating data ready flags.</returns>
	uint8_t GetStatus();

public:
	/// <summary>
	/// Creates a new ST018 coprocessor instance.
	/// </summary>
	/// <param name="console">SNES console instance.</param>
	St018(SnesConsole* console);

	/// <summary>Destructor - cleans up ARM CPU and memory.</summary>
	virtual ~St018();

	/// <summary>Resets ST018 to initial power-on state.</summary>
	void Reset() override;

	/// <summary>Main execution loop - runs ARM CPU.</summary>
	void Run() override;

	/// <summary>Called at end of frame for frame-based processing.</summary>
	void ProcessEndOfFrame() override;

	/// <summary>Loads battery-backed RAM from file.</summary>
	void LoadBattery() override;

	/// <summary>Saves battery-backed RAM to file.</summary>
	void SaveBattery() override;

	/// <summary>Gets pointer to ARM CPU for debugging.</summary>
	/// <returns>Pointer to ARM CPU instance.</returns>
	ArmV3Cpu* GetCpu() { return _cpu.get(); }

	/// <summary>Gets reference to ST018 communication state.</summary>
	/// <returns>ST018 state structure.</returns>
	St018State& GetState();

	/// <summary>
	/// ARM CPU memory read callback.
	/// </summary>
	/// <param name="mode">Access mode flags.</param>
	/// <param name="addr">ARM address.</param>
	/// <returns>Data (byte or word based on mode).</returns>
	uint32_t ReadCpu(ArmV3AccessModeVal mode, uint32_t addr);

	/// <summary>
	/// ARM CPU debug read (no side effects).
	/// </summary>
	/// <param name="mode">Access mode flags.</param>
	/// <param name="addr">ARM address.</param>
	/// <returns>Data value.</returns>
	uint32_t DebugCpuRead(ArmV3AccessModeVal mode, uint32_t addr);

	/// <summary>
	/// ARM CPU memory write callback.
	/// </summary>
	/// <param name="mode">Access mode flags.</param>
	/// <param name="addr">ARM address.</param>
	/// <param name="value">Data to write.</param>
	void WriteCpu(ArmV3AccessModeVal mode, uint32_t addr, uint32_t value);

	/// <summary>Processes an ARM idle cycle for timing.</summary>
	void ProcessIdleCycle();

	/// <summary>
	/// Debug read from SNES address space.
	/// </summary>
	/// <param name="addr">SNES address.</param>
	/// <returns>Data byte.</returns>
	uint8_t DebugRead(uint32_t addr);

	/// <summary>
	/// Debug write to SNES address space.
	/// </summary>
	/// <param name="addr">SNES address.</param>
	/// <param name="value">Data byte.</param>
	void DebugWrite(uint32_t addr, uint8_t value);

	/// <summary>
	/// Reads from ST018 register space.
	/// </summary>
	/// <param name="addr">SNES address.</param>
	/// <returns>Register value.</returns>
	uint8_t Read(uint32_t addr) override;

	/// <summary>
	/// Writes to ST018 register space.
	/// </summary>
	/// <param name="addr">SNES address.</param>
	/// <param name="value">Value to write.</param>
	void Write(uint32_t addr, uint8_t value) override;

	/// <summary>
	/// Peeks ST018 memory without side effects.
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Data value.</returns>
	uint8_t Peek(uint32_t addr) override;

	/// <summary>
	/// Peeks a block of ST018 memory.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer for data.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override;

	/// <summary>
	/// Gets absolute address for debugging (SNES side).
	/// </summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address info.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t address) override;

	/// <summary>
	/// Gets absolute address for debugging (ARM side).
	/// </summary>
	/// <param name="address">ARM address.</param>
	/// <returns>Absolute address info.</returns>
	AddressInfo GetArmAbsoluteAddress(uint32_t address);

	/// <summary>
	/// Gets ARM-relative address from absolute address.
	/// </summary>
	/// <param name="absoluteAddr">Absolute address info.</param>
	/// <returns>ARM relative address or -1 if not in ARM space.</returns>
	int GetArmRelativeAddress(AddressInfo& absoluteAddr);

	/// <summary>
	/// Serializes ST018 state for save states.
	/// </summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override;
};