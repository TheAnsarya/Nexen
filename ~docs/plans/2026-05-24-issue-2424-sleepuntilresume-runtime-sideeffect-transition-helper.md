# Issue #2424 Plan: SleepUntilResume Runtime Side-Effect Transition Helper

## Objective

Extract a composed helper for runtime side-effect state transitions in `SleepUntilResume`.

## Planned Changes

1. Add `SleepUntilResumeRuntimeSideEffectContext` and `SleepUntilResumeRuntimeSideEffectOutcome`.
2. Implement `ResolveSleepUntilResumeRuntimeSideEffectOutcome(...)` as a pure transition helper.
3. Capture wait-arm, screensaver-enable, and notification-sent promotion behavior.

## Acceptance Criteria

1. Runtime side-effect transition logic is centralized and deterministic.
2. Helper preserves current runtime side-effect behavior.
3. Coordinator wiring becomes simpler and less repetitive.
