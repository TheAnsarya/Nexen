#pragma once
#include "pch.h"
#include "SNES/IMemoryHandler.h"
#include "Debugger/DebugTypes.h"
#include "Shared/MemoryType.h"

/// <summary>
/// SA-1 Internal RAM (I-RAM) memory handler.
/// Provides access to the 2KB of fast internal RAM shared between SA-1 and S-CPU.
/// Features page-based write protection to prevent accidental overwrites.
/// I-RAM is accessible at multiple address ranges with the upper half mirrored as open bus.
/// </summary>
/// <remarks>
/// The 2KB I-RAM is divided into 8 pages of 256 bytes each.
/// Write protection is controlled per-page via a bitmask register.
/// Only the lower 2KB is valid RAM; addresses with bit 11 set return 0.
/// </remarks>
class Sa1IRamHandler : public IMemoryHandler {
private:
	/// <summary>Pointer to write protection bitmask (one bit per 256-byte page).</summary>
	uint8_t* _writeEnabled = nullptr;

	/// <summary>Pointer to I-RAM data buffer (2KB).</summary>
	uint8_t* _ram = nullptr;

	/// <summary>
	/// Reads from I-RAM with address validation.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Data byte or 0 if address is out of range.</returns>
	__forceinline uint8_t InternalRead(uint32_t addr) {
		// Bit 11 set means address is outside valid I-RAM range
		if (addr & 0x800) {
			return 0;
		} else {
			return _ram[addr & 0x7FF];
		}
	}

public:
	/// <summary>
	/// Creates a new SA-1 I-RAM handler.
	/// </summary>
	/// <param name="writeProtected">Pointer to write protection bitmask.</param>
	/// <param name="ram">Pointer to 2KB I-RAM buffer.</param>
	Sa1IRamHandler(uint8_t* writeProtected, uint8_t* ram) : IMemoryHandler(MemoryType::Sa1InternalRam) {
		_writeEnabled = writeProtected;
		_ram = ram;
	}

	/// <summary>
	/// Reads from I-RAM.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Data byte.</returns>
	uint8_t Read(uint32_t addr) override {
		return InternalRead(addr);
	}

	/// <summary>
	/// Peeks I-RAM without side effects.
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Data byte.</returns>
	uint8_t Peek(uint32_t addr) override {
		return InternalRead(addr);
	}

	/// <summary>
	/// Peeks a block of I-RAM data.
	/// </summary>
	/// <param name="addr">Starting address (ignored, always reads from 0).</param>
	/// <param name="output">Buffer for data.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override {
		for (int i = 0; i < 0x1000; i++) {
			output[i] = InternalRead(i);
		}
	}

	/// <summary>
	/// Writes to I-RAM with write protection check.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Data byte to write.</param>
	void Write(uint32_t addr, uint8_t value) override {
		// Check write protection: each bit protects a 256-byte page
		if (!(addr & 0x800) && ((*_writeEnabled) & (1 << ((addr >> 8) & 0x07))) != 0) {
			_ram[addr & 0x7FF] = value;
		}
	}

	/// <summary>
	/// Gets absolute address for debugging.
	/// </summary>
	/// <param name="addr">Relative address.</param>
	/// <returns>Absolute address info.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t addr) override {
		AddressInfo info;
		// Upper range is not valid I-RAM
		if (addr & 0x800) {
			info.Address = -1;
			info.Type = MemoryType::SnesMemory;
		} else {
			info.Address = (addr & 0x7FF);
			info.Type = _memoryType;
		}
		return info;
	}
};