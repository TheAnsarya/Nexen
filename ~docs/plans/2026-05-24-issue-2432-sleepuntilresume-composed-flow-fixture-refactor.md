# Issue #2432 Plan: SleepUntilResume Composed-Flow Fixture Refactor

## Objective

Reduce test redundancy by introducing shared composed-flow fixture helpers.

## Planned Changes

1. Add shared phase-context builder for continue-path composed tests.
2. Add shared runtime-bundle-context builder derived from phase outcomes.
3. Refactor new composed-flow integration tests to use shared fixture builders.

## Acceptance Criteria

1. Composed-flow tests use reusable fixture helpers.
2. Test setup duplication is reduced without behavior changes.
3. Fixture helpers remain focused and deterministic.
