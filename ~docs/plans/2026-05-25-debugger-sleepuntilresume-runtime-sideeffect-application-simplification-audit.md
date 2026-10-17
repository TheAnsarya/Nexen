# Debugger SleepUntilResume Runtime Side-Effect Application Simplification Audit (2026-05-25)

## Context

`Debugger::SleepUntilResume` still applied runtime side-effect state transitions inline after runtime bundle resolution.

## Goal

Extract deterministic runtime side-effect application mapping into helper context/outcome policy functions.

## Canonical Slice Trio

1. #2517: extract runtime side-effect application context builder.
2. #2518: add deterministic regression tests for application mapping.
3. #2519: refactor coordinator runtime state updates to helper outcomes.

## Acceptance

- Coordinator consumes runtime side-effect application helper outputs for wait/notification state updates.
- Tests verify mapping and state promotion behavior.
- Runtime behavior remains unchanged.
