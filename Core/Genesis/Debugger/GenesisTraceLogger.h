#pragma once
#include "pch.h"
#include "Debugger/BaseTraceLogger.h"
#include "Genesis/GenesisTypes.h"

class DisassemblyInfo;
class Debugger;
class GenesisVdp;

class GenesisTraceLogger : public BaseTraceLogger<GenesisTraceLogger, GenesisM68kState> {
private:
	GenesisVdp* _vdp = nullptr;

protected:
	RowDataType GetFormatTagType(string& tag) override;

public:
	GenesisTraceLogger(Debugger* debugger, IDebugger* cpuDebugger, GenesisVdp* vdp);

	void GetTraceRow(string& output, GenesisM68kState& cpuState, TraceLogPpuState& ppuState, DisassemblyInfo& disassemblyInfo);
	void LogPpuState();

	__forceinline uint32_t GetProgramCounter(GenesisM68kState& state) { return state.PC & 0x00ffffff; }
	__forceinline uint64_t GetCycleCount(GenesisM68kState& state) { return state.CycleCount; }
	__forceinline uint8_t GetStackPointer(GenesisM68kState& state) { return (uint8_t)(state.A[7] & 0xFF); }
};
