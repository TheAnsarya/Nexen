#include "pch.h"
#include "Debugger/ExpressionEvaluator.h"
#include "Gameboy/Debugger/GbDebugger.h"
#include "Gameboy/GbTypes.h"

TokenSpan ExpressionEvaluator::GetGameboyTokens() {
	static constexpr std::array<TokenEntry, 17> tokens = {{
		{"a",                      EvalValues::RegA},
		{"af",                     EvalValues::RegAF},
		{"b",                      EvalValues::RegB},
		{"bc",                     EvalValues::RegBC},
		{"c",                      EvalValues::RegC},
		{"cycle",                  EvalValues::PpuCycle},
		{"d",                      EvalValues::RegD},
		{"de",                     EvalValues::RegDE},
		{"e",                      EvalValues::RegE},
		{"f",                      EvalValues::RegF},
		{"frame",                  EvalValues::PpuFrameCount},
		{"h",                      EvalValues::RegH},
		{"hl",                     EvalValues::RegHL},
		{"l",                      EvalValues::RegL},
		{"pc",                     EvalValues::RegPC},
		{"scanline",               EvalValues::PpuScanline},
		{"sp",                     EvalValues::RegSP},
	}};
	return tokens;
}

int64_t ExpressionEvaluator::GetGameboyTokenValue(int64_t token, EvalResultType& resultType) {
	auto ppu = [this]() -> GbPpuState {
		GbPpuState ppu;
		((GbDebugger*)_cpuDebugger)->GetPpuState(ppu);
		return ppu;
	};

	GbCpuState& s = (GbCpuState&)((GbDebugger*)_cpuDebugger)->GetState();
	switch (token) {
		case EvalValues::RegA:
			return s.A;
		case EvalValues::RegB:
			return s.B;
		case EvalValues::RegC:
			return s.C;
		case EvalValues::RegD:
			return s.D;
		case EvalValues::RegE:
			return s.E;
		case EvalValues::RegF:
			return s.Flags;
		case EvalValues::RegH:
			return s.H;
		case EvalValues::RegL:
			return s.L;
		case EvalValues::RegAF:
			return (s.A << 8) | s.Flags;
		case EvalValues::RegBC:
			return (s.B << 8) | s.C;
		case EvalValues::RegDE:
			return (s.D << 8) | s.E;
		case EvalValues::RegHL:
			return (s.H << 8) | s.L;
		case EvalValues::RegSP:
			return s.SP;
		case EvalValues::RegPC:
			return s.PC;

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