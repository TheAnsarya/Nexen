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
class ChannelFTraceLogger;
class Emulator;
class ChannelFConsole;

enum class MemoryOperationType;

class ChannelFDebugger final : public IDebugger {
	Debugger* _debugger = nullptr;
	Emulator* _emu = nullptr;
	Disassembler* _disassembler = nullptr;
	MemoryAccessCounter* _memoryAccessCounter = nullptr;
	EmuSettings* _settings = nullptr;

	ChannelFConsole* _console = nullptr;

	unique_ptr<CodeDataLogger> _codeDataLogger;
	unique_ptr<BaseEventManager> _eventManager;
	unique_ptr<CallstackManager> _callstackManager;
	unique_ptr<BreakpointManager> _breakpointManager;
	unique_ptr<ChannelFTraceLogger> _traceLogger;

	uint8_t _prevOpCode = 0x2b; // NOP
	uint32_t _prevProgramCounter = 0;

	ChannelFCpuState _cachedState = {};

	string _cdlFile;

	__forceinline void ProcessCallStackUpdates(AddressInfo& destAddr, uint16_t destPc);

public:
	ChannelFDebugger(Debugger* debugger);
	~ChannelFDebugger();

	void OnBeforeBreak(CpuType cpuType) override;
	void Reset() override;

	uint64_t GetCpuCycleCount(bool forProfiler) override;
	void ResetPrevOpCode() override;

	void ProcessInstruction();
	void ProcessRead(uint32_t addr, uint8_t value, MemoryOperationType type);
	void ProcessWrite(uint32_t addr, uint8_t value, MemoryOperationType type);

	template <MemoryOperationType opType>
	void ProcessMemoryAccess(uint32_t addr, uint8_t value, MemoryType memType);

	void ProcessInterrupt(uint32_t originalPc, uint32_t currentPc, bool forNmi) override;

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
	bool SaveRomToDisk(const string& filename, bool saveAsIps, CdlStripOption stripOption);
	void ProcessInputOverrides(DebugControllerState inputOverrides[8]) override;
	CallstackManager* GetCallstackManager() override;
	IAssembler* GetAssembler() override;
	BaseEventManager* GetEventManager() override;

	BaseState& GetState() override;
	void GetPpuState(BaseState& state) override;
	void SetPpuState(BaseState& state) override;
};
