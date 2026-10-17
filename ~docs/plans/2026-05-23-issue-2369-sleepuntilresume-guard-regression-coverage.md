# Issue #2369 Plan: SleepUntilResume Guard Regression Coverage

## Objective

Add deterministic regression tests for each `SleepUntilResume` guard decision path and the continue path.

## Planned Changes

1. Extend `DebuggerDispatchUtilsTests` to validate decisions for:
	- suspend request
	- execution already stopped
	- break-request main-CPU boundary protection
	- forbidden breakpoint suppression
	- continue path
2. Ensure tests validate guard precedence where relevant.

## Acceptance Criteria

1. All decision outcomes are directly covered by unit tests.
2. Tests remain fast and independent from emulator lifecycle setup.
3. New tests run in the existing targeted debugger utility suite.
