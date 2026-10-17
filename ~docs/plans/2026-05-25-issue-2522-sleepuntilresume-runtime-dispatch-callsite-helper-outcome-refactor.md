# Issue #2522 SleepUntilResume Runtime Dispatch Callsite Helper Outcome Refactor

## Scope

Refactor runtime dispatch callsite in `Debugger::SleepUntilResume` to consume helper execution outcomes.

## Planned Refactor

1. Build runtime dispatch execution context from runtime bundle outcome.
2. Resolve runtime dispatch execution outcome.
3. Replace direct runtime bundle dispatch flag and event payload access at callsite.

## Acceptance Criteria

- Coordinator runtime dispatch callsite uses helper outcomes for dispatch decision and payload.
- Existing behavior remains unchanged.
