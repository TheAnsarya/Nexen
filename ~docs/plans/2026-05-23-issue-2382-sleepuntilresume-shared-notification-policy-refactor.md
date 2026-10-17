# Issue #2382 Plan: SleepUntilResume Shared Notification Policy Refactor

## Objective

Refactor `SleepUntilResume` to consume shared notification and wait-delay policy helpers.

## Planned Changes

1. Replace inline notification gate with `ShouldEmitSleepUntilResumeBreakNotification(...)`.
2. Replace inline sleep-delay ternary with `GetSleepUntilResumeWaitDelayMs(...)`.
3. Keep break orchestration and side-effect sequence unchanged.

## Acceptance Criteria

1. Coordinator flow is clearer and less redundant.
2. Existing behavior remains unchanged.
3. Tests and Release build pass.
