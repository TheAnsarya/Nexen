# Issue #2441 Plan: SleepUntilResume Loop/Post-Loop Bundle Regression Tests

## Objective

Add deterministic tests for composed loop/post-loop bundle outcomes.

## Planned Changes

1. Add waiting-loop test for break-request delay behavior.
2. Add non-waiting loop test with notification-driven post-loop side effects.
3. Assert both loop and post-loop outputs from composed bundle resolver.

## Acceptance Criteria

1. Bundle behavior is covered for waiting and non-waiting scenarios.
2. Post-loop side-effect gating by notification state is asserted.
3. Tests protect loop/post-loop composition behavior from regressions.
