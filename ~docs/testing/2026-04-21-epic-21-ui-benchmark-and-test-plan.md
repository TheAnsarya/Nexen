# Epic 21 UI Benchmark and Test Plan (2026-04-21)

## Purpose

Define measurable UX benchmark and regression gates for #1402 and sub-issues.

## Reproducible Harness

Run the Epic 21 harness script from repository root:

```powershell
.\~scripts\run-epic21-ui-telemetry-benchmarks.ps1 -Label baseline
```

Outputs:

- Baseline snapshot JSON: `~docs/testing/data/epic-21-ui-<label>-<date>.json`
- BenchmarkDotNet reports:
	- `BenchmarkDotNet.Artifacts/results/Nexen.Benchmarks.TasUiBenchmarks-report.csv`
	- `BenchmarkDotNet.Artifacts/results/Nexen.Benchmarks.TasUiBenchmarks-report-github.md`
	- `BenchmarkDotNet.Artifacts/results/Nexen.Benchmarks.TasUiBenchmarks-report.html`

This harness combines lightweight setup-wizard telemetry (`setup-wizard-metrics.json`) with focused TAS UI benchmark scenarios.

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

## Regression Thresholds (#1414)

| Metric | Threshold | Rationale |
|---|---|---|
| Completion rate | `>= 0.70` | Setup completion should remain a clear majority outcome when onboarding flow changes land. |
| Backtracks per outcome | `<= 1.50` | Navigation depth should not increase beyond one to two corrective steps per outcome. |
| Interaction toggles per launch | `<= 3.00` | UI interaction cost should remain low for primary setup tasks. |
| TAS search time @ 100000 frames | `<= 3.00 ms` | Keeps large-movie search responsiveness stable for UI-heavy workflows. |
| TAS contiguous selection time @ 100000 frames | `<= 1.50 us` | Guards selection-path interaction latency for editor usability. |

## Initial Baseline Collection

- Capture baseline before major IA and visual changes.
- Re-run after each merged sub-issue cluster.
- Post summaries to #1414 and #1415.

### Captured Baseline (2026-04-21)

Source artifacts:

- `~docs/testing/data/epic-21-ui-baseline-2026-04-21.json`
- `BenchmarkDotNet.Artifacts/results/Nexen.Benchmarks.TasUiBenchmarks-report.csv`

Telemetry snapshot:

- Launch count: `0`
- Completion count: `0`
- Cancel count: `0`
- Profile toggle count: `21`
- Storage toggle count: `21`
- Backtrack count: `0`

Benchmark baseline:

| Scenario | Movie size | Mean |
|---|---|---|
| Search by button state (A pressed) | `1000` | `5.413 us` |
| Search by button state (A pressed) | `10000` | `82.129 us` |
| Search by button state (A pressed) | `100000` | `2,618.281 us` |
| Select range (1000 contiguous) | `1000` | `1.027 us` |
| Select range (1000 contiguous) | `10000` | `1.359 us` |
| Select range (1000 contiguous) | `100000` | `1.336 us` |
