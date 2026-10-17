# Issue #2497 SleepUntilResume PhaseOutcome Internal Context Builder Regression Tests

## Scope

Add deterministic tests for phase-outcome internal context builder mappings.

## Planned Tests

- Pre-loop bundle context builder maps emission and config fields.
- Loop context builder maps wait/suspend/break fields.
- Post-loop context builder maps notification-sent field.

## Acceptance Criteria

- New tests pass in `DebuggerDispatchUtilsTests`.
- Field mapping behavior is directly verified.
