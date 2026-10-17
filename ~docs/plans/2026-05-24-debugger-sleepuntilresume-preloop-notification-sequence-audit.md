# Debugger SleepUntilResume Pre-Loop Notification Sequence Audit (2026-05-24)

## Scope

Architecture audit of `SleepUntilResume` pre-loop notification sequence for:

1. Pre-break sequence gating
2. Wait-for-resume arming policy
3. Screensaver enable side-effect policy
4. Preservation of existing pre-loop call ordering

## Findings

1. Pre-loop sequence decisions were driven by inline conditionals.
2. Several side effects shared the same emitted-notification signal.
3. Sequence-gating policy was not represented as reusable helper outcomes.

## Simplification Strategy

1. Add typed pre-loop context/outcome helpers in `DebuggerDispatchUtils`.
2. Resolve sequence policy once before pre-loop block execution.
3. Consume helper outcomes in coordinator while preserving behavior and ordering.
4. Add deterministic tests for emitted vs non-emitted sequence outcomes.

## Implemented Slice

This audit maps to:

1. #2400 helper extraction
2. #2401 regression coverage
3. #2402 SleepUntilResume refactor

## Next Candidate Slice

1. Extract notification dispatch block policy into a compact helper outcome model for code-break dispatch prerequisites and notification-sent state update policy.
2. Add deterministic helper tests for dispatch-trigger and notificationSent assignment policy.
