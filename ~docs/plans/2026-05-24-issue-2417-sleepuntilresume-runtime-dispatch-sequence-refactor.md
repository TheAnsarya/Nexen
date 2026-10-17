# Issue #2417 Plan: SleepUntilResume Runtime Dispatch Sequence Refactor

## Objective

Refactor `SleepUntilResume` to consume composed runtime-dispatch outcomes.

## Planned Changes

1. Replace separate break-event and dispatch helper wiring with one runtime-dispatch outcome.
2. Keep execution ordering intact for notification send, event processing, and sent-state updates.
3. Preserve all side effects while reducing coordinator-local setup noise.

## Acceptance Criteria

1. Coordinator logic is simpler and still behavior-preserving.
2. Notification and event-processing ordering remains unchanged.
3. Tests and build pass for touched areas.
