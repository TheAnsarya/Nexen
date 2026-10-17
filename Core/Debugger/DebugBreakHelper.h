#pragma once
#include "pch.h"
#include "Debugger/Debugger.h"
#include "Shared/Emulator.h"

class DebugBreakHelper {
private:
	Debugger* _debugger;
	bool _needBreak = false;

public:
	DebugBreakHelper(Debugger* debugger, bool breakBetweenInstructions = false) {
		_debugger = debugger;

		// Preserve an existing debugger pause. MCP state reads/writes are often
		// issued as a sequence while the emulator is already stopped; treating
		// every cross-thread call as owning the break releases that pause in the
		// destructor and lets the CPU run between calls.
		_needBreak = !debugger->GetEmulator()->IsEmulationThread()
			&& !debugger->GetEmulator()->IsPaused();

		if (_needBreak) {
			// Only attempt to break if this is done in a thread other than the main emulation thread (and the debugger is active)
			while (true) {
				debugger->BreakRequest(false);
				while (!debugger->IsExecutionStopped()) {}

				if (breakBetweenInstructions) {
					if (debugger->GetDebuggerFeatures(debugger->GetMainCpuType()).ChangeProgramCounter) {
						// Execution stopped in-between 2 main cpu instructions, leave loop
						break;
					} else {
						// Execution stopped, but in the middle of an instruction, step forward
						// to the next instruction and try again
						debugger->Step(debugger->GetMainCpuType(), 1, StepType::Step, BreakSource::InternalOperation);
						debugger->BreakRequest(true);
						std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(15));
					}
				} else {
					// Execution stopped, leave loop
					break;
				}
			}
		}
	}

	~DebugBreakHelper() {
		if (_needBreak) {
			_debugger->BreakRequest(true);
		}
	}
};