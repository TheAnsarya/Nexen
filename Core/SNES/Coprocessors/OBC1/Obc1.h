#pragma once
#include "pch.h"
#include "SNES/Coprocessors/BaseCoprocessor.h"

class SnesConsole;

/// <summary>
/// OBC1 (OBject Controller 1) coprocessor emulation.
/// Used by Metal Combat: Falcon's Revenge for sprite processing.
/// Provides hardware-accelerated sprite OAM management and coordinate calculations.
/// </summary>
/// <remarks>
/// The OBC1 manages a sprite attribute table in SRAM with efficient
/// indexed access patterns. It provides:
/// - Base/Low/High address registers for sprite indexing
/// - Direct byte read/write at computed addresses
/// - Automatic address calculation from index values
/// 
/// Register map ($7FF0-$7FFF):
/// - $7FF0-$7FF3: Sprite data access (via computed address)
/// - $7FF4: OAM base address index (0-127)
/// - $7FF5: OBC shift value
/// - $7FF6-$7FFF: Additional control registers
/// 
/// The chip simplifies sprite management by allowing the game
/// to access sprite data through a compact index rather than
/// computing full addresses in software.
/// </remarks>
class Obc1 : public BaseCoprocessor {
private:
	/// <summary>Pointer to battery-backed SRAM.</summary>
	uint8_t* _ram;

	/// <summary>Address mask based on SRAM size.</summary>
	uint32_t _mask;

	/// <summary>
	/// Gets the base address for sprite attribute access.
	/// </summary>
	/// <returns>Base address calculated from index register.</returns>
	uint16_t GetBaseAddress();

	/// <summary>
	/// Gets the low sprite attribute address.
	/// </summary>
	/// <returns>Low address for sprite coordinate data.</returns>
	uint16_t GetLowAddress();

	/// <summary>
	/// Gets the high sprite attribute address.
	/// </summary>
	/// <returns>High address for sprite attribute data.</returns>
	uint16_t GetHighAddress();

	/// <summary>
	/// Reads from SRAM through the OBC1 interface.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Data byte.</returns>
	uint8_t ReadRam(uint16_t addr);

	/// <summary>
	/// Writes to SRAM through the OBC1 interface.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Data byte to write.</param>
	void WriteRam(uint16_t addr, uint8_t value);

public:
	/// <summary>
	/// Creates a new OBC1 coprocessor instance.
	/// </summary>
	/// <param name="console">Reference to SNES console.</param>
	/// <param name="saveRam">Pointer to battery-backed SRAM.</param>
	/// <param name="saveRamSize">Size of SRAM in bytes.</param>
	Obc1(SnesConsole* console, uint8_t* saveRam, uint32_t saveRamSize);

	/// <summary>Resets OBC1 state.</summary>
	void Reset() override;

	/// <summary>
	/// Reads from OBC1 registers or SRAM.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Data byte.</returns>
	uint8_t Read(uint32_t addr) override;

	/// <summary>
	/// Writes to OBC1 registers or SRAM.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Data byte to write.</param>
	void Write(uint32_t addr, uint8_t value) override;

	/// <summary>Serializes OBC1 state for save states.</summary>
	void Serialize(Serializer& s) override;

	/// <summary>
	/// Peeks OBC1 without side effects (for debugging).
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Data byte.</returns>
	uint8_t Peek(uint32_t addr) override;

	/// <summary>
	/// Peeks a block of OBC1 data for debugging.
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
};