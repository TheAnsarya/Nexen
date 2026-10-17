# Issue #2485 SleepUntilResume Pre-Loop Bundle Resolver Refactor

## Scope

Refactor `ResolveSleepUntilResumePreLoopBundleOutcome` to use extracted internal context builders.

## Planned Refactor

1. Replace inline pre-break context mapping with builder helper.
2. Replace inline pre-loop context mapping with builder helper.
3. Replace inline dispatch context mapping with builder helper.

## Acceptance Criteria

- Resolver uses builder helpers for all nested context construction.
- Existing behavior remains unchanged and tests stay green.
