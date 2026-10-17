# Issue #2399 Plan: SleepUntilResume Shared Break-Event Payload Refactor

## Objective

Refactor `SleepUntilResume` to consume shared break-event payload helper outcomes.

## Planned Changes

1. Build break-event context from current coordinator values.
2. Resolve payload helper outcome once before notification dispatch.
3. Dispatch code-break notification using helper-resolved payload.

## Acceptance Criteria

1. Coordinator pre-loop notification block is simplified.
2. Payload behavior remains equivalent.
3. Tests and Release build pass.
