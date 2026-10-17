# Issue #2406 Plan: SleepUntilResume Pre-Loop Policy Bundle Helpers

## Objective

Compose pre-loop policies into a shared bundle helper outcome.

## Planned Changes

1. Add typed bundle context driven by notification emission + debug config flags.
2. Add typed bundle outcome containing pre-break, pre-loop, and dispatch policy outcomes.
3. Build bundle outcome by delegating to existing helper resolvers.

## Acceptance Criteria

1. Bundle helper is pure and behavior-preserving.
2. Coordinator wiring can consume a single composed pre-loop policy object.
3. Existing policy behavior remains unchanged.
