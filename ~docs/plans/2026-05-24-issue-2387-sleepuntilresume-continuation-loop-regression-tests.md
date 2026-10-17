# Issue #2387 Plan: SleepUntilResume Continuation-Loop Regression Tests

## Objective

Add deterministic utility tests for continuation-loop policy outcomes.

## Planned Changes

1. Validate continuation when waiting for resume without suspend.
2. Validate continuation when break requests are pending.
3. Validate stop conditions when wait and break-request conditions are both absent, and when suspend cancels wait path.
4. Validate wait-delay mapping for break-request vs non-break-request states.

## Acceptance Criteria

1. Continuation-loop truth table is covered by direct helper tests.
2. Delay behavior remains locked to expected values.
3. Tests stay fast and deterministic.
