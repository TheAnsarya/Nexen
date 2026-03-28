#pragma once
#include "pch.h"
#include <functional>

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
	std::function<uint8_t(uint16_t)> _read;
	std::function<void(uint16_t, uint8_t)> _write;
	std::function<uint8_t(uint8_t)> _readPort;
	std::function<void(uint8_t, uint8_t)> _writePort;

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

	uint64_t _cycleCount = 0;

	// W register flag bits
	static constexpr uint8_t FlagSign     = 0x08; // Bit 3: Sign (negative)
	static constexpr uint8_t FlagCarry    = 0x04; // Bit 2: Carry/Link
	static constexpr uint8_t FlagZero     = 0x02; // Bit 1: Zero
	static constexpr uint8_t FlagOverflow = 0x01; // Bit 0: Overflow

	// Named scratchpad aliases
	static constexpr uint8_t RegJ  = 9;   // J register (flags backup)
	static constexpr uint8_t RegHU = 10;  // H upper (DC0 backup high)
	static constexpr uint8_t RegHL = 11;  // H lower (DC0 backup low)
	static constexpr uint8_t RegKU = 12;  // K upper (PC1 backup high)
	static constexpr uint8_t RegKL = 13;  // K lower (PC1 backup low)
	static constexpr uint8_t RegQU = 14;  // Q upper (DC0 copy high)
	static constexpr uint8_t RegQL = 15;  // Q lower (DC0 copy low)

	[[nodiscard]] uint8_t Read(uint16_t addr) const {
		return _read ? _read(addr) : 0xff;
	}

	void Write(uint16_t addr, uint8_t value) const {
		if (_write) {
			_write(addr, value);
		}
	}

	[[nodiscard]] uint8_t ReadPort(uint8_t port) const {
		return _readPort ? _readPort(port) : 0xff;
	}

	void WritePort(uint8_t port, uint8_t value) const {
		if (_writePort) {
			_writePort(port, value);
		}
	}

	[[nodiscard]] uint8_t FetchByte() {
		uint8_t value = Read(_pc0);
		_pc0++;
		return value;
	}

	void SetFlags(uint8_t result) {
		_w = 0;
		if (result == 0) _w |= FlagZero;
		if (result & 0x80) _w |= FlagSign;
	}

	void SetFlagsWithCarryOverflow(uint16_t fullResult, uint8_t left, uint8_t right) {
		uint8_t result = (uint8_t)fullResult;
		_w = 0;
		if (result == 0) _w |= FlagZero;
		if (result & 0x80) _w |= FlagSign;
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

	[[nodiscard]] uint8_t ExecuteInstruction();

public:
	ChannelFCpu() = default;

	void SetReadCallback(std::function<uint8_t(uint16_t)> read) { _read = std::move(read); }
	void SetWriteCallback(std::function<void(uint16_t, uint8_t)> write) { _write = std::move(write); }
	void SetReadPortCallback(std::function<uint8_t(uint8_t)> readPort) { _readPort = std::move(readPort); }
	void SetWritePortCallback(std::function<void(uint8_t, uint8_t)> writePort) { _writePort = std::move(writePort); }

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
};
