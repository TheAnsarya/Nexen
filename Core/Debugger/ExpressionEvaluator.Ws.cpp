#include "pch.h"
#include "Debugger/ExpressionEvaluator.h"
#include "WS/Debugger/WsDebugger.h"
#include "WS/WsTypes.h"

TokenSpan ExpressionEvaluator::GetWsTokens() {
	static constexpr std::array<TokenEntry, 26> tokens = {{
		{"ah",                     EvalValues::RegAH},
		{"al",                     EvalValues::RegAL},
		{"ax",                     EvalValues::RegAX},
		{"bh",                     EvalValues::RegBH},
		{"bl",                     EvalValues::RegBL},
		{"bp",                     EvalValues::RegBP},
		{"bx",                     EvalValues::RegBX},
		{"ch",                     EvalValues::RegCH},
		{"cl",                     EvalValues::RegCL},
		{"cs",                     EvalValues::RegCS},
		{"cx",                     EvalValues::RegCX},
		{"cycle",                  EvalValues::PpuCycle},
		{"dh",                     EvalValues::RegDH},
		{"di",                     EvalValues::RegDI},
		{"dl",                     EvalValues::RegDL},
		{"ds",                     EvalValues::RegDS},
		{"dx",                     EvalValues::RegDX},
		{"es",                     EvalValues::RegES},
		{"f",                      EvalValues::RegF},
		{"frame",                  EvalValues::PpuFrameCount},
		{"ip",                     EvalValues::RegIP},
		{"pc",                     EvalValues::RegPC},
		{"scanline",               EvalValues::PpuScanline},
		{"si",                     EvalValues::RegSI},
		{"sp",                     EvalValues::RegSP},
		{"ss",                     EvalValues::RegSS},
	}};
	return tokens;
}

int64_t ExpressionEvaluator::GetWsTokenValue(int64_t token, EvalResultType& resultType) {
	auto ppu = [this]() -> WsPpuState {
		WsPpuState ppu;
		((WsDebugger*)_cpuDebugger)->GetPpuState(ppu);
		return ppu;
	};

	WsCpuState& s = (WsCpuState&)((WsDebugger*)_cpuDebugger)->GetState();
	switch (token) {
		case EvalValues::RegAX:
			return s.AX;
		case EvalValues::RegBX:
			return s.BX;
		case EvalValues::RegCX:
			return s.CX;
		case EvalValues::RegDX:
			return s.DX;
		case EvalValues::RegAL:
			return s.AX & 0xFF;
		case EvalValues::RegBL:
			return s.BX & 0xFF;
		case EvalValues::RegCL:
			return s.CX & 0xFF;
		case EvalValues::RegDL:
			return s.DX & 0xFF;
		case EvalValues::RegAH:
			return s.AX >> 8;
		case EvalValues::RegBH:
			return s.BX >> 8;
		case EvalValues::RegCH:
			return s.CX >> 8;
		case EvalValues::RegDH:
			return s.DX >> 8;

		case EvalValues::RegCS:
			return s.CS;
		case EvalValues::RegDS:
			return s.DS;
		case EvalValues::RegES:
			return s.ES;
		case EvalValues::RegSS:
			return s.SS;
		case EvalValues::RegSI:
			return s.SI;
		case EvalValues::RegDI:
			return s.DI;
		case EvalValues::RegBP:
			return s.BP;
		case EvalValues::RegIP:
			return s.IP;

		case EvalValues::RegF:
			return s.Flags.Get();

		case EvalValues::RegSP:
			return s.SP;
		case EvalValues::RegPC:
			return (s.CS << 4) + s.IP;

		case EvalValues::PpuFrameCount:
			return ppu().FrameCount;
		case EvalValues::PpuCycle:
			return ppu().Cycle;
		case EvalValues::PpuScanline:
			return ppu().Scanline;

		default:
			return 0;
	}
}