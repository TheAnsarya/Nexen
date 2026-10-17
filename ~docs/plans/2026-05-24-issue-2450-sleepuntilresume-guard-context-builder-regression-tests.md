# Issue #2450 Plan - Guard Context Builder Regression Tests

## Goal

Add deterministic tests covering guard-context builder field mapping and decision boundaries.

## Scope

- Add mapping test for all guard fields.
- Add continuation-boundary test validating builder output works with decision policy.

## Acceptance Criteria

- Tests fail on incorrect field mapping.
- Decision continuation boundary remains stable.
- Targeted dispatch-utils suite passes.
