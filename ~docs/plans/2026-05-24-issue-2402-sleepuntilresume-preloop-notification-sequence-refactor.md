# Issue #2402 Plan: SleepUntilResume Shared Pre-Loop Notification Sequence Refactor

## Objective

Refactor `SleepUntilResume` pre-loop notification block to consume shared sequence policy outcomes.

## Planned Changes

1. Build pre-loop context from existing notification emission state.
2. Resolve sequence helper outcome once before pre-loop block.
3. Gate pre-loop side effects through helper outcome while preserving call order.

## Acceptance Criteria

1. Coordinator pre-loop block branch density is reduced.
2. Runtime behavior/order remain equivalent.
3. Tests and Release build pass.
