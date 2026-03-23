#include "pch.h"
#include "Atari2600/Debugger/Atari2600TraceLogger.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600Types.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/Debugger.h"
#include "Debugger/DebugTypes.h"
#include "Utilities/HexUtilities.h"

Atari2600TraceLogger::Atari2600TraceLogger(Debugger* debugger, IDebugger* cpuDebugger, Atari2600Console* console) : BaseTraceLogger(debugger, cpuDebugger, CpuType::Atari2600) {
	_console = console;
}

RowDataType Atari2600TraceLogger::GetFormatTagType(string& tag) {
	if (tag == "A") {
		return RowDataType::A;
	} else if (tag == "X") {
		return RowDataType::X;
	} else if (tag == "Y") {
		return RowDataType::Y;
	} else if (tag == "P") {
		return RowDataType::PS;
	} else if (tag == "SP") {
		return RowDataType::SP;
	} else if (tag == "INTIM") {
		return RowDataType::RiotTimer;
	} else if (tag == "RIF") {
		return RowDataType::RiotInterrupt;
	} else {
		return RowDataType::Text;
	}
}

void Atari2600TraceLogger::GetTraceRow(string& output, Atari2600CpuState& cpuState, TraceLogPpuState& ppuState, DisassemblyInfo& disassemblyInfo) {
	// 6502 status flags: NV-BDIZC
	constexpr char activeStatusLetters[8] = {'N', 'V', '-', 'B', 'D', 'I', 'Z', 'C'};
	constexpr char inactiveStatusLetters[8] = {'n', 'v', '-', 'b', 'd', 'i', 'z', 'c'};

	for (RowPart& rowPart : _rowParts) {
		switch (rowPart.DataType) {
			case RowDataType::A:
				WriteIntValue(output, cpuState.A, rowPart);
				break;
			case RowDataType::X:
				WriteIntValue(output, cpuState.X, rowPart);
				break;
			case RowDataType::Y:
				WriteIntValue(output, cpuState.Y, rowPart);
				break;
			case RowDataType::SP:
				WriteIntValue(output, cpuState.SP, rowPart);
				break;
			case RowDataType::PS:
				GetStatusFlag(activeStatusLetters, inactiveStatusLetters, output, cpuState.PS, rowPart);
				break;
			default:
				ProcessSharedTag(rowPart, output, cpuState, ppuState, disassemblyInfo);
				break;
		}
	}
}

void Atari2600TraceLogger::LogPpuState() {
	Atari2600TiaState tiaState = _console->GetTiaState();
	Atari2600RiotState riotState = _console->GetRiotState();
	_ppuState[_currentPos] = {
		(uint32_t)tiaState.TotalColorClocks,
		(uint32_t)tiaState.ColorClock,
		(int32_t)tiaState.Scanline,
		_console->GetFrameCount(),
		(uint32_t)(riotState.Timer & 0xFFFF),
		riotState.InterruptFlag ? 1u : 0u
	};
}
