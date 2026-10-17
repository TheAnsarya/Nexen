# Debugger SleepUntilResume Pre-Loop Bundle Internal Context Builder Audit (2026-05-25)

## Context

`ResolveSleepUntilResumePreLoopBundleOutcome` still contained inline internal context wiring for pre-break, pre-loop, and dispatch contexts.

## Objective

Extract deterministic internal context builders to simplify resolver structure and reduce duplicated mapping logic.

## Slice Trio

1. #2486: extract internal context builders for pre-loop bundle resolver.
2. #2484: add deterministic builder mapping tests.
3. #2485: refactor resolver to use helper builders.

## Acceptance

- Resolver uses helper builders for all three internal contexts.
- Tests directly verify builder mappings.
- Behavior remains unchanged in composed pre-loop bundle outcomes.
