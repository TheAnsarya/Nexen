# Issue #2428 Plan: SleepUntilResume Runtime Bundle Regression Tests

## Objective

Add deterministic tests for composed runtime bundle helper behavior.

## Planned Changes

1. Add non-emitted runtime bundle path test with disabled dispatch and side effects.
2. Add emitted runtime bundle path test validating dispatch payload and side-effect transitions.
3. Ensure tests validate composed bundle semantics rather than coordinator implementation details.

## Acceptance Criteria

1. Runtime bundle helper coverage includes emitted and non-emitted paths.
2. Dispatch payload wiring and side-effect transitions are asserted.
3. Tests protect against bundle-composition regressions.
