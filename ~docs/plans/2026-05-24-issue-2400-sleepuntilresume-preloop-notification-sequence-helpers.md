# Issue #2400 Plan: SleepUntilResume Pre-Loop Notification Sequence Helpers

## Objective

Extract pre-loop notification sequence policy into shared debugger dispatch helpers.

## Planned Changes

1. Add typed context for break-notification emission eligibility.
2. Add typed outcome for sequence run gating and side-effect eligibility.
3. Keep helper logic pure and behavior-equivalent.

## Acceptance Criteria

1. Coordinator no longer encodes pre-loop sequence-gating policy inline.
2. Pre-loop side-effect eligibility becomes reusable and testable.
3. Runtime behavior remains unchanged.
