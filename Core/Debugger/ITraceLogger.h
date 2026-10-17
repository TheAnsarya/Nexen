#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"

/// <summary>
/// Trace log row representing single instruction execution.
/// </summary>
/// <remarks>
/// Stored in circular buffer (TraceLogger).
/// Used for instruction history, profiling, debugging.
/// </remarks>
struct TraceRow {
	uint32_t ProgramCounter; ///< PC at instruction start
	CpuType Type;            ///< CPU type (for multi-CPU systems)
	uint8_t ByteCode[8];     ///< Machine code bytes (instruction + operands)
	uint8_t ByteCodeSize;    ///< Number of bytes in ByteCode
	uint32_t LogSize;        ///< Size of LogOutput string
	char LogOutput[500];     ///< Formatted log text (registers, disassembly, etc.)
};

/// <summary>
/// Trace logger configuration options.
/// </summary>
struct TraceLoggerOptions {
	bool Enabled;         ///< Enable trace logging
	bool IndentCode;      ///< Indent subroutine calls
	bool UseLabels;       ///< Use labels instead of addresses in output
	char Condition[1000]; ///< Conditional logging expression (only log if true)
	char Format[1000];    ///< Custom log format string
};

/// <summary>
/// Interface for instruction trace logging.
/// Implemented by BaseTraceLogger (platform-agnostic base) and CPU-specific subclasses.
/// </summary>
/// <remarks>
/// Trace logger:
/// - Records every instruction executed
/// - Circular buffer (overwrites oldest entries)
/// - Conditional logging (filter by expression)
/// - Custom formatting (registers, flags, memory, etc.)
///
/// Use cases:
/// - Instruction history ("how did I get here?")
/// - Performance profiling (cycle counts, hot spots)
/// - Regression testing (compare execution traces)
/// - Lua script debugging
/// - TAS replay analysis
///
/// Format string variables:
/// - {PC}: Program counter
/// - {A}, {X}, {Y}: Registers
/// - {SP}, {PS}: Stack pointer, processor status
/// - {Cycles}: CPU cycle count
/// - {Disassembly}: Instruction disassembly
/// - Platform-specific: {PPU}, {Scanline}, etc.
///
/// Thread model:
/// - Called from emulation thread (every instruction)
/// - Lock-free circular buffer for performance
/// </remarks>
class ITraceLogger {
protected:
	bool _enabled = false; ///< Trace logging enabled flag

public:
	static uint64_t NextRowId; ///< Global row ID counter (for sorting)

	/// <summary>
	/// Get row ID for trace offset.
	/// </summary>
	/// <param name="offset">Offset from latest trace (0 = most recent)</param>
	/// <returns>Row ID for history indexing</returns>
	virtual int64_t GetRowId(uint32_t offset) = 0;

	/// <summary>
	/// Get execution trace row.
	/// </summary>
	/// <param name="row">Output trace row</param>
	/// <param name="offset">Offset from latest trace (0 = most recent)</param>
	virtual void GetExecutionTrace(TraceRow& row, uint32_t offset) = 0;

	/// <summary>
	/// Clear trace history buffer.
	/// </summary>
	virtual void Clear() = 0;

	/// <summary>
	/// Set trace logger options.
	/// </summary>
	/// <param name="options">Logger configuration</param>
	virtual void SetOptions(TraceLoggerOptions options) = 0;

	/// <summary>
	/// Check if trace logging enabled.
	/// </summary>
	/// <returns>True if enabled (inline for performance)</returns>
	/// <remarks>
	/// Called every instruction - must be fast (inline).
	/// </remarks>
	__forceinline bool IsEnabled() { return _enabled; }
};
