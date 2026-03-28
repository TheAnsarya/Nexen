# Linux Crash Hardening Fix Order (2026-03-28)

## Scope

Execution ordering for Issue #1049 under Epic #1048.

## Priority Order

1. Runtime dependency correctness gate (highest).
2. Native graphics dependency consistency gate.
3. Cross-distro smoke validation matrix.
4. Warning/reporting automation for regression visibility.
5. Broader modernization upgrades guarded by compatibility checks.

## Ordered Work Items

### 1. Runtime Asset Gate (ICU)

1. Validate `System.Globalization.AppLocalIcu` exact token in runtime config.
2. Assert required ICU files exist in Linux publish output.
3. Fail CI immediately when checks fail.

Status:

1. Implemented in [build.yml](../../.github/workflows/build.yml).

### 2. Native Graphics Asset Gate (Skia/HarfBuzz)

1. Assert `libSkiaSharp.so` and `libHarfBuzzSharp.so` in Linux publish output.
2. Apply checks to both Linux and AppImage jobs.

Status:

1. Implemented in [build.yml](../../.github/workflows/build.yml).

### 3. Warning Delta Visibility

1. Capture build warning output for Windows, Linux, and AppImage jobs.
2. Upload warning reports as CI artifacts for run-to-run deltas.

Status:

1. Implemented in [build.yml](../../.github/workflows/build.yml).

### 4. Distro Reproduction Matrix Maintenance

1. Keep matrix of distro signatures and mitigations updated.
2. Link mitigations to concrete CI checks and commits.

Status:

1. Baseline established in [~docs/testing/linux-runtime-crash-matrix-2026-03-28.md](../testing/linux-runtime-crash-matrix-2026-03-28.md).

### 5. Controlled Modernization Batches

1. Upgrade dependencies in small compatibility-preserving batches.
2. After each batch run, execute: Release build, focused native tests, .NET tests, and a benchmark checkpoint.

Status:

1. Ongoing under #1050 and #1051.

## Exit Criteria for #1049

1. Runtime dependency failures are CI-gated for Linux publish paths.
2. Reproduction matrix shows no open high-severity startup crash signature without mitigation.
3. At least one cross-distro validation pass confirms startup stability with current artifacts.
