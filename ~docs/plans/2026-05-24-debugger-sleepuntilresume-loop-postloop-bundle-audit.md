# Debugger SleepUntilResume Loop/Post-Loop Bundle Audit (2026-05-24)

## Scope

Architecture audit of `SleepUntilResume` loop and post-loop orchestration for:

1. Loop continuation and delay policy evaluation
2. Post-loop side-effect policy evaluation
3. Coordinator-level loop/tail wiring reduction

## Findings

1. Loop continuation and post-loop side-effect policies were helperized but still resolved through separate coordinator contexts.
2. Coordinator retained repetitive context assembly for loop and tail post-loop sections.
3. Composing loop and post-loop outcomes via one helper can reduce orchestration noise while preserving behavior.

## Simplification Strategy

1. Add a composed loop/post-loop bundle helper with one shared context.
2. Delegate bundle internals to existing loop and post-loop resolvers to preserve semantics.
3. Refactor coordinator loop and tail to consume bundle outcomes.
4. Add deterministic tests for waiting/non-waiting loop behavior and notification-driven post-loop side effects.

## Implemented Slice

This audit maps to:

1. #2440 loop/post-loop bundle helper extraction
2. #2441 loop/post-loop bundle regression tests
3. #2442 loop/post-loop bundle refactor

## Next Candidate Slice

1. Evaluate replacing phase-loop resolution calls with direct loop/post-loop bundle usage where appropriate.
2. Continue reducing coordinator-local booleans and context shims in `SleepUntilResume`.
