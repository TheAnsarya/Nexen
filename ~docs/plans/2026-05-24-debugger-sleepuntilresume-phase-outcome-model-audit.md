# Debugger SleepUntilResume Phase Outcome Model Audit (2026-05-24)

## Scope

Architecture audit of `SleepUntilResume` orchestration boundaries across:

1. Guard decision routing
2. Pre-loop notification policy setup
3. Loop continuation and delay policy
4. Post-loop side-effect policy

## Findings

1. Policy extraction was strong at the individual helper level, but coordinator wiring still had phase-by-phase glue code duplication.
2. Guard, pre-loop, loop, and post-loop decisions were resolved independently without a single composed phase contract.
3. `SleepUntilResume` still carried unnecessary local setup and translation noise between policy layers.

## Simplification Strategy

1. Add a phase-level context/outcome model that composes existing helper policies.
2. Reuse existing resolvers inside the phase model to preserve exact behavior.
3. Refactor `SleepUntilResume` to consume phase outcomes directly for guard switch, pre-loop setup, loop wait policy, and post-loop side effects.
4. Add deterministic tests for phase composition and guard/notification transitions.

## Implemented Slice

This audit maps to:

1. #2409 phase outcome model helpers
2. #2410 phase outcome model regression tests
3. #2411 `SleepUntilResume` phase-model refactor

## Next Candidate Slice

1. Extract a `SleepUntilResume` runtime coordinator helper for break payload + dispatch sequencing to further reduce method length.
2. Add composed integration tests that validate emitted and non-emitted execution paths end-to-end through one phase fixture.
