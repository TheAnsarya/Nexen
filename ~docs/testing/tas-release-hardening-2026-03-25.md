# TAS Release Hardening (2026-03-25)

## Scope

This document tracks release-hardening progress for TAS playback and greenzone stability under Epic 23.

Related issues:

- #929 (Epic 23)
- #930 (MaxMemoryBytes enforcement)
- #931 (seek cancellation)
- #932 (prune hot-path optimization)
- #933 (Lynx TAS verification)
- #934 (Atari 2600 TAS verification)

## Implemented Code Changes

1. Greenzone memory budget enforcement
2. Seek cancellation for in-flight seek replacement
3. Prune candidate selection with reusable buffer and no LINQ chain in hot path
4. Greenzone settings UI memory-limit control (MB)
5. TAS seek progress status updates in UI
6. Atari console-switch playback guard by active TAS layout
7. Frame refresh clamping for selection/playback after truncation-like updates
8. Explicit non-Genesis (Lynx/A2600) rerecord failure-path regression coverage

## Test Evidence

Executed targeted managed tests after the changes:

- GreenzoneManagerTests
- GreenzoneManagerOptimizationTests
- TasEditorApiDocAndGuardTests
- TasEditorViewModelBranchAndLayoutTests
- TasEditorWindowConsoleSwitchTests

Observed result: targeted suites passed.

Additional targeted validation:

- `dotnet test Tests/Nexen.Tests.csproj -c Release --filter "FullyQualifiedName~TasEditorViewModelBranchAndLayoutTests" --nologo`

Observed result: 96 passed, 0 failed.

## Benchmark Evidence

Greenzone lookup benchmark run command:

```powershell
dotnet run --project Benchmarks/Nexen.Benchmarks.csproj -c Release -- -j short -m -f "*GreenzoneManagerBenchmarks*Lookup*" --warmupCount 1 --iterationCount 3
```

Selected results from the generated BenchmarkDotNet report:

- GetNearestStateFrame (before start), StateCount=100: mean 16.516 ns
- GetNearestStateFrame (before start), StateCount=1000: mean 15.852 ns
- GetNearestStateFrame (before start), StateCount=10000: mean 18.151 ns
- GetNearestStateFrame (exact match), StateCount=10000: mean 67.366 ns

Artifacts:

- BenchmarkDotNet.Artifacts/results/Nexen.Benchmarks.GreenzoneManagerBenchmarks-report.csv
- BenchmarkDotNet.Artifacts/results/Nexen.Benchmarks.GreenzoneManagerBenchmarks-report-github.md

## Remaining Verification Work

1. #933 Lynx TAS verification
2. #934 Atari 2600 TAS verification
3. Seek-latency benchmark capture for release notes

## Next Execution Plan

1. Add deterministic playback verification for Lynx and Atari 2600 movie frames
2. Add seek/rerecord invalidation assertions with greenzone checkpoints
3. Capture benchmark output and link it in Epic 23 comments
