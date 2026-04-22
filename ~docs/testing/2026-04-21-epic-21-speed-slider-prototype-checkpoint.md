# Epic 21 Speed Menu Slider Prototype Checkpoint (2026-04-21)

## Scope

This checkpoint documents #1410 prototype implementation, usability notes, latency benchmark evidence, input-precision validation, and the ship/no-ship decision path.

## Prototype Implementation

Implemented a prototype dialog launched from Settings -> Speed:

- New menu action: `Speed Slider Prototype...`
- New prototype window: `UI/Windows/SpeedSliderPrototypeWindow.axaml`
- Slider semantics:
	- Pause
	- 25%
	- 50%
	- 100%
	- 200%
	- 300%
	- Maximum speed
- Manual apply model (Apply / Apply and Close) to reduce accidental speed changes.

## UX Notes

- Discoverability:
	- The prototype is directly available under the existing Speed menu where users already adjust speed values.
- Accidental-change risk:
	- Slider movement does not immediately mutate runtime speed; explicit Apply is required.
	- This reduces accidental speed jumps during menu exploration.
- Semantics clarity:
	- Discrete snap points avoid ambiguous intermediate values and preserve existing known presets.
	- Pause and Maximum are represented as explicit slider stops.

## Input Precision Validation

Validation source: `Tests/Utilities/SpeedSliderPrototypeMapperTests.cs`

Verified behaviors:

- Exact mapping for all discrete slider stops.
- Stable speed-to-position conversion for representative values.
- Deterministic midpoint rounding rules:
	- 2.49 -> 2
	- 2.50 -> 3
	- 3.49 -> 3
	- 3.50 -> 4

## Interaction Latency Benchmark

Command:

```powershell
dotnet run --project Benchmarks/Nexen.Benchmarks.csproj -c Release -- --filter "*SpeedSliderPrototypeBenchmarks*" -j short -m
```

Result summary:

- Slider value -> speed selection: 17.368 ns mean
- Current speed -> slider position: 2.569 ns mean
- Allocations: none

Artifacts:

- `BenchmarkDotNet.Artifacts/results/Nexen.Benchmarks.SpeedSliderPrototypeBenchmarks-report-github.md`
- `BenchmarkDotNet.Artifacts/results/Nexen.Benchmarks.SpeedSliderPrototypeBenchmarks-report.html`

## Decision Record (Ship/No-Ship)

Decision: **No-ship as default menu behavior for now; keep as prototype path.**

Rationale:

- The prototype performs well and precision is deterministic.
- Existing direct speed commands remain faster for expert users and already integrated with shortcut muscle memory.
- The prototype should remain available for broader usability testing before replacing the current speed submenu defaults.

Proposed next step if promoted:

- Run structured user validation against accidental-change rates and task completion speed versus current menu presets.

## Acceptance Mapping for #1410

- Prototype + UX notes captured: ✅
- Benchmark interaction latency and input precision: ✅
- Decision recorded for ship/no-ship path: ✅
