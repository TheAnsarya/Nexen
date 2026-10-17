#include "pch.h"
#include "Debugger/ExpressionEvaluator.h"
#include "SMS/Debugger/SmsDebugger.h"
#include "SMS/SmsTypes.h"

TokenSpan ExpressionEvaluator::GetSmsTokens() {
	static constexpr std::array<TokenEntry, 37> tokens = {{
		{"a",                      EvalValues::RegA},
		{"a'",                     EvalValues::RegAltA},
		{"addressreg",             EvalValues::SmsVdpAddressReg},
		{"af",                     EvalValues::RegAF},
		{"af'",                    EvalValues::RegAltAF},
		{"b",                      EvalValues::RegB},
		{"b'",                     EvalValues::RegAltB},
		{"bc",                     EvalValues::RegBC},
		{"bc'",                    EvalValues::RegAltBC},
		{"c",                      EvalValues::RegC},
		{"c'",                     EvalValues::RegAltC},
		{"codereg",                EvalValues::SmsVdpCodeReg},
		{"cycle",                  EvalValues::PpuCycle},
		{"d",                      EvalValues::RegD},
		{"d'",                     EvalValues::RegAltD},
		{"de",                     EvalValues::RegDE},
		{"de'",                    EvalValues::RegAltDE},
		{"e",                      EvalValues::RegE},
		{"e'",                     EvalValues::RegAltE},
		{"f",                      EvalValues::RegF},
		{"f'",                     EvalValues::RegAltF},
		{"frame",                  EvalValues::PpuFrameCount},
		{"h",                      EvalValues::RegH},
		{"h'",                     EvalValues::RegAltH},
		{"hl",                     EvalValues::RegHL},
		{"hl'",                    EvalValues::RegAltHL},
		{"i",                      EvalValues::RegI},
		{"ix",                     EvalValues::RegIX},
		{"iy",                     EvalValues::RegIY},
		{"l",                      EvalValues::RegL},
		{"l'",                     EvalValues::RegAltL},
		{"pc",                     EvalValues::RegPC},
		{"r",                      EvalValues::RegR},
		{"scanline",               EvalValues::PpuScanline},
		{"sp",                     EvalValues::RegSP},
		{"spritecollision",        EvalValues::SpriteCollision},
		{"spriteoverflow",         EvalValues::SpriteOverflow},
	}};
	return tokens;
}

int64_t ExpressionEvaluator::GetSmsTokenValue(int64_t token, EvalResultType& resultType) {
	auto ppu = [this]() -> SmsVdpState {
		SmsVdpState ppu;
		((SmsDebugger*)_cpuDebugger)->GetPpuState(ppu);
		return ppu;
	};

	SmsCpuState& s = (SmsCpuState&)((SmsDebugger*)_cpuDebugger)->GetState();
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
		case EvalValues::RegAltA:
			return s.AltA;
		case EvalValues::RegAltB:
			return s.AltB;
		case EvalValues::RegAltC:
			return s.AltC;
		case EvalValues::RegAltD:
			return s.AltD;
		case EvalValues::RegAltE:
			return s.AltE;
		case EvalValues::RegAltF:
			return s.AltFlags;
		case EvalValues::RegAltH:
			return s.AltH;
		case EvalValues::RegAltL:
			return s.AltL;
		case EvalValues::RegAltAF:
			return (s.AltA << 8) | s.AltFlags;
		case EvalValues::RegAltBC:
			return (s.AltB << 8) | s.AltC;
		case EvalValues::RegAltDE:
			return (s.AltD << 8) | s.AltE;
		case EvalValues::RegAltHL:
			return (s.AltH << 8) | s.AltL;
		case EvalValues::RegIX:
			return (s.IXH << 8) | s.IXL;
		case EvalValues::RegIY:
			return (s.IYH << 8) | s.IYL;
		case EvalValues::RegI:
			return s.I;
		case EvalValues::RegR:
			return s.R;
		case EvalValues::RegSP:
			return s.SP;
		case EvalValues::RegPC:
			return s.PC;

		case EvalValues::SmsVdpAddressReg:
			return ppu().AddressReg;
		case EvalValues::SmsVdpCodeReg:
			return ppu().CodeReg;

		case EvalValues::SpriteCollision:
			return ReturnBool(ppu().SpriteCollision, resultType);
		case EvalValues::SpriteOverflow:
			return ReturnBool(ppu().SpriteOverflow, resultType);

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