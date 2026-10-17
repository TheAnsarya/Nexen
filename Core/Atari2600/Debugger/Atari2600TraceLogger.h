#pragma once
#include "pch.h"
#include "Debugger/BaseTraceLogger.h"
#include "Atari2600/Atari2600Types.h"

class DisassemblyInfo;
class Debugger;
class Atari2600Console;

class Atari2600TraceLogger : public BaseTraceLogger<Atari2600TraceLogger, Atari2600CpuState> {
private:
	Atari2600Console* _console = nullptr;

protected:
	RowDataType GetFormatTagType(string& tag) override;

public:
	Atari2600TraceLogger(Debugger* debugger, IDebugger* cpuDebugger, Atari2600Console* console);

	void GetTraceRow(string& output, Atari2600CpuState& cpuState, TraceLogPpuState& ppuState, DisassemblyInfo& disassemblyInfo);
	void LogPpuState();

	__forceinline uint32_t GetProgramCounter(Atari2600CpuState& state) { return state.PC; }
	__forceinline uint64_t GetCycleCount(Atari2600CpuState& state) { return state.CycleCount; }
	__forceinline uint8_t GetStackPointer(Atari2600CpuState& state) { return state.SP; }
};
