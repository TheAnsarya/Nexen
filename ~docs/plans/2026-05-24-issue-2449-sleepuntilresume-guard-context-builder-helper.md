# Issue #2449 Plan - SleepUntilResume Guard Context Builder Helper

## Goal

Extract helper to compose `SleepUntilResumeGuardContext` from runtime guard inputs.

## Scope

- Add `BuildSleepUntilResumeGuardContext(...)` in `Core/Debugger/DebuggerDispatchUtils.h`.
- Keep helper deterministic and side-effect free.
- Preserve guard-decision semantics.

## Acceptance Criteria

- Helper maps all guard fields correctly.
- Existing decision tests remain valid.
- No behavior regressions.
