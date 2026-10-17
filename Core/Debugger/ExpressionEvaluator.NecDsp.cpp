#include "pch.h"
#include "Debugger/ExpressionEvaluator.h"
#include "SNES/Coprocessors/DSP/NecDspTypes.h"
#include "SNES/Debugger/NecDspDebugger.h"

TokenSpan ExpressionEvaluator::GetNecDspTokens() {
	static constexpr std::array<TokenEntry, 14> tokens = {{
		{"a",                      EvalValues::RegA},
		{"b",                      EvalValues::RegB},
		{"dp",                     EvalValues::RegDP},
		{"dr",                     EvalValues::RegDR},
		{"k",                      EvalValues::RegK},
		{"l",                      EvalValues::RegL},
		{"m",                      EvalValues::RegM},
		{"n",                      EvalValues::RegN},
		{"pc",                     EvalValues::RegPC},
		{"rp",                     EvalValues::RegRP},
		{"sp",                     EvalValues::RegSP},
		{"sr",                     EvalValues::RegSR},
		{"tr",                     EvalValues::RegTR},
		{"trb",                    EvalValues::RegTRB},
	}};
	return tokens;
}

int64_t ExpressionEvaluator::GetNecDspTokenValue(int64_t token, EvalResultType& resultType) {
	NecDspState& s = (NecDspState&)((NecDspDebugger*)_cpuDebugger)->GetState();
	switch (token) {
		case EvalValues::RegA:
			return s.A;
		case EvalValues::RegB:
			return s.B;
		case EvalValues::RegTR:
			return s.TR;
		case EvalValues::RegTRB:
			return s.TRB;
		case EvalValues::RegRP:
			return s.RP;
		case EvalValues::RegDP:
			return s.DP;
		case EvalValues::RegDR:
			return s.DR;
		case EvalValues::RegSR:
			return s.SR;
		case EvalValues::RegK:
			return s.K;
		case EvalValues::RegL:
			return s.L;
		case EvalValues::RegM:
			return s.M;
		case EvalValues::RegN:
			return s.N;
		case EvalValues::RegSP:
			return s.SP;
		case EvalValues::RegPC:
			return s.PC;
		default:
			return 0;
	}
}