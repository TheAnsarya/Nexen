# Issue #2480 SleepUntilResume Loop-Post Bundle Internal Context Builder Regression Tests

## Scope

Add deterministic tests for loop and post-loop internal context builder mappings.

## Planned Tests

- Loop context builder maps wait/suspend/break fields from loop-post bundle context.
- Post-loop context builder maps notification-sent state from loop-post bundle context.

## Acceptance Criteria

- New tests run in `DebuggerDispatchUtilsTests`.
- Field mapping behavior is directly covered for both builders.
