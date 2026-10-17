# Issue #2446 Plan - SleepUntilResume Phase Context Builder Helper

## Goal

Extract a helper that composes `SleepUntilResumePhaseContext` from guard state, break source, break-request state, and config toggles.

## Scope

- Add `BuildSleepUntilResumePhaseContext(...)` to `Core/Debugger/DebuggerDispatchUtils.h`.
- Keep helper deterministic and side-effect free.
- Preserve existing `ResolveSleepUntilResumePhaseOutcome(...)` behavior.

## Acceptance Criteria

- Helper maps guard/source/break/config fields exactly as coordinator previously wired them.
- Existing phase/routing tests remain green.
- No runtime behavior changes.
