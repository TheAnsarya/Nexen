#include "pch.h"
#include "Debugger/Debugger.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/MemoryDumper.h"
#include "Debugger/MemoryAccessCounter.h"
#include "Debugger/CodeDataLogger.h"
#include "Debugger/Disassembler.h"
#include "Debugger/DisassemblySearch.h"
#include "Debugger/BreakpointManager.h"
#include "Debugger/PpuTools.h"
#include "Debugger/DebugBreakHelper.h"
#include "Debugger/LabelManager.h"
#include "Debugger/ScriptManager.h"
#include "Mcp/McpHookManager.h"
#include "Debugger/ScriptHost.h"
#include "Debugger/CallstackManager.h"
#include "Debugger/ExpressionEvaluator.h"
#include "Debugger/BaseEventManager.h"
#include "Debugger/TraceLogFileSaver.h"
#include "Debugger/CdlManager.h"
#include "Debugger/ITraceLogger.h"
#include "Debugger/DebuggerDispatchUtils.h"
#include "SNES/SnesCpuTypes.h"
#include "SNES/SpcTypes.h"
#include "SNES/Coprocessors/SA1/Sa1Types.h"
#include "SNES/Coprocessors/GSU/GsuTypes.h"
#include "SNES/Coprocessors/CX4/Cx4Types.h"
#include "SNES/Coprocessors/DSP/NecDspTypes.h"
#include "SNES/Coprocessors/ST018/ArmV3Types.h"
#include "SNES/Debugger/SnesDebugger.h"
#include "SNES/Debugger/SpcDebugger.h"
#include "SNES/Debugger/GsuDebugger.h"
#include "SNES/Debugger/St018Debugger.h"
#include "SNES/Debugger/NecDspDebugger.h"
#include "SNES/Debugger/Cx4Debugger.h"
#include "NES/Debugger/NesDebugger.h"
#include "NES/NesTypes.h"
#include "Gameboy/Debugger/GbDebugger.h"
#include "Gameboy/GbTypes.h"
#include "PCE/Debugger/PceDebugger.h"
#include "PCE/PceTypes.h"
#include "SMS/Debugger/SmsDebugger.h"
#include "SMS/SmsTypes.h"
#include "GBA/Debugger/GbaDebugger.h"
#include "GBA/GbaTypes.h"
#include "WS/Debugger/WsDebugger.h"
#include "WS/WsTypes.h"
#include "Lynx/LynxTypes.h"
#include "Lynx/Debugger/LynxDebugger.h"
#include "Atari2600/Atari2600Types.h"
#include "Atari2600/Debugger/Atari2600Debugger.h"
#include "ChannelF/ChannelFTypes.h"
#include "ChannelF/Debugger/ChannelFDebugger.h"
#include "Genesis/GenesisTypes.h"
#include "Genesis/Debugger/GenesisDebugger.h"
#include "Shared/BaseControlManager.h"
#include "Shared/EmuSettings.h"
#include "Shared/Audio/SoundMixer.h"
#include "Shared/NotificationManager.h"
#include "Shared/MessageManager.h"
#include "Shared/BaseState.h"
#include "Shared/Emulator.h"
#include "Shared/Interfaces/IConsole.h"
#include "Shared/MemoryOperationType.h"
#include "Shared/EventType.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/FolderUtilities.h"
#include "Utilities/Patches/IpsPatcher.h"
#include "Utilities/PlatformUtilities.h"

// Row ID counter for trace log entries (global across all trace loggers)
uint64_t ITraceLogger::NextRowId = 0;

// Initialize debugger with all debugging subsystems
Debugger::Debugger(Emulator* emu, IConsole* console) {
	_executionStopped = true;  // Start paused for debugger attachment

	_emu = emu;
	_console = console;
	_settings = _emu->GetSettings();

	_consoleType = _emu->GetConsoleType();

	// Get list of CPU types for this console (main CPU + coprocessors)
	vector<CpuType> cpuTypes = _emu->GetCpuTypes();
	MessageManager::Log(std::format("[Debugger] Creating debugger for console={} cpuCount={}", (int)_consoleType, cpuTypes.size()));
	_cpuTypes = unordered_set<CpuType>(cpuTypes.begin(), cpuTypes.end());
	_mainCpuType = cpuTypes[0];  // First CPU is always the main CPU

	// Create shared debugging components
	_labelManager = std::make_unique<LabelManager>(this);          // Symbol/label management
	_memoryDumper = std::make_unique<MemoryDumper>(this);          // Memory viewing/editing
	_disassembler = std::make_unique<Disassembler>(console, this); // Code disassembly
	_disassemblySearch = std::make_unique<DisassemblySearch>(_disassembler.get(), _labelManager.get());  // Search in disassembly
	_memoryAccessCounter = std::make_unique<MemoryAccessCounter>(this);  // Memory access tracking
	_scriptManager = std::make_unique<ScriptManager>(this);        // Lua scripting
	_mcpHooks = std::make_unique<McpHookManager>();                // MCP hook registry
	_traceLogSaver = std::make_unique<TraceLogFileSaver>();        // Trace log file output
	_cdlManager = std::make_unique<CdlManager>(this, _disassembler.get());  // Code/Data log manager

	// Use cpuTypes for iteration (ordered), not _cpuTypes (order is important for coprocessors, etc.)
	for (CpuType type : cpuTypes) {
		unique_ptr<IDebugger>& debugger = _debuggers[(int)type].Debugger;
		MessageManager::Log(std::format("[Debugger] Creating backend for cpuType={}", (int)type));
		switch (type) {
			case CpuType::Snes:
				debugger = std::make_unique<SnesDebugger>(this, CpuType::Snes);
				break;
			case CpuType::Spc:
				debugger = std::make_unique<SpcDebugger>(this);
				break;
			case CpuType::NecDsp:
				debugger = std::make_unique<NecDspDebugger>(this);
				break;
			case CpuType::Sa1:
				debugger = std::make_unique<SnesDebugger>(this, CpuType::Sa1);
				break;
			case CpuType::Gsu:
				debugger = std::make_unique<GsuDebugger>(this);
				break;
			case CpuType::Cx4:
				debugger = std::make_unique<Cx4Debugger>(this);
				break;
			case CpuType::St018:
				debugger = std::make_unique<St018Debugger>(this);
				break;
			case CpuType::Gameboy:
				debugger = std::make_unique<GbDebugger>(this);
				break;
			case CpuType::Nes:
				debugger = std::make_unique<NesDebugger>(this);
				break;
			case CpuType::Pce:
				debugger = std::make_unique<PceDebugger>(this);
				break;
			case CpuType::Sms:
				debugger = std::make_unique<SmsDebugger>(this);
				break;
			case CpuType::Gba:
				debugger = std::make_unique<GbaDebugger>(this);
				break;
			case CpuType::Ws:
				debugger = std::make_unique<WsDebugger>(this);
				break;
			case CpuType::Lynx:
				debugger = std::make_unique<LynxDebugger>(this);
				break;
			case CpuType::Atari2600:
				debugger = std::make_unique<Atari2600Debugger>(this);
				break;
			case CpuType::ChannelF:
				debugger = std::make_unique<ChannelFDebugger>(this);
				break;
			case CpuType::Genesis:
				debugger = std::make_unique<GenesisDebugger>(this);
				break;
			default:
				[[unlikely]] throw std::runtime_error("Unsupported CPU type");
		}
		MessageManager::Log(std::format("[Debugger] Backend ready for cpuType={}", (int)type));

		_debuggers[(int)type].Evaluator = std::make_unique<ExpressionEvaluator>(this, _debuggers[(int)type].Debugger.get(), type);
	}

	for (CpuType type : _cpuTypes) {
		_debuggers[(int)type].Debugger->Init();
		_debuggers[(int)type].Debugger->ProcessConfigChange();
	}
	MessageManager::Log(std::format("[Debugger] Initialized all backends, mainCpuType={}", (int)_mainCpuType));

	_breakRequestCount = 0;
	_suspendRequestCount = 0;

	_cdlManager->RefreshCodeCache(false);

	if (_emu->IsPaused()) {
		// Break on the current instruction if emulation was already paused
		Step(_mainCpuType, 1, StepType::Step, BreakSource::Pause);
	}

	_executionStopped = false;

#ifdef _DEBUG
	if (_mainCpuType == CpuType::Snes) {
		ExpressionEvaluator eval(this, _debuggers[(int)CpuType::Snes].Debugger.get(), CpuType::Snes);
		eval.RunTests();
	}
#endif
}

Debugger::~Debugger() {
	Release();
}

void Debugger::Release() {
	while (_executionStopped) {
		Run();
	}
}

void Debugger::Reset() {
	_memoryAccessCounter->ResetCounts();
	for (int i = 0; i <= (int)DebugUtilities::GetLastCpuType(); i++) {
		if (_debuggers[i].Debugger) {
			_debuggers[i].Debugger->Reset();
		}

		BaseEventManager* evtMgr = GetEventManager((CpuType)i);
		if (evtMgr) {
			// Call twice to clear both current and previous frame
			evtMgr->ClearFrameEvents();
			evtMgr->ClearFrameEvents();
		}
	}
}

template <CpuType type, typename DebuggerType>
DebuggerType* Debugger::GetDebugger() {
	return (DebuggerType*)_debuggers[(int)type].Debugger.get();
}

IDebugger* Debugger::GetMainDebugger() {
	return _debuggers[(int)_mainCpuType].Debugger.get();
}

template <CpuType type>
uint64_t Debugger::GetCpuCycleCount() {
	switch (type) {
		case CpuType::Snes:
			return GetDebugger<type, SnesDebugger>()->GetCpuCycleCount();
		case CpuType::Spc:
			return GetDebugger<type, SpcDebugger>()->GetCpuCycleCount();
		case CpuType::NecDsp:
			return GetDebugger<type, NecDspDebugger>()->GetCpuCycleCount();
		case CpuType::Sa1:
			return GetDebugger<type, SnesDebugger>()->GetCpuCycleCount();
		case CpuType::Gsu:
			return GetDebugger<type, GsuDebugger>()->GetCpuCycleCount();
		case CpuType::Cx4:
			return GetDebugger<type, Cx4Debugger>()->GetCpuCycleCount();
		case CpuType::St018:
			return GetDebugger<type, St018Debugger>()->GetCpuCycleCount();
		case CpuType::Gameboy:
			return GetDebugger<type, GbDebugger>()->GetCpuCycleCount();
		case CpuType::Nes:
			return GetDebugger<type, NesDebugger>()->GetCpuCycleCount();
		case CpuType::Pce:
			return GetDebugger<type, PceDebugger>()->GetCpuCycleCount();
		case CpuType::Sms:
			return GetDebugger<type, SmsDebugger>()->GetCpuCycleCount();
		case CpuType::Gba:
			return GetDebugger<type, GbaDebugger>()->GetCpuCycleCount();
		case CpuType::Ws:
			return GetDebugger<type, WsDebugger>()->GetCpuCycleCount();
		case CpuType::Lynx:
			return GetDebugger<type, LynxDebugger>()->GetCpuCycleCount();
		case CpuType::Atari2600:
			return GetDebugger<type, Atari2600Debugger>()->GetCpuCycleCount();
		case CpuType::ChannelF:
			return GetDebugger<type, ChannelFDebugger>()->GetCpuCycleCount();
		case CpuType::Genesis:
			return GetDebugger<type, GenesisDebugger>()->GetCpuCycleCount();
		default:
			return 0;
			break;
	}
}

bool Debugger::ProcessStepBack(IDebugger* debugger) {
	if (debugger->CheckStepBack()) {
		// Step back target reached, break at the current instruction
		debugger->GetStepRequest()->Break(BreakSource::CpuStep);

		// Reset prev op code flag to prevent debugger code from incorrectly flagging
		// an instruction as the start of a function, etc. after loading the state
		debugger->ResetPrevOpCode();
		return false;
	} else {
		// While step back is running, don't process instructions
		return true;
	}
}

template <CpuType type>
void Debugger::ProcessInstruction() {
	IDebugger* debugger = _debuggers[(int)type].Debugger.get();
	if (debugger->IsStepBack() && ProcessStepBack(debugger)) {
		debugger->AllowChangeProgramCounter = true; // set to true temporarily to allow debugger to pause on break requests when rewinding/step back is active
		SleepOnBreakRequest<type>();
		debugger->AllowChangeProgramCounter = false;
		return;
	}

	debugger->IgnoreBreakpoints = false;
	debugger->AllowChangeProgramCounter = true;

	switch (type) {
		case CpuType::Snes:
			GetDebugger<type, SnesDebugger>()->ProcessInstruction();
			break;
		case CpuType::Spc:
			GetDebugger<type, SpcDebugger>()->ProcessInstruction();
			break;
		case CpuType::NecDsp:
			GetDebugger<type, NecDspDebugger>()->ProcessInstruction();
			break;
		case CpuType::Sa1:
			GetDebugger<type, SnesDebugger>()->ProcessInstruction();
			break;
		case CpuType::Gsu:
			GetDebugger<type, GsuDebugger>()->ProcessInstruction();
			break;
		case CpuType::Cx4:
			GetDebugger<type, Cx4Debugger>()->ProcessInstruction();
			break;
		case CpuType::St018:
			GetDebugger<type, St018Debugger>()->ProcessInstruction();
			break;
		case CpuType::Gameboy:
			GetDebugger<type, GbDebugger>()->ProcessInstruction();
			break;
		case CpuType::Nes:
			GetDebugger<type, NesDebugger>()->ProcessInstruction();
			break;
		case CpuType::Pce:
			GetDebugger<type, PceDebugger>()->ProcessInstruction();
			break;
		case CpuType::Sms:
			GetDebugger<type, SmsDebugger>()->ProcessInstruction();
			break;
		case CpuType::Gba:
			GetDebugger<type, GbaDebugger>()->ProcessInstruction();
			break;
		case CpuType::Ws:
			GetDebugger<type, WsDebugger>()->ProcessInstruction();
			break;
		case CpuType::Lynx:
			GetDebugger<type, LynxDebugger>()->ProcessInstruction();
			break;
		case CpuType::Atari2600:
			GetDebugger<type, Atari2600Debugger>()->ProcessInstruction();
			break;
		case CpuType::ChannelF:
			GetDebugger<type, ChannelFDebugger>()->ProcessInstruction();
			break;
		case CpuType::Genesis:
			GetDebugger<type, GenesisDebugger>()->ProcessInstruction();
			break;
	}

	debugger->AllowChangeProgramCounter = false;

	if (_scriptManager->HasCpuMemoryCallbacks()) {
		MemoryOperationInfo memOp = debugger->InstructionProgress.LastMemOperation;
		AddressInfo relAddr = {(int32_t)memOp.Address, memOp.MemType};
		uint8_t value = (uint8_t)memOp.Value;
		_scriptManager->ProcessMemoryOperation(relAddr, value, MemoryOperationType::ExecOpCode, type, true);
	}
}

template <CpuType type, uint8_t accessWidth, MemoryAccessFlags flags, typename T>
void Debugger::ProcessMemoryRead(uint32_t addr, T& value, MemoryOperationType opType) {
	if (_debuggers[(int)type].Debugger->IsStepBack()) {
		SleepOnBreakRequest<type>();
		return;
	}

	switch (type) {
		case CpuType::Snes:
			GetDebugger<CpuType::Snes, SnesDebugger>()->ProcessRead(addr, value, opType);
			break;
		case CpuType::Spc:
			GetDebugger<CpuType::Spc, SpcDebugger>()->ProcessRead<flags>(addr, value, opType);
			break;
		case CpuType::NecDsp:
			GetDebugger<CpuType::NecDsp, NecDspDebugger>()->ProcessRead(addr, value, opType);
			break;
		case CpuType::Sa1:
			GetDebugger<CpuType::Sa1, SnesDebugger>()->ProcessRead(addr, value, opType);
			break;
		case CpuType::Gsu:
			GetDebugger<CpuType::Gsu, GsuDebugger>()->ProcessRead(addr, value, opType);
			break;
		case CpuType::Cx4:
			GetDebugger<CpuType::Cx4, Cx4Debugger>()->ProcessRead(addr, value, opType);
			break;
		case CpuType::St018:
			GetDebugger<CpuType::St018, St018Debugger>()->ProcessRead<accessWidth>(addr, value, opType);
			break;
		case CpuType::Gameboy:
			GetDebugger<CpuType::Gameboy, GbDebugger>()->ProcessRead(addr, value, opType);
			break;
		case CpuType::Nes:
			GetDebugger<CpuType::Nes, NesDebugger>()->ProcessRead(addr, value, opType);
			break;
		case CpuType::Pce:
			GetDebugger<CpuType::Pce, PceDebugger>()->ProcessRead(addr, value, opType);
			break;
		case CpuType::Sms:
			GetDebugger<CpuType::Sms, SmsDebugger>()->ProcessRead(addr, value, opType);
			break;
		case CpuType::Gba:
			GetDebugger<CpuType::Gba, GbaDebugger>()->ProcessRead<accessWidth>(addr, value, opType);
			break;
		case CpuType::Ws:
			if constexpr (accessWidth <= 2) {
				GetDebugger<CpuType::Ws, WsDebugger>()->ProcessRead<accessWidth>(addr, value, opType);
			}
			break;
		case CpuType::Lynx:
			GetDebugger<CpuType::Lynx, LynxDebugger>()->ProcessRead(addr, value, opType);
			break;
		case CpuType::Atari2600:
			GetDebugger<CpuType::Atari2600, Atari2600Debugger>()->ProcessRead(addr, value, opType);
			break;
		case CpuType::ChannelF:
			GetDebugger<CpuType::ChannelF, ChannelFDebugger>()->ProcessRead(addr, value, opType);
			break;
		case CpuType::Genesis:
			GetDebugger<CpuType::Genesis, GenesisDebugger>()->ProcessRead(addr, value, opType);
			break;
	}

	if (_scriptManager->HasCpuMemoryCallbacks()) {
		ProcessScripts<type>(addr, value, opType);
	}
}

template <CpuType type, uint8_t accessWidth, MemoryAccessFlags flags, typename T>
bool Debugger::ProcessMemoryWrite(uint32_t addr, T& value, MemoryOperationType opType) {
	if (_debuggers[(int)type].Debugger->IsStepBack()) {
		SleepOnBreakRequest<type>();
		return !_debuggers[(int)type].Debugger->GetFrozenAddressManager().IsFrozenAddress(addr);
	}

	switch (type) {
		case CpuType::Snes:
			GetDebugger<CpuType::Snes, SnesDebugger>()->ProcessWrite(addr, value, opType);
			break;
		case CpuType::Spc:
			GetDebugger<CpuType::Spc, SpcDebugger>()->ProcessWrite<flags>(addr, value, opType);
			break;
		case CpuType::NecDsp:
			GetDebugger<CpuType::NecDsp, NecDspDebugger>()->ProcessWrite(addr, value, opType);
			break;
		case CpuType::Sa1:
			GetDebugger<CpuType::Sa1, SnesDebugger>()->ProcessWrite(addr, value, opType);
			break;
		case CpuType::Gsu:
			GetDebugger<CpuType::Gsu, GsuDebugger>()->ProcessWrite(addr, value, opType);
			break;
		case CpuType::Cx4:
			GetDebugger<CpuType::Cx4, Cx4Debugger>()->ProcessWrite(addr, value, opType);
			break;
		case CpuType::St018:
			GetDebugger<CpuType::St018, St018Debugger>()->ProcessWrite<accessWidth>(addr, value, opType);
			break;
		case CpuType::Gameboy:
			GetDebugger<CpuType::Gameboy, GbDebugger>()->ProcessWrite(addr, value, opType);
			break;
		case CpuType::Nes:
			GetDebugger<CpuType::Nes, NesDebugger>()->ProcessWrite(addr, value, opType);
			break;
		case CpuType::Pce:
			GetDebugger<CpuType::Pce, PceDebugger>()->ProcessWrite(addr, value, opType);
			break;
		case CpuType::Sms:
			GetDebugger<CpuType::Sms, SmsDebugger>()->ProcessWrite(addr, value, opType);
			break;
		case CpuType::Gba:
			GetDebugger<CpuType::Gba, GbaDebugger>()->ProcessWrite<accessWidth>(addr, value, opType);
			break;
		case CpuType::Ws:
			if constexpr (accessWidth <= 2) {
				GetDebugger<CpuType::Ws, WsDebugger>()->ProcessWrite<accessWidth>(addr, value, opType);
			}
			break;
		case CpuType::Lynx:
			GetDebugger<CpuType::Lynx, LynxDebugger>()->ProcessWrite(addr, value, opType);
			break;
		case CpuType::Atari2600:
			GetDebugger<CpuType::Atari2600, Atari2600Debugger>()->ProcessWrite(addr, value, opType);
			break;
		case CpuType::ChannelF:
			GetDebugger<CpuType::ChannelF, ChannelFDebugger>()->ProcessWrite(addr, value, opType);
			break;
		case CpuType::Genesis:
			GetDebugger<CpuType::Genesis, GenesisDebugger>()->ProcessWrite(addr, value, opType);
			break;
	}

	if (_scriptManager->HasCpuMemoryCallbacks()) {
		ProcessScripts<type>(addr, value, opType);
	}

	return !_debuggers[(int)type].Debugger->GetFrozenAddressManager().IsFrozenAddress(addr);
}

template <CpuType cpuType, MemoryType memType, MemoryOperationType opType, typename T>
void Debugger::ProcessMemoryAccess(uint32_t addr, T& value) {
	IDebugger* debugger = _debuggers[(int)cpuType].Debugger.get();

	constexpr int accessWidth = std::is_same<T, uint16_t>::value ? 2 : 1;

	if (debugger->IsStepBack()) {
		return;
	}

	AddressInfo addressInfo = {(int32_t)addr, memType};
	MemoryOperationInfo operation(addr, value, opType, memType);

	if constexpr (opType == MemoryOperationType::Write) {
		_memoryAccessCounter->ProcessMemoryWrite<accessWidth>(addressInfo, _emu->GetMasterClock());
	} else {
		_memoryAccessCounter->ProcessMemoryRead<accessWidth>(addressInfo, _emu->GetMasterClock());
	}

	switch (cpuType) {
		default:
			break;
		case CpuType::Sms:
			GetDebugger<CpuType::Sms, SmsDebugger>()->ProcessMemoryAccess<opType>(addr, value, memType);
			break;
		case CpuType::Ws:
			GetDebugger<CpuType::Ws, WsDebugger>()->ProcessMemoryAccess<opType, T>(addr, value, memType);
			break;
		case CpuType::Lynx:
			GetDebugger<CpuType::Lynx, LynxDebugger>()->ProcessMemoryAccess<opType>(addr, value, memType);
			break;
		case CpuType::Atari2600:
			GetDebugger<CpuType::Atari2600, Atari2600Debugger>()->ProcessMemoryAccess<opType>(addr, value, memType);
			break;
		case CpuType::ChannelF:
			GetDebugger<CpuType::ChannelF, ChannelFDebugger>()->ProcessMemoryAccess<opType>(addr, value, memType);
			break;
	}

	if (_scriptManager->HasCpuMemoryCallbacks()) {
		ProcessScripts<cpuType>(addr, value, memType, opType);
	}

	ProcessBreakConditions<accessWidth>(cpuType, *debugger->GetStepRequest(), debugger->GetBreakpointManager(), operation, addressInfo);
}

template <CpuType type>
void Debugger::ProcessIdleCycle() {
	if (_debuggers[(int)type].Debugger->IsStepBack()) {
		SleepOnBreakRequest<type>();
		return;
	}

	_debuggers[(int)type].Debugger->InstructionProgress.LastMemOperation.Type = MemoryOperationType::Idle;

	switch (type) {
		case CpuType::Snes:
			GetDebugger<type, SnesDebugger>()->ProcessIdleCycle();
			break;
		case CpuType::Sa1:
			GetDebugger<type, SnesDebugger>()->ProcessIdleCycle();
			break;
		case CpuType::Pce:
			GetDebugger<type, PceDebugger>()->ProcessIdleCycle();
			break;
	}
}

template <CpuType type>
void Debugger::ProcessHaltedCpu() {
	IDebugger* dbg = _debuggers[(int)type].Debugger.get();
	if (dbg->IsStepBack() && ProcessStepBack(dbg)) {
		dbg->AllowChangeProgramCounter = true; // set to true temporarily to allow debugger to pause on break requests when rewinding/step back is active
		SleepOnBreakRequest<type>();
		dbg->AllowChangeProgramCounter = false;
		return;
	}

	// Set AllowChangeProgramCounter to allow SleepUntilResume to break properly
	dbg->AllowChangeProgramCounter = true;
	dbg->InstructionProgress.CurrentCycle = 0;

	// Process cpu step requests as if each call to ProcessHaltedCpu is an instruction
	StepRequest* req = dbg->GetStepRequest();
	req->ProcessCpuExec();
	if ((int)req->BreakNeeded) {
		SleepUntilResume(type, req->GetBreakSource());
	} else {
		// Also check if a debugger break request is pending
		SleepOnBreakRequest<type>();
	}

	dbg->AllowChangeProgramCounter = false;
}

template <CpuType type>
void Debugger::SleepOnBreakRequest() {
	if (_breakRequestCount) {
		SleepUntilResume(type, BreakSource::Unspecified);
	}
}

template <CpuType type, typename T>
void Debugger::ProcessPpuRead(uint16_t addr, T& value, MemoryType memoryType, MemoryOperationType opType) {
	if (_debuggers[(int)type].Debugger->IsStepBack()) {
		return;
	}

	switch (type) {
		case CpuType::Snes:
			GetDebugger<type, SnesDebugger>()->ProcessPpuRead(addr, value, memoryType);
			break;
		case CpuType::Gameboy:
			GetDebugger<type, GbDebugger>()->ProcessPpuRead(addr, value, memoryType);
			break;
		case CpuType::Nes:
			GetDebugger<type, NesDebugger>()->ProcessPpuRead(addr, value, memoryType, opType);
			break;
		case CpuType::Pce:
			GetDebugger<type, PceDebugger>()->ProcessPpuRead(addr, value, memoryType);
			break;
		case CpuType::Sms:
			GetDebugger<type, SmsDebugger>()->ProcessPpuRead(addr, value, memoryType);
			break;
		default:
			[[unlikely]] throw std::runtime_error("Invalid cpu type");
	}

	if (_scriptManager->HasPpuMemoryCallbacks()) {
		ProcessScripts<type>(addr, value, memoryType, opType);
	}
}

template <CpuType type, typename T>
void Debugger::ProcessPpuWrite(uint16_t addr, T& value, MemoryType memoryType) {
	if (_debuggers[(int)type].Debugger->IsStepBack()) {
		return;
	}

	switch (type) {
		case CpuType::Snes:
			GetDebugger<type, SnesDebugger>()->ProcessPpuWrite(addr, value, memoryType);
			break;
		case CpuType::Gameboy:
			GetDebugger<type, GbDebugger>()->ProcessPpuWrite(addr, value, memoryType);
			break;
		case CpuType::Nes:
			GetDebugger<type, NesDebugger>()->ProcessPpuWrite(addr, value, memoryType);
			break;
		case CpuType::Pce:
			GetDebugger<type, PceDebugger>()->ProcessPpuWrite(addr, value, memoryType);
			break;
		case CpuType::Sms:
			GetDebugger<type, SmsDebugger>()->ProcessPpuWrite(addr, value, memoryType);
			break;
		default:
			[[unlikely]] throw std::runtime_error("Invalid cpu type");
	}

	if (_scriptManager->HasPpuMemoryCallbacks()) {
		ProcessScripts<type>(addr, value, memoryType, MemoryOperationType::Write);
	}
}

template <CpuType type>
void Debugger::ProcessPpuCycle() {
	if (_debuggers[(int)type].Debugger->IsStepBack()) {
		return;
	}

	switch (type) {
		case CpuType::Snes:
			GetDebugger<type, SnesDebugger>()->ProcessPpuCycle();
			break;
		case CpuType::Gameboy:
			GetDebugger<type, GbDebugger>()->ProcessPpuCycle();
			break;
		case CpuType::Nes:
			GetDebugger<type, NesDebugger>()->ProcessPpuCycle();
			break;
		case CpuType::Pce:
			GetDebugger<type, PceDebugger>()->ProcessPpuCycle();
			break;
		case CpuType::Sms:
			GetDebugger<type, SmsDebugger>()->ProcessPpuCycle();
			break;
		case CpuType::Gba:
			GetDebugger<type, GbaDebugger>()->ProcessPpuCycle();
			break;
		case CpuType::Ws:
			GetDebugger<type, WsDebugger>()->ProcessPpuCycle();
			break;
		case CpuType::Lynx:
			GetDebugger<type, LynxDebugger>()->ProcessPpuCycle();
			break;
		case CpuType::Atari2600:
			GetDebugger<type, Atari2600Debugger>()->ProcessPpuCycle();
			break;
		case CpuType::ChannelF:
			// Channel F has no separate PPU cycle
			break;
		default:
			[[unlikely]] throw std::runtime_error("Invalid cpu type");
	}
}

void Debugger::SleepUntilResume(CpuType sourceCpu, BreakSource source, MemoryOperationInfo* operation, int breakpointId) {
	SleepUntilResumeGuardContext guardContext = BuildSleepUntilResumeGuardContext(_suspendRequestCount > 0, _executionStopped, _breakRequestCount > 0, sourceCpu == _mainCpuType, _debuggers[(int)sourceCpu].Debugger->AllowChangeProgramCounter, IsBreakpointForbidden(source, sourceCpu, operation));
	SleepUntilResumeCoordinatorEntryContext entryContext = BuildSleepUntilResumeCoordinatorEntryContext(guardContext, source, _breakRequestCount > 0, false, false);
	SleepUntilResumeCoordinatorEntryOutcome entryOutcome = ResolveSleepUntilResumeCoordinatorEntryOutcome(entryContext);
	SleepUntilResumePhaseContext phaseContext = entryOutcome.PhaseContext;
	SleepUntilResumePhaseOutcome phaseOutcome = entryOutcome.PhaseOutcome;

	switch (phaseOutcome.Decision) {
		case SleepUntilResumeDecision::SkipForSuspendRequest:
			return;
		case SleepUntilResumeDecision::SkipForExecutionAlreadyStopped:
			// Prevent re-entry, which can happen when OnBeforeBreak() below is called, which causes the SPC to run and can trigger a pause.
			// Specifically, this happens when resetting the SNES with the "Break on power/reset" option disabled.
			return;
		case SleepUntilResumeDecision::SkipForBreakRequestMainCpuBoundary:
			// When a break is requested by e.g a debugger call, load/save state, etc. always
			// break in-between 2 instructions of the main CPU, ensuring the state can be saved/loaded safely
			// If SleepUntilResume was called outside of ProcessInstruction, keep running
			return;
		case SleepUntilResumeDecision::SkipForForbiddenBreakpoint:
			ClearPendingBreakExceptions();
			return;
		case SleepUntilResumeDecision::Continue:
			break;
	}

	_executionStopped = true;

	const DebugConfig& debugCfg = _settings->GetDebugConfig();
	entryContext = BuildSleepUntilResumeCoordinatorEntryContext(guardContext, source, _breakRequestCount > 0, debugCfg.SingleBreakpointPerInstruction, debugCfg.DrawPartialFrame);
	entryOutcome = ResolveSleepUntilResumeCoordinatorEntryOutcome(entryContext);
	phaseContext = entryOutcome.PhaseContext;
	phaseOutcome = entryOutcome.PhaseOutcome;
	SleepUntilResumePreBreakActionPlanContext preBreakActionPlanContext = BuildSleepUntilResumePreBreakActionPlanContext(phaseOutcome);
	SleepUntilResumePreBreakActionPlanOutcome preBreakActionPlanOutcome = ResolveSleepUntilResumePreBreakActionPlanOutcome(preBreakActionPlanContext);
	SleepUntilResumePreBreakExecutionContext preBreakExecutionContext = {};
	preBreakExecutionContext.ActionPlan = preBreakActionPlanOutcome;
	SleepUntilResumePreBreakExecutionOutcome preBreakExecutionOutcome = ResolveSleepUntilResumePreBreakExecutionOutcome(preBreakExecutionContext);

	bool notificationSent = false;
	if (preBreakExecutionOutcome.ShouldCallOnBeforeBreak) {
		GetMainDebugger()->OnBeforeBreak(sourceCpu);
	}
	if (preBreakExecutionOutcome.ShouldCallOnBeforePause) {
		_emu->OnBeforePause(false);
	}

	if (preBreakExecutionOutcome.ShouldSetIgnoreBreakpoints) {
		_debuggers[(int)sourceCpu].Debugger->IgnoreBreakpoints = true;
	}

	if (preBreakExecutionOutcome.ShouldCallDrawPartialFrame) {
		_debuggers[(int)sourceCpu].Debugger->DrawPartialFrame();
	}

	if (preBreakExecutionOutcome.ShouldRunRuntimeBundle) {
		SleepUntilResumeRuntimeBundleContext runtimeBundleContext = BuildSleepUntilResumeRuntimeBundleContext(phaseOutcome, sourceCpu, source, breakpointId, operation, notificationSent);
		SleepUntilResumeRuntimeBundleOutcome runtimeBundleOutcome = ResolveSleepUntilResumeRuntimeBundleOutcome(runtimeBundleContext);
		SleepUntilResumeRuntimeSideEffectApplicationContext runtimeSideEffectApplicationContext = BuildSleepUntilResumeRuntimeSideEffectApplicationContext(_waitForBreakResume, notificationSent, runtimeBundleOutcome);
		SleepUntilResumeRuntimeSideEffectApplicationOutcome runtimeSideEffectApplicationOutcome = ResolveSleepUntilResumeRuntimeSideEffectApplicationOutcome(runtimeSideEffectApplicationContext);
		SleepUntilResumeRuntimeDispatchExecutionContext runtimeDispatchExecutionContext = BuildSleepUntilResumeRuntimeDispatchExecutionContext(runtimeBundleOutcome);
		SleepUntilResumeRuntimeDispatchExecutionOutcome runtimeDispatchExecutionOutcome = ResolveSleepUntilResumeRuntimeDispatchExecutionOutcome(runtimeDispatchExecutionContext);

		_waitForBreakResume = runtimeSideEffectApplicationOutcome.WaitForBreakResume;
		if (runtimeDispatchExecutionOutcome.ShouldDispatchCodeBreakNotification) {
			_emu->GetNotificationManager()->SendNotification(ConsoleNotificationType::CodeBreak, &runtimeDispatchExecutionOutcome.Event);
		}
		if (runtimeDispatchExecutionOutcome.ShouldProcessCodeBreakEvent) {
			ProcessEvent(EventType::CodeBreak, sourceCpu);
		}
		notificationSent = runtimeSideEffectApplicationOutcome.NotificationSent;
		if (runtimeSideEffectApplicationOutcome.ShouldEnableScreensaver) {
			PlatformUtilities::EnableScreensaver();
		}
	}

	while (true) {
		SleepUntilResumeLoopPostBundleContext loopPostBundleContext = BuildSleepUntilResumeLoopPostBundleRuntimeContext(_waitForBreakResume, _suspendRequestCount, _breakRequestCount);
		SleepUntilResumeLoopPostBundleOutcome loopPostBundleOutcome = ResolveSleepUntilResumeLoopPostBundleOutcome(loopPostBundleContext);

		if (!loopPostBundleOutcome.Loop.ShouldContinueWaiting) {
			break;
		}

		std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(loopPostBundleOutcome.Loop.WaitDelayMs));
	}

	SleepUntilResumeLoopPostBundleContext postLoopBundleContext = BuildSleepUntilResumePostLoopBundleRuntimeContext(notificationSent);
	SleepUntilResumeLoopPostBundleOutcome postLoopBundleOutcome = ResolveSleepUntilResumeLoopPostBundleOutcome(postLoopBundleContext);

	if (postLoopBundleOutcome.PostLoop.ShouldDisableScreensaver) {
		PlatformUtilities::DisableScreensaver();
	}

	if (postLoopBundleOutcome.PostLoop.ShouldSendDebuggerResumedNotification) {
		_emu->GetNotificationManager()->SendNotification(ConsoleNotificationType::DebuggerResumed);
	}

	_executionStopped = false;
}

bool Debugger::IsBreakpointForbidden(BreakSource source, CpuType sourceCpu, MemoryOperationInfo* operation) {
	if ((source > BreakSource::InternalOperation || source == BreakSource::Breakpoint) && _breakRequestCount == 0) {
		BreakpointManager* bp = _debuggers[(int)sourceCpu].Debugger->GetBreakpointManager();
		uint32_t pc = GetProgramCounter(sourceCpu, true);
		AddressInfo relAddr = {(int32_t)pc, DebugUtilities::GetCpuMemoryType(sourceCpu)};
		AddressInfo absAddr = GetAbsoluteAddress(relAddr);
		return bp->IsForbidden(operation, relAddr, absAddr);
	}

	return false;
}

template <uint8_t accessWidth>
void Debugger::ProcessBreakConditions(CpuType sourceCpu, StepRequest& step, BreakpointManager* bpManager, MemoryOperationInfo& operation, AddressInfo& addressInfo) {
	int breakpointId = bpManager->CheckBreakpoint<accessWidth>(operation, addressInfo, true);
	if (_breakRequestCount || _waitForBreakResume || ((int)step.BreakNeeded && (!_debuggers[(int)sourceCpu].Debugger->IgnoreBreakpoints || step.Type == StepType::CpuCycleStep))) {
		SleepUntilResume(sourceCpu, step.GetBreakSource());
	} else {
		if (breakpointId >= 0 && !_debuggers[(int)sourceCpu].Debugger->IgnoreBreakpoints) {
			SleepUntilResume(sourceCpu, BreakSource::Breakpoint, &operation, breakpointId);
		}
	}
}

template <uint8_t accessWidth>
void Debugger::ProcessPredictiveBreakpoint(CpuType sourceCpu, BreakpointManager* bpManager, MemoryOperationInfo& operation, AddressInfo& addressInfo) {
	if (_debuggers[(int)sourceCpu].Debugger->IgnoreBreakpoints) {
		return;
	}

	int breakpointId = bpManager->CheckBreakpoint<accessWidth>(operation, addressInfo, false);
	if (breakpointId >= 0) {
		SleepUntilResume(sourceCpu, BreakSource::Breakpoint, &operation, breakpointId);
	}
}

template <CpuType type>
void Debugger::ProcessInterrupt(uint32_t originalPc, uint32_t currentPc, bool forNmi) {
	if (_debuggers[(int)type].Debugger->IsStepBack()) {
		return;
	}

	_debuggers[(int)type].Debugger->ProcessInterrupt(originalPc, currentPc, forNmi);
	ProcessEvent(forNmi ? EventType::Nmi : EventType::Irq, type);
}

void Debugger::InternalProcessInterrupt(CpuType cpuType, IDebugger& dbg, StepRequest& stepRequest, AddressInfo& src, uint32_t srcAddr, AddressInfo& dest, uint32_t destAddr, AddressInfo& ret, uint32_t retAddr, uint32_t retSp, bool forNmi) {
	dbg.GetCallstackManager()->Push(src, srcAddr, dest, destAddr, ret, retAddr, retSp, forNmi ? StackFrameFlags::Nmi : StackFrameFlags::Irq);
	if (BaseEventManager* evtMgr = dbg.GetEventManager()) {
		evtMgr->AddEvent(forNmi ? DebugEventType::Nmi : DebugEventType::Irq);
	} else {
		MessageManager::Log(std::format("[Debugger] Missing event manager for cpuType={} while processing interrupt", (int)cpuType));
	}
	stepRequest.ProcessNmiIrq(forNmi);
}

void Debugger::ProcessEvent(EventType type, std::optional<CpuType> cpuTypeOpt) {
	CpuType evtCpuType = cpuTypeOpt.value_or(_mainCpuType);
	CpuType routedCpuType = ResolveEventCpuType(evtCpuType, _mainCpuType, HasCpuType(evtCpuType));
	if (routedCpuType != evtCpuType) {
		MessageManager::Log(std::format("[Debugger] Rerouting event={} from cpuType={} to mainCpuType={}", (int)type, (int)evtCpuType, (int)routedCpuType));
	}

	ProcessEventDispatchContext dispatchContext = {};
	dispatchContext.DebuggerOwnsInstance = _emu->InternalGetDebugger() == this;
	dispatchContext.HasRoutedInputDebugger = HasCpuType(routedCpuType) && _debuggers[(int)routedCpuType].Debugger.get();
	dispatchContext.HasMainInputDebugger = HasCpuType(_mainCpuType) && _debuggers[(int)_mainCpuType].Debugger.get();
	dispatchContext.DebuggerBlocked = _emu->IsDebuggerBlocked();
	dispatchContext.HasRoutedEventManager = GetEventManager(routedCpuType) != nullptr;

	ProcessEventDispatchOutcome dispatchOutcome = ResolveProcessEventDispatchOutcome(type, routedCpuType, _mainCpuType, dispatchContext);
	if (dispatchOutcome.ShouldDispatchScriptEvent) {
		_scriptManager->ProcessEvent(type, routedCpuType);
	} else {
		MessageManager::Log(std::format("[Debugger] Skipping script event dispatch for non-owned debugger instance (event={}, cpuType={})", (int)type, (int)routedCpuType));
	}

	switch (type) {
		default:
			break;

		case EventType::InputPolled:
			HandleInputPolledEvent(evtCpuType, routedCpuType, dispatchOutcome);
			break;

		case EventType::StartFrame:
			HandleStartFrameEvent(routedCpuType, dispatchOutcome);
			break;

		case EventType::Reset:
			Reset();
			break;

		case EventType::StateLoaded:
			HandleStateLoadedEvent();
			break;
	}
}

void Debugger::HandleInputPolledEvent(CpuType requestedCpuType, CpuType routedCpuType, const ProcessEventDispatchOutcome& dispatchOutcome) {
	if (!dispatchOutcome.InputDebuggerCpuType.has_value()) {
		MessageManager::Log(std::format("[Debugger] Ignoring InputPolled for cpuType={} (no available debugger backend)", (int)requestedCpuType));
		return;
	}

	CpuType inputCpuType = dispatchOutcome.InputDebuggerCpuType.value();
	if (inputCpuType != routedCpuType) {
		MessageManager::Log(std::format("[Debugger] Rerouting InputPolled from cpuType={} to mainCpuType={}", (int)requestedCpuType, (int)inputCpuType));
	}

	IDebugger* inputDebugger = _debuggers[(int)inputCpuType].Debugger.get();
	if (!inputDebugger) {
		MessageManager::Log(std::format("[Debugger] Ignoring InputPolled for cpuType={} (resolved backend missing)", (int)requestedCpuType));
		return;
	}

	inputDebugger->ProcessInputOverrides(_inputOverrides);
}

void Debugger::HandleStartFrameEvent(CpuType routedCpuType, const ProcessEventDispatchOutcome& dispatchOutcome) {
	if (dispatchOutcome.ShouldSendEventViewerRefresh) {
		_emu->GetNotificationManager()->SendNotification(ConsoleNotificationType::EventViewerRefresh, (void*)routedCpuType);
	}

	if (dispatchOutcome.ShouldClearFrameEvents) {
		if (BaseEventManager* evtMgr = GetEventManager(routedCpuType)) {
			evtMgr->ClearFrameEvents();
		}
	}
}

void Debugger::HandleStateLoadedEvent() {
	_memoryAccessCounter->ResetCounts();

	for (CpuType cpuType : _cpuTypes) {
		uint32_t pc = _debuggers[(int)cpuType].Debugger->GetProgramCounter(false);
		_debuggers[(int)cpuType].Debugger->SetProgramCounter(pc, true);

		CallstackManager* callstackManager = _debuggers[(int)cpuType].Debugger->GetCallstackManager();
		if (callstackManager) {
			callstackManager->Clear();
		}
	}
}

template <CpuType type, typename T>
void Debugger::ProcessScripts(uint32_t addr, T& value, MemoryOperationType opType) {
	MemoryOperationInfo memOp = GetDebugger<type, IDebugger>()->InstructionProgress.LastMemOperation;
	AddressInfo relAddr = {(int32_t)memOp.Address, memOp.MemType};
	_scriptManager->ProcessMemoryOperation(relAddr, value, opType, type, false);
}

template <CpuType type, typename T>
void Debugger::ProcessScripts(uint32_t addr, T& value, MemoryType memType, MemoryOperationType opType) {
	AddressInfo relAddr = {(int32_t)addr, memType};
	_scriptManager->ProcessMemoryOperation(relAddr, value, opType, type, false);
}

void Debugger::ProcessConfigChange() {
	for (int i = 0; i <= (int)DebugUtilities::GetLastCpuType(); i++) {
		if (_debuggers[i].Debugger) {
			_debuggers[i].Debugger->ProcessConfigChange();
		}
	}
}

void Debugger::GetTokenList(CpuType cpuType, char* tokenList) {
	ExpressionEvaluator expEval(this, nullptr, cpuType);
	expEval.GetTokenList(tokenList);
}

int64_t Debugger::EvaluateExpression(const string& expression, CpuType cpuType, EvalResultType& resultType, bool useCache) {
	MemoryOperationInfo operationInfo{0, 0, MemoryOperationType::Read, MemoryType::None};
	AddressInfo addressInfo = {0, MemoryType::None};
	if (useCache && _debuggers[(int)cpuType].Evaluator) {
		return _debuggers[(int)cpuType].Evaluator->Evaluate(expression, resultType, operationInfo, addressInfo);
	} else if (_debuggers[(int)cpuType].Debugger) {
		ExpressionEvaluator expEval(this, _debuggers[(int)cpuType].Debugger.get(), cpuType);
		return expEval.Evaluate(expression, resultType, operationInfo, addressInfo);
	}

	resultType = EvalResultType::Invalid;
	return 0;
}

void Debugger::Run() {
	for (int i = 0; i <= (int)DebugUtilities::GetLastCpuType(); i++) {
		if (_debuggers[i].Debugger) {
			_debuggers[i].Debugger->ResetStepBackCache();
			_debuggers[i].Debugger->Run();
		}
	}
	_waitForBreakResume = false;
}

void Debugger::ClearPendingBreakExceptions() {
	for (int i = 0; i <= (int)DebugUtilities::GetLastCpuType(); i++) {
		if (_debuggers[i].Debugger) {
			_debuggers[i].Debugger->GetStepRequest()->ClearException();
		}
	}
}

void Debugger::PauseOnNextFrame() {
	// Use BreakSource::PpuStep to prevent "Run single frame" from triggering the "bring to front on pause" feature
	int32_t scanline = GetPauseScanlineForCpu(_mainCpuType);
	if (scanline > 0) {
		Step(_mainCpuType, scanline, StepType::SpecificScanline, BreakSource::PpuStep);
	}
}

void Debugger::Step(CpuType cpuType, int32_t stepCount, StepType type, BreakSource source) {
	DebugBreakHelper helper(this);
	IDebugger* debugger = _debuggers[(int)cpuType].Debugger.get();

	if (debugger) {
		if (type != StepType::StepBack) {
			debugger->ResetStepBackCache();
		} else {
			debugger->StepBack(stepCount);
		}

		debugger->Step(stepCount, type);
		debugger->GetStepRequest()->SetBreakSource(source, false);
	}

	for (int i = 0; i <= (int)DebugUtilities::GetLastCpuType(); i++) {
		if (_debuggers[i].Debugger && _debuggers[i].Debugger.get() != debugger) {
			_debuggers[i].Debugger->ResetStepBackCache();
			_debuggers[i].Debugger->Run();
		}
	}

	_waitForBreakResume = false;
}

bool Debugger::IsPaused() {
	return _waitForBreakResume;
}

bool Debugger::IsExecutionStopped() {
	return _executionStopped || _emu->IsThreadPaused();
}

bool Debugger::HasBreakRequest() {
	return _breakRequestCount > 0;
}

void Debugger::BreakRequest(bool release) {
	if (release) {
		_breakRequestCount--;
	} else {
		_breakRequestCount++;
	}
}

void Debugger::ResetSuspendCounter() {
	_suspendRequestCount = 0;
}

void Debugger::SuspendDebugger(bool release) {
	if (release) {
		if (_suspendRequestCount > 0) {
			_suspendRequestCount--;
		} else {
#ifdef _DEBUG
			// throw std::runtime_error("unexpected debugger suspend::release call");
#endif
		}
	} else {
		_suspendRequestCount++;
	}
}

bool Debugger::IsDebugWindowOpened(CpuType cpuType) {
	auto debugWindowFlag = GetDebuggerFlagForCpu(cpuType);
	if (debugWindowFlag.has_value()) {
		return _settings->CheckDebuggerFlag(debugWindowFlag.value());
	}

	return false;
}

bool Debugger::IsBreakOptionEnabled(BreakSource src) {
	const DebugConfig& cfg = _settings->GetDebugConfig();
	return IsBreakOptionEnabledForSource(src, cfg);
}

static size_t GetCpuStateSize(CpuType cpuType) {
	switch (GetCpuStateLayout(cpuType)) {
		case CpuStateLayout::SnesCpu:
			return sizeof(SnesCpuState);
		case CpuStateLayout::Spc:
			return sizeof(SpcState);
		case CpuStateLayout::NecDsp:
			return sizeof(NecDspState);
		case CpuStateLayout::Gsu:
			return sizeof(GsuState);
		case CpuStateLayout::Cx4:
			return sizeof(Cx4State);
		case CpuStateLayout::ArmV3:
			return sizeof(ArmV3CpuState);
		case CpuStateLayout::GbCpu:
			return sizeof(GbCpuState);
		case CpuStateLayout::NesCpu:
			return sizeof(NesCpuState);
		case CpuStateLayout::PceCpu:
			return sizeof(PceCpuState);
		case CpuStateLayout::SmsCpu:
			return sizeof(SmsCpuState);
		case CpuStateLayout::GbaCpu:
			return sizeof(GbaCpuState);
		case CpuStateLayout::WsCpu:
			return sizeof(WsCpuState);
		case CpuStateLayout::LynxCpu:
			return sizeof(LynxCpuState);
		case CpuStateLayout::Atari2600Cpu:
			return sizeof(Atari2600CpuState);
		case CpuStateLayout::ChannelFCpu:
			return sizeof(ChannelFCpuState);
		case CpuStateLayout::GenesisM68k:
			return sizeof(GenesisM68kState);
		case CpuStateLayout::Unknown:
			break;
	}

	return sizeof(BaseState);
}

void Debugger::BreakImmediately(CpuType sourceCpu, BreakSource source) {
	if (_debuggers[(int)sourceCpu].Debugger->IsStepBack()) {
		return;
	}

	if (IsDebugWindowOpened(sourceCpu) && IsBreakOptionEnabled(source)) {
		SleepUntilResume(sourceCpu, source);
	}
}

void Debugger::GetCpuState(BaseState& dstState, CpuType cpuType) {
	BaseState& srcState = GetCpuStateRef(cpuType);
	memcpy(&dstState, &srcState, GetCpuStateSize(cpuType));
}

void Debugger::SetCpuState(BaseState& srcState, CpuType cpuType) {
	DebugBreakHelper helper(this);
	BaseState& dstState = GetCpuStateRef(cpuType);
	memcpy(&dstState, &srcState, GetCpuStateSize(cpuType));
}

BaseState& Debugger::GetCpuStateRef(CpuType cpuType) {
	return _debuggers[(int)cpuType].Debugger->GetState();
}

template<typename TAction>
void Debugger::ProcessPpuStateAction(BaseState& state, CpuType cpuType, TAction&& action) {
	switch (GetPpuStateBackendForCpu(cpuType)) {
		case PpuStateBackend::Snes:
			action(GetDebugger<CpuType::Snes, SnesDebugger>(), state);
			break;
		case PpuStateBackend::Gameboy:
			action(GetDebugger<CpuType::Gameboy, GbDebugger>(), state);
			break;
		case PpuStateBackend::Nes:
			action(GetDebugger<CpuType::Nes, NesDebugger>(), state);
			break;
		case PpuStateBackend::Pce:
			action(GetDebugger<CpuType::Pce, PceDebugger>(), state);
			break;
		case PpuStateBackend::Sms:
			action(GetDebugger<CpuType::Sms, SmsDebugger>(), state);
			break;
		case PpuStateBackend::Gba:
			action(GetDebugger<CpuType::Gba, GbaDebugger>(), state);
			break;
		case PpuStateBackend::Ws:
			action(GetDebugger<CpuType::Ws, WsDebugger>(), state);
			break;
		case PpuStateBackend::Lynx:
			action(GetDebugger<CpuType::Lynx, LynxDebugger>(), state);
			break;
		case PpuStateBackend::Atari2600:
			action(GetDebugger<CpuType::Atari2600, Atari2600Debugger>(), state);
			break;
		case PpuStateBackend::ChannelF:
			action(GetDebugger<CpuType::ChannelF, ChannelFDebugger>(), state);
			break;
		case PpuStateBackend::None:
		default:
			break;
	}
}

void Debugger::GetPpuState(BaseState& state, CpuType cpuType) {
	ProcessPpuStateAction(state, cpuType, [](auto* debugger, BaseState& targetState) {
		debugger->GetPpuState(targetState);
	});
}

void Debugger::SetPpuState(BaseState& state, CpuType cpuType) {
	DebugBreakHelper helper(this);
	ProcessPpuStateAction(state, cpuType, [](auto* debugger, BaseState& targetState) {
		debugger->SetPpuState(targetState);
	});
}

void Debugger::GetConsoleState(BaseState& state, ConsoleType consoleType) {
	_console->GetConsoleState(state, consoleType);
}

DebuggerFeatures Debugger::GetDebuggerFeatures(CpuType cpuType) {
	if (_debuggers[(int)cpuType].Debugger) {
		return _debuggers[(int)cpuType].Debugger->GetSupportedFeatures();
	}
	return {};
}

void Debugger::SetProgramCounter(CpuType cpuType, uint32_t addr) {
	if (_debuggers[(int)cpuType].Debugger->AllowChangeProgramCounter) {
		_debuggers[(int)cpuType].Debugger->SetProgramCounter(addr);
	}
}

uint32_t Debugger::GetProgramCounter(CpuType cpuType, bool getInstPc) {
	return _debuggers[(int)cpuType].Debugger->GetProgramCounter(getInstPc);
}

uint8_t Debugger::GetCpuFlags(CpuType cpuType, uint32_t addr) {
	return _debuggers[(int)cpuType].Debugger->GetCpuFlags(addr);
}

CpuInstructionProgress Debugger::GetInstructionProgress(CpuType cpuType) {
	CpuInstructionProgress progress = _debuggers[(int)cpuType].Debugger->InstructionProgress;
	progress.CurrentCycle = _debuggers[(int)cpuType].Debugger->GetCpuCycleCount();
	return progress;
}

AddressInfo Debugger::GetAbsoluteAddress(AddressInfo relAddress) {
	return _console->GetAbsoluteAddress(relAddress);
}

AddressInfo Debugger::GetRelativeAddress(AddressInfo absAddress, CpuType cpuType) {
	return _console->GetRelativeAddress(absAddress, cpuType);
}

bool Debugger::HasCpuType(CpuType cpuType) {
	return _cpuTypes.contains(cpuType);
}

void Debugger::SetBreakpoints(Breakpoint breakpoints[], uint32_t length) {
	DebugBreakHelper helper(this);
	for (int i = 0; i <= (int)DebugUtilities::GetLastCpuType(); i++) {
		if (_debuggers[i].Debugger) {
			_debuggers[i].Debugger->GetBreakpointManager()->SetBreakpoints(breakpoints, length);
		}
	}
}

void Debugger::SetInputOverrides(uint32_t index, DebugControllerState state) {
	_inputOverrides[index] = state;
}

void Debugger::GetAvailableInputOverrides(uint8_t* availableIndexes) {
	BaseControlManager* controlManager = _console->GetControlManager();
	for (int i = 0; i < 8; i++) {
		availableIndexes[i] = controlManager->GetControlDeviceByIndex(i) != nullptr;
	}
}

void Debugger::Log(const string& message) {
	auto lock = _logLock.AcquireSafe();
	if (_debuggerLog.size() >= 1000) {
		_debuggerLog.pop_front();
	}
	_debuggerLog.push_back(message);

	// Use '\n' instead of std::endl to avoid flushing stdout on every log call
	std::cout << message << '\n';
}

string Debugger::GetLog() {
	auto lock = _logLock.AcquireSafe();
	string result;
	size_t totalSize = 0;
	for (const string& msg : _debuggerLog) {
		totalSize += msg.size() + 1;
	}
	result.reserve(totalSize);
	for (const string& msg : _debuggerLog) {
		result.append(msg);
		result += '\n';
	}
	return result;
}

bool Debugger::SaveRomToDisk(const string& filename, bool saveAsIps, CdlStripOption stripOption) {
	if (_mainCpuType == CpuType::Snes && _debuggers[(int)CpuType::Gameboy].Debugger) {
		// SGB routes ROM export through the GB debugger backend.
		return _debuggers[(int)CpuType::Gameboy].Debugger->SaveRomToDisk(filename, saveAsIps, stripOption);
	}

	IDebugger* debugger = GetMainDebugger();
	return debugger ? debugger->SaveRomToDisk(filename, saveAsIps, stripOption) : false;
}

FrozenAddressManager* Debugger::GetFrozenAddressManager(CpuType cpuType) {
	if (_debuggers[(int)cpuType].Debugger) {
		return &_debuggers[(int)cpuType].Debugger->GetFrozenAddressManager();
	}
	return nullptr;
}

ITraceLogger* Debugger::GetTraceLogger(CpuType cpuType) {
	if (_debuggers[(int)cpuType].Debugger) {
		return _debuggers[(int)cpuType].Debugger->GetTraceLogger();
	}
	return nullptr;
}

void Debugger::ClearExecutionTrace() {
	DebugBreakHelper helper(this);
	for (CpuType cpuType : _cpuTypes) {
		ITraceLogger* logger = GetTraceLogger(cpuType);
		logger->Clear();
	}
}

uint32_t Debugger::GetExecutionTrace(TraceRow output[], uint32_t startOffset, uint32_t maxLineCount) {
	DebugBreakHelper helper(this);

	uint32_t offsetsByCpu[(int)DebugUtilities::GetLastCpuType() + 1] = {};

	uint32_t count = 0;
	int64_t lastRowId = ITraceLogger::NextRowId;
	while (count < maxLineCount) {
		bool added = false;
		for (CpuType cpuType : _cpuTypes) {
			ITraceLogger* logger = GetTraceLogger(cpuType);
			if (logger) {
				uint32_t& offset = offsetsByCpu[(int)cpuType];
				int64_t rowId = logger->GetRowId(offset);
				if (rowId == -1 || rowId != lastRowId - 1) {
					continue;
				}

				lastRowId = rowId;

				if (startOffset > 0) {
					// Skip rows until the part the UI wants to display is reached
					startOffset--;
				} else {
					if (logger->IsEnabled()) {
						if (output) {
							logger->GetExecutionTrace(output[count], offset);
						}
						count++;
					}
				}
				offset++;
				added = true;
				break;
			}
		}
		if (!added) {
			break;
		}
	}

	return count;
}

PpuTools* Debugger::GetPpuTools(CpuType cpuType) {
	if (_debuggers[(int)cpuType].Debugger) {
		return _debuggers[(int)cpuType].Debugger->GetPpuTools();
	}
	return nullptr;
}

BaseEventManager* Debugger::GetEventManager(CpuType cpuType) {
	if (_debuggers[(int)cpuType].Debugger) {
		return _debuggers[(int)cpuType].Debugger->GetEventManager();
	}
	return nullptr;
}

CallstackManager* Debugger::GetCallstackManager(CpuType cpuType) {
	if (_debuggers[(int)cpuType].Debugger) {
		return _debuggers[(int)cpuType].Debugger->GetCallstackManager();
	}
	return nullptr;
}

IAssembler* Debugger::GetAssembler(CpuType cpuType) {
	if (_debuggers[(int)cpuType].Debugger) {
		return _debuggers[(int)cpuType].Debugger->GetAssembler();
	}
	return nullptr;
}

template void Debugger::ProcessInstruction<CpuType::Snes>();
template void Debugger::ProcessInstruction<CpuType::Sa1>();
template void Debugger::ProcessInstruction<CpuType::Spc>();
template void Debugger::ProcessInstruction<CpuType::Gsu>();
template void Debugger::ProcessInstruction<CpuType::NecDsp>();
template void Debugger::ProcessInstruction<CpuType::Cx4>();
template void Debugger::ProcessInstruction<CpuType::St018>();
template void Debugger::ProcessInstruction<CpuType::Gameboy>();
template void Debugger::ProcessInstruction<CpuType::Nes>();
template void Debugger::ProcessInstruction<CpuType::Pce>();
template void Debugger::ProcessInstruction<CpuType::Sms>();
template void Debugger::ProcessInstruction<CpuType::Gba>();
template void Debugger::ProcessInstruction<CpuType::Ws>();
template void Debugger::ProcessInstruction<CpuType::Lynx>();
template void Debugger::ProcessInstruction<CpuType::Genesis>();
template void Debugger::ProcessInstruction<CpuType::Atari2600>();
template void Debugger::ProcessInstruction<CpuType::ChannelF>();

template void Debugger::ProcessMemoryRead<CpuType::Snes>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Sa1>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Spc>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Spc, 1, MemoryAccessFlags::DspAccess>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Gsu>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::NecDsp>(uint32_t addr, uint32_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::NecDsp>(uint32_t addr, uint16_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Cx4>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::St018, 1>(uint32_t addr, uint32_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::St018, 4>(uint32_t addr, uint32_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Gameboy>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Nes>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Pce>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Sms>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Gba, 1>(uint32_t addr, uint32_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Gba, 2>(uint32_t addr, uint32_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Gba, 4>(uint32_t addr, uint32_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Ws, 1>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Ws, 2>(uint32_t addr, uint16_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Lynx>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Atari2600>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::ChannelF>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template void Debugger::ProcessMemoryRead<CpuType::Genesis>(uint32_t addr, uint8_t& value, MemoryOperationType opType);

template bool Debugger::ProcessMemoryWrite<CpuType::Snes>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Sa1>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Spc>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Spc, 1, MemoryAccessFlags::DspAccess>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Gsu>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::NecDsp>(uint32_t addr, uint16_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Cx4>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::St018, 1>(uint32_t addr, uint32_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::St018, 4>(uint32_t addr, uint32_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Gameboy>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Nes>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Pce>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Sms>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Gba, 1>(uint32_t addr, uint32_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Gba, 2>(uint32_t addr, uint32_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Gba, 4>(uint32_t addr, uint32_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Ws, 1>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Ws, 2>(uint32_t addr, uint16_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Lynx>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Atari2600>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::ChannelF>(uint32_t addr, uint8_t& value, MemoryOperationType opType);
template bool Debugger::ProcessMemoryWrite<CpuType::Genesis>(uint32_t addr, uint8_t& value, MemoryOperationType opType);

template void Debugger::ProcessMemoryAccess<CpuType::Pce, MemoryType::PceAdpcmRam, MemoryOperationType::Write>(uint32_t addr, uint8_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Pce, MemoryType::PceAdpcmRam, MemoryOperationType::Read>(uint32_t addr, uint8_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Pce, MemoryType::PceArcadeCardRam, MemoryOperationType::Write>(uint32_t addr, uint8_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Pce, MemoryType::PceArcadeCardRam, MemoryOperationType::Read>(uint32_t addr, uint8_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Sms, MemoryType::SmsPort, MemoryOperationType::Write>(uint32_t addr, uint8_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Sms, MemoryType::SmsPort, MemoryOperationType::Read>(uint32_t addr, uint8_t& value);

template void Debugger::ProcessMemoryAccess<CpuType::Atari2600, MemoryType::Atari2600TiaRegisters, MemoryOperationType::Write>(uint32_t addr, uint8_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Atari2600, MemoryType::Atari2600TiaRegisters, MemoryOperationType::Read>(uint32_t addr, uint8_t& value);

template void Debugger::ProcessMemoryAccess<CpuType::ChannelF, MemoryType::ChannelFMemory, MemoryOperationType::Write>(uint32_t addr, uint8_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::ChannelF, MemoryType::ChannelFMemory, MemoryOperationType::Read>(uint32_t addr, uint8_t& value);

template void Debugger::ProcessMemoryAccess<CpuType::Ws, MemoryType::WsPort, MemoryOperationType::Write>(uint32_t addr, uint8_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Ws, MemoryType::WsPort, MemoryOperationType::Read>(uint32_t addr, uint8_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Ws, MemoryType::WsPort, MemoryOperationType::Write>(uint32_t addr, uint16_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Ws, MemoryType::WsPort, MemoryOperationType::Read>(uint32_t addr, uint16_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Ws, MemoryType::WsInternalEeprom, MemoryOperationType::Read>(uint32_t addr, uint16_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Ws, MemoryType::WsInternalEeprom, MemoryOperationType::Write>(uint32_t addr, uint16_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Ws, MemoryType::WsCartEeprom, MemoryOperationType::Read>(uint32_t addr, uint16_t& value);
template void Debugger::ProcessMemoryAccess<CpuType::Ws, MemoryType::WsCartEeprom, MemoryOperationType::Write>(uint32_t addr, uint16_t& value);

template void Debugger::ProcessIdleCycle<CpuType::Snes>();
template void Debugger::ProcessIdleCycle<CpuType::Sa1>();
template void Debugger::ProcessIdleCycle<CpuType::Pce>();

template void Debugger::ProcessHaltedCpu<CpuType::Snes>();
template void Debugger::ProcessHaltedCpu<CpuType::Spc>();
template void Debugger::ProcessHaltedCpu<CpuType::Gameboy>();
template void Debugger::ProcessHaltedCpu<CpuType::Sms>();
template void Debugger::ProcessHaltedCpu<CpuType::Gba>();
template void Debugger::ProcessHaltedCpu<CpuType::Ws>();
template void Debugger::ProcessHaltedCpu<CpuType::Lynx>();

template void Debugger::ProcessInterrupt<CpuType::Snes>(uint32_t originalPc, uint32_t currentPc, bool forNmi);
template void Debugger::ProcessInterrupt<CpuType::Sa1>(uint32_t originalPc, uint32_t currentPc, bool forNmi);
template void Debugger::ProcessInterrupt<CpuType::Gameboy>(uint32_t originalPc, uint32_t currentPc, bool forNmi);
template void Debugger::ProcessInterrupt<CpuType::Nes>(uint32_t originalPc, uint32_t currentPc, bool forNmi);
template void Debugger::ProcessInterrupt<CpuType::Pce>(uint32_t originalPc, uint32_t currentPc, bool forNmi);
template void Debugger::ProcessInterrupt<CpuType::Sms>(uint32_t originalPc, uint32_t currentPc, bool forNmi);
template void Debugger::ProcessInterrupt<CpuType::Gba>(uint32_t originalPc, uint32_t currentPc, bool forNmi);
template void Debugger::ProcessInterrupt<CpuType::Ws>(uint32_t originalPc, uint32_t currentPc, bool forNmi);
template void Debugger::ProcessInterrupt<CpuType::Lynx>(uint32_t originalPc, uint32_t currentPc, bool forNmi);
template void Debugger::ProcessInterrupt<CpuType::Genesis>(uint32_t originalPc, uint32_t currentPc, bool forNmi);
template void Debugger::ProcessInterrupt<CpuType::Atari2600>(uint32_t originalPc, uint32_t currentPc, bool forNmi);
template void Debugger::ProcessInterrupt<CpuType::ChannelF>(uint32_t originalPc, uint32_t currentPc, bool forNmi);

template void Debugger::ProcessPpuRead<CpuType::Snes>(uint16_t addr, uint8_t& value, MemoryType memoryType, MemoryOperationType opType);
template void Debugger::ProcessPpuRead<CpuType::Gameboy>(uint16_t addr, uint8_t& value, MemoryType memoryType, MemoryOperationType opType);
template void Debugger::ProcessPpuRead<CpuType::Nes>(uint16_t addr, uint8_t& value, MemoryType memoryType, MemoryOperationType opType);
template void Debugger::ProcessPpuRead<CpuType::Pce>(uint16_t addr, uint16_t& value, MemoryType memoryType, MemoryOperationType opType);
template void Debugger::ProcessPpuRead<CpuType::Pce>(uint16_t addr, uint8_t& value, MemoryType memoryType, MemoryOperationType opType);
template void Debugger::ProcessPpuRead<CpuType::Sms>(uint16_t addr, uint8_t& value, MemoryType memoryType, MemoryOperationType opType);

template void Debugger::ProcessPpuWrite<CpuType::Snes>(uint16_t addr, uint8_t& value, MemoryType memoryType);
template void Debugger::ProcessPpuWrite<CpuType::Gameboy>(uint16_t addr, uint8_t& value, MemoryType memoryType);
template void Debugger::ProcessPpuWrite<CpuType::Nes>(uint16_t addr, uint8_t& value, MemoryType memoryType);
template void Debugger::ProcessPpuWrite<CpuType::Pce>(uint16_t addr, uint16_t& value, MemoryType memoryType);
template void Debugger::ProcessPpuWrite<CpuType::Pce>(uint16_t addr, uint8_t& value, MemoryType memoryType);
template void Debugger::ProcessPpuWrite<CpuType::Sms>(uint16_t addr, uint8_t& value, MemoryType memoryType);

template void Debugger::ProcessPpuCycle<CpuType::Snes>();
template void Debugger::ProcessPpuCycle<CpuType::Gameboy>();
template void Debugger::ProcessPpuCycle<CpuType::Nes>();
template void Debugger::ProcessPpuCycle<CpuType::Pce>();
template void Debugger::ProcessPpuCycle<CpuType::Sms>();
template void Debugger::ProcessPpuCycle<CpuType::Gba>();
template void Debugger::ProcessPpuCycle<CpuType::Ws>();
template void Debugger::ProcessPpuCycle<CpuType::Lynx>();
template void Debugger::ProcessPpuCycle<CpuType::Atari2600>();
template void Debugger::ProcessPpuCycle<CpuType::ChannelF>();

template void Debugger::ProcessBreakConditions<1>(CpuType sourceCpu, StepRequest& step, BreakpointManager* bpManager, MemoryOperationInfo& operation, AddressInfo& addressInfo);
template void Debugger::ProcessBreakConditions<2>(CpuType sourceCpu, StepRequest& step, BreakpointManager* bpManager, MemoryOperationInfo& operation, AddressInfo& addressInfo);
template void Debugger::ProcessBreakConditions<4>(CpuType sourceCpu, StepRequest& step, BreakpointManager* bpManager, MemoryOperationInfo& operation, AddressInfo& addressInfo);

template void Debugger::ProcessPredictiveBreakpoint<1>(CpuType sourceCpu, BreakpointManager* bpManager, MemoryOperationInfo& operation, AddressInfo& addressInfo);
template void Debugger::ProcessPredictiveBreakpoint<2>(CpuType sourceCpu, BreakpointManager* bpManager, MemoryOperationInfo& operation, AddressInfo& addressInfo);
template void Debugger::ProcessPredictiveBreakpoint<4>(CpuType sourceCpu, BreakpointManager* bpManager, MemoryOperationInfo& operation, AddressInfo& addressInfo);
