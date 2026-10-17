# Issue #2518 SleepUntilResume Runtime Side-Effect Application Mapping Regression Tests

## Scope

Add deterministic tests for runtime side-effect application context and outcome mapping.

## Planned Tests

- Context builder maps runtime state and side-effect payload fields.
- Outcome resolver promotes wait/notification state when requested.
- Outcome resolver preserves existing state when no promotion is requested.

## Acceptance Criteria

- New regression tests pass in `DebuggerDispatchUtilsTests`.
- Mapping behavior is explicitly enforced by tests.
