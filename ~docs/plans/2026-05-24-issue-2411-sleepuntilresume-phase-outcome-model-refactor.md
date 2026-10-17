# Issue #2411 Plan: SleepUntilResume Phase Outcome Model Refactor

## Objective

Refactor `SleepUntilResume` to consume a composed phase outcome model instead of wiring policy helpers independently.

## Planned Changes

1. Resolve guard decisions through phase outcome and preserve existing early-return switch behavior.
2. Resolve pre-loop policy setup via phase bundle outcome while preserving ordering and side effects.
3. Resolve loop wait/delay and post-loop side effects through phase outcomes.

## Acceptance Criteria

1. `SleepUntilResume` behavior and ordering remain unchanged.
2. Coordinator-local policy setup is reduced and phase orchestration is clearer.
3. Tests and build pass without new warnings or regressions in touched areas.
