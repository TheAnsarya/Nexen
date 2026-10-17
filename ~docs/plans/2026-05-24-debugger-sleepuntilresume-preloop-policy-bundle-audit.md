# Debugger SleepUntilResume Pre-Loop Policy Bundle Audit (2026-05-24)

## Scope

Architecture audit of `SleepUntilResume` pre-loop policy wiring for:

1. Pre-break side-effect decisions
2. Pre-loop sequence gating decisions
3. Dispatch-trigger decisions

## Findings

1. Pre-loop policy decisions were already extracted but still wired through multiple local helper contexts.
2. Coordinator setup carried repetitive helper construction noise.
3. Related policies were strongly coupled to the same notification-emission state.

## Simplification Strategy

1. Compose pre-loop policy decisions into one helper bundle outcome.
2. Reuse existing helper logic inside bundle resolver to preserve behavior.
3. Reduce coordinator-local setup variables and consume bundled outcomes directly.
4. Add deterministic tests for emitted and non-emitted bundle paths.

## Implemented Slice

This audit maps to:

1. #2406 helper composition
2. #2407 regression coverage
3. #2408 SleepUntilResume refactor

## Next Candidate Slice

1. Introduce a higher-level `SleepUntilResume` phase outcome model (guards, pre-loop, loop, post-loop) for cleaner orchestration boundaries.
2. Add utility tests validating phase-model composition before coordinator extraction.
