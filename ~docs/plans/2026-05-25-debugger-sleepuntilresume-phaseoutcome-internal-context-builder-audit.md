# Debugger SleepUntilResume PhaseOutcome Internal Context Builder Audit (2026-05-25)

## Context

`ResolveSleepUntilResumePhaseOutcome` still performed inline mapping from phase context into pre-loop bundle, loop, and post-loop contexts.

## Goal

Extract internal context builders to reduce resolver boilerplate and centralize deterministic mapping rules.

## Canonical Slice Trio

1. #2496: extract phase-outcome internal context builders.
2. #2497: add deterministic regression tests for builder mappings.
3. #2498: refactor phase-outcome resolver to consume builder helpers.

## Acceptance

- Resolver uses builders for all three nested context constructions.
- Mapping behavior is covered with direct unit tests.
- Existing phase-outcome behavior remains unchanged.
