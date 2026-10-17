# Issue #2498 SleepUntilResume PhaseOutcome Resolver Refactor

## Scope

Refactor `ResolveSleepUntilResumePhaseOutcome` to consume extracted internal context builders.

## Planned Refactor

1. Replace inline pre-loop bundle context wiring with helper builder.
2. Replace inline loop context wiring with helper builder.
3. Replace inline post-loop context wiring with helper builder.

## Acceptance Criteria

- Resolver uses helper builders for nested context construction.
- Existing semantics are preserved and tests remain green.
