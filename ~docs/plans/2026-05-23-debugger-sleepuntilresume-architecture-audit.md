# Debugger SleepUntilResume Architecture Audit (2026-05-23)

## Scope

Focused architecture audit of `Debugger::SleepUntilResume` guard logic and related break-request gating behavior.

## Findings

1. The early-return guard chain mixed multiple concerns inline:
	- suspend-request short-circuit
	- re-entry protection
	- main-CPU safety boundary for pending break requests
	- forbidden breakpoint suppression side effect
2. Guard decision priority was implicit in the branch order and difficult to test independently.
3. Side-effect behavior (`ClearPendingBreakExceptions`) was coupled to inline branching rather than an explicit decision outcome.

## Simplification Strategy

1. Move guard decision rules into shared utility helpers with a typed decision enum.
2. Keep `SleepUntilResume` as coordinator logic using a single evaluated decision switch.
3. Add deterministic utility tests for each decision path and continue path.

## Implemented Slice

This audit feeds the issue trio:

1. #2368 helper extraction
2. #2369 guard regression coverage
3. #2370 coordinator refactor to shared decision pipeline

## Next Candidate Slice

1. Extract and test notification/OnBeforeBreak orchestration decision points.
2. Add helper-driven decomposition for break-notification emission and resume-loop policy.
