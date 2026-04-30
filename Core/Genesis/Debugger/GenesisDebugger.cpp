#include "pch.h"
#include "Genesis/Debugger/GenesisDebugger.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/Debugger/GenesisVdpTools.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Genesis/GenesisTypes.h"
#include "Debugger/Disassembler.h"
#include "Debugger/CallstackManager.h"
#include "Debugger/BreakpointManager.h"
#include "Debugger/Debugger.h"
#include "Debugger/MemoryAccessCounter.h"
#include "Debugger/StepBackManager.h"
#include "Shared/EmuSettings.h"
#include "Shared/Emulator.h"
#include "Shared/MessageManager.h"
#include "Shared/MemoryOperationType.h"

GenesisDebugger::GenesisDebugger(Debugger* debugger) : IDebugger(debugger->GetEmulator()) {
	_debugger = debugger;
	_emu = debugger->GetEmulator();
	_console = (GenesisConsole*)debugger->GetConsole();
	_cpu = _console->GetCpu();
	_memoryManager = _console->GetMemoryManager();

	_disassembler = debugger->GetDisassembler();
	_memoryAccessCounter = debugger->GetMemoryAccessCounter();
	_settings = debugger->GetEmulator()->GetSettings();

	_stepBackManager = std::make_unique<StepBackManager>(_emu, this);
	_callstackManager = std::make_unique<CallstackManager>(debugger, this);
	_breakpointManager = std::make_unique<BreakpointManager>(debugger, this, CpuType::Genesis, nullptr);
	_ppuTools = std::make_unique<GenesisVdpTools>(debugger, _emu, _console);
	_step = std::make_unique<StepRequest>();
}

GenesisDebugger::~GenesisDebugger() {
}

void GenesisDebugger::OnBeforeBreak([[maybe_unused]] CpuType cpuType) {
}

void GenesisDebugger::Reset() {
	_callstackManager->Clear();
	_prevOpCode = 0;
	_prevProgramCounter = 0;
}

uint64_t GenesisDebugger::GetCpuCycleCount([[maybe_unused]] bool forProfiler) {
	return _cpu->GetState().CycleCount;
}

void GenesisDebugger::ProcessInstruction() {
	GenesisM68kState& state = _cpu->GetState();
	uint32_t pc = state.PC & 0x00ffffff;
	uint8_t opCode = _memoryManager->DebugRead8(pc);
	AddressInfo relAddress{(int32_t)pc, MemoryType::GenesisMemory};
	AddressInfo absAddress = _console->GetAbsoluteAddress(relAddress);
	MemoryOperationInfo operation(pc, opCode, MemoryOperationType::ExecOpCode, MemoryType::GenesisMemory);
	InstructionProgress.LastMemOperation = operation;
	InstructionProgress.StartCycle = state.CycleCount;

	if (absAddress.Address >= 0) {
		_disassembler->BuildCache(absAddress, 0, CpuType::Genesis);
	}

	_prevOpCode = opCode;
	_prevProgramCounter = pc;

	_step->ProcessCpuExec();
	if (_breakpointManager) {
		_debugger->ProcessBreakConditions(CpuType::Genesis, *_step.get(), _breakpointManager.get(), operation, absAddress);
	}
}

void GenesisDebugger::ProcessRead(uint32_t addr, uint8_t value, MemoryOperationType type) {
	AddressInfo relAddress{(int32_t)addr, MemoryType::GenesisMemory};
	AddressInfo absAddress = _console->GetAbsoluteAddress(relAddress);
	MemoryOperationInfo operation(addr, value, type, MemoryType::GenesisMemory);
	InstructionProgress.LastMemOperation = operation;

	if (type == MemoryOperationType::ExecOpCode || type == MemoryOperationType::ExecOperand) {
		_memoryAccessCounter->ProcessMemoryExec(absAddress, _cpu->GetState().CycleCount);
	} else {
		_memoryAccessCounter->ProcessMemoryRead(absAddress, _cpu->GetState().CycleCount);
	}

	_step->ProcessCpuCycle();
	if (_breakpointManager) {
		_debugger->ProcessBreakConditions(CpuType::Genesis, *_step.get(), _breakpointManager.get(), operation, absAddress);
	}
}

void GenesisDebugger::ProcessWrite(uint32_t addr, uint8_t value, MemoryOperationType type) {
	AddressInfo relAddress{(int32_t)addr, MemoryType::GenesisMemory};
	AddressInfo absAddress = _console->GetAbsoluteAddress(relAddress);
	MemoryOperationInfo operation(addr, value, type, MemoryType::GenesisMemory);
	InstructionProgress.LastMemOperation = operation;

	_memoryAccessCounter->ProcessMemoryWrite(absAddress, _cpu->GetState().CycleCount);
	_step->ProcessCpuCycle();
	if (_breakpointManager) {
		_debugger->ProcessBreakConditions(CpuType::Genesis, *_step.get(), _breakpointManager.get(), operation, absAddress);
	}
}

void GenesisDebugger::ProcessInterrupt([[maybe_unused]] uint32_t originalPc, [[maybe_unused]] uint32_t currentPc, [[maybe_unused]] bool forNmi) {
}

void GenesisDebugger::Run() {
	_step = std::make_unique<StepRequest>();
}

void GenesisDebugger::Step(int32_t stepCount, StepType type) {
	StepRequest step(type);
	switch (type) {
		case StepType::Step:
			step.StepCount = stepCount;
			break;
		case StepType::CpuCycleStep:
			step.CpuCycleStepCount = stepCount;
			break;
		case StepType::PpuStep:
		case StepType::PpuScanline:
		case StepType::PpuFrame:
			step.CpuCycleStepCount = stepCount;
			break;
		case StepType::StepOver:
		case StepType::StepOut:
		case StepType::SpecificScanline:
			step.StepCount = 1;
			break;
	}
	_step = std::make_unique<StepRequest>(step);
}

StepBackConfig GenesisDebugger::GetStepBackConfig() {
	uint32_t cyclesPerFrame = (uint32_t)(_console->GetMasterClockRate() / _console->GetFps());
	return {
		_cpu->GetState().CycleCount,
		488,
		cyclesPerFrame
	};
}

void GenesisDebugger::DrawPartialFrame() {
}

DebuggerFeatures GenesisDebugger::GetSupportedFeatures() {
	DebuggerFeatures features = {};
	features.RunToIrq = false;
	features.RunToNmi = false;
	features.StepOver = false;
	features.StepOut = false;
	features.StepBack = false;
	features.CallStack = false;
	features.ChangeProgramCounter = AllowChangeProgramCounter;
	features.CpuCycleStep = true;
	features.CpuVectorCount = 0;
	return features;
}

void GenesisDebugger::SetProgramCounter(uint32_t addr, bool updateDebuggerOnly) {
	uint32_t effectiveAddr = addr & 0x00ffffff;
	if (!updateDebuggerOnly) {
		GenesisM68kState state = _cpu->GetState();
		uint32_t oldPc = state.PC & 0x00ffffff;
		state.PC = effectiveAddr;
		_cpu->SetState(state);
		static uint64_t pcWriteCount = 0;
		pcWriteCount++;
		if (effectiveAddr == 0 || pcWriteCount <= 64 || (pcWriteCount % 2048) == 0) {
			MessageManager::Log(std::format("[Genesis][Debugger] SetProgramCounter #{} oldPc=${:06x} newPc=${:06x} updateDebuggerOnly=0", pcWriteCount, oldPc, effectiveAddr));
		}
	}
	_prevProgramCounter = effectiveAddr;
	_prevOpCode = _memoryManager->DebugRead8(_prevProgramCounter);
}

uint32_t GenesisDebugger::GetProgramCounter(bool getInstPc) {
	return getInstPc ? _prevProgramCounter : (_cpu->GetState().PC & 0x00ffffff);
}

CallstackManager* GenesisDebugger::GetCallstackManager() {
	return _callstackManager.get();
}

BreakpointManager* GenesisDebugger::GetBreakpointManager() {
	return _breakpointManager.get();
}

IAssembler* GenesisDebugger::GetAssembler() {
	return nullptr;
}

BaseEventManager* GenesisDebugger::GetEventManager() {
	return nullptr;
}

BaseState& GenesisDebugger::GetState() {
	_cachedState = _cpu->GetState();
	return (BaseState&)_cachedState;
}

void GenesisDebugger::GetPpuState(BaseState& state) {
	(GenesisVdpState&)state = _console->GetState().Vdp;
}

void GenesisDebugger::SetPpuState([[maybe_unused]] BaseState& state) {
}

ITraceLogger* GenesisDebugger::GetTraceLogger() {
	return nullptr;
}

PpuTools* GenesisDebugger::GetPpuTools() {
	return _ppuTools.get();
}

void GenesisDebugger::ProcessInputOverrides(DebugControllerState inputOverrides[8]) {
	[[maybe_unused]] DebugControllerState state = inputOverrides[0];
}
