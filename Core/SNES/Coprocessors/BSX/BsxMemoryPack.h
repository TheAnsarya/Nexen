#pragma once
#include "pch.h"
#include <memory>
#include "SNES/RamHandler.h"
#include "SNES/IMemoryHandler.h"
#include "Utilities/ISerializable.h"

class BsxMemoryPackHandler;
class SnesConsole;

/// <summary>
/// BS-X Memory Pack emulation.
/// Emulates the removable flash memory cartridges used for storing Satellaview content.
/// Supports read/write/erase operations compatible with flash memory protocols.
/// </summary>
/// <remarks>
/// Memory Packs were flash memory cartridges that stored downloaded content:
/// - Available in 8Mbit (1MB) sizes, expandable to 32Mbit
/// - Used standard flash memory command protocols
/// - Could store multiple downloaded games
/// - Battery-backed for data retention
/// 
/// Flash command protocol:
/// - Write $AA to $5555, $55 to $2AAA to initiate command
/// - Various commands: Read, Program, Sector Erase, Chip Erase
/// - Vendor/device ID read for identification
/// 
/// The emulation optionally persists flash data to disk for save state.
/// </remarks>
class BsxMemoryPack : public ISerializable {
private:
	/// <summary>Reference to SNES console.</summary>
	SnesConsole* _console = nullptr;

	/// <summary>Original data for reset.</summary>
	vector<uint8_t> _orgData;

	/// <summary>Flash memory data buffer.</summary>
	std::unique_ptr<uint8_t[]> _data;

	/// <summary>Size of flash memory in bytes.</summary>
	uint32_t _dataSize = 0;

	/// <summary>Memory handlers for memory map integration.</summary>
	vector<unique_ptr<IMemoryHandler>> _handlers;

	/// <summary>Calculated size code for status register.</summary>
	uint8_t _calculatedSize = 0x0C;

	/// <summary>True to save flash data to disk.</summary>
	bool _persistFlash = false;

	/// <summary>Command Status Register enable flag.</summary>
	bool _enableCsr = false;

	/// <summary>Extended Status Register enable flag.</summary>
	bool _enableEsr = false;

	/// <summary>Vendor Info mode enable flag.</summary>
	bool _enableVendorInfo = false;

	/// <summary>Single byte write mode flag.</summary>
	bool _writeByte = false;

	/// <summary>Current flash command sequence.</summary>
	uint16_t _command = 0;

public:
	/// <summary>
	/// Creates a new Memory Pack instance.
	/// </summary>
	/// <param name="console">Reference to SNES console.</param>
	/// <param name="data">Initial flash data.</param>
	/// <param name="persistFlash">True to save changes to disk.</param>
	BsxMemoryPack(SnesConsole* console, vector<uint8_t>& data, bool persistFlash);

	/// <summary>Destructor.</summary>
	virtual ~BsxMemoryPack();

	/// <summary>Saves flash data to battery file.</summary>
	void SaveBattery();

	/// <summary>Serializes Memory Pack state for save states.</summary>
	void Serialize(Serializer& s) override;

	/// <summary>
	/// Processes a flash command write.
	/// </summary>
	/// <param name="value">Command/data byte.</param>
	/// <param name="page">Page address (for sector operations).</param>
	void ProcessCommand(uint8_t value, uint32_t page);

	/// <summary>Resets Memory Pack to initial state.</summary>
	void Reset();

	/// <summary>
	/// Gets memory handlers for memory map integration.
	/// </summary>
	/// <returns>Reference to handler vector.</returns>
	vector<unique_ptr<IMemoryHandler>>& GetMemoryHandlers();

	/// <summary>
	/// Gets flash data pointer for debugging.
	/// </summary>
	/// <returns>Pointer to flash data.</returns>
	uint8_t* DebugGetMemoryPack();

	/// <summary>
	/// Gets flash data size for debugging.
	/// </summary>
	/// <returns>Size in bytes.</returns>
	uint32_t DebugGetMemoryPackSize();

	/// <summary>
	/// Memory Pack page handler for memory mapping.
	/// Provides read/write access to a single 4KB page with flash protocol support.
	/// </summary>
	class BsxMemoryPackHandler : public RamHandler {
		/// <summary>Reference to parent Memory Pack.</summary>
		BsxMemoryPack* _memPack;

		/// <summary>Page number this handler manages.</summary>
		uint32_t _page;

	public:
		/// <summary>
		/// Creates a new Memory Pack page handler.
		/// </summary>
		/// <param name="memPack">Reference to parent Memory Pack.</param>
		/// <param name="offset">Byte offset (divided by page size for page number).</param>
		BsxMemoryPackHandler(BsxMemoryPack* memPack, uint32_t offset);

		/// <summary>
		/// Reads from Memory Pack page.
		/// </summary>
		/// <param name="addr">Address to read.</param>
		/// <returns>Data byte or status register.</returns>
		uint8_t Read(uint32_t addr) override;

		/// <summary>
		/// Writes to Memory Pack page (flash command or data).
		/// </summary>
		/// <param name="addr">Address to write.</param>
		/// <param name="value">Command/data byte.</param>
		void Write(uint32_t addr, uint8_t value) override;
	};
};
