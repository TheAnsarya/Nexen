#pragma once
#include "pch.h"
#include "Debugger/BaseTraceLogger.h"
#include "Lynx/LynxTypes.h"

class DisassemblyInfo;
class Debugger;
class LynxMikey;

/// <summary>
/// Trace logger for the Atari Lynx 65C02 CPU.
///
/// Logs per-instruction CPU state for debugging and TAS verification:
///   - Program counter (PC)
///   - Accumulator (A), X, Y registers
///   - Stack pointer (SP)
///   - Processor status flags (NV-BDIZC)
///   - Cycle count
///   - Current scanline and display cycle from Mikey
///
/// Output format matches other 6502-based cores (NES, PCE) for familiarity.
/// Supports all standard trace format tags:
///   [PC] [A] [X] [Y] [SP] [PS] [CYC] [SL] [Disassembly]
///
/// The trace log is essential for:
///   - Debugging game logic
///   - Verifying TAS movie determinism
///   - Comparing execution paths between versions
///   - Detecting desync issues in replays
///
/// References:
///   - ~docs/DEBUGGER.md (Section: Trace Logger)
///   - Core/PCE/Debugger/PceTraceLogger.h (reference implementation)
/// </summary>
class LynxTraceLogger : public BaseTraceLogger<LynxTraceLogger, LynxCpuState> {
private:
	LynxMikey* _mikey = nullptr;

protected:
	RowDataType GetFormatTagType(string& tag) override;

public:
	LynxTraceLogger(Debugger* debugger, IDebugger* cpuDebugger, LynxMikey* mikey);

	void GetTraceRow(string& output, LynxCpuState& cpuState, TraceLogPpuState& ppuState, DisassemblyInfo& disassemblyInfo);
	void LogPpuState();

	__forceinline uint32_t GetProgramCounter(LynxCpuState& state) { return state.PC; }
	__forceinline uint64_t GetCycleCount(LynxCpuState& state) { return state.CycleCount; }
	__forceinline uint8_t GetStackPointer(LynxCpuState& state) { return state.SP; }
};
