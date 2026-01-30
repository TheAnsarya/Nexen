#pragma once
#include "pch.h"
#include <unordered_map>
#include "Debugger/DebugTypes.h"
#include "Debugger/DebugUtilities.h"
#include "Shared/MemoryType.h"
#include "Utilities/SimpleLock.h"

class SnesMemoryManager;
class NesConsole;
class BaseCartridge;
class Spc;
class Gameboy;
class PceConsole;
class SmsConsole;
class GbaConsole;
class WsConsole;
class Emulator;
class Debugger;

/// <summary>
/// Single undo operation for memory modification.
/// </summary>
struct UndoEntry {
	MemoryType MemType;           ///< Memory type modified
	uint32_t StartAddress;        ///< Start address of modification
	vector<uint8_t> OriginalData; ///< Original data before modification
};

/// <summary>
/// Batch of undo operations (single user action).
/// </summary>
struct UndoBatch {
	vector<UndoEntry> Entries; ///< All memory modifications in this batch
};

/// <summary>
/// Provides debugger memory read/write access with undo support.
/// </summary>
/// <remarks>
/// Architecture:
/// - Central memory access for debugger (all CPUs, all memory types)
/// - Undo/redo stack for memory modifications
/// - Platform-specific memory handling (SNES, NES, GB, GBA, etc.)
/// - Side effect control (read without triggering hardware)
///
/// Memory types:
/// - CPU memory (PRG ROM, Work RAM, Save RAM, etc.)
/// - Video memory (VRAM, OAM, CGRAM/palette)
/// - Audio memory (APU RAM, SPC RAM, etc.)
/// - Cartridge memory (ROM banks, SRAM, etc.)
///
/// Undo functionality:
/// - _undoHistory: Stack of UndoBatch (one per user action)
/// - Each batch contains multiple UndoEntry (multi-byte edits)
/// - PerformUndo(): Restore original values from top batch
/// - Max undo depth configurable
///
/// Side effects:
/// - disableSideEffects=true: Read/write without hardware effects
/// - Use case: Memory viewer reads, expression evaluation
/// - disableSideEffects=false: Normal read/write with side effects
/// - Use case: Debugger "write" commands, patch application
///
/// Performance:
/// - Direct memory buffer access where possible
/// - Lock-protected undo stack (_undoLock)
/// - Cached memory size/support checks
///
/// Use cases:
/// - Memory viewer (display memory contents)
/// - Memory editor (modify memory with undo)
/// - Disassembler (read instruction bytes)
/// - Expression evaluator (peek/read memory)
/// - Cheat engine (set memory values)
/// </remarks>
class MemoryDumper {
private:
	Emulator* _emu = nullptr;                                           ///< Emulator instance
	Spc* _spc = nullptr;                                                ///< SNES SPC700 APU
	Gameboy* _gameboy = nullptr;                                        ///< Game Boy console
	SnesMemoryManager* _memoryManager = nullptr;                        ///< SNES memory manager
	NesConsole* _nesConsole = nullptr;                                  ///< NES console
	PceConsole* _pceConsole = nullptr;                                  ///< PC Engine console
	SmsConsole* _smsConsole = nullptr;                                  ///< Sega Master System console
	GbaConsole* _gbaConsole = nullptr;                                  ///< Game Boy Advance console
	WsConsole* _wsConsole = nullptr;                                    ///< WonderSwan console
	BaseCartridge* _cartridge = nullptr;                                ///< Active cartridge
	Debugger* _debugger = nullptr;                                      ///< Main debugger
	bool _isMemorySupported[DebugUtilities::GetMemoryTypeCount()] = {}; ///< Supported memory types

	SimpleLock _undoLock;          ///< Undo history lock
	deque<UndoBatch> _undoHistory; ///< Undo stack (most recent first)

	/// <summary>
	/// Internal memory read (platform-specific).
	/// </summary>
	/// <param name="memoryType">Memory type</param>
	/// <param name="address">Address</param>
	/// <param name="disableSideEffects">True to read without side effects</param>
	/// <returns>Byte value</returns>
	uint8_t InternalGetMemoryValue(MemoryType memoryType, uint32_t address, bool disableSideEffects = true);

	/// <summary>
	/// Internal memory write (platform-specific).
	/// </summary>
	/// <param name="memoryType">Memory type</param>
	/// <param name="startAddress">Start address</param>
	/// <param name="data">Data to write</param>
	/// <param name="length">Data length</param>
	/// <param name="disableSideEffects">True to write without side effects</param>
	/// <param name="undoAllowed">True to add to undo history</param>
	void InternalSetMemoryValues(MemoryType memoryType, uint32_t startAddress, uint8_t* data, uint32_t length, bool disableSideEffects, bool undoAllowed);

public:
	/// <summary>
	/// Constructor for memory dumper.
	/// </summary>
	/// <param name="debugger">Main debugger instance</param>
	MemoryDumper(Debugger* debugger);

	/// <summary>
	/// Get direct memory buffer pointer.
	/// </summary>
	/// <param name="type">Memory type</param>
	/// <returns>Memory buffer or nullptr if not supported</returns>
	uint8_t* GetMemoryBuffer(MemoryType type);

	/// <summary>
	/// Get memory size.
	/// </summary>
	/// <param name="type">Memory type</param>
	/// <returns>Memory size in bytes</returns>
	uint32_t GetMemorySize(MemoryType type);

	/// <summary>
	/// Get entire memory state (copy to buffer).
	/// </summary>
	/// <param name="type">Memory type</param>
	/// <param name="buffer">Output buffer (must be GetMemorySize() bytes)</param>
	void GetMemoryState(MemoryType type, uint8_t* buffer);

	/// <summary>
	/// Read single byte from memory.
	/// </summary>
	/// <param name="memoryType">Memory type</param>
	/// <param name="address">Address</param>
	/// <param name="disableSideEffects">True to read without side effects</param>
	/// <returns>Byte value</returns>
	uint8_t GetMemoryValue(MemoryType memoryType, uint32_t address, bool disableSideEffects = true);

	/// <summary>
	/// Read byte range from memory.
	/// </summary>
	/// <param name="memoryType">Memory type</param>
	/// <param name="start">Start address</param>
	/// <param name="end">End address (inclusive)</param>
	/// <param name="output">Output buffer</param>
	void GetMemoryValues(MemoryType memoryType, uint32_t start, uint32_t end, uint8_t* output);

	/// <summary>
	/// Read 16-bit word from memory (little-endian).
	/// </summary>
	/// <param name="memoryType">Memory type</param>
	/// <param name="address">Address</param>
	/// <param name="disableSideEffects">True to read without side effects</param>
	/// <returns>16-bit word</returns>
	uint16_t GetMemoryValue16(MemoryType memoryType, uint32_t address, bool disableSideEffects = true);

	/// <summary>
	/// Read 32-bit dword from memory (little-endian).
	/// </summary>
	/// <param name="memoryType">Memory type</param>
	/// <param name="address">Address</param>
	/// <param name="disableSideEffects">True to read without side effects</param>
	/// <returns>32-bit dword</returns>
	uint32_t GetMemoryValue32(MemoryType memoryType, uint32_t address, bool disableSideEffects = true);

	/// <summary>
	/// Write 16-bit word to memory (little-endian).
	/// </summary>
	/// <param name="memoryType">Memory type</param>
	/// <param name="address">Address</param>
	/// <param name="value">16-bit word</param>
	/// <param name="disableSideEffects">True to write without side effects</param>
	void SetMemoryValue16(MemoryType memoryType, uint32_t address, uint16_t value, bool disableSideEffects = true);

	/// <summary>
	/// Write 32-bit dword to memory (little-endian).
	/// </summary>
	/// <param name="memoryType">Memory type</param>
	/// <param name="address">Address</param>
	/// <param name="value">32-bit dword</param>
	/// <param name="disableSideEffects">True to write without side effects</param>
	void SetMemoryValue32(MemoryType memoryType, uint32_t address, uint32_t value, bool disableSideEffects);

	/// <summary>
	/// Write single byte to memory.
	/// </summary>
	/// <param name="memoryType">Memory type</param>
	/// <param name="address">Address</param>
	/// <param name="value">Byte value</param>
	/// <param name="disableSideEffects">True to write without side effects</param>
	void SetMemoryValue(MemoryType memoryType, uint32_t address, uint8_t value, bool disableSideEffects = true);

	/// <summary>
	/// Write byte array to memory.
	/// </summary>
	/// <param name="memoryType">Memory type</param>
	/// <param name="address">Start address</param>
	/// <param name="data">Data to write</param>
	/// <param name="length">Data length</param>
	void SetMemoryValues(MemoryType memoryType, uint32_t address, uint8_t* data, uint32_t length);

	/// <summary>
	/// Set entire memory state (copy from buffer).
	/// </summary>
	/// <param name="type">Memory type</param>
	/// <param name="buffer">Input buffer</param>
	/// <param name="length">Buffer length</param>
	void SetMemoryState(MemoryType type, uint8_t* buffer, uint32_t length);

	/// <summary>
	/// Check if undo history exists.
	/// </summary>
	/// <returns>True if undo available</returns>
	bool HasUndoHistory();

	/// <summary>
	/// Undo last memory modification batch.
	/// </summary>
	/// <remarks>
	/// Restores original values from top UndoBatch.
	/// Removes batch from undo history.
	/// </remarks>
	void PerformUndo();
};