#pragma once
#include "pch.h"
#include "Utilities/SimpleLock.h"
#include "Debugger/DebugUtilities.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/DebuggerFeatures.h"
#include "Shared/SettingTypes.h"

class IConsole;
class Emulator;
class SnesCpu;
class SnesPpu;
class Spc;
class BaseCartridge;
class SnesMemoryManager;
class InternalRegisters;
class SnesDmaController;
class EmuSettings;

class ExpressionEvaluator;
class MemoryDumper;
class MemoryAccessCounter;
class Disassembler;
class DisassemblySearch;
class BreakpointManager;
class PpuTools;
class CodeDataLogger;
class CallstackManager;
class LabelManager;
class CdlManager;
class ScriptManager;
class Breakpoint;
class BaseEventManager;
class IAssembler;
class IDebugger;
class ITraceLogger;
class TraceLogFileSaver;
class FrozenAddressManager;

struct TraceRow;
struct BaseState;

enum class EventType;
enum class MemoryOperationType;
enum class EvalResultType : int32_t;

/// <summary>Per-CPU debugger context with expression evaluator</summary>
struct CpuInfo {
	unique_ptr<IDebugger> Debugger;        ///< CPU-specific debugger (breakpoints, stepping)
	unique_ptr<ExpressionEvaluator> Evaluator; ///< Watch/conditional expression evaluator
};

/// <summary>
/// Central debugger coordinator for all emulated systems.
/// Manages breakpoints, watches, disassembly, memory inspection, and scripting.
/// </summary>
/// <remarks>
/// The Debugger provides comprehensive debugging capabilities:
///
/// **Per-CPU Debugging (IDebugger):**
/// - Breakpoints (execution, read, write, IRQ/NMI)
/// - Step in/out/over with proper call stack tracking
/// - Register inspection and modification
/// - Disassembly with labels and comments
///
/// **Memory Tools:**
/// - MemoryDumper: Read/write any memory region
/// - MemoryAccessCounter: Track read/write/execute access patterns
/// - FrozenAddressManager: Freeze memory values
/// - CodeDataLogger: Mark code vs data regions
///
/// **Analysis Tools:**
/// - Disassembler: Multi-CPU disassembly with auto-detection
/// - LabelManager: Symbol/label database
/// - CallstackManager: Track function calls and returns
/// - CdlManager: Import/export CDL files for other tools
///
/// **Scripting:**
/// - ScriptManager: Lua script execution
/// - Event callbacks for memory access, frame, etc.
/// - Input automation for TAS
///
/// **Trace Logging:**
/// - Per-instruction logging with configurable format
/// - File output with TraceLogFileSaver
/// - Memory access logging
///
/// **Threading Model:**
/// - Debugger hooks called from emulation thread
/// - _executionStopped: Pauses emulation for inspection
/// - _breakRequestCount: Pending break requests from UI
/// - _suspendRequestCount: Temporary suspension (stepping)
///
/// **Template Methods:**
/// ProcessMemoryRead/Write: Called on every memory access
/// - Template on CpuType for zero-overhead when not debugging
/// - Checks breakpoints, updates access counters, logs traces
/// </remarks>
class Debugger {
private:
	Emulator* _emu = nullptr;      ///< Parent emulator
	IConsole* _console = nullptr;  ///< Current console instance

	EmuSettings* _settings = nullptr;

	/// Per-CPU debugger instances (indexed by CpuType)
	CpuInfo _debuggers[(int)DebugUtilities::GetLastCpuType() + 1];

	CpuType _mainCpuType = CpuType::Snes;     ///< Primary CPU for this console
	unordered_set<CpuType> _cpuTypes;          ///< All CPUs in current console
	ConsoleType _consoleType = ConsoleType::Snes;

	// Debugging subsystems
	unique_ptr<ScriptManager> _scriptManager;           ///< Lua script execution
	unique_ptr<MemoryDumper> _memoryDumper;             ///< Memory read/write access
	unique_ptr<MemoryAccessCounter> _memoryAccessCounter; ///< Access pattern tracking
	unique_ptr<CodeDataLogger> _codeDataLogger;         ///< Code/data classification
	unique_ptr<Disassembler> _disassembler;             ///< Multi-CPU disassembly
	unique_ptr<DisassemblySearch> _disassemblySearch;   ///< Search disassembly
	unique_ptr<LabelManager> _labelManager;             ///< Symbol/label database
	unique_ptr<CdlManager> _cdlManager;                 ///< CDL file management

	unique_ptr<TraceLogFileSaver> _traceLogSaver;       ///< Trace log file output

	SimpleLock _logLock;
	std::deque<string> _debuggerLog;  ///< Debug message log (deque for cache-friendly ring buffer)

	// Execution control
	atomic<bool> _executionStopped;       ///< Emulation paused for debugging
	atomic<uint32_t> _breakRequestCount;  ///< Pending break requests
	atomic<uint32_t> _suspendRequestCount; ///< Temporary suspensions

	DebugControllerState _inputOverrides[8] = {};  ///< Controller input overrides

	bool _waitForBreakResume = false;

	void Reset();

	__noinline bool ProcessStepBack(IDebugger* debugger);

	// Template helpers for CPU-specific operations
	template <CpuType type, typename DebuggerType>
	DebuggerType* GetDebugger();
	template <CpuType type>
	uint64_t GetCpuCycleCount();
	template <CpuType type, typename T>
	void ProcessScripts(uint32_t addr, T& value, MemoryOperationType opType);
	template <CpuType type, typename T>
	void ProcessScripts(uint32_t addr, T& value, MemoryType memType, MemoryOperationType opType);

	[[nodiscard]] bool IsDebugWindowOpened(CpuType cpuType);
	[[nodiscard]] bool IsBreakOptionEnabled(BreakSource src);
	template <CpuType type>
	void SleepOnBreakRequest();

	void ClearPendingBreakExceptions();

	[[nodiscard]] bool IsBreakpointForbidden(BreakSource source, CpuType sourceCpu, MemoryOperationInfo* operation);

public:
	Debugger(Emulator* emu, IConsole* console);
	~Debugger();
	void Release();

	template <CpuType type>
	void ProcessInstruction();
	template <CpuType type, uint8_t accessWidth = 1, MemoryAccessFlags flags = MemoryAccessFlags::None, typename T>
	void ProcessMemoryRead(uint32_t addr, T& value, MemoryOperationType opType);
	template <CpuType type, uint8_t accessWidth = 1, MemoryAccessFlags flags = MemoryAccessFlags::None, typename T>
	bool ProcessMemoryWrite(uint32_t addr, T& value, MemoryOperationType opType);

	template <CpuType cpuType, MemoryType memType, MemoryOperationType opType, typename T>
	void ProcessMemoryAccess(uint32_t addr, T& value);

	template <CpuType type>
	void ProcessIdleCycle();
	template <CpuType type>
	void ProcessHaltedCpu();
	template <CpuType type, typename T>
	void ProcessPpuRead(uint16_t addr, T& value, MemoryType memoryType, MemoryOperationType opType);
	template <CpuType type, typename T>
	void ProcessPpuWrite(uint16_t addr, T& value, MemoryType memoryType);
	template <CpuType type>
	void ProcessPpuCycle();
	template <CpuType type>
	void ProcessInterrupt(uint32_t originalPc, uint32_t currentPc, bool forNmi);

	void InternalProcessInterrupt(CpuType cpuType, IDebugger& dbg, StepRequest& stepRequest, AddressInfo& src, uint32_t srcAddr, AddressInfo& dest, uint32_t destAddr, AddressInfo& ret, uint32_t retAddr, uint32_t retSp, bool forNmi);

	void ProcessEvent(EventType type, std::optional<CpuType> cpuType);

	void ProcessConfigChange();

	void GetTokenList(CpuType cpuType, char* tokenList);
	int64_t EvaluateExpression(const string& expression, CpuType cpuType, EvalResultType& resultType, bool useCache);

	void Run();
	void PauseOnNextFrame();
	void Step(CpuType cpuType, int32_t stepCount, StepType type, BreakSource source = BreakSource::Unspecified);
	[[nodiscard]] bool IsPaused();
	[[nodiscard]] bool IsExecutionStopped();

	[[nodiscard]] bool HasBreakRequest();
	void BreakRequest(bool release);
	void ResetSuspendCounter();
	void SuspendDebugger(bool release);

	__noinline void BreakImmediately(CpuType sourceCpu, BreakSource source);

	template <uint8_t accessWidth = 1>
	void ProcessPredictiveBreakpoint(CpuType sourceCpu, BreakpointManager* bpManager, MemoryOperationInfo& operation, AddressInfo& addressInfo);
	template <uint8_t accessWidth = 1>
	void ProcessBreakConditions(CpuType sourceCpu, StepRequest& step, BreakpointManager* bpManager, MemoryOperationInfo& operation, AddressInfo& addressInfo);

	void SleepUntilResume(CpuType sourceCpu, BreakSource source, MemoryOperationInfo* operation = nullptr, int breakpointId = -1);

	void GetCpuState(BaseState& dstState, CpuType cpuType);
	void SetCpuState(BaseState& srcState, CpuType cpuType);
	BaseState& GetCpuStateRef(CpuType cpuType);

	void GetPpuState(BaseState& state, CpuType cpuType);
	void SetPpuState(BaseState& srcState, CpuType cpuType);

	void GetConsoleState(BaseState& state, ConsoleType consoleType);

	[[nodiscard]] DebuggerFeatures GetDebuggerFeatures(CpuType cpuType);
	[[nodiscard]] uint32_t GetProgramCounter(CpuType cpuType, bool forInstStart);
	[[nodiscard]] uint8_t GetCpuFlags(CpuType cpuType, uint32_t addr);
	[[nodiscard]] CpuInstructionProgress GetInstructionProgress(CpuType cpuType);
	void SetProgramCounter(CpuType cpuType, uint32_t addr);

	[[nodiscard]] AddressInfo GetAbsoluteAddress(AddressInfo relAddress);
	[[nodiscard]] AddressInfo GetRelativeAddress(AddressInfo absAddress, CpuType cpuType);

	[[nodiscard]] bool HasCpuType(CpuType cpuType);

	void SetBreakpoints(Breakpoint breakpoints[], uint32_t length);

	void SetInputOverrides(uint32_t index, DebugControllerState state);
	void GetAvailableInputOverrides(uint8_t* availableIndexes);

	void Log(const string& message);
	[[nodiscard]] string GetLog();

	[[nodiscard]] bool SaveRomToDisk(const string& filename, bool saveAsIps, CdlStripOption stripOption);

	void ClearExecutionTrace();
	[[nodiscard]] uint32_t GetExecutionTrace(TraceRow output[], uint32_t startOffset, uint32_t maxLineCount);

	[[nodiscard]] CpuType GetMainCpuType() { return _mainCpuType; }
	[[nodiscard]] IDebugger* GetMainDebugger();

	[[nodiscard]] TraceLogFileSaver* GetTraceLogFileSaver() { return _traceLogSaver.get(); }
	[[nodiscard]] MemoryDumper* GetMemoryDumper() { return _memoryDumper.get(); }
	[[nodiscard]] MemoryAccessCounter* GetMemoryAccessCounter() { return _memoryAccessCounter.get(); }
	[[nodiscard]] Disassembler* GetDisassembler() { return _disassembler.get(); }
	[[nodiscard]] DisassemblySearch* GetDisassemblySearch() { return _disassemblySearch.get(); }
	[[nodiscard]] LabelManager* GetLabelManager() { return _labelManager.get(); }
	[[nodiscard]] CdlManager* GetCdlManager() { return _cdlManager.get(); }
	[[nodiscard]] ScriptManager* GetScriptManager() { return _scriptManager.get(); }
	[[nodiscard]] IConsole* GetConsole() { return _console; }
	[[nodiscard]] Emulator* GetEmulator() { return _emu; }

	[[nodiscard]] FrozenAddressManager* GetFrozenAddressManager(CpuType cpuType);
	[[nodiscard]] ITraceLogger* GetTraceLogger(CpuType cpuType);
	[[nodiscard]] PpuTools* GetPpuTools(CpuType cpuType);
	[[nodiscard]] BaseEventManager* GetEventManager(CpuType cpuType);
	[[nodiscard]] CallstackManager* GetCallstackManager(CpuType cpuType);
	[[nodiscard]] IAssembler* GetAssembler(CpuType cpuType);
};
