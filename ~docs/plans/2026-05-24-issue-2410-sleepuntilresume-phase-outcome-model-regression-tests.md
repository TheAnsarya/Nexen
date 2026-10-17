# Issue #2410 Plan: SleepUntilResume Phase Outcome Model Regression Tests

## Objective

Add deterministic tests for the composed `SleepUntilResume` phase outcome model.

## Planned Changes

1. Add tests for guard skip decisions propagated through phase outcomes.
2. Add tests for emitted and non-emitted notification paths and their bundled pre-loop outcomes.
3. Add tests validating loop and post-loop policy composition through one phase resolver.

## Acceptance Criteria

1. Tests cover both `Continue` and skip guard outcomes.
2. Tests validate pre-loop bundle enable/disable behavior based on notification emission policy.
3. Tests validate loop delay/continuation and post-loop side-effect propagation through phase outcome.
