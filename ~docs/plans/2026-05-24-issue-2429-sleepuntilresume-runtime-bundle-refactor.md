# Issue #2429 Plan: SleepUntilResume Runtime Bundle Refactor

## Objective

Refactor `SleepUntilResume` runtime pre-loop block to consume the runtime bundle helper outcome.

## Planned Changes

1. Replace separate runtime dispatch and side-effect helper wiring with one runtime bundle resolver call.
2. Preserve behavior and ordering for notification send, event process, wait-arm, screensaver, and sent-state transitions.
3. Keep coordinator runtime code focused on orchestration steps.

## Acceptance Criteria

1. Runtime pre-loop block behavior remains unchanged.
2. Coordinator-local runtime setup is reduced.
3. Tests/build pass for touched areas.
