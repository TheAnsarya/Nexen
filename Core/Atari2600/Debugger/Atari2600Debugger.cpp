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
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600Types.h"
#include "Atari2600/Debugger/Atari2600Debugger.h"
#include "Atari2600/Debugger/Atari2600DisUtils.h"
#include "Atari2600/Debugger/Atari2600TraceLogger.h"
#include "Atari2600/Debugger/Atari2600EventManager.h"
#include "Atari2600/Debugger/Atari2600Assembler.h"
#include "Atari2600/Debugger/Atari2600PpuTools.h"
#include "Shared/BaseControlManager.h"
#include "Atari2600/Atari2600Controller.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/FolderUtilities.h"
#include "Utilities/Patches/IpsPatcher.h"
#include "Shared/EmuSettings.h"
#include "Shared/SettingTypes.h"
#include "Shared/Emulator.h"
#include "Shared/MemoryOperationType.h"

Atari2600Debugger::Atari2600Debugger(Debugger* debugger) : IDebugger(debugger->GetEmulator()) {
	_debugger = debugger;
	_emu = debugger->GetEmulator();
	Atari2600Console* console = (Atari2600Console*)debugger->GetConsole();

	_console = console;

	_disassembler = debugger->GetDisassembler();
	_memoryAccessCounter = debugger->GetMemoryAccessCounter();
	_settings = debugger->GetEmulator()->GetSettings();

	_codeDataLogger = std::make_unique<CodeDataLogger>(debugger, MemoryType::Atari2600PrgRom,
		_emu->GetMemory(MemoryType::Atari2600PrgRom).Size, CpuType::Atari2600, _emu->GetCrc32());

	_cdlFile = _codeDataLogger->GetCdlFilePath(_emu->GetRomInfo().RomFile.GetFileName());
	_codeDataLogger->LoadCdlFile(_cdlFile, _settings->GetDebugConfig().AutoResetCdl);

	_stepBackManager = std::make_unique<StepBackManager>(_emu, this);
	_eventManager = std::make_unique<Atari2600EventManager>(debugger, console);
	_callstackManager = std::make_unique<CallstackManager>(debugger, this);
	_breakpointManager = std::make_unique<BreakpointManager>(debugger, this, CpuType::Atari2600, _eventManager.get());
	_traceLogger = std::make_unique<Atari2600TraceLogger>(debugger, this, console);
	_assembler = std::make_unique<Atari2600Assembler>(debugger->GetLabelManager());
	_ppuTools = std::make_unique<Atari2600PpuTools>(debugger, _emu, console);
	_step = std::make_unique<StepRequest>();
}

Atari2600Debugger::~Atari2600Debugger() {
	_codeDataLogger->SaveCdlFile(_cdlFile);
}

void Atari2600Debugger::OnBeforeBreak(CpuType cpuType) {
}

void Atari2600Debugger::Reset() {
	_callstackManager->Clear();
	ResetPrevOpCode();
}

uint64_t Atari2600Debugger::GetCpuCycleCount(bool forProfiler) {
	return _cachedState.CycleCount;
}

void Atari2600Debugger::ResetPrevOpCode() {
	_prevOpCode = 0x01;
}

void Atari2600Debugger::ProcessInstruction() {
	_cachedState = _console->GetCpuState();
	uint16_t pc = _cachedState.PC;
	uint8_t opCode = _console->DebugRead(pc);
	AddressInfo relAddr{(int32_t)pc, MemoryType::Atari2600Memory};
	AddressInfo addressInfo = _console->GetAbsoluteAddress(relAddr);
	MemoryOperationInfo operation(pc, opCode, MemoryOperationType::ExecOpCode, MemoryType::Atari2600Memory);
	InstructionProgress.LastMemOperation = operation;
	InstructionProgress.StartCycle = _cachedState.CycleCount;

	bool needDisassemble = _traceLogger->IsEnabled() || _settings->CheckDebuggerFlag(DebuggerFlags::Atari2600DebuggerEnabled);
	if (addressInfo.Address >= 0) {
		if (addressInfo.Type == MemoryType::Atari2600PrgRom) {
			_codeDataLogger->SetCode(addressInfo.Address, Atari2600DisUtils::GetOpFlags(_prevOpCode, pc, _prevProgramCounter));
		}
		if (needDisassemble) {
			_disassembler->BuildCache(addressInfo, 0, CpuType::Atari2600);
		}
	}

	ProcessCallStackUpdates(addressInfo, pc, _cachedState.SP);

	_prevOpCode = opCode;
	_prevProgramCounter = pc;
	_prevStackPointer = _cachedState.SP;

	_step->ProcessCpuExec();

	if (_settings->CheckDebuggerFlag(DebuggerFlags::Atari2600DebuggerEnabled)) {
		if (opCode == 0x00 && _settings->GetDebugConfig().Atari2600BreakOnBrk) {
			_step->Break(BreakSource::BreakOnBrk);
		}
		if (_breakpointManager) {
			_debugger->ProcessBreakConditions(CpuType::Atari2600, *_step.get(), _breakpointManager.get(), operation, addressInfo);
		}
	}
}

void Atari2600Debugger::ProcessRead(uint32_t addr, uint8_t value, MemoryOperationType type) {
	AddressInfo relAddr = {(int32_t)(addr & 0x1FFF), MemoryType::Atari2600Memory};
	AddressInfo addressInfo = _console->GetAbsoluteAddress(relAddr);
	MemoryOperationInfo operation(addr, value, type, MemoryType::Atari2600Memory);
	InstructionProgress.LastMemOperation = operation;

	if (addressInfo.Address >= 0 && addressInfo.Type == MemoryType::Atari2600PrgRom) {
		_codeDataLogger->SetData(addressInfo.Address);
	}

	if (type == MemoryOperationType::ExecOpCode) {
		if (_traceLogger->IsEnabled()) {
			_cachedState = _console->GetCpuState();
			DisassemblyInfo disInfo = _disassembler->GetDisassemblyInfo(addressInfo, addr, _cachedState.PS, CpuType::Atari2600);
			_traceLogger->Log(_cachedState, disInfo, operation, addressInfo);
		}
		_memoryAccessCounter->ProcessMemoryExec(addressInfo, _cachedState.CycleCount);
	} else {
		if (_traceLogger->IsEnabled()) {
			_traceLogger->LogNonExec(operation, addressInfo);
		}
		_memoryAccessCounter->ProcessMemoryRead(addressInfo, _cachedState.CycleCount);
	}

	_step->ProcessCpuCycle();

	if (_settings->CheckDebuggerFlag(DebuggerFlags::Atari2600DebuggerEnabled)) {
		if (_breakpointManager) {
			_debugger->ProcessBreakConditions(CpuType::Atari2600, *_step.get(), _breakpointManager.get(), operation, addressInfo);
		}
	}
}

void Atari2600Debugger::ProcessWrite(uint32_t addr, uint8_t value, MemoryOperationType type) {
	AddressInfo relAddr = {(int32_t)(addr & 0x1FFF), MemoryType::Atari2600Memory};
	AddressInfo addressInfo = _console->GetAbsoluteAddress(relAddr);
	MemoryOperationInfo operation(addr, value, type, MemoryType::Atari2600Memory);
	InstructionProgress.LastMemOperation = operation;

	if (addressInfo.Address >= 0 && addressInfo.Type == MemoryType::Atari2600Ram) {
		_disassembler->InvalidateCache(addressInfo, CpuType::Atari2600);
	}

	if (_traceLogger->IsEnabled()) {
		_traceLogger->LogNonExec(operation, addressInfo);
	}

	_memoryAccessCounter->ProcessMemoryWrite(addressInfo, _cachedState.CycleCount);
	_step->ProcessCpuCycle();

	if (_settings->CheckDebuggerFlag(DebuggerFlags::Atari2600DebuggerEnabled)) {
		if (_breakpointManager) {
			_debugger->ProcessBreakConditions(CpuType::Atari2600, *_step.get(), _breakpointManager.get(), operation, addressInfo);
		}
	}
}

void Atari2600Debugger::ProcessCallStackUpdates(AddressInfo& destAddr, uint16_t destPc, uint8_t sp) {
	if (Atari2600DisUtils::IsJumpToSub(_prevOpCode)) {
		uint8_t opSize = Atari2600DisUtils::GetOpSize(_prevOpCode);
		uint32_t returnPc = (_prevProgramCounter + opSize) & 0x1FFF;
		AddressInfo relPrev = {(int32_t)_prevProgramCounter, MemoryType::Atari2600Memory};
		AddressInfo relRet = {(int32_t)returnPc, MemoryType::Atari2600Memory};
		AddressInfo srcAddress = _console->GetAbsoluteAddress(relPrev);
		AddressInfo retAddress = _console->GetAbsoluteAddress(relRet);
		_callstackManager->Push(srcAddress, _prevProgramCounter, destAddr, destPc, retAddress, returnPc, _prevStackPointer, StackFrameFlags::None);
	} else if (Atari2600DisUtils::IsReturnInstruction(_prevOpCode)) {
		_callstackManager->Pop(destAddr, destPc, sp);

		if (_step->BreakAddress == static_cast<int32_t>(destPc) && _step->BreakStackPointer == sp) {
			_step->Break(BreakSource::CpuStep);
		}
	}
}

void Atari2600Debugger::ProcessInterrupt(uint32_t originalPc, uint32_t currentPc, bool forNmi) {
	AddressInfo relOrig = {(int32_t)originalPc, MemoryType::Atari2600Memory};
	AddressInfo relCur = {(int32_t)currentPc, MemoryType::Atari2600Memory};
	AddressInfo ret = _console->GetAbsoluteAddress(relOrig);
	AddressInfo dest = _console->GetAbsoluteAddress(relCur);

	if (dest.Type == MemoryType::Atari2600PrgRom && dest.Address >= 0) {
		_codeDataLogger->SetCode(dest.Address, CdlFlags::SubEntryPoint);
	}

	_cachedState = _console->GetCpuState();
	uint8_t originalSp = _cachedState.SP + 3;
	_prevStackPointer = originalSp;

	ProcessCallStackUpdates(ret, originalPc, originalSp);
	ResetPrevOpCode();

	_debugger->InternalProcessInterrupt(
		CpuType::Atari2600, *this, *_step.get(),
		ret, originalPc, dest, currentPc, ret, originalPc, originalSp, forNmi);
}

void Atari2600Debugger::ProcessPpuCycle() {
	if (_step->HasRequest) {
		if (_step->PpuStepCount > 0) {
			_step->PpuStepCount--;
			if (_step->PpuStepCount <= 0) {
				_step->PpuStepCount = 0;
				_debugger->SleepUntilResume(CpuType::Atari2600, _step->GetBreakSource());
			}
		}
	}
}

void Atari2600Debugger::Run() {
	_step = std::make_unique<StepRequest>();
}

void Atari2600Debugger::Step(int32_t stepCount, StepType type) {
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
			if (Atari2600DisUtils::IsJumpToSub(_prevOpCode)) {
				step.BreakAddress = (_prevProgramCounter + Atari2600DisUtils::GetOpSize(_prevOpCode)) & 0x1FFF;
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
			// Atari 2600: 228 color clocks per scanline, 1 CPU cycle = 3 color clocks, so 76 CPU cycles per scanline
			step.PpuStepCount = 76 * stepCount;
			break;
		case StepType::PpuFrame:
			// ~262 scanlines per NTSC frame * 76 CPU cycles per scanline
			step.PpuStepCount = 76 * 262 * stepCount;
			break;
		case StepType::SpecificScanline:
			step.BreakScanline = stepCount;
			break;
	}
	_step = std::make_unique<StepRequest>(step);
}

StepBackConfig Atari2600Debugger::GetStepBackConfig() {
	return {
		_cachedState.CycleCount,
		76,         // CPU cycles per scanline
		76 * 262    // CPU cycles per frame (NTSC)
	};
}

void Atari2600Debugger::DrawPartialFrame() {
	_console->DebugRenderFrame();
}

DebuggerFeatures Atari2600Debugger::GetSupportedFeatures() {
	DebuggerFeatures features = {};
	features.RunToIrq = false; // 6507 has no IRQ pin
	features.RunToNmi = false; // 6507 has no NMI pin
	features.StepOver = true;
	features.StepOut = true;
	features.StepBack = true;
	features.CallStack = true;
	features.ChangeProgramCounter = AllowChangeProgramCounter;
	features.CpuCycleStep = true;

	// 6507 has only RESET vector (no IRQ/NMI pins)
	features.CpuVectors[0] = {"Reset", 0x1FFC};
	features.CpuVectorCount = 1;

	return features;
}

void Atari2600Debugger::SetProgramCounter(uint32_t addr, bool updateDebuggerOnly) {
	if (!updateDebuggerOnly) {
		Atari2600CpuState state = _console->GetCpuState();
		state.PC = static_cast<uint16_t>(addr & 0x1FFF);
		_console->SetCpuState(state);
	}
	_prevOpCode = _console->DebugRead(addr & 0x1FFF);
	_prevProgramCounter = static_cast<uint16_t>(addr & 0x1FFF);
	_cachedState = _console->GetCpuState();
	_prevStackPointer = _cachedState.SP;
}

uint32_t Atari2600Debugger::GetProgramCounter(bool getInstPc) {
	return getInstPc ? _prevProgramCounter : _console->GetCpuState().PC;
}

CallstackManager* Atari2600Debugger::GetCallstackManager() {
	return _callstackManager.get();
}

BreakpointManager* Atari2600Debugger::GetBreakpointManager() {
	return _breakpointManager.get();
}

IAssembler* Atari2600Debugger::GetAssembler() {
	return _assembler.get();
}

BaseEventManager* Atari2600Debugger::GetEventManager() {
	return _eventManager.get();
}

BaseState& Atari2600Debugger::GetState() {
	_cachedState = _console->GetCpuState();
	return _cachedState;
}

void Atari2600Debugger::GetPpuState(BaseState& state) {
	// TIA state accessible via console
	reinterpret_cast<Atari2600TiaState&>(state) = _console->GetTiaState();
}

void Atari2600Debugger::SetPpuState(BaseState& state) {
	_console->SetTiaState(reinterpret_cast<Atari2600TiaState&>(state));
}

ITraceLogger* Atari2600Debugger::GetTraceLogger() {
	return _traceLogger.get();
}

PpuTools* Atari2600Debugger::GetPpuTools() {
	return _ppuTools.get();
}

bool Atari2600Debugger::SaveRomToDisk(const string& filename, bool saveAsIps, CdlStripOption stripOption) {
	vector<uint8_t> output;

	uint8_t* prgRom = _debugger->GetMemoryDumper()->GetMemoryBuffer(MemoryType::Atari2600PrgRom);
	uint32_t prgRomSize = _debugger->GetMemoryDumper()->GetMemorySize(MemoryType::Atari2600PrgRom);
	vector<uint8_t> rom = vector<uint8_t>(prgRom, prgRom + prgRomSize);

	if (saveAsIps) {
		vector<uint8_t> originalRom;
		(void)_emu->GetRomInfo().RomFile.ReadFile(originalRom);
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

void Atari2600Debugger::ProcessInputOverrides(DebugControllerState inputOverrides[8]) {
	BaseControlManager* controlManager = _console->GetControlManager();
	for (int i = 0; i < 2; i++) {
		shared_ptr<Atari2600Controller> controller = std::dynamic_pointer_cast<Atari2600Controller>(controlManager->GetControlDeviceByIndex(i));
		if (controller && inputOverrides[i].HasPressedButton()) {
			controller->SetBitValue(Atari2600Controller::Buttons::Up, inputOverrides[i].Up);
			controller->SetBitValue(Atari2600Controller::Buttons::Down, inputOverrides[i].Down);
			controller->SetBitValue(Atari2600Controller::Buttons::Left, inputOverrides[i].Left);
			controller->SetBitValue(Atari2600Controller::Buttons::Right, inputOverrides[i].Right);
			controller->SetBitValue(Atari2600Controller::Buttons::Fire, inputOverrides[i].A);
		}
	}
	controlManager->RefreshHubState();
}
