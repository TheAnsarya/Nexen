# Issue #2443 Plan - SleepUntilResume Runtime Context Builder Helper

## Goal

Extract a helper that builds `SleepUntilResumeRuntimeBundleContext` from phase outcome and runtime payload.

## Scope

- Add `BuildSleepUntilResumeRuntimeBundleContext(...)` to `Core/Debugger/DebuggerDispatchUtils.h`.
- Keep helper pure and deterministic.
- Avoid behavioral changes to dispatch or side-effect policies.

## Acceptance Criteria

- Helper returns context fields equivalent to current coordinator wiring.
- Existing runtime bundle outcome tests still pass.
- No functional regressions in `SleepUntilResume` flows.
