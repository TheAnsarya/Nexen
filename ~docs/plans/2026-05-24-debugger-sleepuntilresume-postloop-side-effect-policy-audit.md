# Debugger SleepUntilResume Post-Loop Side-Effect Policy Audit (2026-05-24)

## Scope

Architecture audit of `SleepUntilResume` post-loop side effects for:

1. Screensaver disable policy
2. Debugger resumed notification policy
3. Shared dependency on break-notification sent state

## Findings

1. Post-loop side effects were encoded as inline coordinator conditionals.
2. Both side effects were driven by the same notification state signal.
3. Policy was not explicitly represented as a reusable, testable outcome model.

## Simplification Strategy

1. Extract typed post-loop helper context/outcome in `DebuggerDispatchUtils`.
2. Resolve side-effect policy once after loop completion.
3. Apply post-loop side effects from helper outcome.
4. Add deterministic tests for notification-sent and not-sent paths.

## Implemented Slice

This audit maps to:

1. #2391 helper extraction
2. #2392 regression coverage
3. #2393 SleepUntilResume refactor

## Next Candidate Slice

1. Extract notification-dispatch payload policy (event construction and optional memory operation propagation) into helper outcomes.
2. Add helper tests for payload population and dispatch gating assumptions.
