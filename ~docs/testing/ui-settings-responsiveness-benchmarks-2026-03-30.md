# UI Settings Responsiveness Benchmarks (2026-03-30)

## Scope

This document captures a focused benchmark checkpoint for issue #1046: startup-adjacent path-building cost and UI list-refresh responsiveness.

## Benchmark Commands

```powershell
dotnet run --project Benchmarks/Nexen.Benchmarks.csproj -c Release -- --filter "*UiPerformanceBenchmarks.ReuseListAcrossFrames*" --job short --warmupCount 1 --iterationCount 3
dotnet run --project Benchmarks/Nexen.Benchmarks.csproj -c Release -- --filter "*GameDataManagerBenchmarks.BuildPath_Typical*" --job short --warmupCount 1 --iterationCount 3
```

## Environment

- BenchmarkDotNet: 0.15.8
- Runtime: .NET 10.0.4, x64 RyuJIT
- CPU: Intel Core i7-8700K
- Configuration: Release

## Results

| Benchmark | Mean | Notes |
|---|---:|---|
| `UiPerformanceBenchmarks.ReuseListAcrossFrames` | 921.5 ns | ViewModel-list refresh path representative of settings/navigation responsiveness |
| `GameDataManagerBenchmarks.BuildPath_Typical` | 181.2 ns | Startup/load-adjacent game-data path build operation |

## Interpretation

- Navigation-related list update paths remain sub-microsecond in this targeted probe.
- Startup/load-adjacent path construction remains well below microsecond scale per operation.
- No immediate responsiveness regression signal was observed in this checkpoint.

## Artifact Paths

- `BenchmarkDotNet.Artifacts/results/Nexen.Benchmarks.UiPerformanceBenchmarks-report-github.md`
- `BenchmarkDotNet.Artifacts/results/Nexen.Benchmarks.GameDataManagerBenchmarks-report-github.md`

## Follow-Up

- Expand this checkpoint with startup cold-path measurements when dedicated startup benchmark harness is added.
- Keep #1046 closed with this baseline and reopen if UX regressions are reported.
