#include "pch.h"
#include "Lynx/Debugger/LynxTraceLogger.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxTypes.h"
#include "Lynx/LynxMikey.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/Debugger.h"
#include "Debugger/DebugTypes.h"
#include "Utilities/HexUtilities.h"

LynxTraceLogger::LynxTraceLogger(Debugger* debugger, IDebugger* cpuDebugger, LynxMikey* mikey) : BaseTraceLogger(debugger, cpuDebugger, CpuType::Lynx) {
	_mikey = mikey;
}

RowDataType LynxTraceLogger::GetFormatTagType(string& tag) {
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
	} else {
		return RowDataType::Text;
	}
}

void LynxTraceLogger::GetTraceRow(string& output, LynxCpuState& cpuState, TraceLogPpuState& ppuState, DisassemblyInfo& disassemblyInfo) {
	// 65C02 status flags: NV-BDIZC
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

void LynxTraceLogger::LogPpuState() {
	LynxMikeyState& mikey = _mikey->GetState();
	_ppuState[_currentPos] = {
		0,                          // hClock (Lynx doesn't have a traditional h-counter)
		0,                          // hClockAlternate
		mikey.CurrentScanline,      // scanline
		_mikey->GetFrameCount()     // frameCount
	};
}
