# Debugger SleepUntilResume Continuation-Loop Policy Audit (2026-05-24)

## Scope

Architecture audit of `SleepUntilResume` continuation-loop logic for:

1. Wait-for-break-resume state
2. Suspend-request gating
3. Break-request continuation override
4. Loop wait-delay mapping

## Findings

1. Continuation policy was encoded inline in coordinator loop condition.
2. Continuation and delay decisions were split across condition and helper call-sites.
3. Policy could not be directly validated as one composable outcome.

## Simplification Strategy

1. Extract typed loop context/outcome helper in `DebuggerDispatchUtils`.
2. Resolve continuation and delay once per loop iteration.
3. Keep coordinator behavior equivalent while reducing condition complexity.
4. Add deterministic tests for continuation truth table and delay behavior.

## Implemented Slice

This audit maps to:

1. #2386 helper extraction
2. #2387 regression coverage
3. #2388 SleepUntilResume refactor

## Next Candidate Slice

1. Extract post-loop resume-notification/screensaver policy into typed helper outcomes.
2. Add helper tests for notification-sent dependent post-loop side effects.
