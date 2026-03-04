#pragma once
#include "pch.h"
#include "SNES/Coprocessors/BaseCoprocessor.h"
#include "SNES/Coprocessors/SDD1/Sdd1Types.h"

class SnesConsole;
class Sdd1Mmc;

/// <summary>
/// S-DD1 (Super Data Decompression 1) coprocessor emulation.
/// A hardware decompression chip developed by Ricoh for Nintendo.
/// Uses a proprietary compression algorithm based on Golomb coding
/// and probability estimation to decompress graphics data.
/// Used in: Star Ocean, Street Fighter Alpha 2.
/// </summary>
/// <remarks>
/// The S-DD1 intercepts DMA transfers and decompresses data on-the-fly.
/// Games store compressed graphics in ROM and the S-DD1 transparently
/// decompresses them during VRAM transfers, allowing much larger
/// games than would otherwise fit on available ROM chips.
/// Algorithm reverse-engineered by Andreas Naive in 2003.
/// </remarks>
class Sdd1 : public BaseCoprocessor {
private:
	/// <summary>Complete S-DD1 state.</summary>
	Sdd1State _state;

	/// <summary>Memory mapping controller with integrated decompressor.</summary>
	unique_ptr<Sdd1Mmc> _sdd1Mmc;

	/// <summary>Handler for CPU register access passthrough.</summary>
	IMemoryHandler* _cpuRegisterHandler;

public:
	/// <summary>
	/// Creates a new S-DD1 coprocessor instance.
	/// </summary>
	/// <param name="console">SNES console instance.</param>
	Sdd1(SnesConsole* console);

	/// <summary>
	/// Destructor defined out-of-line in .cpp where Sdd1Mmc is complete,
	/// required for unique_ptr<Sdd1Mmc> with forward-declared type.
	/// </summary>
	~Sdd1();

	/// <summary>
	/// Serializes S-DD1 state for save states.
	/// </summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override;

	/// <summary>
	/// Reads from S-DD1 register space.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Register value.</returns>
	uint8_t Read(uint32_t addr) override;

	/// <summary>
	/// Peeks S-DD1 register without side effects.
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Register value.</returns>
	uint8_t Peek(uint32_t addr) override;

	/// <summary>
	/// Peeks a block of S-DD1 memory.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer for data.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override;

	/// <summary>
	/// Writes to S-DD1 register space.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Value to write.</param>
	void Write(uint32_t addr, uint8_t value) override;

	/// <summary>
	/// Gets absolute address for debugging.
	/// </summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address info.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t address) override;

	/// <summary>Resets S-DD1 to initial power-on state.</summary>
	void Reset() override;
};
