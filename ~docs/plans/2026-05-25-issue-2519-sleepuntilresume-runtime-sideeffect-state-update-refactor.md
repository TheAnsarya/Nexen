# Issue #2519 SleepUntilResume Runtime Side-Effect State Update Refactor

## Scope

Refactor coordinator runtime state updates in `Debugger::SleepUntilResume` to consume runtime side-effect application helper outcomes.

## Planned Refactor

1. Build application context from runtime bundle outcome and runtime state.
2. Resolve application outcome to compute updated wait/notification states.
3. Replace direct branching-based state assignments with helper-derived values.

## Acceptance Criteria

- Coordinator runtime side-effect state updates are helper-driven.
- Existing behavior is preserved.
