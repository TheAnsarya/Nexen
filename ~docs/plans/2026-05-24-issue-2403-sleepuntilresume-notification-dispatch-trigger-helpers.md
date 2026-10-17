# Issue #2403 Plan: SleepUntilResume Notification-Dispatch Trigger Helpers

## Objective

Extract notification-dispatch trigger policy into shared debugger dispatch helpers.

## Planned Changes

1. Add typed context for pre-loop sequence enablement.
2. Add typed outcome for code-break dispatch, event processing, and sent-state transition decisions.
3. Keep helper logic pure and behavior-equivalent.

## Acceptance Criteria

1. Coordinator no longer hardcodes dispatch-trigger and sent-state decisions inline.
2. Trigger policy is reusable and directly testable.
3. Runtime behavior remains unchanged.
