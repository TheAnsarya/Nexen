# Issue #2385 Plan: SleepUntilResume Shared Pre-Break Side-Effect Refactor

## Objective

Refactor `SleepUntilResume` to consume shared pre-break side-effect policy outcomes.

## Planned Changes

1. Build helper context once from notification eligibility and debug config.
2. Resolve helper outcome once and use it for side-effect execution.
3. Preserve ordering and behavior of pause preamble sequence.

## Acceptance Criteria

1. Coordinator branch density is reduced.
2. Side-effect behavior remains unchanged.
3. Tests and Release build pass.
