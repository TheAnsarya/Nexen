# Issue #2381 Plan: SleepUntilResume Notification/Loop Policy Regression Tests

## Objective

Add deterministic regression coverage for `SleepUntilResume` notification and loop-delay policy helpers.

## Planned Changes

1. Test notification policy for:
	- explicit break sources
	- `BreakSource::Unspecified` with and without active break requests
2. Test loop-delay policy mapping:
	- break-request active => `1` ms
	- break-request inactive => `10` ms

## Acceptance Criteria

1. Tests cover true/false notification outcomes.
2. Tests cover both delay outputs.
3. Utility test suite remains fast and deterministic.
