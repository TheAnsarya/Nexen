# Issue #2453 Plan - Loop/Post Context Builder Regression Tests

## Goal

Add deterministic tests validating loop/post context builder mapping and idle composition behavior.

## Scope

- Add test covering all loop/post context field mappings.
- Add test for non-emitted idle combination and composed loop/post outcomes.

## Acceptance Criteria

- Tests fail on incorrect field mapping.
- Idle combination behavior remains stable.
- Targeted dispatch-utils tests pass.
