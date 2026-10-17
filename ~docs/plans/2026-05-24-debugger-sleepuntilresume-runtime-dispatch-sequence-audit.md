# Debugger SleepUntilResume Runtime Dispatch Sequence Audit (2026-05-24)

## Scope

Architecture audit of `SleepUntilResume` runtime dispatch sequencing for:

1. Break-event payload construction
2. Dispatch notification/process trigger decisions
3. Notification-sent state transitions

## Findings

1. Break payload construction and dispatch trigger logic were split across separate helper invocations in coordinator code.
2. Dispatch behavior was deterministic but required multiple local objects and repeated wiring.
3. Coordinator readability still suffered in the most side-effect-heavy pre-loop block.

## Simplification Strategy

1. Introduce a composed runtime-dispatch helper outcome that contains break-event payload and dispatch decisions.
2. Keep helper pure and delegate to existing break-event and dispatch policy helpers.
3. Refactor coordinator dispatch block to consume one runtime-dispatch outcome object.
4. Add deterministic regression tests for emitted and non-emitted dispatch sequence behavior.

## Implemented Slice

This audit maps to:

1. #2415 runtime dispatch sequence helper extraction
2. #2416 runtime dispatch sequence regression tests
3. #2417 runtime dispatch sequence refactor

## Next Candidate Slice

1. Compose wait-arming + screensaver toggles + notification-sent transition into one runtime side-effect outcome model.
2. Continue reducing coordinator-local booleans by routing through typed state-transition helpers.
