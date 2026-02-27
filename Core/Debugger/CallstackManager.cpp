#include "pch.h"
#include "Debugger/CallstackManager.h"
#include "Debugger/Debugger.h"
#include "Debugger/IDebugger.h"
#include "Debugger/DebugBreakHelper.h"
#include "Debugger/Profiler.h"

CallstackManager::CallstackManager(Debugger* debugger, IDebugger* cpuDebugger) {
	_debugger = debugger;
	_callstackHead = 0;
	_callstackSize = 0;
	_profiler.reset(new Profiler(debugger, cpuDebugger));
}

CallstackManager::~CallstackManager() {
}

void CallstackManager::Push(AddressInfo& src, uint32_t srcAddr, AddressInfo& dest, uint32_t destAddr, AddressInfo& ret, uint32_t returnAddress, uint32_t returnStackPointer, StackFrameFlags flags) {
	if (_callstackSize >= MaxCallstackSize - 1) {
		// Ring buffer is full — oldest entry is silently overwritten by advancing head
		// (the entry at _callstackHead is the oldest, and we write over it)
		_callstackSize = MaxCallstackSize - 1;
	}

	StackFrameInfo& stackFrame = _callstackArray[_callstackHead];
	stackFrame.Source = srcAddr;
	stackFrame.AbsSource = src;
	stackFrame.Target = destAddr;
	stackFrame.AbsTarget = dest;
	stackFrame.Return = returnAddress;
	stackFrame.ReturnStackPointer = returnStackPointer;
	stackFrame.AbsReturn = ret;
	stackFrame.Flags = flags;

	_callstackHead = (_callstackHead + 1) % MaxCallstackSize;
	_callstackSize++;
	if (_callstackSize > MaxCallstackSize) {
		_callstackSize = MaxCallstackSize;
	}

	_profiler->StackFunction(dest, flags);
}

void CallstackManager::Pop(AddressInfo& dest, uint32_t destAddress, uint32_t stackPointer) {
	if (_callstackSize == 0) {
		return;
	}

	// Get and remove the top frame (back of the ring buffer)
	uint32_t topIdx = (_callstackHead - 1 + MaxCallstackSize) % MaxCallstackSize;
	StackFrameInfo prevFrame = _callstackArray[topIdx];
	_callstackHead = topIdx;
	_callstackSize--;
	_profiler->UnstackFunction();

	uint32_t returnAddr = prevFrame.Return;

	if (_callstackSize > 0 && destAddress != returnAddr) {
		// Mismatch, try to find a matching address higher in the stack
		bool foundMatch = false;
		for (int i = (int)_callstackSize - 1; i >= 0; i--) {
			uint32_t idx = (_callstackHead - 1 - ((_callstackSize - 1) - i) + MaxCallstackSize) % MaxCallstackSize;
			if (destAddress == _callstackArray[idx].Return) {
				// Found a matching stack frame, unstack until that point
				foundMatch = true;
				int framesToPop = (int)_callstackSize - 1 - i;
				for (int j = framesToPop; j >= 0; j--) {
					_callstackHead = (_callstackHead - 1 + MaxCallstackSize) % MaxCallstackSize;
					_callstackSize--;
					_profiler->UnstackFunction();
				}
				break;
			}
		}

		if (!foundMatch) {
			// Couldn't find a matching frame
			// If the new stack pointer doesn't match the last frame, push a new frame for it
			// Otherwise, presume that the code has returned to the last function on the stack
			uint32_t backIdx = (_callstackHead - 1 + MaxCallstackSize) % MaxCallstackSize;
			if (_callstackArray[backIdx].ReturnStackPointer != stackPointer) {
				Push(prevFrame.AbsReturn, returnAddr, dest, destAddress, prevFrame.AbsReturn, returnAddr, stackPointer, StackFrameFlags::None);
			}
		}
	}
}

void CallstackManager::GetCallstack(StackFrameInfo* callstackArray, uint32_t& callstackSize) {
	DebugBreakHelper helper(_debugger);
	// Copy from ring buffer to linear array, oldest to newest
	for (uint32_t i = 0; i < _callstackSize; i++) {
		uint32_t idx = (_callstackHead - _callstackSize + i + MaxCallstackSize) % MaxCallstackSize;
		callstackArray[i] = _callstackArray[idx];
	}
	callstackSize = _callstackSize;
}

int32_t CallstackManager::GetReturnAddress() {
	DebugBreakHelper helper(_debugger);
	if (_callstackSize == 0) {
		return -1;
	}
	uint32_t topIdx = (_callstackHead - 1 + MaxCallstackSize) % MaxCallstackSize;
	return _callstackArray[topIdx].Return;
}

int64_t CallstackManager::GetReturnStackPointer() {
	DebugBreakHelper helper(_debugger);
	if (_callstackSize == 0) {
		return -1;
	}
	uint32_t topIdx = (_callstackHead - 1 + MaxCallstackSize) % MaxCallstackSize;
	return _callstackArray[topIdx].ReturnStackPointer;
}

Profiler* CallstackManager::GetProfiler() {
	return _profiler.get();
}

void CallstackManager::Clear() {
	_callstackHead = 0;
	_callstackSize = 0;
	_profiler->ResetState();
}
