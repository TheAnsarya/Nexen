# Issue #2477 SleepUntilResume Runtime-Bundle Internal Context Builder Regression Tests

## Scope

Add deterministic tests for internal context builders used by runtime-bundle resolution.

## Planned Tests

- Runtime dispatch context builder maps pre-break and payload fields.
- Runtime side-effect context builder maps wait/screensaver/notification mark inputs.

## Acceptance Criteria

- New tests pass in `DebuggerDispatchUtilsTests`.
- Builder field mapping is directly covered without relying only on composed resolver tests.
