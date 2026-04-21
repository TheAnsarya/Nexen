# Epic 21 UI Benchmark and Test Plan (2026-04-21)

## Purpose

Define measurable UX benchmark and regression gates for #1402 and sub-issues.

## Benchmarks

### B1 Settings Navigation Cost

- Task: reach active-system controller config from main settings.
- Metric: click count and elapsed time.
- Target: reduce click count versus baseline.

### B2 First-Run Setup Efficiency

- Task: complete setup wizard with defaults.
- Metric: completion time and backtrack count.
- Target: lower median completion time versus baseline.

### B3 Dense Window Usability

- Task: change options near bottom of Debugger settings tabs.
- Metric: clipped controls count, scroll failures, completion time.
- Target: zero clipped controls and zero scroll dead-ends.

### B4 Input Context Landing

- Task: open Input settings while SNES ROM loaded.
- Metric: route correctness and time to first edit.
- Target: always lands on active-system pane.

## Automated Test Matrix

- Window creation and resizability checks for settings windows.
- Scroll container presence checks in dense tabs.
- Route selection tests for Input settings active system landing.
- Theme switching smoke checks (light and dark).

## Manual Test Matrix

- Desktop mouse workflow at 100% and 150% scaling.
- Touch-like large target validation on high-DPI display.
- Keyboard-only navigation through settings tabs.
- Controller navigation pass for key settings routes.

## Evidence Format

For each benchmark run record:

- Build/commit hash
- Task script
- Baseline value
- Current value
- Delta and pass/fail threshold

## Initial Baseline Collection

- Capture baseline before major IA and visual changes.
- Re-run after each merged sub-issue cluster.
- Post summaries to #1414 and #1415.
