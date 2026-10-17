# Issue #2366 Plan: ProcessEvent Per-Event Handler Split

## Objective

Simplify `Debugger::ProcessEvent` by moving event-branch bodies into dedicated handler methods while keeping the dispatcher behavior-equivalent.

## Planned Changes

1. Introduce dedicated handlers for:
	- `InputPolled`
	- `StartFrame`
	- `StateLoaded`
2. Keep ProcessEvent responsible only for:
	- resolving routed CPU
	- computing dispatch outcome
	- dispatching to handlers by event type

## Acceptance Criteria

1. ProcessEvent reads as a coordinator, not an implementation monolith.
2. Event behavior is unchanged.
3. Existing and new tests pass.
