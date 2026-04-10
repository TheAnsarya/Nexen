#pragma once

#include "pch.h"
#include <memory>

#include "NES/INesMemoryHandler.h"
#include "NES/OpenBusHandler.h"
#include "NES/InternalRamHandler.h"
#include "Shared/MemoryOperationType.h"
#include "Utilities/ISerializable.h"

class BaseMapper;
class CheatManager;
class Emulator;
class NesConsole;

/// <summary>
/// NES memory manager - handles CPU memory bus and I/O device routing.
/// Manages the 64KB address space with mapper-specific bank switching.
/// </summary>
/// <remarks>
/// **Memory Map:**
/// - $0000-$07FF: 2KB internal RAM (mirrored to $1FFF)
/// - $2000-$2007: PPU registers (mirrored every 8 bytes to $3FFF)
/// - $4000-$4017: APU and I/O registers
/// - $4018-$FFFF: Cartridge space (PRG-ROM/RAM, mapper registers)
///
/// **Handler System:**
/// - Each address range is mapped to an INesMemoryHandler
/// - Handlers provide Read() and Write() implementations
/// - Supports handler override for expansion audio, FDS, etc.
///
/// **Open Bus:**
/// - Reads from unmapped addresses return last bus value
/// - Some mappers use open bus for bank switching detection
///
/// **Special Cases:**
/// - FamicomBox uses 8KB internal RAM instead of 2KB
/// - Expansion audio chips register handlers at $4020+
/// </remarks>
class NesMemoryManager : public ISerializable {
private:
	static constexpr int CpuMemorySize = 0x10000;  ///< 64KB address space
	static constexpr int NesInternalRamSize = 0x800;   ///< Standard 2KB RAM
	static constexpr int FamicomBoxInternalRamSize = 0x2000;  ///< FamicomBox 8KB RAM

	Emulator* _emu = nullptr;           ///< Emulator instance
	CheatManager* _cheatManager = nullptr;  ///< Game Genie/cheat support
	NesConsole* _console = nullptr;     ///< Parent console
	BaseMapper* _mapper = nullptr;      ///< Cartridge mapper

	std::unique_ptr<uint8_t[]> _internalRam;  ///< Internal work RAM
	uint32_t _internalRamSize = 0;            ///< RAM size (2KB or 8KB)
	uint8_t* _internalRamPtr = nullptr;       ///< Direct pointer for fast-path reads
	uint16_t _internalRamMask = 0;            ///< Address mask (0x7FF or 0x1FFF)

	OpenBusHandler _openBusHandler = {};  ///< Handler for unmapped addresses
	unique_ptr<INesMemoryHandler> _internalRamHandler;  ///< Handler for $0000-$1FFF
	std::unique_ptr<INesMemoryHandler*[]> _ramReadHandlers;   ///< Read handler per address
	std::unique_ptr<INesMemoryHandler*[]> _ramWriteHandlers;  ///< Write handler per address

	/// <summary>Initialize handler array for an address range.</summary>
	/// <param name="memoryHandlers">Handler array to populate</param>
	/// <param name="handler">Handler to assign</param>
	/// <param name="addresses">Address list (or nullptr for all)</param>
	/// <param name="allowOverride">Allow overwriting existing handlers</param>
	void InitializeMemoryHandlers(INesMemoryHandler** memoryHandlers, INesMemoryHandler* handler, vector<uint16_t>* addresses, bool allowOverride);

protected:
	void Serialize(Serializer& s) override;

public:
	/// <summary>Construct memory manager with mapper.</summary>
	/// <param name="console">Parent NES console</param>
	/// <param name="mapper">Cartridge mapper for PRG mapping</param>
	NesMemoryManager(NesConsole* console, BaseMapper* mapper);
	virtual ~NesMemoryManager();

	/// <summary>Reset memory state.</summary>
	/// <param name="softReset">True for soft reset, false for hard reset</param>
	void Reset(bool softReset);

	/// <summary>Register I/O device at its default addresses.</summary>
	/// <param name="handler">Device handler with GetAddresses()</param>
	void RegisterIODevice(INesMemoryHandler* handler);

	/// <summary>Register write handler for address range.</summary>
	void RegisterWriteHandler(INesMemoryHandler* handler, uint32_t start, uint32_t end);

	/// <summary>Register read handler for address range.</summary>
	void RegisterReadHandler(INesMemoryHandler* handler, uint32_t start, uint32_t end);

	/// <summary>Unregister I/O device from all addresses.</summary>
	void UnregisterIODevice(INesMemoryHandler* handler);

	/// <summary>Debug read without side effects.</summary>
	uint8_t DebugRead(uint16_t addr);

	/// <summary>Debug read 16-bit word (little-endian).</summary>
	uint16_t DebugReadWord(uint16_t addr);

	/// <summary>Debug write with optional side effect suppression.</summary>
	void DebugWrite(uint16_t addr, uint8_t value, bool disableSideEffects = true);

	/// <summary>Get pointer to internal RAM.</summary>
	[[nodiscard]] uint8_t* GetInternalRam();

	/// <summary>CPU memory read.</summary>
	/// <param name="addr">16-bit address</param>
	/// <param name="operationType">Type of read (normal, operand, etc.)</param>
	/// <returns>Byte at address</returns>
	uint8_t Read(uint16_t addr, MemoryOperationType operationType = MemoryOperationType::Read);

	/// <summary>CPU memory write.</summary>
	/// <param name="addr">16-bit address</param>
	/// <param name="value">Byte to write</param>
	/// <param name="operationType">Type of write</param>
	void Write(uint16_t addr, uint8_t value, MemoryOperationType operationType);

	/// <summary>Get open bus value (last value on bus).</summary>
	/// <param name="mask">Bits to return (others are 0)</param>
	[[nodiscard]] uint8_t GetOpenBus(uint8_t mask = 0xFF);

	/// <summary>Get internal open bus (PPU/APU)</summary>
	[[nodiscard]] uint8_t GetInternalOpenBus(uint8_t mask = 0xFF);
};
