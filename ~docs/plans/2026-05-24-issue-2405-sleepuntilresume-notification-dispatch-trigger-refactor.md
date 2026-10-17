# Issue #2405 Plan: SleepUntilResume Notification-Dispatch Trigger Refactor

## Objective

Refactor `SleepUntilResume` dispatch block to consume shared notification-dispatch trigger outcomes.

## Planned Changes

1. Build dispatch context from pre-loop sequence outcome.
2. Resolve dispatch outcome once before notification dispatch.
3. Apply dispatch/process/sent-state actions through helper outcomes.

## Acceptance Criteria

1. Coordinator dispatch block branch density is reduced.
2. Dispatch behavior and ordering remain equivalent.
3. Tests and Release build pass.
