#pragma once
#include "pch.h"
#include "SNES/IMemoryHandler.h"
#include "SNES/Coprocessors/BSX/BsxStream.h"
#include "Utilities/ISerializable.h"

class SnesConsole;
class SnesMemoryManager;

/// <summary>
/// BS-X Satellaview base unit emulation.
/// Emulates the satellite data receiver that connected to the SNES via the expansion port.
/// Provided downloadable game content and live radio broadcasts in Japan (1995-2000).
/// </summary>
/// <remarks>
/// The Satellaview was a satellite modem peripheral for the SNES in Japan.
/// It received data broadcasts from St.GIGA satellite radio, allowing:
/// - Downloadable games (stored in memory packs)
/// - Live audio broadcasts (SoundLink)
/// - Time-limited content ("SoundLink Games")
/// - News and entertainment content
/// 
/// Register map ($2188-$219F):
/// - $2188-$218D: Stream data access
/// - $218E-$218F: Stream status/control
/// - $2190-$2197: Date/time registers
/// - $2198-$219F: Additional control
/// 
/// This emulation simulates the data streams using pre-recorded BS-X data files.
/// </remarks>
class BsxSatellaview : public IMemoryHandler, public ISerializable {
private:
	/// <summary>Handler for B-bus ($2100-$21FF) access forwarding.</summary>
	IMemoryHandler* _bBusHandler;

	/// <summary>Reference to SNES console.</summary>
	SnesConsole* _console;

	/// <summary>Reference to memory manager for timing.</summary>
	SnesMemoryManager* _memoryManager;

	/// <summary>Data stream channels (2 channels supported).</summary>
	BsxStream _stream[2];

	/// <summary>Stream selection register.</summary>
	uint8_t _streamReg;

	/// <summary>External output register.</summary>
	uint8_t _extOutput;

	/// <summary>Custom date override (-1 for real time).</summary>
	int64_t _customDate;

	/// <summary>Previous master clock value for timing.</summary>
	uint64_t _prevMasterClock;

	/// <summary>Processes elapsed clock cycles for stream timing.</summary>
	void ProcessClocks();

public:
	/// <summary>
	/// Creates a new Satellaview instance.
	/// </summary>
	/// <param name="console">Reference to SNES console.</param>
	/// <param name="bBusHandler">Handler for B-bus access.</param>
	BsxSatellaview(SnesConsole* console, IMemoryHandler* bBusHandler);

	/// <summary>Resets Satellaview state.</summary>
	void Reset();

	/// <summary>
	/// Reads from Satellaview registers.
	/// </summary>
	/// <param name="addr">Address to read ($2188-$219F).</param>
	/// <returns>Register data.</returns>
	uint8_t Read(uint32_t addr) override;

	/// <summary>
	/// Peeks Satellaview without side effects.
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
	/// Writes to Satellaview registers.
	/// </summary>
	/// <param name="addr">Address to write ($2188-$219F).</param>
	/// <param name="value">Data to write.</param>
	void Write(uint32_t addr, uint8_t value) override;

	/// <summary>
	/// Gets absolute address for debugging.
	/// </summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address info.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t address) override;

	/// <summary>Serializes Satellaview state for save states.</summary>
	void Serialize(Serializer& s) override;
};