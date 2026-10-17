# Issue #2365 Plan: ProcessEvent Script-Ownership and Event-Target Helpers

## Objective

Centralize ProcessEvent script-dispatch ownership checks and event-target decision logic in shared debugger dispatch helpers.

## Planned Changes

1. Add helper functions in `Core/Debugger/DebuggerDispatchUtils.h` for:
	- script-dispatch ownership gate
	- input-debugger target resolution
	- composed ProcessEvent branch-decision outcome
2. Refactor `Debugger::ProcessEvent` to consume helper output instead of duplicating inline checks.

## Acceptance Criteria

1. Behavior remains unchanged for script dispatch ownership and InputPolled fallback.
2. ProcessEvent has reduced inline decision branching.
3. New helper behavior is covered by debugger dispatch utility tests.
