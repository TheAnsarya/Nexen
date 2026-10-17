# Debugger SleepUntilResume Pre-Break Side-Effect Audit (2026-05-24)

## Scope

Architecture audit of `SleepUntilResume` pre-break side-effect branching for:

1. Single-breakpoint ignore toggling
2. Partial-frame draw behavior

## Findings

1. Side-effect decisions were encoded inline and coupled to break orchestration flow.
2. Side-effect gating used repeated config checks in coordinator code.
3. The policy was not directly reusable or unit-testable in isolation.

## Simplification Strategy

1. Extract typed helper context/outcome in `DebuggerDispatchUtils`.
2. Resolve side-effect decisions via one pure helper call.
3. Keep coordinator behavior equivalent while reducing branch density.
4. Add deterministic tests for emitted vs non-emitted notification contexts and config combinations.

## Implemented Slice

This audit maps to:

1. #2383 helper extraction
2. #2384 regression coverage
3. #2385 SleepUntilResume refactor

## Next Candidate Slice

1. Extract resume-loop continuation policy (wait-for-break-resume and suspend interplay) into typed helper outcomes.
2. Add helper tests for resume-loop continuation truth table before coordinator reduction.
