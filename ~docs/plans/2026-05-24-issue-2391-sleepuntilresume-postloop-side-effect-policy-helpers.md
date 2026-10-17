# Issue #2391 Plan: SleepUntilResume Post-Loop Side-Effect Policy Helpers

## Objective

Extract post-loop side-effect decisions into shared debugger dispatch helpers.

## Planned Changes

1. Add typed post-loop context containing notification-sent state.
2. Add typed post-loop outcome containing side-effect decisions.
3. Keep policy pure and behavior-equivalent.

## Acceptance Criteria

1. Coordinator tail no longer hardcodes post-loop side-effect gating.
2. Policy is reusable and directly testable.
3. Runtime behavior remains unchanged.
