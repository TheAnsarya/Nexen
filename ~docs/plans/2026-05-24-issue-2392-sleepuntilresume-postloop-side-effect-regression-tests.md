# Issue #2392 Plan: SleepUntilResume Post-Loop Side-Effect Regression Tests

## Objective

Add deterministic utility tests for post-loop side-effect policy outcomes.

## Planned Changes

1. Validate side effects are disabled when no break notification was sent.
2. Validate side effects are enabled when break notification was sent.
3. Keep coverage fast and independent from runtime execution loops.

## Acceptance Criteria

1. Helper outcomes are locked for both notification states.
2. Tests are deterministic and low-overhead.
3. Regressions in post-loop side-effect policy are caught quickly.
