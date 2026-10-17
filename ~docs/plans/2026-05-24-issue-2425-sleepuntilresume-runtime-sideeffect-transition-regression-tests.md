# Issue #2425 Plan: SleepUntilResume Runtime Side-Effect Transition Regression Tests

## Objective

Add deterministic tests for runtime side-effect transition outcomes.

## Planned Changes

1. Add non-emitted path test for no wait-arm, no screensaver enable, and unchanged notification-sent state.
2. Add emitted path test for wait-arm and screensaver enable with notification-sent promotion.
3. Keep coverage focused on pure helper transition semantics.

## Acceptance Criteria

1. Transition helper behavior is locked for emitted and non-emitted paths.
2. Notification-sent promotion logic is explicitly validated.
3. Tests protect against coordinator transition drift.
