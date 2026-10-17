# Issue #2454 Plan - SleepUntilResume Loop/Post Context Builder Refactor

## Goal

Refactor `Debugger::SleepUntilResume` to use loop/post context builder helper for loop and post-loop calls.

## Scope

- Replace manual loop context assignment in wait loop.
- Replace manual post-loop context assignment for notification side effects.
- Preserve policy evaluation order and behavior.

## Acceptance Criteria

- `Debugger.cpp` loop/post context wiring uses helper invocations.
- No behavior regressions in SleepUntilResume loop/post policy flow.
- Validation suite passes.
