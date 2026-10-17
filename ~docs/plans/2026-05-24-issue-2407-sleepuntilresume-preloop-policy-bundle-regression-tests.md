# Issue #2407 Plan: SleepUntilResume Pre-Loop Policy Bundle Regression Tests

## Objective

Add deterministic utility tests for composed pre-loop bundle outcomes.

## Planned Changes

1. Validate bundle output when notification is not emitted.
2. Validate bundle output when notification is emitted with side effects enabled.
3. Assert consistency across pre-break, pre-loop, and dispatch sub-outcomes.

## Acceptance Criteria

1. Composed bundle outcomes are locked by tests.
2. Cross-policy consistency is validated deterministically.
3. Tests remain fast and isolated from runtime orchestration.
