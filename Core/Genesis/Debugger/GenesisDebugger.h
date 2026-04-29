#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/IDebugger.h"
#include "Genesis/GenesisTypes.h"

class Disassembler;
class Debugger;
class CallstackManager;
class MemoryAccessCounter;
class BreakpointManager;
class EmuSettings;
class Emulator;
class GenesisConsole;
class GenesisM68k;
class GenesisMemoryManager;

enum class MemoryOperationType;

class GenesisDebugger final : public IDebugger {
	Debugger* _debugger = nullptr;
	Emulator* _emu = nullptr;
	Disassembler* _disassembler = nullptr;
	MemoryAccessCounter* _memoryAccessCounter = nullptr;
	EmuSettings* _settings = nullptr;

	GenesisConsole* _console = nullptr;
	GenesisM68k* _cpu = nullptr;
	GenesisMemoryManager* _memoryManager = nullptr;

	unique_ptr<CallstackManager> _callstackManager;
	unique_ptr<BreakpointManager> _breakpointManager;

	GenesisM68kState _cachedState = {};
	uint16_t _prevOpCode = 0;
	uint32_t _prevProgramCounter = 0;

public:
	GenesisDebugger(Debugger* debugger);
	~GenesisDebugger();

	void OnBeforeBreak(CpuType cpuType) override;
	void Reset() override;

	void ProcessInstruction();
	void ProcessRead(uint32_t addr, uint8_t value, MemoryOperationType type);
	void ProcessWrite(uint32_t addr, uint8_t value, MemoryOperationType type);

	void ProcessInterrupt(uint32_t originalPc, uint32_t currentPc, bool forNmi) override;

	void Run() override;
	void Step(int32_t stepCount, StepType type) override;
	StepBackConfig GetStepBackConfig() override;

	void DrawPartialFrame() override;

	DebuggerFeatures GetSupportedFeatures() override;
	void SetProgramCounter(uint32_t addr, bool updateDebuggerOnly = false) override;
	uint32_t GetProgramCounter(bool getInstPc) override;
	uint64_t GetCpuCycleCount(bool forProfiler) override;

	BreakpointManager* GetBreakpointManager() override;
	ITraceLogger* GetTraceLogger() override;
	PpuTools* GetPpuTools() override;
	void ProcessInputOverrides(DebugControllerState inputOverrides[8]) override;
	CallstackManager* GetCallstackManager() override;
	IAssembler* GetAssembler() override;
	BaseEventManager* GetEventManager() override;

	BaseState& GetState() override;
	void GetPpuState(BaseState& state) override;
	void SetPpuState(BaseState& state) override;
};
