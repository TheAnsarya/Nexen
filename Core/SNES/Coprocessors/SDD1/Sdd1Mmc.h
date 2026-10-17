#pragma once
#include "pch.h"
#include "SNES/IMemoryHandler.h"
#include "SNES/Coprocessors/SDD1/Sdd1Types.h"
#include "SNES/Coprocessors/SDD1/Sdd1Decomp.h"
#include "Utilities/ISerializable.h"

class BaseCartridge;

/// <summary>
/// S-DD1 Memory Management Controller.
/// Handles ROM bank switching and integrates the decompression engine
/// to transparently decompress data during DMA transfers.
/// </summary>
/// <remarks>
/// The MMC intercepts reads from the high ROM banks ($C0-$FF) and can either
/// pass through raw ROM data or decompress it based on the current S-DD1 state.
/// Bank selection is controlled by writing to S-DD1 registers $4804-$4807.
/// </remarks>
class Sdd1Mmc : public IMemoryHandler, public ISerializable {
private:
	/// <summary>Pointer to S-DD1 state for bank selection and DMA info.</summary>
	Sdd1State* _state;

	/// <summary>ROM memory handlers from the cartridge.</summary>
	vector<unique_ptr<IMemoryHandler>>* _romHandlers;

	/// <summary>Mask for handler array indexing.</summary>
	uint32_t _handlerMask;

	/// <summary>S-DD1 decompression engine.</summary>
	Sdd1Decomp _decompressor;

	/// <summary>
	/// Gets the appropriate ROM handler for an address.
	/// </summary>
	/// <param name="addr">Address to map.</param>
	/// <returns>Pointer to memory handler.</returns>
	IMemoryHandler* GetHandler(uint32_t addr);

	/// <summary>
	/// Applies ROM mirroring to address.
	/// </summary>
	/// <param name="addr">Original address.</param>
	/// <returns>Mirrored address.</returns>
	uint16_t ProcessRomMirroring(uint32_t addr);

public:
	/// <summary>
	/// Creates a new S-DD1 MMC instance.
	/// </summary>
	/// <param name="state">Reference to S-DD1 state.</param>
	/// <param name="cart">Cartridge for ROM access.</param>
	Sdd1Mmc(Sdd1State& state, BaseCartridge* cart);

	/// <summary>
	/// Reads ROM data (may be decompressed).
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>ROM data byte.</returns>
	uint8_t ReadRom(uint32_t addr);

	/// <summary>
	/// Reads from MMC address space with decompression.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Data byte (possibly decompressed).</returns>
	virtual uint8_t Read(uint32_t addr) override;

	/// <summary>
	/// Peeks memory without triggering decompression.
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Raw ROM data byte.</returns>
	virtual uint8_t Peek(uint32_t addr) override;

	/// <summary>
	/// Peeks a block of ROM data.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer for data.</param>
	virtual void PeekBlock(uint32_t addr, uint8_t* output) override;

	/// <summary>
	/// Writes to MMC (ignored - ROM is read-only).
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Value to write (ignored).</param>
	virtual void Write(uint32_t addr, uint8_t value) override;

	/// <summary>
	/// Gets absolute address for debugging.
	/// </summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address info.</returns>
	virtual AddressInfo GetAbsoluteAddress(uint32_t address) override;

	/// <summary>
	/// Serializes MMC state for save states.
	/// </summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override;
};