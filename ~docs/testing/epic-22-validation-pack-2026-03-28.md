# Epic 22 Validation Pack (2026-03-28)

## Build and Warning Evidence

## Baseline

1. Release build performed with MSBuild (`Nexen.sln`, `Release|x64`).
2. Warning scans executed via log extraction (`build-epic22-warnings.log`).

## Findings

1. Initial modernization pass introduced MSB3277 version conflicts in `Nexen.Benchmarks.csproj`.
2. Conflict cluster reduced by restoring compatible Avalonia/Dock train in UI project.
3. Remaining `System.IO.Hashing` conflict resolved by explicit package reference in [Benchmarks/Nexen.Benchmarks.csproj](../../Benchmarks/Nexen.Benchmarks.csproj).
4. Final warning scan for this pass returned no warning matches under the configured scan pattern.

## Regression Tests

1. Native gtest checkpoint:
- Command: `Core.Tests.exe --gtest_filter=NotificationManagerTests.* --gtest_brief=1`
- Result: 3/3 passed.

2. Managed tests checkpoint:
- Command: `dotnet test --no-build -c Release`
- Result: 587/587 passed.

## Benchmark Checkpoint

1. Command:
- `Core.Benchmarks.exe --benchmark_filter=BM_Atari2600_PerformanceGate_Corpus --benchmark_repetitions=3`

2. Result summary:
- Mean time: 1,127,732 ns.
- Median time: 1,129,122 ns.
- Mean throughput: 2.644k items/s.

## Runtime Compatibility Validation

1. CI now checks Linux publish runtime config and ICU native assets in:
- Linux matrix build job.
- AppImage build job.

2. Expected assertions:
- `System.Globalization.AppLocalIcu` must be `72.1.0.3`.
- Publish output must contain:
- `libicudata.so.72.1.0.3`
- `libicui18n.so.72.1.0.3`
- `libicuuc.so.72.1.0.3`

## Coverage and Risks

1. Covered:
- Release build quality gate.
- Focused native regression.
- Full managed test suite.
- Performance sanity checkpoint.
- Linux ICU runtime artifact checks in CI.

2. Remaining risk:
- Linux-specific runtime behavior still requires cross-distro runtime validation on real targets.
