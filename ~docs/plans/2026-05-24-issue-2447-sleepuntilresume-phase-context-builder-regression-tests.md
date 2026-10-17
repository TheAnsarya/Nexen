# Issue #2447 Plan - SleepUntilResume Phase Context Builder Regression Tests

## Goal

Add deterministic regression coverage for phase-context builder mapping.

## Scope

- Add mapping test covering guard/source/break/config fields.
- Add mapping test covering non-emitted configuration inputs.
- Ensure tests validate default optional fields remain unchanged.

## Acceptance Criteria

- Tests fail if builder mapping changes unexpectedly.
- Coverage includes emitted and non-emitted source scenarios.
- Targeted dispatch-utils suite passes.
