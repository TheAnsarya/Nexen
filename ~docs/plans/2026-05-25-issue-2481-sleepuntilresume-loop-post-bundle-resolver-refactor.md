# Issue #2481 SleepUntilResume Loop-Post Bundle Resolver Refactor

## Scope

Refactor loop-post bundle resolver to consume extracted internal context builders.

## Planned Refactor

1. Replace inline loop-context field wiring with `BuildSleepUntilResumeLoopContext(...)`.
2. Replace inline post-loop context field wiring with `BuildSleepUntilResumePostLoopContext(...)`.
3. Keep resolver output semantics unchanged.

## Acceptance Criteria

- Resolver uses both helper builders.
- Existing and new tests pass with unchanged behavior.
