#include "pch.h"
#include <cctype>
#include "Genesis/Debugger/GenesisTraceLogger.h"
#include "Genesis/GenesisVdp.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/Debugger.h"
#include "Debugger/DebugTypes.h"

GenesisTraceLogger::GenesisTraceLogger(Debugger* debugger, IDebugger* cpuDebugger, GenesisVdp* vdp) : BaseTraceLogger(debugger, cpuDebugger, CpuType::Genesis) {
	_vdp = vdp;
}

RowDataType GenesisTraceLogger::GetFormatTagType(string& tag) {
	if (tag == "SP") {
		return RowDataType::SP;
	} else if (tag == "SR") {
		return RowDataType::SR;
	}

	if (tag.size() == 2 && (tag[0] == 'D' || tag[0] == 'A') && std::isdigit((unsigned char)tag[1])) {
		int reg = tag[1] - '0';
		if (reg >= 0 && reg <= 7) {
			return tag[0] == 'D' ? (RowDataType)((int)RowDataType::R0 + reg) : (RowDataType)((int)RowDataType::R8 + reg);
		}
	}

	return RowDataType::Text;
}

void GenesisTraceLogger::GetTraceRow(string& output, GenesisM68kState& cpuState, TraceLogPpuState& ppuState, DisassemblyInfo& disassemblyInfo) {
	constexpr char activeStatusLetters[16] = {'T', '-', 'S', '-', '-', 'I', 'I', 'I', '-', '-', '-', 'X', 'N', 'Z', 'V', 'C'};
	constexpr char inactiveStatusLetters[16] = {'t', '-', 's', '-', '-', 'i', 'i', 'i', '-', '-', '-', 'x', 'n', 'z', 'v', 'c'};

	for (RowPart& rowPart : _rowParts) {
		switch (rowPart.DataType) {
			case RowDataType::R0:
			case RowDataType::R1:
			case RowDataType::R2:
			case RowDataType::R3:
			case RowDataType::R4:
			case RowDataType::R5:
			case RowDataType::R6:
			case RowDataType::R7: {
				int index = (int)rowPart.DataType - (int)RowDataType::R0;
				WriteIntValue(output, cpuState.D[index], rowPart);
				break;
			}

			case RowDataType::R8:
			case RowDataType::R9:
			case RowDataType::R10:
			case RowDataType::R11:
			case RowDataType::R12:
			case RowDataType::R13:
			case RowDataType::R14:
			case RowDataType::R15: {
				int index = (int)rowPart.DataType - (int)RowDataType::R8;
				WriteIntValue(output, cpuState.A[index], rowPart);
				break;
			}

			case RowDataType::SP:
				WriteIntValue(output, cpuState.A[7], rowPart);
				break;

			case RowDataType::SR:
				GetStatusFlag(activeStatusLetters, inactiveStatusLetters, output, cpuState.SR, rowPart, 16);
				break;

			default:
				ProcessSharedTag(rowPart, output, cpuState, ppuState, disassemblyInfo);
				break;
		}
	}
}

void GenesisTraceLogger::LogPpuState() {
	GenesisVdpState vdpState = _vdp->GetState();
	_ppuState[_currentPos] = {
		(uint32_t)(_cpuState[_currentPos].CycleCount & 0xFFFFFFFF),
		vdpState.HCounter,
		(int32_t)vdpState.VCounter,
		vdpState.FrameCount,
		0,
		0
	};
}
