# Debugger SleepUntilResume Entry and Pre-Break Execution Simplification Audit (2026-05-25)

## Context

`Debugger::SleepUntilResume` still contained manual coordinator-entry context wiring and pre-break execution branch choreography.

## Goal

Reduce coordinator responsibility by extracting deterministic entry-context builders and pre-break execution decision helpers in `DebuggerDispatchUtils`.

## Canonical Slice Trio

1. #2505: extract coordinator-entry context builder.
2. #2506: add deterministic regression tests for coordinator-entry builder mapping.
3. #2507: refactor pre-break execution branching into helper outcome.

## Acceptance

- `Debugger::SleepUntilResume` uses helper builders/outcomes instead of manual mapping and nested execution branching.
- New tests cover deterministic mapping and pre-break execution decisions.
- Runtime behavior remains unchanged.
