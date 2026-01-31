#pragma once
#include "pch.h"
#include "SNES/IMemoryHandler.h"
#include "SNES/Coprocessors/SA1/Sa1Cpu.h"
#include "SNES/Coprocessors/SA1/Sa1Types.h"

/// <summary>
/// SA-1 interrupt vector redirection handler.
/// Allows the SA-1 to override the S-CPU's NMI and IRQ vectors to point
/// to custom handlers without modifying the actual ROM data.
/// </summary>
/// <remarks>
/// The SA-1 can redirect the S-CPU's interrupt vectors:
/// - NMI vector ($FFEA-$FFEB) can be replaced with CpuNmiVector
/// - IRQ vector ($FFEE-$FFEF) can be replaced with CpuIrqVector
/// 
/// This enables games to intercept interrupts for SA-1 communication
/// without patching the ROM. When UseCpuNmiVector/UseCpuIrqVector is
/// set, reads from those vector addresses return the custom values.
/// </remarks>
class Sa1VectorHandler : public IMemoryHandler {
private:
	/// <summary>Underlying memory handler for non-vector reads.</summary>
	IMemoryHandler* _handler;

	/// <summary>Pointer to SA-1 state for vector redirection flags.</summary>
	Sa1State* _state;

public:
	/// <summary>
	/// Creates a new SA-1 vector handler.
	/// </summary>
	/// <param name="handler">Underlying memory handler (ROM).</param>
	/// <param name="state">Pointer to SA-1 state.</param>
	Sa1VectorHandler(IMemoryHandler* handler, Sa1State* state) : IMemoryHandler(handler->GetMemoryType()) {
		_handler = handler;
		_state = state;
	}

	/// <summary>
	/// Reads from memory, returning redirected vectors if enabled.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Data byte (or redirected vector address).</returns>
	uint8_t Read(uint32_t addr) override {
		// Check if address is in the vector table range
		if (addr >= Sa1Cpu::NmiVector && addr <= Sa1Cpu::ResetVector + 1) {
			// Override NMI vector if redirection is enabled
			if (_state->UseCpuNmiVector) {
				if (addr == Sa1Cpu::NmiVector) {
					return (uint8_t)_state->CpuNmiVector;  // Low byte
				} else if (addr == Sa1Cpu::NmiVector + 1) {
					return (uint8_t)(_state->CpuNmiVector >> 8);  // High byte
				}
			}
			// Override IRQ vector if redirection is enabled
			if (_state->UseCpuIrqVector) {
				if (addr == Sa1Cpu::IrqVector) {
					return (uint8_t)_state->CpuIrqVector;  // Low byte
				} else if (addr == Sa1Cpu::IrqVector + 1) {
					return (uint8_t)(_state->CpuIrqVector >> 8);  // High byte
				}
			}
		}

		// Fall through to underlying handler for all other addresses
		return _handler->Read(addr);
	}

	/// <summary>
	/// Peeks memory (same as Read, redirection applies).
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Data byte.</returns>
	uint8_t Peek(uint32_t addr) override {
		return Read(addr);
	}

	/// <summary>
	/// Peeks a block of memory data.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer for data.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override {
		_handler->PeekBlock(addr, output);
	}

	/// <summary>
	/// Writes to underlying memory (vectors are ROM, so typically no-op).
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Data byte to write.</param>
	void Write(uint32_t addr, uint8_t value) override {
		_handler->Write(addr, value);
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