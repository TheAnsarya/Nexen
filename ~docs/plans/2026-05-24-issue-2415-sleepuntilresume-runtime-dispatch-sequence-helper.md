# Issue #2415 Plan: SleepUntilResume Runtime Dispatch Sequence Helper

## Objective

Extract a composed runtime helper that unifies break-event payload assembly and dispatch trigger decisions.

## Planned Changes

1. Add `SleepUntilResumeRuntimeDispatchContext` and `SleepUntilResumeRuntimeDispatchOutcome`.
2. Implement `ResolveSleepUntilResumeRuntimeDispatchOutcome(...)` as a pure composition of existing break-event and dispatch policy helpers.
3. Keep behavior unchanged while reducing coordinator policy wiring.

## Acceptance Criteria

1. Helper outcome includes both break-event payload and dispatch decisions.
2. Helper preserves existing payload and dispatch semantics.
3. No behavior regressions in coordinator flow.
