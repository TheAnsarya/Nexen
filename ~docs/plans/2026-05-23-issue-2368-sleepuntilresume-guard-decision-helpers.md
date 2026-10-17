# Issue #2368 Plan: SleepUntilResume Guard Decision Helpers

## Objective

Extract `SleepUntilResume` early-return guard decisions into shared debugger dispatch helpers with explicit typed outcomes.

## Planned Changes

1. Add a guard-input context struct and decision enum to `DebuggerDispatchUtils`.
2. Add a pure decision evaluator returning the selected skip/continue outcome.
3. Preserve existing behavior and decision precedence.

## Acceptance Criteria

1. Guard rules are reusable and testable independently from `Debugger` runtime state.
2. Decision priority remains equivalent to current behavior.
3. No runtime behavior regression.
