# Issue #2426 Plan: SleepUntilResume Runtime Side-Effect Transition Refactor

## Objective

Refactor `SleepUntilResume` to consume runtime side-effect transition outcomes.

## Planned Changes

1. Replace inline wait-arm/screensaver/sent-state transition logic with composed helper outcome.
2. Preserve behavior and side-effect ordering in runtime pre-loop dispatch flow.
3. Keep coordinator focused on orchestration while helper owns transition semantics.

## Acceptance Criteria

1. Runtime side effects remain behavior-preserving.
2. Coordinator flow is simpler and transition logic is explicit.
3. Tests and build succeed for touched areas.
