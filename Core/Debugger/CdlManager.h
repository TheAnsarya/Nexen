#pragma once
#include "pch.h"
#include "Shared/MemoryType.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/DebugUtilities.h"

class CodeDataLogger;
class Debugger;
class Disassembler;

/// <summary>
/// Code/Data Logger (CDL) manager for all memory types.
/// </summary>
/// <remarks>
/// Architecture:
/// - Manages array of CodeDataLogger instances (one per MemoryType)
/// - Tracks which ROM bytes are code vs data
/// - Saves/loads CDL files (.cdl format)
///
/// CDL tracking:
/// - Code: Bytes executed as instructions
/// - Data: Bytes accessed as data (read/write)
/// - JumpTarget: Branch/jump destinations
/// - SubEntryPoint: Subroutine entry points
///
/// CDL operations:
/// - GetCdlData(): Get CDL flags for address range
/// - SetCdlData(): Set CDL flags (load from file)
/// - MarkBytesAs(): Manually mark bytes (code/data/etc)
/// - GetCdlStatistics(): Get code/data byte counts
///
/// CDL file format:
/// - Binary file, one byte per ROM byte
/// - Each byte contains CdlFlags bitfield
/// - Used for disassembly quality (verified code vs data)
///
/// Use cases:
/// - Improve disassembly accuracy (skip data as code)
/// - ROM coverage analysis (how much code executed)
/// - Function discovery (find subroutines)
/// - ROM stripping (remove unused code)
/// </remarks>
class CdlManager {
private:
	CodeDataLogger* _codeDataLoggers[DebugUtilities::GetMemoryTypeCount()] = {}; ///< CDL instances per MemoryType
	Debugger* _debugger = nullptr;                                               ///< Debugger instance
	Disassembler* _disassembler = nullptr;                                       ///< Disassembler instance

public:
	/// <summary>
	/// Constructor for CDL manager.
	/// </summary>
	CdlManager(Debugger* debugger, Disassembler* disassembler);

	/// <summary>
	/// Get CDL data for address range.
	/// </summary>
	/// <param name="offset">Start offset</param>
	/// <param name="length">Length in bytes</param>
	/// <param name="memoryType">Memory type</param>
	/// <param name="cdlData">Output CDL flags array</param>
	void GetCdlData(uint32_t offset, uint32_t length, MemoryType memoryType, uint8_t* cdlData);

	/// <summary>
	/// Get CDL flags for single address.
	/// </summary>
	/// <param name="memType">Memory type</param>
	/// <param name="addr">Address</param>
	/// <returns>CDL flags (-1 if not tracked)</returns>
	int16_t GetCdlFlags(MemoryType memType, uint32_t addr);

	/// <summary>
	/// Set CDL data (load from file).
	/// </summary>
	void SetCdlData(MemoryType memType, uint8_t* cdlData, uint32_t length);

	/// <summary>
	/// Manually mark bytes with flags.
	/// </summary>
	void MarkBytesAs(MemoryType memType, uint32_t start, uint32_t end, uint8_t flags);

	/// <summary>
	/// Get CDL statistics (code/data byte counts).
	/// </summary>
	CdlStatistics GetCdlStatistics(MemoryType memType);

	/// <summary>
	/// Get list of subroutine addresses.
	/// </summary>
	/// <param name="memType">Memory type</param>
	/// <param name="functions">Output function address array</param>
	/// <param name="maxSize">Maximum array size</param>
	/// <returns>Number of functions found</returns>
	uint32_t GetCdlFunctions(MemoryType memType, uint32_t functions[], uint32_t maxSize);

	/// <summary>
	/// Reset CDL data (clear all flags).
	/// </summary>
	void ResetCdl(MemoryType memType);

	/// <summary>
	/// Load CDL from file.
	/// </summary>
	void LoadCdlFile(MemoryType memType, char* cdlFile);

	/// <summary>
	/// Save CDL to file.
	/// </summary>
	void SaveCdlFile(MemoryType memType, char* cdlFile);

	/// <summary>
	/// Register CodeDataLogger for memory type (platform-specific).
	/// </summary>
	void RegisterCdl(MemoryType memType, CodeDataLogger* cdl);

	/// <summary>
	/// Refresh disassembly cache after CDL changes.
	/// </summary>
	void RefreshCodeCache(bool resetPrgCache = true);

	/// <summary>
	/// Get CodeDataLogger for memory type.
	/// </summary>
	CodeDataLogger* GetCodeDataLogger(MemoryType memType);
};
