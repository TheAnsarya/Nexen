# Debugger SleepUntilResume Break-Event Payload Policy Audit (2026-05-24)

## Scope

Architecture audit of `SleepUntilResume` break-event payload assembly for:

1. Source CPU/source/breakpoint mapping
2. Optional memory operation propagation
3. Notification payload preparation before code-break dispatch

## Findings

1. Break-event payload assembly was inline in coordinator flow.
2. Optional operation copy behavior was embedded in local branching.
3. Payload policy was not directly reusable or unit-testable as one outcome.

## Simplification Strategy

1. Extract typed payload context/outcome helper in `DebuggerDispatchUtils`.
2. Resolve payload once from coordinator inputs before dispatch.
3. Keep notification behavior equivalent while reducing branch density.
4. Add deterministic tests for operation-present and operation-absent paths.

## Implemented Slice

This audit maps to:

1. #2396 helper extraction
2. #2398 regression coverage
3. #2399 SleepUntilResume refactor

## Next Candidate Slice

1. Extract pre-loop notification side-effect sequence policy (`OnBeforeBreak`, `OnBeforePause`, code-break notification sequencing) into shared helper outcomes.
2. Add helper tests to lock sequencing preconditions and dispatch gating assumptions.
