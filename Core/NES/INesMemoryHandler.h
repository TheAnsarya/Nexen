#pragma once

#include "pch.h"
#include "NES/NesTypes.h"

/// <summary>
/// Describes memory address ranges handled by a memory handler.
/// </summary>
/// <remarks>
/// Used during initialization to register which addresses
/// a mapper or peripheral responds to for reads and/or writes.
/// </remarks>
class MemoryRanges {
private:
	/// <summary>Addresses this handler reads from.</summary>
	vector<uint16_t> _ramReadAddresses;

	/// <summary>Addresses this handler writes to.</summary>
	vector<uint16_t> _ramWriteAddresses;

	/// <summary>Whether this handler can override existing mappings.</summary>
	bool _allowOverride = false;

public:
	/// <summary>Gets the list of read addresses.</summary>
	/// <returns>Pointer to read address vector.</returns>
	vector<uint16_t>* GetRAMReadAddresses() { return &_ramReadAddresses; }

	/// <summary>Gets the list of write addresses.</summary>
	/// <returns>Pointer to write address vector.</returns>
	vector<uint16_t>* GetRAMWriteAddresses() { return &_ramWriteAddresses; }

	/// <summary>Gets whether override is allowed.</summary>
	/// <returns>true if this handler can override existing mappings.</returns>
	bool GetAllowOverride() {
		return _allowOverride;
	}

	/// <summary>Enables overriding existing mappings.</summary>
	void SetAllowOverride() {
		_allowOverride = true;
	}

	/// <summary>
	/// Adds an address range to this handler.
	/// </summary>
	/// <param name="operation">Read, Write, or Any (both).</param>
	/// <param name="start">Start address.</param>
	/// <param name="end">End address (inclusive). If 0, uses start.</param>
	void AddHandler(MemoryOperation operation, uint16_t start, uint16_t end = 0) {
		if (end == 0) {
			end = start;
		}

		if (operation == MemoryOperation::Read || operation == MemoryOperation::Any) {
			_ramReadAddresses.reserve(_ramReadAddresses.size() + (end - start + 1));
			for (uint32_t i = start; i <= end; i++) {
				_ramReadAddresses.push_back((uint16_t)i);
			}
		}

		if (operation == MemoryOperation::Write || operation == MemoryOperation::Any) {
			_ramWriteAddresses.reserve(_ramWriteAddresses.size() + (end - start + 1));
			for (uint32_t i = start; i <= end; i++) {
				_ramWriteAddresses.push_back((uint16_t)i);
			}
		}
	}
};

/// <summary>
/// Interface for NES memory-mapped hardware handlers.
/// </summary>
/// <remarks>
/// **Memory Map Overview:**
/// The NES CPU has a 16-bit address space ($0000-$FFFF):
/// - $0000-$1FFF: RAM (2KB, mirrored 4×)
/// - $2000-$3FFF: PPU registers (8 bytes, mirrored)
/// - $4000-$401F: APU and I/O registers
/// - $4020-$FFFF: Cartridge space (PRG-ROM, mapper registers)
///
/// **Handler Types:**
/// - Mappers: PRG/CHR banking, expansion audio
/// - Expansion devices: FDS, expansion RAM
/// - Input devices: Controllers, light guns
///
/// **Implementation Notes:**
/// - ReadRam() is called on CPU read cycles
/// - WriteRam() is called on CPU write cycles
/// - PeekRam() reads without side effects (for debugging)
/// - GetMemoryRanges() registers handled addresses
/// </remarks>
class INesMemoryHandler {
public:
	/// <summary>
	/// Gets the memory address ranges this handler responds to.
	/// </summary>
	/// <param name="ranges">Range collection to add addresses to.</param>
	virtual void GetMemoryRanges(MemoryRanges& ranges) = 0;

	/// <summary>
	/// Reads from a handled address.
	/// </summary>
	/// <param name="addr">Address being read.</param>
	/// <returns>Value at address.</returns>
	virtual uint8_t ReadRam(uint16_t addr) = 0;

	/// <summary>
	/// Writes to a handled address.
	/// </summary>
	/// <param name="addr">Address being written.</param>
	/// <param name="value">Value to write.</param>
	virtual void WriteRam(uint16_t addr, uint8_t value) = 0;

	/// <summary>
	/// Reads from address without side effects (for debugging).
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Value at address (default: 0).</returns>
	virtual uint8_t PeekRam(uint16_t addr) { return 0; }

	virtual ~INesMemoryHandler() {}
};