# Issue #2404 Plan: SleepUntilResume Notification-Dispatch Trigger Regression Tests

## Objective

Add deterministic utility tests for notification-dispatch trigger helper outcomes.

## Planned Changes

1. Validate dispatch/process/sent-state outcomes are disabled when sequence is disabled.
2. Validate dispatch/process/sent-state outcomes are enabled when sequence is enabled.
3. Keep tests fast and deterministic.

## Acceptance Criteria

1. Trigger and sent-state outcomes are locked by helper tests.
2. Regressions in dispatch-trigger policy are caught in utility coverage.
3. Tests remain low-overhead and stable.
