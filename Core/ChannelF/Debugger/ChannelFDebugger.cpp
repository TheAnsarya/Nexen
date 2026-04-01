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
#include "ChannelF/ChannelFConsole.h"
#include "ChannelF/ChannelFTypes.h"
#include "ChannelF/Debugger/ChannelFDebugger.h"
#include "ChannelF/Debugger/ChannelFDisUtils.h"
#include "ChannelF/Debugger/ChannelFTraceLogger.h"
#include "ChannelF/Debugger/ChannelFEventManager.h"
#include "Shared/BaseControlManager.h"
#include "ChannelF/ChannelFController.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/FolderUtilities.h"
#include "Utilities/Patches/IpsPatcher.h"
#include "Shared/EmuSettings.h"
#include "Shared/SettingTypes.h"
#include "Shared/Emulator.h"
#include "Shared/MemoryOperationType.h"

ChannelFDebugger::ChannelFDebugger(Debugger* debugger) : IDebugger(debugger->GetEmulator()) {
	_debugger = debugger;
	_emu = debugger->GetEmulator();
	ChannelFConsole* console = (ChannelFConsole*)debugger->GetConsole();

	_console = console;

	_disassembler = debugger->GetDisassembler();
	_memoryAccessCounter = debugger->GetMemoryAccessCounter();
	_settings = debugger->GetEmulator()->GetSettings();

	_codeDataLogger = std::make_unique<CodeDataLogger>(debugger, MemoryType::ChannelFCartRom,
		_emu->GetMemory(MemoryType::ChannelFCartRom).Size, CpuType::ChannelF, _emu->GetCrc32());

	try {
		_cdlFile = _codeDataLogger->GetCdlFilePath(_emu->GetRomInfo().RomFile.GetFileName());
		_codeDataLogger->LoadCdlFile(_cdlFile, _settings->GetDebugConfig().AutoResetCdl);
	} catch (std::exception&) {
		_cdlFile.clear();
	}

	_stepBackManager = std::make_unique<StepBackManager>(_emu, this);
	_eventManager = std::make_unique<ChannelFEventManager>(debugger, console);
	_callstackManager = std::make_unique<CallstackManager>(debugger, this);
	_breakpointManager = std::make_unique<BreakpointManager>(debugger, this, CpuType::ChannelF, _eventManager.get());
	_traceLogger = std::make_unique<ChannelFTraceLogger>(debugger, this, console);
	_step = std::make_unique<StepRequest>();
}

ChannelFDebugger::~ChannelFDebugger() {
	if (!_cdlFile.empty()) {
		_codeDataLogger->SaveCdlFile(_cdlFile);
	}
}

void ChannelFDebugger::OnBeforeBreak([[maybe_unused]] CpuType cpuType) {
}

void ChannelFDebugger::Reset() {
	_callstackManager->Clear();
	ResetPrevOpCode();
}

uint64_t ChannelFDebugger::GetCpuCycleCount([[maybe_unused]] bool forProfiler) {
	return _cachedState.CycleCount;
}

void ChannelFDebugger::ResetPrevOpCode() {
	_prevOpCode = 0x2b; // NOP
}

void ChannelFDebugger::ProcessInstruction() {
	_cachedState = _console->GetCpuState();
	uint16_t pc = _cachedState.PC0;
	uint8_t opCode = _console->DebugRead(pc);
	AddressInfo relAddr{(int32_t)pc, MemoryType::ChannelFBiosRom};
	AddressInfo addressInfo = _console->GetAbsoluteAddress(relAddr);
	MemoryOperationInfo operation(pc, opCode, MemoryOperationType::ExecOpCode, MemoryType::ChannelFBiosRom);
	InstructionProgress.LastMemOperation = operation;
	InstructionProgress.StartCycle = _cachedState.CycleCount;

	bool needDisassemble = _traceLogger->IsEnabled() || _settings->CheckDebuggerFlag(DebuggerFlags::ChannelFDebuggerEnabled);
	if (addressInfo.Address >= 0) {
		if (addressInfo.Type == MemoryType::ChannelFCartRom) {
			_codeDataLogger->SetCode(addressInfo.Address, ChannelFDisUtils::GetOpFlags(_prevOpCode, pc, _prevProgramCounter));
		}
		if (needDisassemble) {
			_disassembler->BuildCache(addressInfo, 0, CpuType::ChannelF);
		}
	}

	ProcessCallStackUpdates(addressInfo, pc);

	_prevOpCode = opCode;
	_prevProgramCounter = pc;

	_step->ProcessCpuExec();

	if (_settings->CheckDebuggerFlag(DebuggerFlags::ChannelFDebuggerEnabled)) {
		if (_breakpointManager) {
			_debugger->ProcessBreakConditions(CpuType::ChannelF, *_step.get(), _breakpointManager.get(), operation, addressInfo);
		}
	}
}

void ChannelFDebugger::ProcessRead(uint32_t addr, uint8_t value, MemoryOperationType type) {
	AddressInfo relAddr = {(int32_t)addr, MemoryType::ChannelFBiosRom};
	AddressInfo addressInfo = _console->GetAbsoluteAddress(relAddr);
	MemoryOperationInfo operation(addr, value, type, MemoryType::ChannelFBiosRom);
	InstructionProgress.LastMemOperation = operation;

	if (addressInfo.Address >= 0 && addressInfo.Type == MemoryType::ChannelFCartRom) {
		_codeDataLogger->SetData(addressInfo.Address);
	}

	if (type == MemoryOperationType::ExecOpCode) {
		if (_traceLogger->IsEnabled()) {
			_cachedState = _console->GetCpuState();
			DisassemblyInfo disInfo = _disassembler->GetDisassemblyInfo(addressInfo, addr, 0, CpuType::ChannelF);
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

	if (_settings->CheckDebuggerFlag(DebuggerFlags::ChannelFDebuggerEnabled)) {
		if (_breakpointManager) {
			_debugger->ProcessBreakConditions(CpuType::ChannelF, *_step.get(), _breakpointManager.get(), operation, addressInfo);
		}
	}
}

void ChannelFDebugger::ProcessWrite(uint32_t addr, uint8_t value, MemoryOperationType type) {
	AddressInfo relAddr = {(int32_t)addr, MemoryType::ChannelFBiosRom};
	AddressInfo addressInfo = _console->GetAbsoluteAddress(relAddr);
	MemoryOperationInfo operation(addr, value, type, MemoryType::ChannelFBiosRom);
	InstructionProgress.LastMemOperation = operation;

	if (_traceLogger->IsEnabled()) {
		_traceLogger->LogNonExec(operation, addressInfo);
	}

	_memoryAccessCounter->ProcessMemoryWrite(addressInfo, _cachedState.CycleCount);
	_step->ProcessCpuCycle();

	if (_settings->CheckDebuggerFlag(DebuggerFlags::ChannelFDebuggerEnabled)) {
		if (_breakpointManager) {
			_debugger->ProcessBreakConditions(CpuType::ChannelF, *_step.get(), _breakpointManager.get(), operation, addressInfo);
		}
	}
}

void ChannelFDebugger::ProcessCallStackUpdates(AddressInfo& destAddr, uint16_t destPc) {
	if (ChannelFDisUtils::IsJumpToSub(_prevOpCode)) {
		uint8_t opSize = ChannelFDisUtils::GetOpSize(_prevOpCode);
		uint32_t returnPc = _prevProgramCounter + opSize;
		AddressInfo relPrev = {(int32_t)_prevProgramCounter, MemoryType::ChannelFBiosRom};
		AddressInfo relRet = {(int32_t)returnPc, MemoryType::ChannelFBiosRom};
		AddressInfo srcAddress = _console->GetAbsoluteAddress(relPrev);
		AddressInfo retAddress = _console->GetAbsoluteAddress(relRet);
		_callstackManager->Push(srcAddress, _prevProgramCounter, destAddr, destPc, retAddress, returnPc, 0, StackFrameFlags::None);
	} else if (ChannelFDisUtils::IsReturnInstruction(_prevOpCode)) {
		_callstackManager->Pop(destAddr, destPc, 0);

		if (_step->BreakAddress == static_cast<int32_t>(destPc)) {
			_step->Break(BreakSource::CpuStep);
		}
	}
}

void ChannelFDebugger::ProcessInterrupt(uint32_t originalPc, uint32_t currentPc, [[maybe_unused]] bool forNmi) {
	AddressInfo relOrig = {(int32_t)originalPc, MemoryType::ChannelFBiosRom};
	AddressInfo relCur = {(int32_t)currentPc, MemoryType::ChannelFBiosRom};
	AddressInfo ret = _console->GetAbsoluteAddress(relOrig);
	AddressInfo dest = _console->GetAbsoluteAddress(relCur);

	if (dest.Type == MemoryType::ChannelFCartRom && dest.Address >= 0) {
		_codeDataLogger->SetCode(dest.Address, CdlFlags::SubEntryPoint);
	}

	_cachedState = _console->GetCpuState();
	ResetPrevOpCode();
}

void ChannelFDebugger::Run() {
	_step = std::make_unique<StepRequest>();
}

void ChannelFDebugger::Step(int32_t stepCount, StepType type) {
	StepRequest step(type);
	switch (type) {
		case StepType::Step:
			step.StepCount = stepCount;
			break;
		case StepType::StepOut:
			step.BreakAddress = _callstackManager->GetReturnAddress();
			break;
		case StepType::StepOver:
			if (ChannelFDisUtils::IsJumpToSub(_prevOpCode)) {
				step.BreakAddress = _prevProgramCounter + ChannelFDisUtils::GetOpSize(_prevOpCode);
			} else {
				step.StepCount = 1;
			}
			break;
		case StepType::CpuCycleStep:
			step.CpuCycleStepCount = stepCount;
			break;
		case StepType::PpuStep:
		case StepType::PpuScanline:
		case StepType::PpuFrame:
			// Channel F has no separate PPU — step by CPU cycles
			step.CpuCycleStepCount = stepCount;
			break;
		case StepType::SpecificScanline:
			step.BreakScanline = stepCount;
			break;
	}
	_step = std::make_unique<StepRequest>(step);
}

StepBackConfig ChannelFDebugger::GetStepBackConfig() {
	uint32_t cyclesPerFrame = (uint32_t)(_console->GetMasterClockRate() / _console->GetFps());
	return {
		_cachedState.CycleCount,
		128,                                     // Cycles per "scanline" (arbitrary for Channel F)
		cyclesPerFrame                           // Cycles per frame
	};
}

void ChannelFDebugger::DrawPartialFrame() {
	_console->DebugRenderFrame();
}

DebuggerFeatures ChannelFDebugger::GetSupportedFeatures() {
	DebuggerFeatures features = {};
	features.RunToIrq = false;
	features.RunToNmi = false;
	features.StepOver = true;
	features.StepOut = true;
	features.StepBack = true;
	features.CallStack = true;
	features.ChangeProgramCounter = AllowChangeProgramCounter;
	features.CpuCycleStep = true;

	// F8 has no traditional interrupt vectors exposed as CPU vectors
	features.CpuVectorCount = 0;

	return features;
}

void ChannelFDebugger::SetProgramCounter(uint32_t addr, bool updateDebuggerOnly) {
	if (!updateDebuggerOnly) {
		ChannelFCpuState state = _console->GetCpuState();
		state.PC0 = static_cast<uint16_t>(addr);
		_console->SetCpuState(state);
	}
	_prevOpCode = _console->DebugRead(static_cast<uint16_t>(addr));
	_prevProgramCounter = addr;
	_cachedState = _console->GetCpuState();
}

uint32_t ChannelFDebugger::GetProgramCounter(bool getInstPc) {
	return getInstPc ? _prevProgramCounter : _console->GetCpuState().PC0;
}

CallstackManager* ChannelFDebugger::GetCallstackManager() {
	return _callstackManager.get();
}

BreakpointManager* ChannelFDebugger::GetBreakpointManager() {
	return _breakpointManager.get();
}

IAssembler* ChannelFDebugger::GetAssembler() {
	return nullptr;
}

BaseEventManager* ChannelFDebugger::GetEventManager() {
	return _eventManager.get();
}

BaseState& ChannelFDebugger::GetState() {
	_cachedState = _console->GetCpuState();
	return _cachedState;
}

void ChannelFDebugger::GetPpuState([[maybe_unused]] BaseState& state) {
	// Channel F has no separate PPU
}

void ChannelFDebugger::SetPpuState([[maybe_unused]] BaseState& state) {
	// Channel F has no separate PPU
}

ITraceLogger* ChannelFDebugger::GetTraceLogger() {
	return _traceLogger.get();
}

PpuTools* ChannelFDebugger::GetPpuTools() {
	return nullptr; // No separate PPU tools for Channel F
}

bool ChannelFDebugger::SaveRomToDisk(const string& filename, bool saveAsIps, CdlStripOption stripOption) {
	vector<uint8_t> output;

	uint8_t* cartRom = _debugger->GetMemoryDumper()->GetMemoryBuffer(MemoryType::ChannelFCartRom);
	uint32_t cartRomSize = _debugger->GetMemoryDumper()->GetMemorySize(MemoryType::ChannelFCartRom);
	vector<uint8_t> rom = vector<uint8_t>(cartRom, cartRom + cartRomSize);

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

void ChannelFDebugger::ProcessInputOverrides(DebugControllerState inputOverrides[8]) {
	BaseControlManager* controlManager = _console->GetControlManager();
	for (int i = 0; i < 2; i++) {
		shared_ptr<ChannelFController> controller = std::dynamic_pointer_cast<ChannelFController>(controlManager->GetControlDeviceByIndex(i));
		if (controller && inputOverrides[i].HasPressedButton()) {
			controller->SetBitValue(ChannelFController::Buttons::Right, inputOverrides[i].Right);
			controller->SetBitValue(ChannelFController::Buttons::Left, inputOverrides[i].Left);
			controller->SetBitValue(ChannelFController::Buttons::Back, inputOverrides[i].Down);
			controller->SetBitValue(ChannelFController::Buttons::Forward, inputOverrides[i].Up);
			controller->SetBitValue(ChannelFController::Buttons::Push, inputOverrides[i].A);
			controller->SetBitValue(ChannelFController::Buttons::Pull, inputOverrides[i].B);
			controller->SetBitValue(ChannelFController::Buttons::TwistCW, inputOverrides[i].R);
			controller->SetBitValue(ChannelFController::Buttons::TwistCCW, inputOverrides[i].L);
		}
	}
	controlManager->RefreshHubState();
}

template <MemoryOperationType opType>
void ChannelFDebugger::ProcessMemoryAccess(uint32_t addr, uint8_t value, MemoryType memType) {
	MemoryOperationInfo operation(addr, value, opType, memType);
	_eventManager->AddEvent(DebugEventType::Register, operation);
}

template void ChannelFDebugger::ProcessMemoryAccess<MemoryOperationType::Read>(uint32_t addr, uint8_t value, MemoryType memType);
template void ChannelFDebugger::ProcessMemoryAccess<MemoryOperationType::Write>(uint32_t addr, uint8_t value, MemoryType memType);
