#pragma once
#include "pch.h"
#include "SNES/IMemoryHandler.h"
#include "SNES/Coprocessors/GSU/GsuTypes.h"
#include "Shared/MemoryType.h"

/// <summary>
/// Memory handler for GSU (Super FX) ROM access arbitration.
/// Manages bus conflicts between the main SNES CPU and the GSU coprocessor.
/// When the GSU is running and has ROM access, the main CPU sees predetermined
/// values instead of actual ROM data (similar to open bus behavior).
/// </summary>
/// <remarks>
/// The GSU and main CPU share access to the cartridge ROM.
/// While GSU is running with GsuRomAccess set, CPU ROM reads return
/// specific patterns based on address bits rather than ROM contents.
/// </remarks>
class GsuRomHandler : public IMemoryHandler {
private:
	/// <summary>Pointer to GSU state for checking access flags.</summary>
	GsuState* _state;

	/// <summary>Underlying ROM handler for actual ROM access.</summary>
	IMemoryHandler* _romHandler;

public:
	/// <summary>
	/// Creates a GSU ROM handler.
	/// </summary>
	/// <param name="state">Reference to GSU state.</param>
	/// <param name="romHandler">Underlying ROM memory handler.</param>
	GsuRomHandler(GsuState& state, IMemoryHandler* romHandler) : IMemoryHandler(MemoryType::SnesPrgRom) {
		_romHandler = romHandler;
		_state = &state;
	}

	/// <summary>
	/// Reads from ROM or returns bus conflict pattern.
	/// When GSU is running with ROM access, returns predetermined values.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>ROM data or conflict pattern.</returns>
	uint8_t Read(uint32_t addr) override {
		// If GSU is not running or doesn't have ROM access, return actual ROM data
		if (!_state->SFR.Running || !_state->GsuRomAccess) {
			return _romHandler->Read(addr);
		}

		// GSU has bus - return predetermined pattern based on address
		// Odd addresses return 0x01
		if (addr & 0x01) {
			return 0x01;
		}

		// Even addresses return pattern based on lower nibble
		switch (addr & 0x0E) {
			default:
			case 2:
			case 6:
			case 8:
			case 0x0C:
				return 0;

			case 4:
				return 0x04;
			case 0x0A:
				return 0x08;
			case 0x0E:
				return 0x0C;
		}
	}

	/// <summary>
	/// Peeks ROM without side effects.
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>ROM data or conflict pattern.</returns>
	uint8_t Peek(uint32_t addr) override {
		return Read(addr);
	}

	/// <summary>
	/// Peeks a block of ROM data.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer to receive data.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override {
		for (int i = 0; i < 0x1000; i++) {
			output[i] = Read(i);
		}
	}

	/// <summary>
	/// Write to ROM - ignored (ROM is read-only).
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Value to write (ignored).</param>
	void Write(uint32_t addr, uint8_t value) override {
		// ROM is read-only, writes are ignored
	}

	/// <summary>
	/// Gets absolute address for debugging.
	/// Returns invalid address when GSU has bus control.
	/// </summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address info or invalid if GSU owns bus.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t address) override {
		if (!_state->SFR.Running || !_state->GsuRomAccess) {
			return _romHandler->GetAbsoluteAddress(address);
		} else {
			return {-1, MemoryType::None};
		}
	}
};