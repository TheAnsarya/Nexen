# Issue #2383 Plan: SleepUntilResume Pre-Break Side-Effect Policy Helpers

## Objective

Extract pre-break side-effect policy into shared debugger dispatch helpers.

## Planned Changes

1. Add typed context for notification eligibility and side-effect config flags.
2. Add typed outcome with resolved side-effect booleans.
3. Keep policy logic pure and behavior-equivalent.

## Acceptance Criteria

1. Coordinator no longer contains repeated config-gating logic.
2. Policy decisions are reusable and testable.
3. Runtime behavior remains unchanged.
