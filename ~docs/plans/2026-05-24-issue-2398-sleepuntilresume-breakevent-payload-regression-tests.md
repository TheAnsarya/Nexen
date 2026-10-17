# Issue #2398 Plan: SleepUntilResume Break-Event Payload Regression Tests

## Objective

Add deterministic utility tests for break-event payload helper outcomes.

## Planned Changes

1. Validate payload fields are populated correctly for standard break-event context.
2. Validate optional operation copy behavior when operation is provided.
3. Validate no-operation path remains stable.

## Acceptance Criteria

1. Payload fields are locked by helper tests.
2. Optional operation behavior is covered and regression-safe.
3. Tests stay fast and deterministic.
