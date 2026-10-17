# Issue #2448 Plan - SleepUntilResume Phase Context Builder Refactor

## Goal

Refactor `Debugger::SleepUntilResume` to consume phase-context builder helper for both phase outcome passes.

## Scope

- Replace manual phase context wiring before first phase-outcome evaluation.
- Replace manual config-toggle assignment by rebuilding context through helper.
- Keep coordinator behavior and evaluation order unchanged.

## Acceptance Criteria

- `Debugger.cpp` uses helper for both phase context constructions.
- Existing composed flow behavior remains unchanged.
- Targeted tests and Release build validations pass.
