# Issue #2506 SleepUntilResume Coordinator Entry Context Builder Regression Tests

## Scope

Add deterministic tests validating coordinator-entry context builder field mapping.

## Planned Tests

- Guard, source, break-request, single-breakpoint, and draw-partial-frame fields map exactly.
- Mapping tests run in `DebuggerDispatchUtilsTests`.

## Acceptance Criteria

- New tests pass with expected field values.
- Mapping behavior is directly enforced by regression coverage.
