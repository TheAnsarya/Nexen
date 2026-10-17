# Issue #2463 Plan - SleepUntilResume Coordinator Entry Refactor

## Goal

Refactor `Debugger::SleepUntilResume` to consume coordinator entry helper for both entry passes.

## Scope

- Replace direct entry phase construction and resolution with helper usage.
- Preserve current control flow and side-effect ordering.

## Acceptance Criteria

- `Debugger.cpp` entry path uses coordinator entry helper.
- Behavior remains unchanged under existing policy tests.
- Validation passes.
