#include "pch.h"
#include "Debugger/ExpressionEvaluator.h"
#include "SNES/SnesCpuTypes.h"
#include "SNES/SnesPpuTypes.h"
#include "SNES/Debugger/SnesDebugger.h"

TokenSpan ExpressionEvaluator::GetSnesTokens() {
	static constexpr std::array<TokenEntry, 22> tokens = {{
		{"a",                      EvalValues::RegA},
		{"cycle",                  EvalValues::PpuCycle},
		{"d",                      EvalValues::RegD},
		{"db",                     EvalValues::RegDB},
		{"frame",                  EvalValues::PpuFrameCount},
		{"hclock",                 EvalValues::PpuHClock},
		{"irq",                    EvalValues::Irq},
		{"nmi",                    EvalValues::Nmi},
		{"pc",                     EvalValues::RegPC},
		{"ps",                     EvalValues::RegPS},
		{"pscarry",                EvalValues::RegPS_Carry},
		{"psdecimal",              EvalValues::RegPS_Decimal},
		{"psindex",                EvalValues::RegPS_Index},
		{"psinterrupt",            EvalValues::RegPS_Interrupt},
		{"psmemory",               EvalValues::RegPS_Memory},
		{"psnegative",             EvalValues::RegPS_Negative},
		{"psoverflow",             EvalValues::RegPS_Overflow},
		{"pszero",                 EvalValues::RegPS_Zero},
		{"scanline",               EvalValues::PpuScanline},
		{"sp",                     EvalValues::RegSP},
		{"x",                      EvalValues::RegX},
		{"y",                      EvalValues::RegY},
	}};
	return tokens;
}

int64_t ExpressionEvaluator::GetSnesTokenValue(int64_t token, EvalResultType& resultType) {
	auto getPpuState = [this]() -> SnesPpuState {
		SnesPpuState ppu;
		((SnesDebugger*)_cpuDebugger)->GetPpuState(ppu);
		return ppu;
	};

	SnesCpuState& s = (SnesCpuState&)((SnesDebugger*)_cpuDebugger)->GetState();
	switch (token) {
		case EvalValues::RegA:
			return s.A;
		case EvalValues::RegX:
			return s.X;
		case EvalValues::RegY:
			return s.Y;
		case EvalValues::RegSP:
			return s.SP;
		case EvalValues::RegPS:
			return s.PS;
		case EvalValues::RegDB:
			return s.DBR;
		case EvalValues::RegD:
			return s.D;
		case EvalValues::RegPC:
			return (s.K << 16) | s.PC;
		case EvalValues::Nmi:
			return ReturnBool(s.NmiFlagCounter > 0 || s.NeedNmi, resultType);
		case EvalValues::Irq:
			return ReturnBool(s.IrqSource, resultType);
		case EvalValues::PpuFrameCount:
			return getPpuState().FrameCount;
		case EvalValues::PpuCycle:
			return getPpuState().Cycle;
		case EvalValues::PpuHClock:
			return getPpuState().HClock;
		case EvalValues::PpuScanline:
			return getPpuState().Scanline;

		case EvalValues::RegPS_Carry:
			return ReturnBool(s.PS & ProcFlags::Carry, resultType);
		case EvalValues::RegPS_Zero:
			return ReturnBool(s.PS & ProcFlags::Zero, resultType);
		case EvalValues::RegPS_Interrupt:
			return ReturnBool(s.PS & ProcFlags::IrqDisable, resultType);
		case EvalValues::RegPS_Memory:
			return ReturnBool(s.PS & ProcFlags::MemoryMode8, resultType);
		case EvalValues::RegPS_Index:
			return ReturnBool(s.PS & ProcFlags::IndexMode8, resultType);
		case EvalValues::RegPS_Decimal:
			return ReturnBool(s.PS & ProcFlags::Decimal, resultType);
		case EvalValues::RegPS_Overflow:
			return ReturnBool(s.PS & ProcFlags::Overflow, resultType);
		case EvalValues::RegPS_Negative:
			return ReturnBool(s.PS & ProcFlags::Negative, resultType);

		default:
			return 0;
	}
}