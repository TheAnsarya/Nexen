#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/IDebugger.h"

class Disassembler;
class Debugger;
class CallstackManager;
class MemoryAccessCounter;
class CodeDataLogger;
class EmuSettings;
class BreakpointManager;
class IAssembler;
class BaseEventManager;
class LynxTraceLogger;
class Emulator;
class LynxConsole;
class LynxCpu;
class LynxMemoryManager;
class LynxPpuTools;

enum class MemoryOperationType;

/// <summary>
/// Atari Lynx debugger — 65C02 CPU debugger implementation.
///
/// Provides instruction stepping, breakpoints, callstack,
/// code data logger, trace logging, and memory inspection.
/// Follows the same pattern as PceDebugger (also 6502-based).
/// </summary>
class LynxDebugger final : public IDebugger {
	Debugger* _debugger = nullptr;
	Emulator* _emu = nullptr;
	Disassembler* _disassembler = nullptr;
	MemoryAccessCounter* _memoryAccessCounter = nullptr;
	EmuSettings* _settings = nullptr;

	LynxConsole* _console = nullptr;
	LynxCpu* _cpu = nullptr;
	LynxMemoryManager* _memoryManager = nullptr;

	unique_ptr<CodeDataLogger> _codeDataLogger;
	unique_ptr<BaseEventManager> _eventManager;
	unique_ptr<CallstackManager> _callstackManager;
	unique_ptr<BreakpointManager> _breakpointManager;
	unique_ptr<LynxTraceLogger> _traceLogger;
	unique_ptr<IAssembler> _assembler;
	unique_ptr<LynxPpuTools> _ppuTools;

	uint8_t _prevOpCode = 0x01;
	uint32_t _prevProgramCounter = 0;
	uint8_t _prevStackPointer = 0;

	string _cdlFile;

	__forceinline void ProcessCallStackUpdates(AddressInfo& destAddr, uint16_t destPc, uint8_t sp);
	bool IsRegister(uint32_t addr);

public:
	LynxDebugger(Debugger* debugger);
	~LynxDebugger();

	void OnBeforeBreak(CpuType cpuType) override;
	void Reset() override;

	uint64_t GetCpuCycleCount(bool forProfiler) override;
	void ResetPrevOpCode() override;

	void ProcessInstruction();
	void ProcessRead(uint32_t addr, uint8_t value, MemoryOperationType type);
	void ProcessWrite(uint32_t addr, uint8_t value, MemoryOperationType type);

	void ProcessInterrupt(uint32_t originalPc, uint32_t currentPc, bool forNmi) override;
	void ProcessPpuCycle();

	void Run() override;
	void Step(int32_t stepCount, StepType type) override;
	StepBackConfig GetStepBackConfig() override;

	void DrawPartialFrame() override;

	DebuggerFeatures GetSupportedFeatures() override;
	void SetProgramCounter(uint32_t addr, bool updateDebuggerOnly = false) override;
	uint32_t GetProgramCounter(bool getInstPc) override;

	BreakpointManager* GetBreakpointManager() override;
	ITraceLogger* GetTraceLogger() override;
	PpuTools* GetPpuTools() override;
	bool SaveRomToDisk(string filename, bool saveAsIps, CdlStripOption stripOption);
	void ProcessInputOverrides(DebugControllerState inputOverrides[8]) override;
	CallstackManager* GetCallstackManager() override;
	IAssembler* GetAssembler() override;
	BaseEventManager* GetEventManager() override;

	BaseState& GetState() override;
	void GetPpuState(BaseState& state) override;
	void SetPpuState(BaseState& state) override;
};
