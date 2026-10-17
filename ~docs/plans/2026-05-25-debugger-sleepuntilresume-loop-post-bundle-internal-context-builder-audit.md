# Debugger SleepUntilResume Loop-Post Bundle Internal Context Builder Audit (2026-05-25)

## Context

`ResolveSleepUntilResumeLoopPostBundleOutcome` still performed inline nested context mapping to loop and post-loop contexts. The mapping is deterministic and should be encapsulated by helper builders.

## Goals

- Reduce resolver boilerplate and mapping duplication.
- Keep loop/post context construction explicit and testable.
- Preserve behavior across wait/break/suspend and notification states.

## Slice Trio

1. #2482: extract loop-post internal context builders.
2. #2480: add regression tests for internal context builders.
3. #2481: refactor resolver to consume builders.

## Acceptance

- Resolver uses helper builders for loop/post context construction.
- Tests verify field-level mapping for both builders.
- Existing loop-post bundle behavior is unchanged.
