# Issue #2452 Plan - SleepUntilResume Loop/Post Context Builder Helper

## Goal

Extract helper for composing `SleepUntilResumeLoopPostBundleContext` from runtime loop/post inputs.

## Scope

- Add `BuildSleepUntilResumeLoopPostBundleContext(...)` in `Core/Debugger/DebuggerDispatchUtils.h`.
- Keep helper deterministic and side-effect free.
- Preserve loop and post-loop policy semantics.

## Acceptance Criteria

- Helper maps all context fields correctly.
- Existing loop/post policy behavior is unchanged.
- No runtime regressions.
