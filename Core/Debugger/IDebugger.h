#pragma once
#include "pch.h"
#include "Debugger/DebuggerFeatures.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/StepBackManager.h"
#include "Debugger/FrozenAddressManager.h"

enum class StepType;
class BreakpointManager;
class CallstackManager;
class IAssembler;
class BaseEventManager;
class CodeDataLogger;
class ITraceLogger;
class PpuTools;
class Emulator;
struct BaseState;
enum class EventType;
enum class MemoryOperationType;

/// <summary>
/// Abstract base class for CPU-specific debuggers.
/// Implemented by SnesDebugger, NesDebugger, GbDebugger, GbaDebugger, PceDebugger, SmsDebugger, WsDebugger.
/// </summary>
/// <remarks>
/// Architecture:
/// - Debugger owns array of IDebugger instances (one per CPU type)
/// - Each IDebugger manages CPU-specific debug state and tools
/// - Provides abstractions for stepping, breakpoints, disassembly, tracing
/// 
/// Core responsibilities:
/// - Execution control (run, step, step back, breakpoints)
/// - State inspection (registers, memory, CPU flags)
/// - Address translation (relative <-> absolute)
/// - Breakpoint management (per-CPU breakpoint lists)
/// - Callstack tracking (subroutine call/return)
/// - Trace logging (instruction execution history)
/// - Memory freezing (lock values for debugging/cheating)
/// 
/// Debug features:
/// - Step execution (into, over, out, back)
/// - Breakpoints (execute, read, write, conditional)
/// - Rewind/step-back (restore previous CPU states)
/// - Frozen addresses (prevent memory writes)
/// - Input overrides (TAS-style frame advance)
/// 
/// Thread model:
/// - All methods called from emulation thread
/// - Step() may block until breakpoint hit
/// - StepBack() rewinds emulation state
/// </remarks>
// TODOv2 rename/refactor to BaseDebugger
class IDebugger {
protected:
	unique_ptr<StepRequest> _step;                   ///< Active step request (step into/over/out)
	unique_ptr<StepBackManager> _stepBackManager;    ///< Rewind/step-back state manager

	FrozenAddressManager _frozenAddressManager;      ///< Locked memory addresses

public:
	bool IgnoreBreakpoints = false;                  ///< Temporarily disable breakpoints
	bool AllowChangeProgramCounter = false;          ///< Allow PC modification during debugging
	CpuInstructionProgress InstructionProgress = {}; ///< Current instruction fetch/decode state

	/// <summary>
	/// Constructor with step-back manager initialization.
	/// </summary>
	/// <param name="emu">Emulator instance</param>
	IDebugger(Emulator* emu) : _stepBackManager(new StepBackManager(emu, this)) {}
	virtual ~IDebugger() = default;

	/// <summary>
	/// Get active step request.
	/// </summary>
	/// <returns>Step request or nullptr if not stepping</returns>
	StepRequest* GetStepRequest() { return _step.get(); }
	
	/// <summary>
	/// Check if step-back should trigger.
	/// </summary>
	/// <returns>True if step-back requested and available</returns>
	bool CheckStepBack() { return _stepBackManager->CheckStepBack(); }
	
	/// <summary>
	/// Check if currently rewinding (step-back in progress).
	/// </summary>
	bool IsStepBack() { return _stepBackManager->IsRewinding(); }
	
	/// <summary>
	/// Clear step-back history cache.
	/// </summary>
	void ResetStepBackCache() { return _stepBackManager->ResetCache(); }
	
	/// <summary>
	/// Step back N instructions or frames.
	/// </summary>
	/// <param name="stepCount">Number of steps (negative for instructions, positive for frames)</param>
	void StepBack(int32_t stepCount) { return _stepBackManager->StepBack((StepBackType)stepCount); }
	
	/// <summary>
	/// Get configuration for step-back feature.
	/// </summary>
	/// <returns>Step-back config with cycle count and cache settings</returns>
	virtual StepBackConfig GetStepBackConfig() { return {GetCpuCycleCount(), 0, 0}; }

	/// <summary>
	/// Get frozen address manager.
	/// </summary>
	FrozenAddressManager& GetFrozenAddressManager() { return _frozenAddressManager; }

	/// <summary>
	/// Reset previous opcode tracker (for instruction history).
	/// </summary>
	virtual void ResetPrevOpCode() {}

	/// <summary>
	/// Callback before breakpoint triggers.
	/// </summary>
	/// <param name="cpuType">CPU that hit breakpoint</param>
	virtual void OnBeforeBreak(CpuType cpuType) {}

	/// <summary>
	/// Step execution (into, over, out, back).
	/// </summary>
	/// <param name="stepCount">Number of steps (1 for single step)</param>
	/// <param name="type">Step type (into/over/out/back)</param>
	virtual void Step(int32_t stepCount, StepType type) = 0;
	
	/// <summary>
	/// Reset debugger state (clear breakpoints, reset step).
	/// </summary>
	virtual void Reset() = 0;
	
	/// <summary>
	/// Resume execution (clear step request).
	/// </summary>
	virtual void Run() = 0;

	/// <summary>
	/// Initialize debugger (load symbols, setup tools).
	/// </summary>
	virtual void Init() {}
	
	/// <summary>
	/// Process debugger configuration change (settings update).
	/// </summary>
	virtual void ProcessConfigChange() {}

	/// <summary>
	/// Process CPU interrupt (IRQ/NMI) for callstack/profiler.
	/// </summary>
	/// <param name="originalPc">PC before interrupt vector</param>
	/// <param name="currentPc">PC after interrupt vector (handler address)</param>
	/// <param name="forNmi">True if NMI, false if IRQ</param>
	virtual void ProcessInterrupt(uint32_t originalPc, uint32_t currentPc, bool forNmi) {}
	
	/// <summary>
	/// Process input overrides from debugger.
	/// </summary>
	/// <param name="inputOverrides">Controller state overrides (8 ports)</param>
	virtual void ProcessInputOverrides(DebugControllerState inputOverrides[8]) {}

	/// <summary>
	/// Draw partial frame (for mid-frame debugging).
	/// </summary>
	virtual void DrawPartialFrame() {}

	/// <summary>
	/// Get supported debugger features for this CPU.
	/// </summary>
	virtual DebuggerFeatures GetSupportedFeatures() { return {}; }
	
	/// <summary>
	/// Get CPU cycle count since power-on.
	/// </summary>
	/// <param name="forProfiler">True for profiler (may use different clock)</param>
	virtual uint64_t GetCpuCycleCount(bool forProfiler = false) { return 0; }
	
	/// <summary>
	/// Get program counter value.
	/// </summary>
	/// <param name="getInstPc">True to get instruction start PC, false for current PC</param>
	virtual uint32_t GetProgramCounter(bool getInstPc) = 0;
	
	/// <summary>
	/// Set program counter value.
	/// </summary>
	/// <param name="addr">New PC value</param>
	/// <param name="updateDebuggerOnly">True to update debug view only (not actual CPU)</param>
	virtual void SetProgramCounter(uint32_t addr, bool updateDebuggerOnly = false) = 0;

	/// <summary>
	/// Get CPU flags for address (code/data/indirect)
	/// </summary>
	/// <param name="addr">Memory address</param>
	/// <returns>CPU flags byte</returns>
	virtual uint8_t GetCpuFlags(uint32_t addr) { return 0; }

	/// <summary>
	/// Get breakpoint manager.
	/// </summary>
	virtual BreakpointManager* GetBreakpointManager() = 0;
	
	/// <summary>
	/// Get callstack manager.
	/// </summary>
	virtual CallstackManager* GetCallstackManager() = 0;
	
	/// <summary>
	/// Get CPU-specific assembler.
	/// </summary>
	virtual IAssembler* GetAssembler() = 0;
	
	/// <summary>
	/// Get event manager (for PPU/APU events).
	/// </summary>
	virtual BaseEventManager* GetEventManager() = 0;
	
	/// <summary>
	/// Get trace logger.
	/// </summary>
	virtual ITraceLogger* GetTraceLogger() = 0;
	
	/// <summary>
	/// Get PPU tools (if available for this platform).
	/// </summary>
	virtual PpuTools* GetPpuTools() { return nullptr; }

	/// <summary>
	/// Get ROM header data.
	/// </summary>
	/// <param name="headerData">Output header buffer</param>
	/// <param name="size">Output header size</param>
	virtual void GetRomHeader(uint8_t* headerData, uint32_t& size) {}

	/// <summary>
	/// Get CPU state.
	/// </summary>
	virtual BaseState& GetState() = 0;
	
	/// <summary>
	/// Get PPU state.
	/// </summary>
	/// <param name="state">Output PPU state</param>
	virtual void GetPpuState(BaseState& state) {};
	
	/// <summary>
	/// Set PPU state.
	/// </summary>
	/// <param name="state">Input PPU state</param>
	virtual void SetPpuState(BaseState& state) {};
};
