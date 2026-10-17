# Issue #2496 SleepUntilResume PhaseOutcome Internal Context Builders

## Scope

Add helper builders for phase-outcome internal context construction.

## Planned Changes

- Add `BuildSleepUntilResumePreLoopBundleContext(const SleepUntilResumePhaseContext&, bool)`.
- Add `BuildSleepUntilResumeLoopContext(const SleepUntilResumePhaseContext&)`.
- Add `BuildSleepUntilResumePostLoopContext(const SleepUntilResumePhaseContext&)`.

## Acceptance Criteria

- Helpers compile and are consumed by phase-outcome resolver.
- Mapping behavior is deterministic and behavior-preserving.
