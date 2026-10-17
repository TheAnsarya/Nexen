# Issue #2409 Plan: SleepUntilResume Phase Outcome Model Helpers

## Objective

Add a higher-level phase model that composes existing `SleepUntilResume` policy helpers.

## Planned Changes

1. Add `SleepUntilResumePhaseContext` to represent guard, notification, loop, and post-loop phase inputs.
2. Add `SleepUntilResumePhaseOutcome` to expose composed decision/bundle/loop/post-loop outputs.
3. Implement `ResolveSleepUntilResumePhaseOutcome(...)` by delegating to existing policy resolvers.

## Acceptance Criteria

1. Phase helper remains pure and behavior-preserving.
2. Phase helper composes guard decision, pre-loop bundle, loop outcome, and post-loop outcome.
3. Existing helper-level policy behavior is unchanged.
