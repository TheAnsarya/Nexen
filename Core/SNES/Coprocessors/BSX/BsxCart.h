#pragma once
#include "pch.h"
#include <memory>
#include "SNES/Coprocessors/BaseCoprocessor.h"

class SnesConsole;
class SnesMemoryManager;
class BsxMemoryPack;
class BsxSatellaview;

/// <summary>
/// BS-X cartridge slot adapter emulation.
/// Emulates the BS-X BIOS cartridge that provided the interface for Satellaview content.
/// The BS-X cartridge plugged into the SNES and provided Memory Pack slots.
/// </summary>
/// <remarks>
/// The BS-X BIOS cartridge contained:
/// - BIOS ROM with the Satellaview interface/town
/// - PSRAM (Pseudo-SRAM) for downloaded content execution
/// - Memory Pack slot for storing downloaded games
/// - Satellaview interface hardware
/// 
/// Register map ($5000-$500F):
/// - $5000-$5003: Memory mapping control
/// - $5004-$5005: PSRAM control
/// - $5006-$500F: Additional configuration
/// 
/// PSRAM provides volatile storage that received downloaded game data
/// from the satellite broadcasts.
/// </remarks>
class BsxCart : public BaseCoprocessor {
private:
	/// <summary>Reference to SNES console.</summary>
	SnesConsole* _console;

	/// <summary>Reference to memory manager for mappings.</summary>
	SnesMemoryManager* _memoryManager;

	/// <summary>Memory pack handler for downloaded content storage.</summary>
	BsxMemoryPack* _memPack;

	/// <summary>Satellaview receiver interface.</summary>
	unique_ptr<BsxSatellaview> _satellaview;

	/// <summary>PSRAM buffer for downloaded content.</summary>
	std::unique_ptr<uint8_t[]> _psRam;

	/// <summary>PSRAM size in bytes.</summary>
	uint32_t _psRamSize = 0;

	/// <summary>PSRAM memory handlers for mapping.</summary>
	vector<unique_ptr<IMemoryHandler>> _psRamHandlers;

	/// <summary>BS-X control registers.</summary>
	uint8_t _regs[0x10] = {};

	/// <summary>Dirty flags for register changes.</summary>
	uint8_t _dirtyRegs[0x10] = {};

	/// <summary>True if any register has changed.</summary>
	bool _dirty = false;

	/// <summary>Updates memory mappings based on register changes.</summary>
	void UpdateMemoryMappings();

public:
	/// <summary>
	/// Creates a new BS-X cartridge instance.
	/// </summary>
	/// <param name="console">Reference to SNES console.</param>
	/// <param name="memPack">Memory pack for content storage.</param>
	BsxCart(SnesConsole* console, BsxMemoryPack* memPack);

	/// <summary>Destructor.</summary>
	virtual ~BsxCart();

	/// <summary>
	/// Reads from BS-X registers.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Register data.</returns>
	uint8_t Read(uint32_t addr) override;

	/// <summary>
	/// Writes to BS-X registers.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Data to write.</param>
	void Write(uint32_t addr, uint8_t value) override;

	/// <summary>Resets BS-X state.</summary>
	void Reset() override;

	/// <summary>Serializes BS-X state for save states.</summary>
	void Serialize(Serializer& s) override;

	/// <summary>
	/// Peeks BS-X without side effects.
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Register data.</returns>
	uint8_t Peek(uint32_t addr) override;

	/// <summary>
	/// Peeks a block of data for debugging.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer for data.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override;

	/// <summary>
	/// Gets absolute address for debugging.
	/// </summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address info.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t address) override;

	/// <summary>
	/// Gets pointer to PSRAM for debugging.
	/// </summary>
	/// <returns>Pointer to PSRAM buffer.</returns>
	uint8_t* DebugGetPsRam();

	/// <summary>
	/// Gets PSRAM size for debugging.
	/// </summary>
	/// <returns>PSRAM size in bytes.</returns>
	uint32_t DebugGetPsRamSize();
};