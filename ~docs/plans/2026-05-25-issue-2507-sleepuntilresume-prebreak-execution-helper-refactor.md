# Issue #2507 SleepUntilResume Pre-Break Execution Helper Refactor

## Scope

Refactor pre-break execution branching in `Debugger::SleepUntilResume` into helper outcome policy.

## Planned Refactor

1. Add `SleepUntilResumePreBreakExecutionContext` and `SleepUntilResumePreBreakExecutionOutcome`.
2. Add resolver mapping pre-break action plan to execution decisions.
3. Refactor coordinator callsite to consume helper outcome.

## Acceptance Criteria

- Nested pre-break branching in coordinator is reduced.
- Helper outcome deterministically controls runtime pre-break side effects.
- Existing behavior remains unchanged.
