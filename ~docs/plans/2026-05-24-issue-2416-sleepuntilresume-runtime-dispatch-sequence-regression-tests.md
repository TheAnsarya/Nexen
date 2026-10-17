# Issue #2416 Plan: SleepUntilResume Runtime Dispatch Sequence Regression Tests

## Objective

Add deterministic tests for composed runtime dispatch-sequence behavior.

## Planned Changes

1. Add tests for non-emitted dispatch sequence path where dispatch/process/sent-state remain disabled.
2. Add tests for emitted dispatch sequence path where dispatch/process/sent-state are enabled.
3. Validate break-event payload propagation, including optional operation copy behavior.

## Acceptance Criteria

1. Runtime-dispatch helper behavior is fully covered for emitted and non-emitted paths.
2. Payload fields and optional operation copy behavior are asserted.
3. Tests guard against future dispatch-sequence policy drift.
