# Debugger SleepUntilResume Notification Policy Audit (2026-05-23)

## Scope

Architecture audit of `Debugger::SleepUntilResume` notification-emission and resume-loop delay policy.

## Findings

1. Notification eligibility logic was embedded inline with break orchestration flow.
2. Resume-loop wait delay selection used an inline ternary with magic constants.
3. Policy logic was not reusable or directly testable independently of coordinator flow.

## Simplification Strategy

1. Extract pure helper functions in `DebuggerDispatchUtils` for:
	- break-notification eligibility
	- wait-delay selection
2. Keep `SleepUntilResume` as coordinator logic consuming helper decisions.
3. Add deterministic helper tests for source/break-request combinations and delay mapping.

## Implemented Slice

This audit maps to:

1. #2371 helper extraction
2. #2381 regression coverage
3. #2382 coordinator refactor

## Next Candidate Slice

1. Extract helper-driven policy for `SingleBreakpointPerInstruction`/`DrawPartialFrame` pre-break side effects.
2. Add tests for break preamble side-effect gating before deeper coordinator decomposition.
