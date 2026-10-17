# Issue #2440 Plan: SleepUntilResume Loop/Post-Loop Bundle Helper

## Objective

Extract a composed helper that resolves `SleepUntilResume` loop and post-loop outcomes from one context.

## Planned Changes

1. Add `SleepUntilResumeLoopPostBundleContext` and `SleepUntilResumeLoopPostBundleOutcome`.
2. Implement `ResolveSleepUntilResumeLoopPostBundleOutcome(...)` by composing existing loop and post-loop resolvers.
3. Keep helper pure and behavior-preserving.

## Acceptance Criteria

1. Loop and post-loop outcomes are available from one bundle resolver call.
2. Existing loop and post-loop semantics are preserved.
3. Helper improves coordinator readability by reducing local policy wiring.
