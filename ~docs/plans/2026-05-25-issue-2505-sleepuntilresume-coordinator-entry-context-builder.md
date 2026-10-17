# Issue #2505 SleepUntilResume Coordinator Entry Context Builder

## Scope

Extract deterministic helper for constructing `SleepUntilResumeCoordinatorEntryContext`.

## Planned Changes

- Add `BuildSleepUntilResumeCoordinatorEntryContext(...)` to centralize guard/source/runtime policy mapping.
- Replace direct field assignment in `Debugger::SleepUntilResume` with helper calls.

## Acceptance Criteria

- Coordinator entry context construction no longer uses manual field assignments.
- Behavior remains unchanged.
