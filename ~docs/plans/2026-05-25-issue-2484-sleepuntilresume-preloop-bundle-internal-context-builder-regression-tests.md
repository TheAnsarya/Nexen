# Issue #2484 SleepUntilResume Pre-Loop Bundle Internal Context Builder Regression Tests

## Scope

Add deterministic tests for internal context builder mappings used by pre-loop bundle resolver.

## Planned Tests

- Pre-break context builder maps notification/single-breakpoint/draw-partial fields.
- Pre-loop context builder maps notification field.
- Dispatch context builder maps run-sequence flag from pre-loop outcome.

## Acceptance Criteria

- New tests pass in `DebuggerDispatchUtilsTests`.
- Builder mapping behavior is directly covered.
