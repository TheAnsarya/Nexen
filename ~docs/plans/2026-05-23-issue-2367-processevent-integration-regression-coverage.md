# Issue #2367 Plan: ProcessEvent Integration-Level Regression Coverage

## Objective

Add regression coverage for composed ProcessEvent behavior, validating routing and branch outcomes together.

## Planned Changes

1. Extend `DebuggerDispatchUtilsTests` with composed behavior tests for:
	- script ownership gate + InputPolled fallback target resolution
	- StartFrame notification and frame-event clearing conditions
2. Keep tests deterministic and independent from emulator runtime lifecycle setup.

## Acceptance Criteria

1. Tests cover multi-rule ProcessEvent outcomes, not only single lookup helpers.
2. Targeted debugger utility suite remains fast and stable.
3. Build and targeted tests pass in Release x64 workflow.
