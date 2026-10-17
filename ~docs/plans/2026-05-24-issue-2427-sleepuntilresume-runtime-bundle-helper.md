# Issue #2427 Plan: SleepUntilResume Runtime Bundle Helper

## Objective

Extract a composed runtime bundle helper for `SleepUntilResume` runtime pre-loop orchestration.

## Planned Changes

1. Add `SleepUntilResumeRuntimeBundleContext` and `SleepUntilResumeRuntimeBundleOutcome`.
2. Implement `ResolveSleepUntilResumeRuntimeBundleOutcome(...)` by composing runtime dispatch and runtime side-effect helpers.
3. Keep behavior unchanged while reducing coordinator runtime wiring.

## Acceptance Criteria

1. Runtime bundle helper is pure and behavior-preserving.
2. Runtime bundle helper exposes dispatch and side-effect outcomes together.
3. Coordinator runtime block can consume one composed bundle outcome.
