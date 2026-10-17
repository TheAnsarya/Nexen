# Debugger SleepUntilResume Loop Runtime Snapshot Simplification Audit (2026-05-25)

## Context

`Debugger::SleepUntilResume` still converted runtime loop state (`_suspendRequestCount`, `_breakRequestCount`, wait flag) inline when building loop/post-loop bundle contexts.

## Goal

Introduce deterministic runtime snapshot builders in `DebuggerDispatchUtils` to centralize loop and post-loop context mapping.

## Canonical Slice Trio

1. #2508: extract runtime loop-state snapshot builders.
2. #2509: add deterministic regression tests for loop snapshot builders.
3. #2510: refactor SleepUntilResume wait-loop callsite to consume snapshot builders.

## Acceptance

- Coordinator loop uses runtime snapshot helpers instead of inline bool/count conversion.
- Tests enforce mapping behavior and notification propagation.
- Wait-loop behavior remains unchanged.
