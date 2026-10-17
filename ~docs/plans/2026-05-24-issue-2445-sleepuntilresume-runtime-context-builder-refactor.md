# Issue #2445 Plan - SleepUntilResume Runtime Context Builder Refactor

## Goal

Refactor `Debugger::SleepUntilResume` to consume the runtime context builder helper.

## Scope

- Replace manual context wiring block with helper invocation.
- Preserve existing runtime bundle evaluation order and side effects.
- Keep coordinator logic behavior-identical.

## Acceptance Criteria

- `Debugger.cpp` runtime context mapping is reduced to helper call.
- Runtime dispatch and side-effect behavior remains unchanged.
- Targeted tests and Release build pass.
