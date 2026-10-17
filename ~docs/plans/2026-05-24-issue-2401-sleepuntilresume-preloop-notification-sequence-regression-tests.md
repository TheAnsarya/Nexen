# Issue #2401 Plan: SleepUntilResume Pre-Loop Notification Sequence Regression Tests

## Objective

Add deterministic utility tests for pre-loop notification sequence helper outcomes.

## Planned Changes

1. Validate sequence and side-effect outcomes are disabled when notification is not emitted.
2. Validate sequence and side-effect outcomes are enabled when notification is emitted.
3. Keep tests fast and independent from runtime side effects.

## Acceptance Criteria

1. Sequence-gating outcomes are locked by helper tests.
2. Side-effect eligibility outcomes are regression-safe.
3. Tests remain deterministic and low-overhead.
