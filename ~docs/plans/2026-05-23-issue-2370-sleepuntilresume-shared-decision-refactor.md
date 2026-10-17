# Issue #2370 Plan: SleepUntilResume Shared Decision Refactor

## Objective

Refactor `Debugger::SleepUntilResume` to use the shared guard decision pipeline and reduce inline branch complexity.

## Planned Changes

1. Build a guard context from current debugger state.
2. Evaluate the shared helper once.
3. Route through a decision switch and retain required side effects (forbidden-breakpoint pending exception clear).

## Acceptance Criteria

1. `SleepUntilResume` guard handling is clearer and easier to maintain.
2. Side effects occur only on the correct decision path.
3. Existing behavior remains unchanged and validation passes.
