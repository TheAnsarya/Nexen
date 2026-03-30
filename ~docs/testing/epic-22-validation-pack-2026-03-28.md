# Epic 22 Validation Pack (2026-03-28)

## Build and Warning Evidence

### Baseline

1. Release build performed with MSBuild (`Nexen.sln`, `Release|x64`).
2. Warning scans executed via log extraction (`build-epic22-warnings.log`).

### Findings

1. Initial modernization pass introduced MSB3277 version conflicts in `Nexen.Benchmarks.csproj`.
2. Conflict cluster reduced by restoring compatible Avalonia/Dock train in UI project.
3. Remaining `System.IO.Hashing` conflict resolved by explicit package reference in [Benchmarks/Nexen.Benchmarks.csproj](../../Benchmarks/Nexen.Benchmarks.csproj).
4. Final warning scan for this pass returned no warning matches under the configured scan pattern.

## Regression Tests

1. Native gtest checkpoint.

- Command: `Core.Tests.exe --gtest_filter=NotificationManagerTests.* --gtest_brief=1`
- Result: 3/3 passed.

1. Managed tests checkpoint.

- Command: `dotnet test --no-build -c Release`
- Result: 587/587 passed.

## Benchmark Checkpoint

1. Command.

- `Core.Benchmarks.exe --benchmark_filter=BM_Atari2600_PerformanceGate_Corpus --benchmark_repetitions=3`

1. Result summary.

- Mean time: 1,127,732 ns.
- Median time: 1,129,122 ns.
- Mean throughput: 2.644k items/s.

## Runtime Compatibility Validation

1. CI now checks Linux publish runtime config and ICU native assets in both Linux matrix and AppImage jobs.

2. CI now captures runtime dependency/file reports for Windows publish, Linux publish, AppImage publish, and macOS publish artifacts.

3. CI now enforces required native runtime asset presence on all primary publish surfaces.

2. Expected assertions.

- `System.Globalization.AppLocalIcu` must be `72.1.0.3`.
- Publish output must contain `libicudata.so.72.1.0.3`.
- Publish output must contain `libicui18n.so.72.1.0.3`.
- Publish output must contain `libicuuc.so.72.1.0.3`.

3. Cross-platform required native asset assertions.

- Windows publish must contain `Nexen.exe`, `libSkiaSharp.dll`, `libHarfBuzzSharp.dll`.
- Linux/AppImage publish must include ICU app-local assets plus graphics natives already validated in prior steps.
- macOS app bundle must contain `Nexen` binary, `libSkiaSharp*.dylib`, and `libHarfBuzzSharp*.dylib`.

## CI Regression Gate Expansion (#1055)

1. Warning delta artifacts are now paired with an optional fail gate in all build jobs.

- Windows.
- Linux.
- AppImage.

1. Gate behavior.

- `WARNING_FAIL_ON_REGRESSION=1` is now enabled in `build.yml`.
- Jobs fail when warning delta is positive.

2. Baseline calibration source runs (#1056).

- 23720620324 (master)
- 23720623170 (v1.4.9)
- 23720627204 (feature/channel-f-remaining)
- 23721111328 (feature/channel-f-remaining)
- 23721114532 (master)

3. Baseline values applied.

- `WARNING_BASELINE_WINDOWS=0`
- `WARNING_BASELINE_LINUX=4063`
- `WARNING_BASELINE_APPIMAGE=145`

4. Rationale.

- Linux baseline uses the observed max from gcc matrix runs to avoid false-positive failures while still detecting warning regressions above known current levels.
- AppImage baseline uses stable repeated count observed across x64/ARM64 jobs.
- Windows baseline remains zero from repeated scans.

1. Linux crash smoke checks now run for Linux publish binary (`Nexen`) and AppImage artifact (`Nexen.AppImage`).

2. Smoke-launch logic.

- Uses `xvfb-run` with timeout budget to detect early startup crashes.
- Treats `segmentation fault`, `sigsegv`, `core dumped`, `abort`, and `fatal` signatures as failures.
- Uploads smoke logs for post-failure triage.

## Coverage and Risks

1. Covered.

- Release build quality gate.
- Focused native regression.
- Full managed test suite.
- Performance sanity checkpoint.
- Linux ICU runtime artifact checks in CI.

1. Remaining risk.

- Linux-specific runtime behavior still requires cross-distro runtime validation on real targets.
- Runtime dependency checks are now CI-enforced across publish platforms, but distro-specific driver/runtime stacks still require field validation.
- Warning fail gate defaults to disabled until baseline values are tuned from real CI history.
