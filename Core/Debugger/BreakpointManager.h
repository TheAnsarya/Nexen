#pragma once
#include "pch.h"
#include "Debugger/Breakpoint.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/DebugUtilities.h"

class ExpressionEvaluator;
class Debugger;
class IDebugger;
class BaseEventManager;
struct ExpressionData;
enum class MemoryOperationType;

/// <summary>
/// Manages breakpoints for specific CPU and handles breakpoint evaluation.
/// </summary>
/// <remarks>
/// Architecture:
/// - One BreakpointManager per CPU type
/// - Owned by CPU-specific IDebugger implementation
/// - Shared expression evaluator for conditional breakpoints
///
/// Breakpoint organization:
/// - Breakpoints grouped by operation type (execute, read, write)
/// - Per-type arrays for fast lookup (no search entire list)
/// - RPN expression cache (reverse polish notation for evaluation)
///
/// Breakpoint evaluation:
/// 1. Fast path: Check if any breakpoints exist for operation type
/// 2. Address match: Linear search through breakpoints for type
/// 3. Condition eval: Evaluate RPN expression if breakpoint has condition
/// 4. Result: Breakpoint ID if match, -1 if no match
///
/// Forbidden breakpoints:
/// - Special breakpoints that prevent other breakpoints from triggering
/// - Used for "break on all except X" scenarios
/// - Checked first before normal breakpoints
///
/// Performance:
/// - __forceinline hot path methods (called every instruction/memory access)
/// - Early exit if no breakpoints for operation type
/// - Access width templates for compile-time optimization
/// </remarks>
class BreakpointManager {
private:
	static constexpr int BreakpointTypeCount = (int)MemoryOperationType::PpuRenderingRead + 1; ///< Max operation types

	Debugger* _debugger;             ///< Main debugger instance
	IDebugger* _cpuDebugger;         ///< CPU-specific debugger
	CpuType _cpuType;                ///< CPU type for this manager
	BaseEventManager* _eventManager; ///< Event manager (for marked breakpoints)

	vector<Breakpoint> _breakpoints[BreakpointTypeCount]; ///< Breakpoints by operation type
	vector<ExpressionData> _rpnList[BreakpointTypeCount]; ///< RPN expression cache per type
	bool _hasBreakpoint;                                  ///< True if any breakpoints exist
	bool _hasBreakpointType[BreakpointTypeCount] = {};    ///< Per-type existence flags

	vector<Breakpoint> _forbidBreakpoints; ///< Forbidden breakpoint list
	vector<ExpressionData> _forbidRpn;     ///< Forbidden RPN expressions

	unique_ptr<ExpressionEvaluator> _bpExpEval; ///< Expression evaluator (for conditions)

	/// <summary>
	/// Convert memory operation type to breakpoint type.
	/// </summary>
	/// <param name="type">Memory operation type</param>
	/// <returns>Corresponding breakpoint type</returns>
	BreakpointType GetBreakpointType(MemoryOperationType type);

	/// <summary>
	/// Internal breakpoint check implementation.
	/// </summary>
	/// <typeparam name="accessWidth">Access width in bytes (1/2/4)</typeparam>
	/// <param name="operationInfo">Memory operation details</param>
	/// <param name="address">Address info (memory type, absolute address)</param>
	/// <param name="processMarkedBreakpoints">True to process mark-only breakpoints</param>
	/// <returns>Breakpoint ID if match, -1 if no match</returns>
	template <uint8_t accessWidth>
	int InternalCheckBreakpoint(MemoryOperationInfo operationInfo, AddressInfo& address, bool processMarkedBreakpoints);

public:
	/// <summary>
	/// Constructor for breakpoint manager.
	/// </summary>
	/// <param name="debugger">Main debugger instance</param>
	/// <param name="cpuDebugger">CPU-specific debugger</param>
	/// <param name="cpuType">CPU type</param>
	/// <param name="eventManager">Event manager (for marking)</param>
	BreakpointManager(Debugger* debugger, IDebugger* cpuDebugger, CpuType cpuType, BaseEventManager* eventManager);

	/// <summary>
	/// Set breakpoint list (replaces all existing).
	/// </summary>
	/// <param name="breakpoints">New breakpoint array</param>
	/// <param name="count">Number of breakpoints</param>
	/// <remarks>
	/// Processing:
	/// 1. Clear existing breakpoints
	/// 2. Group breakpoints by operation type
	/// 3. Compile conditional expressions to RPN
	/// 4. Update per-type existence flags
	/// </remarks>
	void SetBreakpoints(Breakpoint breakpoints[], uint32_t count);

	/// <summary>
	/// Check if breakpoint is forbidden (blocked by forbid list).
	/// </summary>
	/// <param name="memoryOpPtr">Memory operation info</param>
	/// <param name="relAddr">Relative address</param>
	/// <param name="absAddr">Absolute address</param>
	/// <returns>True if breakpoint should not trigger</returns>
	/// <remarks>
	/// Forbidden breakpoints:
	/// - Used for "break on all except X"
	/// - Checked before normal breakpoints
	/// - Example: Break on all writes except to $2000
	/// </remarks>
	bool IsForbidden(MemoryOperationInfo* memoryOpPtr, AddressInfo& relAddr, AddressInfo& absAddr);

	/// <summary>
	/// Check if any breakpoints exist.
	/// </summary>
	/// <returns>True if at least one breakpoint enabled</returns>
	__forceinline bool HasBreakpoints() { return _hasBreakpoint; }

	/// <summary>
	/// Check if breakpoints exist for operation type.
	/// </summary>
	/// <param name="opType">Memory operation type</param>
	/// <returns>True if breakpoints exist for this type</returns>
	/// <remarks>
	/// Inline for performance (called every memory access).
	/// Early exit optimization - avoid breakpoint check if none exist.
	/// </remarks>
	__forceinline bool HasBreakpointForType(MemoryOperationType opType);

	/// <summary>
	/// Check if memory operation triggers breakpoint.
	/// </summary>
	/// <typeparam name="accessWidth">Access width in bytes (1/2/4)</typeparam>
	/// <param name="operationInfo">Memory operation details</param>
	/// <param name="address">Address info</param>
	/// <param name="processMarkedBreakpoints">True to process mark-only breakpoints</param>
	/// <returns>Breakpoint ID if triggered, -1 if no match</returns>
	/// <remarks>
	/// Template specializations:
	/// - accessWidth=1: Byte access (LDA, STA)
	/// - accessWidth=2: Word access (16-bit mode, LDA word)
	/// - accessWidth=4: DMA transfer, 32-bit ARM access
	///
	/// Inline for performance (hot path - every instruction).
	/// </remarks>
	template <uint8_t accessWidth = 1>
	__forceinline int CheckBreakpoint(MemoryOperationInfo operationInfo, AddressInfo& address, bool processMarkedBreakpoints);
};

/// <summary>
/// Inline implementation: Check if breakpoints exist for operation type.
/// </summary>
__forceinline bool BreakpointManager::HasBreakpointForType(MemoryOperationType opType) {
	return _hasBreakpointType[(int)opType];
}

/// <summary>
/// Inline implementation: Check breakpoint with early exit optimization.
/// </summary>
template <uint8_t accessWidth>
__forceinline int BreakpointManager::CheckBreakpoint(MemoryOperationInfo operationInfo, AddressInfo& address, bool processMarkedBreakpoints) {
	if (!_hasBreakpointType[(int)operationInfo.Type]) {
		return -1; // Fast path: No breakpoints for this operation type
	}
	return InternalCheckBreakpoint<accessWidth>(operationInfo, address, processMarkedBreakpoints);
}