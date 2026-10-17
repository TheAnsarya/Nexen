# Issue #2386 Plan: SleepUntilResume Continuation-Loop Policy Helpers

## Objective

Extract continuation-loop decisions into shared debugger dispatch helpers.

## Planned Changes

1. Add typed loop context for wait, suspend, and break-request state.
2. Add typed loop outcome containing continuation and wait-delay decisions.
3. Keep helper logic pure and behavior-equivalent.

## Acceptance Criteria

1. Coordinator while-condition is replaced with helper outcome usage.
2. Continuation and delay decisions are reusable and testable.
3. Runtime behavior remains unchanged.
