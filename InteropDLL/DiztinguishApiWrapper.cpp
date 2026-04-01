#include "Common.h"
#include "Core/Shared/Emulator.h"
#include "Core/Shared/DebuggerRequest.h"
#include "Core/Debugger/Debugger.h"
#include "Core/SNES/Debugger/SnesDebugger.h"
#include "Core/Debugger/DiztinguishBridge.h"
#include <functional>

extern unique_ptr<Emulator> _emu;

template<typename T>
T WrapSnesDebuggerCall(std::function<T(SnesDebugger* snesDebugger)> func)
{
	DebuggerRequest dbgRequest = _emu->GetDebugger(true);
	if(dbgRequest.GetDebugger()) {
		SnesDebugger* snesDebugger = dbgRequest.GetDebugger()->GetDebugger<CpuType::Snes, SnesDebugger>();
		if(snesDebugger) {
			return func(snesDebugger);
		}
	}
	return {};
}

template<>
void WrapSnesDebuggerCall<void>(std::function<void(SnesDebugger* snesDebugger)> func)
{
	DebuggerRequest dbgRequest = _emu->GetDebugger(true);
	if(dbgRequest.GetDebugger()) {
		SnesDebugger* snesDebugger = dbgRequest.GetDebugger()->GetDebugger<CpuType::Snes, SnesDebugger>();
		if(snesDebugger) {
			func(snesDebugger);
		}
	}
}

extern "C" {
	// DiztinGUIsh Bridge Server Control
	DllExport bool __stdcall DiztinguishApi_StartServer(uint16_t port) {
		// Only supported for SNES console
		if(_emu->GetConsoleType() != ConsoleType::Snes) {
			return false;
		}
		
		// Ensure debugger is initialized first
		_emu->InitDebugger();
		
		return WrapSnesDebuggerCall<bool>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->StartServer(port) : false;
		});
	}

	DllExport void __stdcall DiztinguishApi_StopServer() {
		WrapSnesDebuggerCall<void>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			if(bridge) {
				bridge->StopServer();
			}
		});
	}

	DllExport bool __stdcall DiztinguishApi_IsServerRunning() {
		// Only supported for SNES console
		if(_emu->GetConsoleType() != ConsoleType::Snes) {
			return false;
		}
		
		return WrapSnesDebuggerCall<bool>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->IsServerRunning() : false;
		});
	}

	DllExport bool __stdcall DiztinguishApi_IsClientConnected() {
		return WrapSnesDebuggerCall<bool>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->IsClientConnected() : false;
		});
	}

	DllExport uint16_t __stdcall DiztinguishApi_GetServerPort() {
		return WrapSnesDebuggerCall<uint16_t>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->GetPort() : 0;
		});
	}

	// DiztinGUIsh Bridge Statistics
	DllExport uint64_t __stdcall DiztinguishApi_GetMessagesSent() {
		return WrapSnesDebuggerCall<uint64_t>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->GetMessagesSent() : 0;
		});
	}

	DllExport uint64_t __stdcall DiztinguishApi_GetMessagesReceived() {
		return WrapSnesDebuggerCall<uint64_t>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->GetMessagesReceived() : 0;
		});
	}

	DllExport uint64_t __stdcall DiztinguishApi_GetBytesSent() {
		return WrapSnesDebuggerCall<uint64_t>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->GetBytesSent() : 0;
		});
	}

	DllExport uint64_t __stdcall DiztinguishApi_GetBytesReceived() {
		return WrapSnesDebuggerCall<uint64_t>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->GetBytesReceived() : 0;
		});
	}

	DllExport uint64_t __stdcall DiztinguishApi_GetConnectionDuration() {
		return WrapSnesDebuggerCall<uint64_t>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->GetConnectionDuration() : 0;
		});
	}

	DllExport double __stdcall DiztinguishApi_GetBandwidthKBps() {
		return WrapSnesDebuggerCall<double>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->GetBandwidthKBps() : 0.0;
		});
	}

	// Diagnostic functions for streaming health
	DllExport uint32_t __stdcall DiztinguishApi_GetCurrentFrame() {
		return WrapSnesDebuggerCall<uint32_t>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->GetCurrentFrame() : 0;
		});
	}

	DllExport uint32_t __stdcall DiztinguishApi_GetTraceBufferSize() {
		return WrapSnesDebuggerCall<uint32_t>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->GetTraceBufferSize() : 0;
		});
	}

	DllExport bool __stdcall DiztinguishApi_IsConfigReceived() {
		return WrapSnesDebuggerCall<bool>([&](SnesDebugger* dbg) {
			DiztinguishBridge* bridge = dbg->GetDiztinguishBridge();
			return bridge ? bridge->IsConfigReceived() : false;
		});
	}
}