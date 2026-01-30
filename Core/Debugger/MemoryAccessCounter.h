#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/DebugUtilities.h"
#include "Shared/MemoryType.h"

class Debugger;
class SnesMemoryManager;
class Spc;
class SnesConsole;
class Sa1;
class Gsu;
class Cx4;
class Gameboy;

/// <summary>
/// Access counters and timestamps for a memory address.
/// </summary>
struct AddressCounters {
	uint64_t ReadStamp;    ///< Last read timestamp (master clock)
	uint64_t WriteStamp;   ///< Last write timestamp (master clock)
	uint64_t ExecStamp;    ///< Last execute timestamp (master clock)
	uint32_t ReadCounter;  ///< Total read count
	uint32_t WriteCounter; ///< Total write count
	uint32_t ExecCounter;  ///< Total execute count
};

/// <summary>
/// Result of memory read operation.
/// </summary>
enum class ReadResult {
	Normal,          ///< Normal read (address previously written)
	FirstUninitRead, ///< First read of uninitialized address
	UninitRead       ///< Subsequent read of uninitialized address
};

/// <summary>
/// Tracks memory access counts and timestamps for debugger.
/// </summary>
/// <remarks>
/// Purpose:
/// - Count reads, writes, executions per address
/// - Track last access timestamp for each operation type
/// - Detect uninitialized memory reads
/// - Generate memory heatmaps (frequently accessed addresses)
///
/// Access tracking:
/// - ProcessMemoryRead(): Increment read counter, update timestamp
/// - ProcessMemoryWrite(): Increment write counter, update timestamp
/// - ProcessMemoryExec(): Increment execute counter, update timestamp
///
/// Uninitialized read detection:
/// - FirstUninitRead: First read before any write
/// - UninitRead: Subsequent reads before write
/// - _enableBreakOnUninitRead: Break on first uninit read
///
/// Data structure:
/// - _counters[memType][address]: AddressCounters for each byte
/// - One vector per memory type (ROM, RAM, VRAM, etc.)
/// - Memory allocated on demand
///
/// Template ProcessMemory functions:
/// - accessWidth: 1/2/4 bytes (compile-time optimization)
/// - Updates counters for all bytes in access width
///
/// Use cases:
/// - Memory viewer heatmap (color by access frequency)
/// - Detect unused ROM space (ExecCounter == 0)
/// - Find uninitialized variables (FirstUninitRead)
/// - Performance analysis (most accessed addresses)
/// </remarks>
class MemoryAccessCounter {
private:
	vector<AddressCounters> _counters[DebugUtilities::GetMemoryTypeCount()]; ///< Access counters per memory type

	Debugger* _debugger = nullptr;         ///< Main debugger instance
	bool _enableBreakOnUninitRead = false; ///< Break on uninitialized read

public:
	/// <summary>
	/// Constructor for memory access counter.
	/// </summary>
	/// <param name="debugger">Main debugger instance</param>
	MemoryAccessCounter(Debugger* debugger);

	/// <summary>
	/// Process memory read operation.
	/// </summary>
	/// <typeparam name="accessWidth">Access width (1, 2, or 4 bytes)</typeparam>
	/// <param name="addressInfo">Address information</param>
	/// <param name="masterClock">Master clock timestamp</param>
	/// <returns>Read result (normal or uninitialized)</returns>
	/// <remarks>
	/// Uninitialized read detection:
	/// - If WriteStamp == 0 and ReadStamp == 0: FirstUninitRead
	/// - If WriteStamp == 0 and ReadStamp > 0: UninitRead
	/// - Else: Normal
	/// </remarks>
	template <uint8_t accessWidth = 1>
	ReadResult ProcessMemoryRead(AddressInfo& addressInfo, uint64_t masterClock);

	/// <summary>
	/// Process memory write operation.
	/// </summary>
	/// <typeparam name="accessWidth">Access width (1, 2, or 4 bytes)</typeparam>
	/// <param name="addressInfo">Address information</param>
	/// <param name="masterClock">Master clock timestamp</param>
	template <uint8_t accessWidth = 1>
	void ProcessMemoryWrite(AddressInfo& addressInfo, uint64_t masterClock);

	/// <summary>
	/// Process memory execute operation.
	/// </summary>
	/// <typeparam name="accessWidth">Access width (1, 2, or 4 bytes)</typeparam>
	/// <param name="addressInfo">Address information</param>
	/// <param name="masterClock">Master clock timestamp</param>
	template <uint8_t accessWidth = 1>
	void ProcessMemoryExec(AddressInfo& addressInfo, uint64_t masterClock);

	/// <summary>
	/// Reset all access counters and timestamps.
	/// </summary>
	void ResetCounts();

	/// <summary>
	/// Get access counters for address range.
	/// </summary>
	/// <param name="offset">Start offset</param>
	/// <param name="length">Range length</param>
	/// <param name="memoryType">Memory type</param>
	/// <param name="counts">Output counters array (must be length elements)</param>
	void GetAccessCounts(uint32_t offset, uint32_t length, MemoryType memoryType, AddressCounters counts[]);
};