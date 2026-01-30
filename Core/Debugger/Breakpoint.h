#pragma once
#include "pch.h"

enum class CpuType : uint8_t;
enum class MemoryType;
struct AddressInfo;
enum class BreakpointType;
enum class BreakpointTypeFlags;
enum class MemoryOperationType;
struct MemoryOperationInfo;

/// <summary>
/// Debugger breakpoint with conditional expression support.
/// </summary>
/// <remarks>
/// Breakpoint types:
/// - Execute: Break before instruction execution
/// - Read: Break on memory read
/// - Write: Break on memory write
/// - ReadWrite: Break on read or write
///
/// Conditional breakpoints:
/// - Expression evaluated when breakpoint triggered
/// - Only break if expression returns true
/// - Access to registers, memory, CPU state
/// - Examples: "A == $ff", "X > 10", "[WRAM+$100] != 0"
///
/// Address range:
/// - Single address: _startAddr == _endAddr
/// - Range: _startAddr to _endAddr (inclusive)
/// - Supports ROM, RAM, register ranges
///
/// Mark events:
/// - _markEvent: True to mark in event viewer (don't break)
/// - Used for logging/visualization without pausing
///
/// Dummy operations:
/// - _ignoreDummyOperations: Skip invalid/dummy reads
/// - CPU dummy reads (6502/65816 addressing modes)
/// - DMA dummy cycles
/// </remarks>
class Breakpoint {
public:
	/// <summary>
	/// Check if breakpoint matches memory operation.
	/// </summary>
	/// <typeparam name="accessWidth">Access width in bytes (1/2/4)</typeparam>
	/// <param name="opInfo">Memory operation info (read/write, address)</param>
	/// <param name="info">Address info (memory type, absolute address)</param>
	/// <returns>True if breakpoint triggered</returns>
	/// <remarks>
	/// Matching logic:
	/// 1. Check address in range [_startAddr, _endAddr]
	/// 2. Check memory type matches
	/// 3. Check operation type (read/write/execute)
	/// 4. Check dummy operation filter
	/// 5. Evaluate conditional expression (if present)
	///
	/// Access width:
	/// - 1 byte: LDA, STA (8-bit)
	/// - 2 bytes: LDA (16-bit mode), word access
	/// - 4 bytes: DMA transfers, 32-bit ARM access
	/// </remarks>
	template <uint8_t accessWidth = 1>
	bool Matches(MemoryOperationInfo& opInfo, AddressInfo& info);

	/// <summary>
	/// Check if breakpoint has specific type flag.
	/// </summary>
	/// <param name="type">Breakpoint type to check</param>
	bool HasBreakpointType(BreakpointType type);

	/// <summary>
	/// Get conditional expression string.
	/// </summary>
	string GetCondition();

	/// <summary>
	/// Check if breakpoint has condition.
	/// </summary>
	bool HasCondition();

	/// <summary>
	/// Get unique breakpoint ID.
	/// </summary>
	uint32_t GetId();

	/// <summary>
	/// Get CPU type this breakpoint applies to.
	/// </summary>
	CpuType GetCpuType();

	/// <summary>
	/// Check if breakpoint enabled.
	/// </summary>
	bool IsEnabled();

	/// <summary>
	/// Check if breakpoint marks events (doesn't break).
	/// </summary>
	bool IsMarked();

	/// <summary>
	/// Check if breakpoint allowed for operation type.
	/// </summary>
	/// <param name="opType">Memory operation type</param>
	bool IsAllowedForOpType(MemoryOperationType opType);

private:
	uint32_t _id;                ///< Unique ID
	CpuType _cpuType;            ///< Target CPU
	MemoryType _memoryType;      ///< Target memory type (ROM/RAM/etc.)
	BreakpointTypeFlags _type;   ///< Breakpoint type flags
	int32_t _startAddr;          ///< Range start (-1 for any address)
	int32_t _endAddr;            ///< Range end
	bool _enabled;               ///< Enabled flag
	bool _markEvent;             ///< Mark in event viewer (don't break)
	bool _ignoreDummyOperations; ///< Ignore dummy reads/writes
	char _condition[1000];       ///< Conditional expression
};