# Issue #2408 Plan: SleepUntilResume Pre-Loop Policy Bundle Refactor

## Objective

Refactor `SleepUntilResume` to consume composed pre-loop bundle outcomes.

## Planned Changes

1. Replace individual pre-loop helper context setup with bundle context setup.
2. Consume bundle sub-outcomes directly in coordinator pre-loop block.
3. Preserve behavior and call ordering.

## Acceptance Criteria

1. Coordinator pre-loop setup becomes simpler and less repetitive.
2. Behavior and sequencing remain equivalent.
3. Tests and Release build pass.
