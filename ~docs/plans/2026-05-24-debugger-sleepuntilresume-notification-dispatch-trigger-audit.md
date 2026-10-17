# Debugger SleepUntilResume Notification-Dispatch Trigger Audit (2026-05-24)

## Scope

Architecture audit of `SleepUntilResume` notification-dispatch trigger policy for:

1. Code-break notification dispatch trigger
2. Code-break event processing trigger
3. `notificationSent` transition policy

## Findings

1. Dispatch trigger and sent-state transitions were encoded inline in coordinator flow.
2. Dispatch and process triggers were tightly coupled to pre-loop sequence enablement.
3. Trigger policy was not represented as a reusable helper outcome.

## Simplification Strategy

1. Add typed dispatch context/outcome helper in `DebuggerDispatchUtils`.
2. Resolve dispatch policy once from pre-loop sequence state.
3. Consume helper outcomes in coordinator while preserving behavior and ordering.
4. Add deterministic tests for enabled/disabled dispatch-trigger paths.

## Implemented Slice

This audit maps to:

1. #2403 helper extraction
2. #2404 regression coverage
3. #2405 SleepUntilResume refactor

## Next Candidate Slice

1. Extract remaining grouped pre-loop side-effect dispatch (ignore-breakpoints + partial-frame + wait-arm) into an integrated helper bundle outcome to reduce repeated local policy objects.
2. Add helper tests for bundled pre-loop side-effect composition.
