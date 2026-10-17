# Issue #2393 Plan: SleepUntilResume Shared Post-Loop Side-Effect Refactor

## Objective

Refactor `SleepUntilResume` post-loop tail to consume shared post-loop side-effect outcomes.

## Planned Changes

1. Build post-loop context from notification-sent state.
2. Resolve helper outcome once after loop completion.
3. Apply screensaver and resume-notification side effects from helper outcome.

## Acceptance Criteria

1. Coordinator post-loop branch density is reduced.
2. Behavior remains equivalent to existing side-effect execution.
3. Tests and Release build pass.
