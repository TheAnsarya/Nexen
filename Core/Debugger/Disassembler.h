#pragma once
#include "pch.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/DebugUtilities.h"

class IConsole;
class Debugger;
class LabelManager;
class CodeDataLogger;
class MemoryDumper;
class DisassemblySearch;
class EmuSettings;
struct SnesCpuState;
enum class CpuType : uint8_t;

/// <summary>
/// Cached disassembly data for a memory type.
/// </summary>
struct DisassemblerSource {
	vector<DisassemblyInfo> Cache; ///< Disassembly cache (one entry per byte in memory type)
	uint32_t Size = 0;             ///< Cache size (matches memory type size)
};

/// <summary>
/// Generates and caches disassembly for all CPU types.
/// </summary>
/// <remarks>
/// Architecture:
/// - One disassembler shared across all CPUs
/// - Separate cache per memory type (ROM, RAM, SRAM, etc.)
/// - Cache stores DisassemblyInfo for each byte (opcode, operands, formatting)
///
/// Cache organization:
/// - _sources[]: One DisassemblerSource per memory type
/// - Cache[address]: DisassemblyInfo at that address
/// - Lazy initialization (built on first access)
/// - Invalidation on code modification
///
/// Disassembly output:
/// - Formatted assembly text with labels
/// - Byte code display
/// - Effective addresses
/// - CPU flags (M/X flags for 65816)
///
/// Performance optimizations:
/// - __forceinline GetDisassemblyInfo() for hot path
/// - Cached disassembly (parse once, display many times)
/// - Early exit for uninitialized addresses
///
/// Use cases:
/// - Debugger disassembly view
/// - Trace logger output formatting
/// - Breakpoint display (show instruction at breakpoint)
/// - Code search (find instruction patterns)
/// </remarks>
class Disassembler {
private:
	friend class DisassemblySearch;

	IConsole* _console;          ///< Console instance
	EmuSettings* _settings;      ///< Settings instance
	Debugger* _debugger;         ///< Main debugger
	LabelManager* _labelManager; ///< Label manager for symbol lookup
	MemoryDumper* _memoryDumper; ///< Memory dumper for byte reads

	DisassemblerSource _sources[DebugUtilities::GetMemoryTypeCount()] = {}; ///< Disassembly cache per memory type

	/// <summary>
	/// Initialize disassembly cache for memory type.
	/// </summary>
	/// <param name="type">Memory type to initialize</param>
	void InitSource(MemoryType type);

	/// <summary>
	/// Get disassembly cache for memory type.
	/// </summary>
	/// <param name="type">Memory type</param>
	/// <returns>Disassembler source with cache</returns>
	DisassemblerSource& GetSource(MemoryType type);

	/// <summary>
	/// Get formatted line data for address.
	/// </summary>
	/// <param name="result">Disassembly result</param>
	/// <param name="type">CPU type</param>
	/// <param name="memType">Memory type</param>
	/// <param name="data">Output code line data</param>
	void GetLineData(DisassemblyResult& result, CpuType type, MemoryType memType, CodeLineData& data);

	/// <summary>
	/// Find row in disassembly results matching address.
	/// </summary>
	/// <param name="rows">Disassembly results</param>
	/// <param name="address">Target address</param>
	/// <param name="returnFirstRow">True to return first row if no exact match</param>
	/// <returns>Matching row index or -1 if not found</returns>
	int32_t GetMatchingRow(vector<DisassemblyResult>& rows, uint32_t address, bool returnFirstRow);

	/// <summary>
	/// Disassemble entire bank.
	/// </summary>
	/// <param name="cpuType">CPU type</param>
	/// <param name="bank">Bank number</param>
	/// <returns>Disassembly results for bank</returns>
	vector<DisassemblyResult> Disassemble(CpuType cpuType, uint16_t bank);

	/// <summary>
	/// Get maximum bank number for CPU type.
	/// </summary>
	/// <param name="cpuType">CPU type</param>
	/// <returns>Max bank number</returns>
	uint16_t GetMaxBank(CpuType cpuType);

public:
	/// <summary>
	/// Constructor for disassembler.
	/// </summary>
	/// <param name="console">Console instance</param>
	/// <param name="debugger">Debugger instance</param>
	Disassembler(IConsole* console, Debugger* debugger);

	/// <summary>
	/// Build disassembly cache for address.
	/// </summary>
	/// <param name="addrInfo">Address info</param>
	/// <param name="cpuFlags">CPU flags (M/X for 65816)</param>
	/// <param name="type">CPU type</param>
	/// <returns>Cache size</returns>
	uint32_t BuildCache(AddressInfo& addrInfo, uint8_t cpuFlags, CpuType type);

	/// <summary>
	/// Reset program ROM cache.
	/// </summary>
	void ResetPrgCache();

	/// <summary>
	/// Invalidate cache at address.
	/// </summary>
	/// <param name="addrInfo">Address to invalidate</param>
	/// <param name="type">CPU type</param>
	void InvalidateCache(AddressInfo addrInfo, CpuType type);

	/// <summary>
	/// Get disassembly info for address (hot path).
	/// </summary>
	/// <param name="info">Address info</param>
	/// <param name="cpuAddress">CPU-relative address</param>
	/// <param name="cpuFlags">CPU flags</param>
	/// <param name="type">CPU type</param>
	/// <returns>Disassembly info</returns>
	/// <remarks>
	/// Inline for performance (called every instruction in trace logger).
	/// Returns cached DisassemblyInfo or initializes new one.
	/// </remarks>
	__forceinline DisassemblyInfo GetDisassemblyInfo(AddressInfo& info, uint32_t cpuAddress, uint8_t cpuFlags, CpuType type) {
		DisassemblyInfo disassemblyInfo;
		if (info.Address >= 0) {
			disassemblyInfo = GetSource(info.Type).Cache[info.Address];
		}

		if (!disassemblyInfo.IsInitialized()) {
			disassemblyInfo.Initialize(cpuAddress, cpuFlags, type, DebugUtilities::GetCpuMemoryType(type), _memoryDumper);
		}
		return disassemblyInfo;
	}

	/// <summary>
	/// Get formatted disassembly output for address range.
	/// </summary>
	/// <param name="type">CPU type</param>
	/// <param name="address">Start address</param>
	/// <param name="output">Output code lines (array)</param>
	/// <param name="rowCount">Number of rows to generate</param>
	/// <returns>Number of bytes covered</returns>
	uint32_t GetDisassemblyOutput(CpuType type, uint32_t address, CodeLineData output[], uint32_t rowCount);

	/// <summary>
	/// Get address at row offset from start address.
	/// </summary>
	/// <param name="type">CPU type</param>
	/// <param name="address">Start address</param>
	/// <param name="rowOffset">Row offset (positive or negative)</param>
	/// <returns>Address at row offset or -1 if out of range</returns>
	int32_t GetDisassemblyRowAddress(CpuType type, uint32_t address, int32_t rowOffset);
};