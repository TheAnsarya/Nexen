#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"

class Debugger;
class Profiler;
class IDebugger;

/// <summary>
/// Manages CPU call stack tracking and profiling.
/// </summary>
/// <remarks>
/// Architecture:
/// - One CallstackManager per CPU
/// - Tracks subroutine call/return flow
/// - Provides callstack for debugger UI
/// - Integrates with profiler for performance analysis
///
/// Call stack tracking:
/// - Push on JSR/CALL (subroutine call)
/// - Pop on RTS/RET (subroutine return)
/// - Detects invalid returns (stack corruption)
/// - Handles interrupts (IRQ/NMI) separately
///
/// Stack frame information:
/// - Source address (where JSR executed)
/// - Destination address (subroutine entry point)
/// - Return address (where RTS will return to)
/// - Stack pointer at call time
/// - Flags (interrupt, function call, etc.)
///
/// Profiler integration:
/// - Tracks time spent in each function
/// - Inclusive vs exclusive time
/// - Call count statistics
/// - Hot spot detection
///
/// Thread model:
/// - All methods called from emulation thread
/// - Inline IsReturnAddrMatch() for performance (every RTS)
/// </remarks>
class CallstackManager {
private:
	Debugger* _debugger;              ///< Parent debugger instance
	deque<StackFrameInfo> _callstack; ///< Call stack (LIFO)
	unique_ptr<Profiler> _profiler;   ///< Performance profiler

public:
	/// <summary>
	/// Constructor for callstack manager.
	/// </summary>
	/// <param name="debugger">Main debugger instance</param>
	/// <param name="cpuDebugger">CPU-specific debugger</param>
	CallstackManager(Debugger* debugger, IDebugger* cpuDebugger);
	~CallstackManager();

	/// <summary>
	/// Push stack frame on subroutine call.
	/// </summary>
	/// <param name="src">Source address info (caller)</param>
	/// <param name="srcAddr">Source address (absolute)</param>
	/// <param name="dest">Destination address info (callee)</param>
	/// <param name="destAddr">Destination address (subroutine entry)</param>
	/// <param name="ret">Return address info</param>
	/// <param name="returnAddress">Return address (after JSR)</param>
	/// <param name="returnStackPointer">Stack pointer at call time</param>
	/// <param name="flags">Stack frame flags (interrupt, etc.)</param>
	/// <remarks>
	/// Called on:
	/// - JSR (6502/65816)
	/// - CALL (Z80, Game Boy)
	/// - BL (ARM)
	/// - Interrupts (IRQ/NMI/etc.)
	///
	/// Profiler:
	/// - Starts function timer
	/// - Increments call count
	/// </remarks>
	void Push(AddressInfo& src, uint32_t srcAddr, AddressInfo& dest, uint32_t destAddr, AddressInfo& ret, uint32_t returnAddress, uint32_t returnStackPointer, StackFrameFlags flags);

	/// <summary>
	/// Pop stack frame on subroutine return.
	/// </summary>
	/// <param name="dest">Destination address info (return target)</param>
	/// <param name="destAddr">Destination address</param>
	/// <param name="stackPointer">Current stack pointer</param>
	/// <remarks>
	/// Called on:
	/// - RTS (6502/65816)
	/// - RET (Z80, Game Boy)
	/// - BX LR (ARM)
	/// - RTI (interrupt return)
	///
	/// Validation:
	/// - Checks if return address matches expected
	/// - Warns on stack corruption (invalid return)
	///
	/// Profiler:
	/// - Stops function timer
	/// - Records execution time
	/// </remarks>
	void Pop(AddressInfo& dest, uint32_t destAddr, uint32_t stackPointer);

	/// <summary>
	/// Check if address matches any return address in callstack.
	/// </summary>
	/// <param name="destAddr">Address to check</param>
	/// <returns>True if address is valid return target</returns>
	/// <remarks>
	/// Used for:
	/// - "Step Out" debugger feature (step until return)
	/// - Return address validation
	/// - Callstack corruption detection
	///
	/// Inline for performance (called every RTS instruction).
	/// </remarks>
	__forceinline bool IsReturnAddrMatch(uint32_t destAddr) {
		if (_callstack.empty()) {
			return false;
		}

		for (auto itt = _callstack.rbegin(); itt != _callstack.rend(); itt++) {
			if ((*itt).Return == destAddr) {
				return true;
			}
		}

		return false;
	}

	/// <summary>
	/// Get current callstack snapshot.
	/// </summary>
	/// <param name="callstackArray">Output stack frame array</param>
	/// <param name="callstackSize">Input: max size, Output: actual size</param>
	void GetCallstack(StackFrameInfo* callstackArray, uint32_t& callstackSize);

	/// <summary>
	/// Get return address of topmost stack frame.
	/// </summary>
	/// <returns>Return address or -1 if callstack empty</returns>
	int32_t GetReturnAddress();

	/// <summary>
	/// Get return stack pointer of topmost frame.
	/// </summary>
	/// <returns>Stack pointer or -1 if callstack empty</returns>
	int64_t GetReturnStackPointer();

	/// <summary>
	/// Get profiler instance.
	/// </summary>
	Profiler* GetProfiler();

	/// <summary>
	/// Clear callstack (on reset/game load).
	/// </summary>
	void Clear();
};