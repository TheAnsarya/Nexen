# Issue #2509 SleepUntilResume Runtime Loop-State Snapshot Builder Regression Tests

## Scope

Add deterministic tests for runtime loop snapshot builder mapping.

## Planned Tests

- Runtime loop snapshot converts suspend/break counts into expected boolean fields.
- Post-loop runtime snapshot maps notification state and clears loop booleans.

## Acceptance Criteria

- New tests pass in `DebuggerDispatchUtilsTests`.
- Snapshot behavior is enforced by direct field mapping assertions.
