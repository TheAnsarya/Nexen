# Issue #2473 SleepUntilResume Pre-Break Action Plan Helper

## Scope

Add typed helper models in DebuggerDispatchUtils for pre-break action planning derived from SleepUntilResume phase outcomes.

## Design

- Introduce `SleepUntilResumePreBreakActionPlanContext` with:
	- `ShouldRunPreBreakSequence`
	- `ShouldIgnoreBreakpoints`
	- `ShouldDrawPartialFrame`
- Introduce `SleepUntilResumePreBreakActionPlanOutcome` with:
	- `ShouldCallOnBeforeBreak`
	- `ShouldCallOnBeforePause`
	- `ShouldIgnoreBreakpoints`
	- `ShouldDrawPartialFrame`
- Add resolver:
	- `ResolveSleepUntilResumePreBreakActionPlanOutcome(...)`
- Add builder:
	- `BuildSleepUntilResumePreBreakActionPlanContext(const SleepUntilResumePhaseOutcome&)`

## Acceptance Criteria

- Helper structs and resolver compile in Core.
- Builder maps phase pre-loop/pre-break fields correctly.
- Resolver preserves deterministic behavior for run/non-run sequence states.
