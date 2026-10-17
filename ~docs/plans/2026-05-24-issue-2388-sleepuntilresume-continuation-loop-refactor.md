# Issue #2388 Plan: SleepUntilResume Shared Continuation-Loop Policy Refactor

## Objective

Refactor `SleepUntilResume` continuation loop to consume shared policy outcomes.

## Planned Changes

1. Build loop context each iteration from live wait/suspend/break-request state.
2. Resolve loop outcome each iteration and branch via `ShouldContinueWaiting`.
3. Sleep using helper-provided wait delay while preserving behavior.

## Acceptance Criteria

1. Coordinator loop condition complexity is reduced.
2. Behavior is equivalent to previous continuation semantics.
3. Tests and Release build pass.
