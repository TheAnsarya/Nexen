#pragma once
#include "pch.h"
#include "Debugger/BaseTraceLogger.h"
#include "Lynx/LynxTypes.h"

class DisassemblyInfo;
class Debugger;
class LynxMikey;

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
