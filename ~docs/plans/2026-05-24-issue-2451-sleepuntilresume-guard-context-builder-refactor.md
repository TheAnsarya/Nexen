# Issue #2451 Plan - SleepUntilResume Guard Context Builder Refactor

## Goal

Refactor `Debugger::SleepUntilResume` to consume guard-context builder helper.

## Scope

- Replace manual guard-context field assignments with helper invocation.
- Preserve guard evaluation ordering and behavior.

## Acceptance Criteria

- `Debugger.cpp` uses helper for guard-context construction.
- Existing policy behavior is preserved.
- Targeted tests and Release build validations pass.
