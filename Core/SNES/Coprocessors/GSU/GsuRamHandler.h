#pragma once
#include "pch.h"
#include "SNES/IMemoryHandler.h"
#include "SNES/Coprocessors/GSU/GsuTypes.h"
#include "Shared/MemoryType.h"

/// <summary>
/// Memory handler for GSU (Super FX) RAM access arbitration.
/// Manages bus conflicts between the main SNES CPU and the GSU coprocessor
/// for access to the GSU work RAM (typically 32KB or 64KB).
/// </summary>
/// <remarks>
/// The GSU has dedicated work RAM that can be accessed by both the GSU
/// and the main SNES CPU. When the GSU is running and has RAM access,
/// the main CPU cannot read or write the RAM - reads return open bus
/// and writes are ignored.
/// </remarks>
class GsuRamHandler : public IMemoryHandler {
private:
	/// <summary>Pointer to GSU state for checking access flags.</summary>
	GsuState* _state;

	/// <summary>Underlying RAM handler for actual memory access.</summary>
	IMemoryHandler* _handler;

public:
	/// <summary>
	/// Creates a GSU RAM handler.
	/// </summary>
	/// <param name="state">Reference to GSU state.</param>
	/// <param name="handler">Underlying RAM memory handler.</param>
	GsuRamHandler(GsuState& state, IMemoryHandler* handler) : IMemoryHandler(MemoryType::GsuWorkRam) {
		_handler = handler;
		_state = &state;
	}

	/// <summary>
	/// Reads from GSU RAM if accessible.
	/// Returns open bus (0) when GSU has RAM control.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>RAM data or 0 if GSU owns bus.</returns>
	uint8_t Read(uint32_t addr) override {
		// If GSU is not running or doesn't have RAM access, allow CPU access
		if (!_state->SFR.Running || !_state->GsuRamAccess) {
			return _handler->Read(addr);
		}

		// GSU has bus control - return open bus value
		// TODO: proper open bus implementation
		return 0;
	}

	/// <summary>
	/// Peeks GSU RAM without side effects.
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>RAM data or 0 if GSU owns bus.</returns>
	uint8_t Peek(uint32_t addr) override {
		return Read(addr);
	}

	/// <summary>
	/// Peeks a block of GSU RAM.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer to receive data.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override {
		for (int i = 0; i < 0x1000; i++) {
			output[i] = Read(i);
		}
	}

	/// <summary>
	/// Writes to GSU RAM if accessible.
	/// Write is ignored when GSU has RAM control.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Data byte to write.</param>
	void Write(uint32_t addr, uint8_t value) override {
		// Only allow write when GSU doesn't have bus control
		if (!_state->SFR.Running || !_state->GsuRamAccess) {
			_handler->Write(addr, value);
		}
		// Otherwise write is ignored
	}

	/// <summary>
	/// Gets absolute address for debugging.
	/// Returns invalid address when GSU has bus control.
	/// </summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address info or invalid if GSU owns bus.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t address) override {
		if (!_state->SFR.Running || !_state->GsuRamAccess) {
			return _handler->GetAbsoluteAddress(address);
		} else {
			return {-1, MemoryType::None};
		}
	}
};