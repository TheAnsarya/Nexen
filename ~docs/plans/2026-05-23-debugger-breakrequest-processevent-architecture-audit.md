# Debugger BreakRequest and ProcessEvent Architecture Audit (2026-05-23)

## Scope

Audit focused on two debugger hotspots with high branching and duplicated decision logic:

1. `Debugger::ProcessEvent`
2. `Debugger::SleepUntilResume` and break-request gating paths

## Findings

1. `ProcessEvent` previously mixed four concerns in one method:
	- CPU routing
	- script ownership gate
	- per-event branch execution
	- fallback target resolution for InputPolled
2. Start-frame behavior combined notification and event-manager responsibilities inline.
3. Break-request paths remain behavior-critical and correctness-sensitive; they should stay accuracy-first and avoid broad control-flow rewrites.

## Simplification Strategy

1. Move pure routing/decision rules into `DebuggerDispatchUtils` as composable helpers.
2. Keep `ProcessEvent` as a thin coordinator and delegate branch bodies to named handlers.
3. Add composed regression tests that validate multi-rule outcomes, not only isolated map lookups.
4. Keep BreakRequest changes incremental: extract decision helpers only when behavior is fully covered by focused tests.

## Implemented Slice

This audit produced the current implementation slice tracked by:

1. #2365 helper extraction
2. #2366 per-event handler split
3. #2367 integration-level regression coverage

## Next Candidate Slice

1. Extract a pure helper for break-request stop eligibility currently embedded in `SleepUntilResume` guards.
2. Add focused tests for break-request guard combinations before any coordinator rewiring.
