# Debugger SleepUntilResume Runtime Bundle Audit (2026-05-24)

## Scope

Architecture audit of `SleepUntilResume` runtime pre-loop block composition for:

1. Runtime dispatch-sequence decisions and payloads
2. Runtime side-effect state transitions
3. Coordinator-local runtime wiring reduction

## Findings

1. Runtime dispatch and side-effect transitions were already extracted, but still wired through separate helper contexts in coordinator code.
2. Runtime pre-loop orchestration remained split across multiple local helper setup blocks.
3. Runtime branch readability could improve further with one composed runtime bundle outcome.

## Simplification Strategy

1. Add a runtime bundle helper that composes runtime dispatch and runtime side-effect outcomes.
2. Preserve existing helper semantics by delegating composition through existing resolvers.
3. Refactor coordinator runtime block to consume one bundle outcome object.
4. Add deterministic tests for emitted and non-emitted runtime bundle paths.

## Implemented Slice

This audit maps to:

1. #2427 runtime bundle helper extraction
2. #2428 runtime bundle regression tests
3. #2429 runtime bundle refactor

## Next Candidate Slice

1. Add integration-level SleepUntilResume orchestration tests for full emitted/non-emitted flow behavior.
2. Evaluate extraction of loop/post-loop orchestration into a dedicated coordinator-level runtime phase helper.
