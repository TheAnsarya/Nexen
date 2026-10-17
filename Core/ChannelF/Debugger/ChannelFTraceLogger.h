#pragma once
#include "pch.h"
#include "Debugger/BaseTraceLogger.h"
#include "ChannelF/ChannelFTypes.h"

class DisassemblyInfo;
class Debugger;
class ChannelFConsole;

class ChannelFTraceLogger : public BaseTraceLogger<ChannelFTraceLogger, ChannelFCpuState> {
private:
	ChannelFConsole* _console = nullptr;

protected:
	RowDataType GetFormatTagType(string& tag) override;

public:
	ChannelFTraceLogger(Debugger* debugger, IDebugger* cpuDebugger, ChannelFConsole* console);

	void GetTraceRow(string& output, ChannelFCpuState& cpuState, TraceLogPpuState& ppuState, DisassemblyInfo& disassemblyInfo);
	void LogPpuState();

	__forceinline uint32_t GetProgramCounter(ChannelFCpuState& state) { return state.PC0; }
	__forceinline uint64_t GetCycleCount(ChannelFCpuState& state) { return state.CycleCount; }
	__forceinline uint8_t GetStackPointer([[maybe_unused]] ChannelFCpuState& state) { return 0; }
};
