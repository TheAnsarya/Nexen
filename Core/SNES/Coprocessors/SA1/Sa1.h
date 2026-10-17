#pragma once
#include "pch.h"
#include <memory>
#include "SNES/Coprocessors/BaseCoprocessor.h"
#include "SNES/MemoryMappings.h"
#include "SNES/Coprocessors/SA1/Sa1Types.h"

class SnesConsole;
class Emulator;
class SnesCpu;
class Sa1Cpu;
class SnesMemoryManager;
class BaseCartridge;

enum class MemoryOperationType;

/// <summary>
/// SA-1 coprocessor - High-speed 65816 CPU with enhanced features.
/// Provides 10.74 MHz processing, character conversion, and DMA.
/// </summary>
/// <remarks>
/// **Overview:**
/// The SA-1 is a complete second 65816 CPU running at 10.74 MHz (3x main CPU speed)
/// with additional features for graphics processing and memory management.
///
/// **Key Features:**
/// - **65816 CPU @ 10.74 MHz**: 3x faster than main CPU
/// - **2KB Internal RAM**: Fast access for SA-1
/// - **Character Conversion**: Hardware-accelerated tile format conversion
/// - **Arithmetic Unit**: Hardware multiply/divide
/// - **Variable-Length Bit Processing**: For compressed data
/// - **DMA**: High-speed memory transfers
///
/// **Memory Access:**
/// - Both CPUs can access ROM simultaneously
/// - Configurable ROM banking (up to 8MB)
/// - BW-RAM (battery-backed work RAM) access control
/// - I-RAM visible to both CPUs with arbitration
///
/// **Inter-Processor Communication:**
/// - Message registers for CPU→SA1 and SA1→CPU
/// - NMI/IRQ triggering between processors
/// - Shared register space ($2200-$23FF)
///
/// **Character Conversion:**
/// - Type 1: Background tile format conversion
/// - Type 2: Sprite tile format conversion
/// - Hardware acceleration for Mode 7 effects
///
/// **Games:** Super Mario RPG, Kirby Super Star, Kirby's Dream Land 3
/// </remarks>
class Sa1 : public BaseCoprocessor {
private:
	/// <summary>SA-1 internal RAM size (2KB).</summary>
	static constexpr int InternalRamSize = 0x800;

	unique_ptr<Sa1Cpu> _cpu;
	SnesConsole* _console;
	Emulator* _emu;
	SnesMemoryManager* _memoryManager;
	BaseCartridge* _cart;
	SnesCpu* _snesCpu;

	Sa1State _state = {};
	uint64_t _resetCount = 0;
	std::unique_ptr<uint8_t[]> _iRam;

	MemoryType _lastAccessMemType;
	uint8_t _openBus;

	unique_ptr<IMemoryHandler> _iRamHandlerCpu;
	unique_ptr<IMemoryHandler> _iRamHandlerSa1;
	unique_ptr<IMemoryHandler> _bwRamHandler;
	unique_ptr<IMemoryHandler> _cpuVectorHandler;

	vector<unique_ptr<IMemoryHandler>> _cpuBwRamHandlers;

	MemoryMappings _mappings;

	void UpdateBank(uint8_t index, uint8_t value);
	void UpdatePrgRomMappings();
	void UpdateVectorMappings();
	void UpdateSaveRamMappings();

	void IncVarLenPosition();
	void ProcessMathOp();
	void RunCharConvertType2();

	void ProcessInterrupts();
	void RunDma();

	void Sa1RegisterWrite(uint16_t addr, uint8_t value);
	uint8_t Sa1RegisterRead(uint16_t addr);

	void WriteSharedRegisters(uint16_t addr, uint8_t value);

	void WriteInternalRam(uint32_t addr, uint8_t value);
	void WriteBwRam(uint32_t addr, uint8_t value);

public:
	Sa1(SnesConsole* console);
	virtual ~Sa1();

	void WriteSa1(uint32_t addr, uint8_t value, MemoryOperationType type);
	uint8_t ReadSa1(uint32_t addr, MemoryOperationType type = MemoryOperationType::Read);

	void CpuRegisterWrite(uint16_t addr, uint8_t value);
	uint8_t CpuRegisterRead(uint16_t addr);

	uint8_t ReadCharConvertType1(uint32_t addr);

	// Inherited via BaseCoprocessor
	void Serialize(Serializer& s) override;
	uint8_t Read(uint32_t addr) override;
	uint8_t Peek(uint32_t addr) override;
	void PeekBlock(uint32_t addr, uint8_t* output) override;
	void Write(uint32_t addr, uint8_t value) override;
	AddressInfo GetAbsoluteAddress(uint32_t address) override;

	void Run() override;
	void Reset() override;
	uint64_t GetResetCount() const { return _resetCount; }

	MemoryType GetSa1MemoryType();
	bool IsSnesCpuFastRomSpeed();
	MemoryType GetSnesCpuMemoryType();

	uint8_t* DebugGetInternalRam();
	uint32_t DebugGetInternalRamSize();

	Sa1State& GetState();
	SnesCpuState& GetCpuState();

	uint16_t ReadVector(uint16_t vector);
	MemoryMappings* GetMemoryMappings();
	void LoadBattery() override;
	void SaveBattery() override;
};
