#include "pch.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/Disassembler.h"
#include "Debugger/CallstackManager.h"
#include "Debugger/BreakpointManager.h"
#include "Debugger/BaseEventManager.h"
#include "Debugger/CodeDataLogger.h"
#include "Debugger/Debugger.h"
#include "Debugger/MemoryDumper.h"
#include "Debugger/MemoryAccessCounter.h"
#include "Debugger/ExpressionEvaluator.h"
#include "Debugger/StepBackManager.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxCpu.h"
#include "Lynx/LynxMikey.h"
#include "Lynx/LynxMemoryManager.h"
#include "Lynx/Debugger/LynxDebugger.h"
#include "Lynx/Debugger/LynxDisUtils.h"
#include "Lynx/Debugger/LynxTraceLogger.h"
#include "Lynx/Debugger/LynxEventManager.h"
#include "Lynx/Debugger/LynxAssembler.h"
#include "Lynx/LynxController.h"
#include "Shared/BaseControlManager.h"
#include "Lynx/Debugger/LynxPpuTools.h"
#include "Lynx/Debugger/DummyLynxCpu.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/FolderUtilities.h"
#include "Utilities/Patches/IpsPatcher.h"
#include "Shared/BaseControlManager.h"
#include "Shared/EmuSettings.h"
#include "Shared/SettingTypes.h"
#include "Shared/Emulator.h"
#include "Shared/MemoryOperationType.h"

LynxDebugger::LynxDebugger(Debugger* debugger) : IDebugger(debugger->GetEmulator()) {
	_debugger = debugger;
	_emu = debugger->GetEmulator();
	LynxConsole* console = (LynxConsole*)debugger->GetConsole();

	_console = console;
	_cpu = console->GetCpu();
	_memoryManager = console->GetMemoryManager();

	_disassembler = debugger->GetDisassembler();
	_memoryAccessCounter = debugger->GetMemoryAccessCounter();
	_settings = debugger->GetEmulator()->GetSettings();

	_codeDataLogger.reset(new CodeDataLogger(debugger, MemoryType::LynxPrgRom,
		_emu->GetMemory(MemoryType::LynxPrgRom).Size, CpuType::Lynx, _emu->GetCrc32()));

	_cdlFile = _codeDataLogger->GetCdlFilePath(_emu->GetRomInfo().RomFile.GetFileName());
	_codeDataLogger->LoadCdlFile(_cdlFile, _settings->GetDebugConfig().AutoResetCdl);

	_stepBackManager.reset(new StepBackManager(_emu, this));
	_eventManager.reset(new LynxEventManager(debugger, console));
	_callstackManager.reset(new CallstackManager(debugger, this));
	_breakpointManager.reset(new BreakpointManager(debugger, this, CpuType::Lynx, _eventManager.get()));
	_traceLogger.reset(new LynxTraceLogger(debugger, this, console->GetMikey()));
	_assembler.reset(new LynxAssembler(debugger->GetLabelManager()));
	_ppuTools.reset(new LynxPpuTools(debugger, _emu, console));
	_dummyCpu.reset(new DummyLynxCpu(_emu, _memoryManager));
	_step.reset(new StepRequest());
}

LynxDebugger::~LynxDebugger() {
	_codeDataLogger->SaveCdlFile(_cdlFile);
}

void LynxDebugger::OnBeforeBreak(CpuType cpuType) {
	// No pre-break sync needed for Lynx (no separate PPU clock)
}

void LynxDebugger::Reset() {
	_callstackManager->Clear();
	ResetPrevOpCode();
}

uint64_t LynxDebugger::GetCpuCycleCount(bool forProfiler) {
	return _cpu->GetCycleCount();
}

void LynxDebugger::ResetPrevOpCode() {
	_prevOpCode = 0x01;
}

void LynxDebugger::ProcessInstruction() {
	LynxCpuState& state = _cpu->GetState();
	uint16_t pc = state.PC;
	uint8_t opCode = _memoryManager->DebugRead(pc);
	AddressInfo addressInfo = _memoryManager->GetAbsoluteAddress(pc);
	MemoryOperationInfo operation(pc, opCode, MemoryOperationType::ExecOpCode, MemoryType::LynxMemory);
	InstructionProgress.LastMemOperation = operation;
	InstructionProgress.StartCycle = _cpu->GetCycleCount();

	bool needDisassemble = _traceLogger->IsEnabled() || _settings->CheckDebuggerFlag(DebuggerFlags::LynxDebuggerEnabled);
	if (addressInfo.Address >= 0) {
		if (addressInfo.Type == MemoryType::LynxPrgRom) {
			_codeDataLogger->SetCode(addressInfo.Address, LynxDisUtils::GetOpFlags(_prevOpCode, pc, _prevProgramCounter));
		}
		if (needDisassemble) {
			_disassembler->BuildCache(addressInfo, 0, CpuType::Lynx);
		}
	}

	ProcessCallStackUpdates(addressInfo, pc, state.SP);

	_prevOpCode = opCode;
	_prevProgramCounter = pc;
	_prevStackPointer = state.SP;

	_step->ProcessCpuExec();

	if (_settings->CheckDebuggerFlag(DebuggerFlags::LynxDebuggerEnabled)) {
		if (opCode == 0x00 && _settings->GetDebugConfig().LynxBreakOnBrk) {
			_step->Break(BreakSource::BreakOnBrk);
		}
		if (_breakpointManager) {
			_debugger->ProcessBreakConditions(CpuType::Lynx, *_step.get(), _breakpointManager.get(), operation, addressInfo);
		}

		if (_step->StepCount != 0 && _breakpointManager->HasBreakpoints() && _settings->GetDebugConfig().UsePredictiveBreakpoints) {
			_dummyCpu->SetDummyState(_cpu->GetState());
			_dummyCpu->Exec();
			for (uint32_t i = 1; i < _dummyCpu->GetOperationCount(); i++) {
				MemoryOperationInfo memOp = _dummyCpu->GetOperationInfo(i);
				if (_breakpointManager->HasBreakpointForType(memOp.Type)) {
					AddressInfo absAddr = _memoryManager->GetAbsoluteAddress(memOp.Address);
					_debugger->ProcessPredictiveBreakpoint(CpuType::Lynx, _breakpointManager.get(), memOp, absAddr);
				}
			}
		}
	}
}

void LynxDebugger::ProcessRead(uint32_t addr, uint8_t value, MemoryOperationType type) {
	AddressInfo addressInfo = _memoryManager->GetAbsoluteAddress(static_cast<uint16_t>(addr));
	MemoryOperationInfo operation(addr, value, type, MemoryType::LynxMemory);
	InstructionProgress.LastMemOperation = operation;

	if (IsRegister(addr)) {
		_eventManager->AddEvent(DebugEventType::Register, operation);
	}

	if (addressInfo.Address >= 0 && addressInfo.Type == MemoryType::LynxPrgRom) {
		_codeDataLogger->SetData(addressInfo.Address);
	}

	if (type == MemoryOperationType::ExecOpCode) {
		if (_traceLogger->IsEnabled()) {
			LynxCpuState& state = _cpu->GetState();
			DisassemblyInfo disInfo = _disassembler->GetDisassemblyInfo(addressInfo, addr, state.PS, CpuType::Lynx);
			_traceLogger->Log(state, disInfo, operation, addressInfo);
		}
		_memoryAccessCounter->ProcessMemoryExec(addressInfo, _cpu->GetCycleCount());
	} else {
		if (_traceLogger->IsEnabled()) {
			_traceLogger->LogNonExec(operation, addressInfo);
		}
		_memoryAccessCounter->ProcessMemoryRead(addressInfo, _cpu->GetCycleCount());
	}

	_step->ProcessCpuCycle();

	if (_settings->CheckDebuggerFlag(DebuggerFlags::LynxDebuggerEnabled)) {
		if (_breakpointManager) {
			_debugger->ProcessBreakConditions(CpuType::Lynx, *_step.get(), _breakpointManager.get(), operation, addressInfo);
		}
	}
}

void LynxDebugger::ProcessWrite(uint32_t addr, uint8_t value, MemoryOperationType type) {
	AddressInfo addressInfo = _memoryManager->GetAbsoluteAddress(static_cast<uint16_t>(addr));
	MemoryOperationInfo operation(addr, value, type, MemoryType::LynxMemory);
	InstructionProgress.LastMemOperation = operation;

	if (IsRegister(addr)) {
		_eventManager->AddEvent(DebugEventType::Register, operation);
	}

	if (addressInfo.Address >= 0 && (addressInfo.Type == MemoryType::LynxWorkRam)) {
		_disassembler->InvalidateCache(addressInfo, CpuType::Lynx);
	}

	if (_traceLogger->IsEnabled()) {
		_traceLogger->LogNonExec(operation, addressInfo);
	}

	_memoryAccessCounter->ProcessMemoryWrite(addressInfo, _cpu->GetCycleCount());
	_step->ProcessCpuCycle();

	if (_settings->CheckDebuggerFlag(DebuggerFlags::LynxDebuggerEnabled)) {
		if (_breakpointManager) {
			_debugger->ProcessBreakConditions(CpuType::Lynx, *_step.get(), _breakpointManager.get(), operation, addressInfo);
		}
	}
}

bool LynxDebugger::IsRegister(uint32_t addr) {
	// Suzy: $FC00-$FCFF, Mikey: $FD00-$FDFF
	return (addr >= LynxConstants::SuzyBase && addr <= LynxConstants::MikeyEnd);
}

void LynxDebugger::ProcessCallStackUpdates(AddressInfo& destAddr, uint16_t destPc, uint8_t sp) {
	if (LynxDisUtils::IsJumpToSub(_prevOpCode)) {
		// JSR
		uint8_t opSize = LynxDisUtils::GetOpSize(_prevOpCode);
		uint32_t returnPc = (_prevProgramCounter + opSize) & 0xFFFF;
		AddressInfo srcAddress = _memoryManager->GetAbsoluteAddress(_prevProgramCounter);
		AddressInfo retAddress = _memoryManager->GetAbsoluteAddress(returnPc);
		_callstackManager->Push(srcAddress, _prevProgramCounter, destAddr, destPc, retAddress, returnPc, _prevStackPointer, StackFrameFlags::None);
	} else if (LynxDisUtils::IsReturnInstruction(_prevOpCode)) {
		// RTS, RTI
		_callstackManager->Pop(destAddr, destPc, sp);

		if (_step->BreakAddress == static_cast<int32_t>(destPc) && _step->BreakStackPointer == sp) {
			_step->Break(BreakSource::CpuStep);
		}
	}
}

void LynxDebugger::ProcessInterrupt(uint32_t originalPc, uint32_t currentPc, bool forNmi) {
	AddressInfo ret = _memoryManager->GetAbsoluteAddress(originalPc);
	AddressInfo dest = _memoryManager->GetAbsoluteAddress(currentPc);

	if (dest.Type == MemoryType::LynxPrgRom && dest.Address >= 0) {
		_codeDataLogger->SetCode(dest.Address, CdlFlags::SubEntryPoint);
	}

	uint8_t originalSp = _cpu->GetState().SP + 3;
	_prevStackPointer = originalSp;

	// If a call/return occurred just before IRQ, process it now
	ProcessCallStackUpdates(ret, originalPc, originalSp);
	ResetPrevOpCode();

	_debugger->InternalProcessInterrupt(
		CpuType::Lynx, *this, *_step.get(),
		ret, originalPc, dest, currentPc, ret, originalPc, originalSp, forNmi);
}

void LynxDebugger::ProcessPpuCycle() {
	if (_step->HasRequest) {
		if (_step->PpuStepCount > 0) {
			_step->PpuStepCount--;
			if (_step->PpuStepCount <= 0) {
				_step->PpuStepCount = 0;
				_debugger->SleepUntilResume(CpuType::Lynx, _step->GetBreakSource());
			}
		}
	}
}

void LynxDebugger::Run() {
	_step.reset(new StepRequest());
}

void LynxDebugger::Step(int32_t stepCount, StepType type) {
	StepRequest step(type);
	switch (type) {
		case StepType::Step:
			step.StepCount = stepCount;
			break;
		case StepType::StepOut:
			step.BreakAddress = _callstackManager->GetReturnAddress();
			step.BreakStackPointer = _callstackManager->GetReturnStackPointer();
			break;

		case StepType::StepOver:
			if (LynxDisUtils::IsJumpToSub(_prevOpCode)) {
				step.BreakAddress = (_prevProgramCounter + LynxDisUtils::GetOpSize(_prevOpCode)) & 0xFFFF;
				step.BreakStackPointer = _prevStackPointer;
			} else {
				step.StepCount = 1;
			}
			break;

		case StepType::CpuCycleStep:
			step.CpuCycleStepCount = stepCount;
			break;
		case StepType::PpuStep:
			step.PpuStepCount = stepCount;
			break;
		case StepType::PpuScanline:
			step.PpuStepCount = LynxConstants::CpuCyclesPerScanline * stepCount;
			break;
		case StepType::PpuFrame:
			step.PpuStepCount = LynxConstants::CpuCyclesPerScanline * LynxConstants::ScanlineCount * stepCount;
			break;
		case StepType::SpecificScanline:
			step.BreakScanline = stepCount;
			break;
	}
	_step.reset(new StepRequest(step));
}

StepBackConfig LynxDebugger::GetStepBackConfig() {
	return {
		_cpu->GetCycleCount(),
		LynxConstants::CpuCyclesPerScanline,
		LynxConstants::CpuCyclesPerFrame
	};
}

void LynxDebugger::DrawPartialFrame() {
	// Lynx doesn't have a separate PPU — Mikey renders scanlines inline
	// Nothing to do for partial frame drawing
}

DebuggerFeatures LynxDebugger::GetSupportedFeatures() {
	DebuggerFeatures features = {};
	features.RunToIrq = true;
	features.RunToNmi = false;
	features.StepOver = true;
	features.StepOut = true;
	features.StepBack = true;
	features.CallStack = true;
	features.ChangeProgramCounter = AllowChangeProgramCounter;
	features.CpuCycleStep = true;

	// 65C02 vectors
	features.CpuVectors[0] = {"IRQ", 0xFFFE};
	features.CpuVectors[1] = {"NMI", 0xFFFA};
	features.CpuVectors[2] = {"Reset", 0xFFFC};
	features.CpuVectorCount = 3;

	return features;
}

void LynxDebugger::SetProgramCounter(uint32_t addr, bool updateDebuggerOnly) {
	if (!updateDebuggerOnly) {
		_cpu->GetState().PC = static_cast<uint16_t>(addr);
	}
	_prevOpCode = _memoryManager->DebugRead(addr);
	_prevProgramCounter = static_cast<uint16_t>(addr);
	_prevStackPointer = _cpu->GetState().SP;
}

uint32_t LynxDebugger::GetProgramCounter(bool getInstPc) {
	return getInstPc ? _prevProgramCounter : _cpu->GetState().PC;
}

CallstackManager* LynxDebugger::GetCallstackManager() {
	return _callstackManager.get();
}

BreakpointManager* LynxDebugger::GetBreakpointManager() {
	return _breakpointManager.get();
}

IAssembler* LynxDebugger::GetAssembler() {
	return _assembler.get();
}

BaseEventManager* LynxDebugger::GetEventManager() {
	return _eventManager.get();
}

BaseState& LynxDebugger::GetState() {
	return _cpu->GetState();
}

void LynxDebugger::GetPpuState(BaseState& state) {
	// Lynx has no separate PPU state — Mikey state is accessed via console state
	(LynxMikeyState&)state = _console->GetMikey()->GetState();
}

void LynxDebugger::SetPpuState(BaseState& state) {
	// Not supported — Mikey state is read-only from debugger
}

ITraceLogger* LynxDebugger::GetTraceLogger() {
	return _traceLogger.get();
}

PpuTools* LynxDebugger::GetPpuTools() {
	return _ppuTools.get();
}

bool LynxDebugger::SaveRomToDisk(string filename, bool saveAsIps, CdlStripOption stripOption) {
	vector<uint8_t> output;

	uint8_t* prgRom = _debugger->GetMemoryDumper()->GetMemoryBuffer(MemoryType::LynxPrgRom);
	uint32_t prgRomSize = _debugger->GetMemoryDumper()->GetMemorySize(MemoryType::LynxPrgRom);
	vector<uint8_t> rom = vector<uint8_t>(prgRom, prgRom + prgRomSize);

	if (saveAsIps) {
		vector<uint8_t> originalRom;
		_emu->GetRomInfo().RomFile.ReadFile(originalRom);
		output = IpsPatcher::CreatePatch(originalRom, rom);
	} else {
		if (stripOption != CdlStripOption::StripNone) {
			_codeDataLogger->StripData(rom.data(), stripOption);
		}
		output = rom;
	}

	ofstream file(filename, ios::out | ios::binary);
	if (file) {
		file.write((char*)output.data(), output.size());
		file.close();
		return true;
	}
	return false;
}

void LynxDebugger::ProcessInputOverrides(DebugControllerState inputOverrides[8]) {
	BaseControlManager* controlManager = _console->GetControlManager();
	for (int i = 0; i < 8; i++) {
		shared_ptr<LynxController> controller = std::dynamic_pointer_cast<LynxController>(controlManager->GetControlDeviceByIndex(i));
		if (controller && inputOverrides[i].HasPressedButton()) {
			controller->SetBitValue(LynxController::Buttons::A, inputOverrides[i].A);
			controller->SetBitValue(LynxController::Buttons::B, inputOverrides[i].B);
			controller->SetBitValue(LynxController::Buttons::Option1, inputOverrides[i].L);
			controller->SetBitValue(LynxController::Buttons::Option2, inputOverrides[i].R);
			controller->SetBitValue(LynxController::Buttons::Pause, inputOverrides[i].Start);
			controller->SetBitValue(LynxController::Buttons::Up, inputOverrides[i].Up);
			controller->SetBitValue(LynxController::Buttons::Down, inputOverrides[i].Down);
			controller->SetBitValue(LynxController::Buttons::Left, inputOverrides[i].Left);
			controller->SetBitValue(LynxController::Buttons::Right, inputOverrides[i].Right);
		}
	}
	controlManager->RefreshHubState();
}
