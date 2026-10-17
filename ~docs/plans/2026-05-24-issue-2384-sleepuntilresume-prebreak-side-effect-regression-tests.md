# Issue #2384 Plan: SleepUntilResume Pre-Break Side-Effect Regression Tests

## Objective

Add deterministic utility tests for pre-break side-effect policy outcomes.

## Planned Changes

1. Validate side effects are disabled when break-notification is not emitted.
2. Validate side effects mirror config flags when break-notification is emitted.
3. Cover representative flag combinations.

## Acceptance Criteria

1. All relevant policy branches are covered by direct helper tests.
2. Tests remain fast and independent from emulator runtime.
3. Expected outcomes are locked against regression.
