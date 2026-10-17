# Issue #2521 SleepUntilResume Runtime Dispatch Execution Policy Mapping Regression Tests

## Scope

Add deterministic tests for runtime dispatch execution policy context/outcome mapping.

## Planned Tests

- Context builder maps dispatch flags and break-event payload.
- Outcome resolver maps execution flags and event payload for coordinator consumption.

## Acceptance Criteria

- New tests pass in `DebuggerDispatchUtilsTests`.
- Mapping behavior is enforced through direct assertions.
