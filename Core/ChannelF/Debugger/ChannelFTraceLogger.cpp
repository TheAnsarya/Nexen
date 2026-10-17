#include "pch.h"
#include "ChannelF/Debugger/ChannelFTraceLogger.h"
#include "ChannelF/ChannelFConsole.h"
#include "ChannelF/ChannelFTypes.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/Debugger.h"
#include "Debugger/DebugTypes.h"
#include "Utilities/HexUtilities.h"

ChannelFTraceLogger::ChannelFTraceLogger(Debugger* debugger, IDebugger* cpuDebugger, ChannelFConsole* console) : BaseTraceLogger(debugger, cpuDebugger, CpuType::ChannelF) {
	_console = console;
}

RowDataType ChannelFTraceLogger::GetFormatTagType(string& tag) {
	if (tag == "A") {
		return RowDataType::A;
	} else if (tag == "W") {
		return RowDataType::PS;
	} else if (tag == "ISAR") {
		return RowDataType::I;
	} else if (tag == "PC1") {
		return RowDataType::K;
	} else if (tag == "DC0") {
		return RowDataType::X;
	} else if (tag == "DC1") {
		return RowDataType::Y;
	} else {
		return RowDataType::Text;
	}
}

void ChannelFTraceLogger::GetTraceRow(string& output, ChannelFCpuState& cpuState, TraceLogPpuState& ppuState, DisassemblyInfo& disassemblyInfo) {
	// F8 status flags in W register: ----SCZO (Sign, Carry, Zero, Overflow)
	constexpr char activeStatusLetters[8] = {'-', '-', '-', '-', 'S', 'C', 'Z', 'O'};
	constexpr char inactiveStatusLetters[8] = {'-', '-', '-', '-', 's', 'c', 'z', 'o'};

	for (RowPart& rowPart : _rowParts) {
		switch (rowPart.DataType) {
			case RowDataType::A:
				WriteIntValue(output, cpuState.A, rowPart);
				break;
			case RowDataType::PS:
				GetStatusFlag(activeStatusLetters, inactiveStatusLetters, output, cpuState.W, rowPart);
				break;
			case RowDataType::I:
				WriteIntValue(output, cpuState.ISAR, rowPart);
				break;
			case RowDataType::K:
				WriteIntValue(output, cpuState.PC1, rowPart);
				break;
			case RowDataType::X:
				WriteIntValue(output, cpuState.DC0, rowPart);
				break;
			case RowDataType::Y:
				WriteIntValue(output, cpuState.DC1, rowPart);
				break;
			default:
				ProcessSharedTag(rowPart, output, cpuState, ppuState, disassemblyInfo);
				break;
		}
	}
}

void ChannelFTraceLogger::LogPpuState() {
	// Channel F has no separate PPU — video is memory-mapped via I/O ports
	// Log minimal frame state
	_ppuState[_currentPos] = {
		0,  // Cycle
		0,  // HClock
		0,  // Scanline
		_console->GetFrameCount(),
		0,  // Aux0
		0   // Aux1
	};
}
