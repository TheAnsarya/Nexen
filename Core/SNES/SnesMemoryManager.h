#pragma once
#include "pch.h"
#include <memory>
#include "SNES/MemoryMappings.h"
#include "Debugger/DebugTypes.h"
#include "Utilities/ISerializable.h"
#include "Shared/MemoryType.h"

class IMemoryHandler;
class RegisterHandlerA;
class RegisterHandlerB;
class InternalRegisters;
class RamHandler;
class BaseCartridge;
class Emulator;
class SnesConsole;
class SnesPpu;
class SnesCpu;
class CheatManager;
enum class MemoryOperationType;

/// <summary>Types of scheduled SNES memory events.</summary>
enum class SnesEventType : uint8_t {
	HdmaInit,       ///< HDMA initialization at start of frame
	DramRefresh,    ///< DRAM refresh cycle (steals CPU cycles)
	HdmaStart,      ///< HDMA transfer during H-blank
	EndOfScanline   ///< End of current scanline
};

/// <summary>
/// SNES Memory Manager - handles all bus access and timing.
/// Manages memory mappings, DMA/HDMA, and CPU bus interactions.
/// </summary>
/// <remarks>
/// **Memory Map (24-bit address space):**
/// - Banks $00-$3F, $80-$BF:
///   - $0000-$1FFF: Low RAM (mirror)
///   - $2000-$20FF: Unused
///   - $2100-$213F: PPU registers (Bus B)
///   - $2140-$217F: APU registers (Bus B)
///   - $2180-$2183: WRAM access (Bus B)
///   - $4000-$40FF: Joypad registers (Bus A)
///   - $4200-$44FF: CPU registers (Bus A)
///   - $8000-$FFFF: ROM (LoROM) or varies (HiROM)
/// - Banks $7E-$7F: Work RAM (128KB)
/// - Banks $40-$7D, $C0-$FF: Cartridge ROM/RAM/Expansion
///
/// **Bus Architecture:**
/// - Bus A: CPU address bus (registers $4000-$44FF)
/// - Bus B: Lower byte only ($2100-$21FF - PPU, APU, WRAM)
///
/// **DMA/HDMA:**
/// - 8 DMA channels (0-7)
/// - Channels 0-3 typically for general DMA
/// - Channels 4-7 typically for HDMA
/// - HDMA runs during H-blank automatically
///
/// **Timing:**
/// - 21.477 MHz master clock
/// - CPU at 3.58 MHz (slow) or 2.68 MHz (fast)
/// - DRAM refresh steals ~40 cycles per scanline
/// - Open bus behavior on unmapped addresses
///
/// **Speed Modes:**
/// - FastROM: 6 master clocks per CPU cycle
/// - SlowROM: 8 master clocks per CPU cycle
/// - XSlow: 12 master clocks (registers)
/// </remarks>
class SnesMemoryManager : public ISerializable {
public:
	/// <summary>Work RAM size (128KB).</summary>
	constexpr static uint32_t WorkRamSize = 0x20000;

private:
	/// <summary>Console instance.</summary>
	SnesConsole* _console = nullptr;

	/// <summary>Emulator instance.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Bus A register handler.</summary>
	unique_ptr<RegisterHandlerA> _registerHandlerA;

	/// <summary>Bus B register handler.</summary>
	unique_ptr<RegisterHandlerB> _registerHandlerB;

	/// <summary>Internal CPU registers.</summary>
	InternalRegisters* _regs = nullptr;

	/// <summary>PPU instance.</summary>
	SnesPpu* _ppu = nullptr;

	/// <summary>CPU instance.</summary>
	SnesCpu* _cpu = nullptr;

	/// <summary>Cartridge handler.</summary>
	BaseCartridge* _cart = nullptr;

	/// <summary>Cheat manager.</summary>
	CheatManager* _cheatManager = nullptr;

	/// <summary>Work RAM (128KB).</summary>
	std::unique_ptr<uint8_t[]> _workRam;

	/// <summary>Master clock counter.</summary>
	uint64_t _masterClock = 0;

	/// <summary>Horizontal clock position.</summary>
	uint16_t _hClock = 0;

	/// <summary>Next event clock position.</summary>
	uint16_t _nextEventClock = 0;

	/// <summary>DRAM refresh position.</summary>
	uint16_t _dramRefreshPosition = 0;

	/// <summary>Next scheduled event type.</summary>
	SnesEventType _nextEvent = SnesEventType::DramRefresh;

	/// <summary>Memory type for Bus A access.</summary>
	MemoryType _memTypeBusA = MemoryType::SnesPrgRom;

	/// <summary>Current CPU speed (master clocks per cycle).</summary>
	uint8_t _cpuSpeed = 8;

	/// <summary>Open bus value.</summary>
	uint8_t _openBus = 0;

	/// <summary>Memory mapping tables.</summary>
	MemoryMappings _mappings = {};

	/// <summary>Work RAM handlers.</summary>
	vector<unique_ptr<IMemoryHandler>> _workRamHandlers;

	/// <summary>Master clock timing lookup table.</summary>
	uint8_t _masterClockTable[0x800] = {};

	/// <summary>Execution function pointer type.</summary>
	typedef void (SnesMemoryManager::*Func)();

	/// <summary>Read execution callback.</summary>
	Func _execRead = nullptr;

	/// <summary>Write execution callback.</summary>
	Func _execWrite = nullptr;

	/// <summary>Increments master clock by template cycles.</summary>
	template <uint8_t clocks>
	void IncMasterClock();

	/// <summary>Updates execution callbacks.</summary>
	void UpdateExecCallbacks();

	/// <summary>Executes one memory cycle.</summary>
	__forceinline void Exec();

	/// <summary>Processes scheduled events.</summary>
	void ProcessEvent();

public:
	/// <summary>Initializes memory manager.</summary>
	void Initialize(SnesConsole* console);
	virtual ~SnesMemoryManager();

	/// <summary>Resets memory manager state.</summary>
	void Reset();

	/// <summary>Generates master clock timing table.</summary>
	void GenerateMasterClockTable();

	/// <summary>Increments master clock by 4.</summary>
	void IncMasterClock4();

	/// <summary>Increments master clock by 6.</summary>
	void IncMasterClock6();

	/// <summary>Increments master clock by 8.</summary>
	void IncMasterClock8();

	/// <summary>Increments master clock by 40.</summary>
	void IncMasterClock40();

	/// <summary>Increments master clock for startup.</summary>
	void IncMasterClockStartup();

	/// <summary>Increments master clock by arbitrary value.</summary>
	void IncrementMasterClockValue(uint16_t value);

	/// <summary>Reads byte from memory.</summary>
	uint8_t Read(uint32_t addr, MemoryOperationType type);

	/// <summary>DMA read from memory.</summary>
	uint8_t ReadDma(uint32_t addr, bool forBusA);

	/// <summary>Peek byte (no side effects).</summary>
	uint8_t Peek(uint32_t addr);

	/// <summary>Peek word (no side effects).</summary>
	uint16_t PeekWord(uint32_t addr);

	/// <summary>Peek block of memory.</summary>
	void PeekBlock(uint32_t addr, uint8_t* dest);

	/// <summary>Writes byte to memory.</summary>
	void Write(uint32_t addr, uint8_t value, MemoryOperationType type);

	/// <summary>DMA write to memory.</summary>
	void WriteDma(uint32_t addr, uint8_t value, bool forBusA);

	/// <summary>Gets current open bus value.</summary>
	uint8_t GetOpenBus();

	/// <summary>Gets master clock value.</summary>
	uint64_t GetMasterClock();

	/// <summary>Gets horizontal clock position.</summary>
	uint16_t GetHClock();

	/// <summary>Debug access to Work RAM.</summary>
	uint8_t* DebugGetWorkRam();

	/// <summary>Gets memory mapping tables.</summary>
	MemoryMappings* GetMemoryMappings();

	/// <summary>Gets CPU speed for address.</summary>
	uint8_t GetCpuSpeed(uint32_t addr);

	/// <summary>Gets current CPU speed.</summary>
	uint8_t GetCpuSpeed();

	/// <summary>Sets current CPU speed.</summary>
	void SetCpuSpeed(uint8_t speed);

	/// <summary>Gets Bus A memory type.</summary>
	MemoryType GetMemoryTypeBusA();

	/// <summary>Checks if address is a register.</summary>
	bool IsRegister(uint32_t cpuAddress);

	/// <summary>Checks if address is Work RAM.</summary>
	bool IsWorkRam(uint32_t cpuAddress);

	/// <summary>Gets current WRAM access position.</summary>
	uint32_t GetWramPosition();

	/// <summary>Serializes state for save states.</summary>
	void Serialize(Serializer& s) override;
};
