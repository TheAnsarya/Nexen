# Issue #2396 Plan: SleepUntilResume Break-Event Payload Policy Helpers

## Objective

Extract break-event payload assembly policy into shared debugger dispatch helpers.

## Planned Changes

1. Add typed context for source CPU, source reason, breakpoint ID, and optional operation pointer.
2. Add typed outcome containing payload-ready event data and operation-presence signal.
3. Keep helper logic pure and behavior-equivalent.

## Acceptance Criteria

1. Coordinator no longer manually assembles break-event payload fields inline.
2. Optional operation propagation is represented and testable.
3. Runtime behavior remains unchanged.
