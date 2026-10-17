#pragma once
#include "pch.h"
#include "ChannelF/ChannelFMemoryManager.h"

class Emulator;

/// <summary>
/// Fairchild F8 CPU core for Channel F emulation.
///
/// The F8 is an 8-bit processor with a unique architecture:
/// - 8-bit accumulator (A) and status register (W)
/// - 64-byte internal scratchpad RAM (R0-R63)
/// - 6-bit Indirect Scratchpad Address Register (ISAR)
/// - Two 16-bit program counters (PC0 active, PC1 backup/stack)
/// - Two 16-bit data counters (DC0 active, DC1 backup)
/// - Big-endian addressing for multi-byte operands
/// - I/O port space separate from memory (ports 0-255)
/// </summary>
class ChannelFCpu {
private:
	ChannelFMemoryManager* _memoryManager = nullptr;
	Emulator* _emu = nullptr;

	// Test-only: raw memory pointers for unit tests without full emulator
	uint8_t* _testMemory = nullptr;
	uint8_t* _testPorts = nullptr;

	// Registers
	uint8_t _a = 0;          // Accumulator
	uint8_t _w = 0;          // Status register (flags)
	uint8_t _isar = 0;       // Indirect Scratchpad Address Register (6-bit)
	uint16_t _pc0 = 0;       // Program Counter (active)
	uint16_t _pc1 = 0;       // Program Counter (backup/stack)
	uint16_t _dc0 = 0;       // Data Counter 0 (active)
	uint16_t _dc1 = 0;       // Data Counter 1 (backup)
	uint8_t _scratchpad[64] = {}; // Internal scratchpad RAM (R0-R63)
	bool _interruptsEnabled = false;
	bool _irqLine = false;             // External interrupt request line
	uint16_t _interruptVector = 0;     // Interrupt vector address (set by 3853 SMI)
	uint8_t _lastOpcode = 0;           // Last executed opcode (for privileged check)

	uint64_t _cycleCount = 0;

	// W register flag bits (matches F8 hardware: S=bit0, C=bit1, Z=bit2, O=bit3)
	static constexpr uint8_t FlagSign     = 0x01; // Bit 0: Sign (complementary: set when positive)
	static constexpr uint8_t FlagCarry    = 0x02; // Bit 1: Carry/Link
	static constexpr uint8_t FlagZero     = 0x04; // Bit 2: Zero
	static constexpr uint8_t FlagOverflow = 0x08; // Bit 3: Overflow
	static constexpr uint8_t FlagICB      = 0x10; // Bit 4: Interrupt Control Bit
	static constexpr uint8_t FlagsMask    = 0x0f; // Status flags only (bits 0-3)

	// Named scratchpad aliases
	static constexpr uint8_t RegJ  = 9;   // J register (flags backup)
	static constexpr uint8_t RegHU = 10;  // H upper (DC0 backup high)
	static constexpr uint8_t RegHL = 11;  // H lower (DC0 backup low)
	static constexpr uint8_t RegKU = 12;  // K upper (PC1 backup high)
	static constexpr uint8_t RegKL = 13;  // K lower (PC1 backup low)
	static constexpr uint8_t RegQU = 14;  // Q upper (DC0 copy high)
	static constexpr uint8_t RegQL = 15;  // Q lower (DC0 copy low)

	[[nodiscard]] uint8_t Read(uint16_t addr) const {
		if (_memoryManager) [[likely]] {
			return _memoryManager->ReadMemory(addr);
		}
		return _testMemory[addr];
	}

	void Write(uint16_t addr, uint8_t value) const {
		if (_memoryManager) [[likely]] {
			_memoryManager->WriteMemory(addr, value);
			return;
		}
		_testMemory[addr] = value;
	}

	[[nodiscard]] uint8_t ReadPort(uint8_t port) const;
	void WritePort(uint8_t port, uint8_t value) const;

	[[nodiscard]] uint8_t FetchByte() {
		uint8_t value = Read(_pc0);
		_pc0++;
		return value;
	}

	void SetFlags(uint8_t result) {
		_w &= ~FlagsMask;
		if (result == 0) _w |= FlagZero;
		if (~result & 0x80) _w |= FlagSign; // complementary: set when positive
	}

	void SetFlagsWithCarryOverflow(uint16_t fullResult, uint8_t left, uint8_t right) {
		uint8_t result = (uint8_t)fullResult;
		_w &= ~FlagsMask;
		if (result == 0) _w |= FlagZero;
		if (~result & 0x80) _w |= FlagSign; // complementary: set when positive
		if (fullResult > 0xff) _w |= FlagCarry;
		if (((left ^ result) & (right ^ result) & 0x80) != 0) _w |= FlagOverflow;
	}

	[[nodiscard]] uint8_t GetScratchpad(uint8_t reg) const {
		return _scratchpad[reg & 0x3f];
	}

	void SetScratchpad(uint8_t reg, uint8_t value) {
		_scratchpad[reg & 0x3f] = value;
	}

	[[nodiscard]] uint8_t GetIsarReg() const {
		return _scratchpad[_isar & 0x3f];
	}

	void SetIsarReg(uint8_t value) {
		_scratchpad[_isar & 0x3f] = value;
	}

	[[nodiscard]] static bool IsPrivilegedOpcode(uint8_t opcode);
	uint8_t DeliverInterrupt();
	[[nodiscard]] uint8_t ExecuteInstruction();

public:
	ChannelFCpu() = default;

	void Init(ChannelFMemoryManager* memMgr, Emulator* emu) { _memoryManager = memMgr; _emu = emu; }
	void InitTestMode(uint8_t* memory, uint8_t* ports) { _testMemory = memory; _testPorts = ports; }

	void Reset();
	void StepCycles(uint32_t targetCycles);
	[[nodiscard]] uint8_t StepOne();

	// State export/import for serialization
	void ExportState(
		uint8_t& a, uint8_t& w, uint8_t& isar,
		uint16_t& pc0, uint16_t& pc1, uint16_t& dc0, uint16_t& dc1,
		uint64_t& cycleCount, bool& interruptsEnabled,
		uint8_t* scratchpadOut
	) const;

	void ImportState(
		uint8_t a, uint8_t w, uint8_t isar,
		uint16_t pc0, uint16_t pc1, uint16_t dc0, uint16_t dc1,
		uint64_t cycleCount, bool interruptsEnabled,
		const uint8_t* scratchpadIn
	);

	// Interrupt control
	void SetIrqLine(bool level) { _irqLine = level; }
	void SetInterruptVector(uint16_t vector) { _interruptVector = vector; }

	// Accessors for debugger / tests
	[[nodiscard]] uint16_t GetPc() const { return _pc0; }
	[[nodiscard]] uint8_t GetA() const { return _a; }
	[[nodiscard]] uint8_t GetW() const { return _w; }
	[[nodiscard]] uint8_t GetIsar() const { return _isar; }
	[[nodiscard]] uint16_t GetPc1() const { return _pc1; }
	[[nodiscard]] uint16_t GetDc0() const { return _dc0; }
	[[nodiscard]] uint16_t GetDc1() const { return _dc1; }
	[[nodiscard]] uint64_t GetCycleCount() const { return _cycleCount; }
	[[nodiscard]] const uint8_t* GetScratchpadData() const { return _scratchpad; }
	[[nodiscard]] bool GetIrqLine() const { return _irqLine; }
	[[nodiscard]] uint16_t GetInterruptVector() const { return _interruptVector; }
};
