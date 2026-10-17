# Issue #2371 Plan: SleepUntilResume Break-Notification Policy Helpers

## Objective

Extract break-notification eligibility and resume-loop delay policy into shared debugger dispatch helpers.

## Planned Changes

1. Add helper for notification eligibility based on `BreakSource` and active break-request state.
2. Add helper for wait-delay selection for resume loop iterations.
3. Keep helper behavior semantically equivalent to current `SleepUntilResume` logic.

## Acceptance Criteria

1. Policy logic is reusable and directly testable.
2. `SleepUntilResume` contains less inline policy branching.
3. No behavior regression.
