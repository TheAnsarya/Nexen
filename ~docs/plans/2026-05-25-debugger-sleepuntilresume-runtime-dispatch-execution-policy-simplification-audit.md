# Debugger SleepUntilResume Runtime Dispatch Execution Policy Simplification Audit (2026-05-25)

## Context

`Debugger::SleepUntilResume` still handled runtime dispatch execution flags and event payload wiring inline after runtime bundle resolution.

## Goal

Extract deterministic runtime dispatch execution policy helper context/outcome mapping to reduce coordinator callsite boilerplate.

## Canonical Slice Trio

1. #2520: extract runtime dispatch execution policy context builder.
2. #2521: add deterministic regression tests for dispatch execution policy mapping.
3. #2522: refactor coordinator runtime dispatch callsite to helper outcomes.

## Acceptance

- Runtime dispatch execution logic uses helper-derived outcomes for notification dispatch and event processing.
- Tests enforce mapping behavior for flags and break event payload.
- Runtime behavior remains unchanged.
