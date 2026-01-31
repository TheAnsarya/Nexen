#pragma once
#include "pch.h"
#include "SNES/IMemoryHandler.h"
#include "SNES/RamHandler.h"
#include "SNES/Coprocessors/SA1/Sa1Cpu.h"
#include "SNES/Coprocessors/SA1/Sa1Types.h"
#include "SNES/Coprocessors/SA1/Sa1.h"

/// <summary>
/// BW-RAM memory handler for S-CPU (main SNES CPU) access.
/// Manages BW-RAM access from the main CPU side with character conversion support.
/// Returns converted graphics data when character conversion DMA Type 1 is active.
/// </summary>
/// <remarks>
/// Character Conversion Type 1: SA-1 performs automatic graphics format conversion
/// when the S-CPU reads from BW-RAM during an active conversion DMA. This is used
/// to convert linear bitmap data to SNES planar tile format in hardware.
/// 
/// Write protection is enforced based on BwWriteProtectedArea register settings.
/// Both CPUs can independently enable/disable their write access.
/// </remarks>
class CpuBwRamHandler : public IMemoryHandler {
private:
	/// <summary>Underlying RAM handler for actual memory access.</summary>
	RamHandler* _handler = nullptr;

	/// <summary>Pointer to SA-1 state for mode flags.</summary>
	Sa1State* _state = nullptr;

	/// <summary>Reference to SA-1 coprocessor for character conversion.</summary>
	Sa1* _sa1 = nullptr;

public:
	/// <summary>
	/// Creates a new S-CPU BW-RAM handler.
	/// </summary>
	/// <param name="handler">Underlying RAM handler.</param>
	/// <param name="state">Pointer to SA-1 state.</param>
	/// <param name="sa1">Reference to SA-1 coprocessor.</param>
	CpuBwRamHandler(RamHandler* handler, Sa1State* state, Sa1* sa1) : IMemoryHandler(handler->GetMemoryType()) {
		_handler = handler;
		_sa1 = sa1;
		_state = state;
	}

	/// <summary>
	/// Reads from BW-RAM, returning converted data if char conversion is active.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Data byte (or converted graphics data).</returns>
	uint8_t Read(uint32_t addr) override {
		if (_state->CharConvDmaActive) {
			// Return character conversion result instead of raw memory
			return _sa1->ReadCharConvertType1(addr);
		} else {
			return _handler->Read(addr);
		}
	}

	/// <summary>
	/// Peeks BW-RAM (same as Read, conversion applies).
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Data byte.</returns>
	uint8_t Peek(uint32_t addr) override {
		return Read(addr);
	}

	/// <summary>
	/// Peeks a block of BW-RAM data.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer for data.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override {
		_handler->PeekBlock(addr, output);
	}

	/// <summary>
	/// Writes to BW-RAM with write protection enforcement.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Data byte to write.</param>
	/// <remarks>
	/// Write is allowed if either CPU has write enabled globally,
	/// or if the address is outside the write-protected region.
	/// Protected region size is 256 << BwWriteProtectedArea bytes.
	/// </remarks>
	void Write(uint32_t addr, uint8_t value) override {
		if (_state->Sa1BwWriteEnabled || _state->CpuBwWriteEnabled) {
			// Global write enabled - allow all writes
			_handler->Write(addr, value);
		} else {
			// Calculate protected region size (256 to 256KB)
			uint32_t size = 256 << (_state->BwWriteProtectedArea > 0x0A ? 0x0A : _state->BwWriteProtectedArea);
			// Check if address is in $40-$4F bank range (linear mapping)
			if ((addr & 0xE00000) == 0x400000 && (addr & 0x3FFFF) >= size) {
				_handler->Write(addr, value);
			} else if (_handler->GetOffset() + (addr & 0xFFF) >= size) {
				// Other mappings: check handler offset + local address
				_handler->Write(addr, value);
			}
		}
	}

	/// <summary>
	/// Gets absolute address for debugging.
	/// </summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address info.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t address) override {
		return _handler->GetAbsoluteAddress(address);
	}
};