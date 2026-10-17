# Modern Library Baseline and Upgrade Policy (2026-03-29)

## Scope

This policy documents the Epic 22 modernization baseline and safe-upgrade cadence for critical dependencies.

Tracked by:

- #1050 Modern library/version baseline and upgrade policy
- #1048 Epic 22 parent

## Baseline Snapshot

1. .NET SDK/runtime line: `10.x`
2. C# language/runtime target: .NET 10 projects across UI/tools
3. C++ standard/toolchain target:

- C++23 (`/std:c++latest`)
- v145 toolset on Windows CI
- GCC 13 / LLVM 18 on Linux CI

4. Key dependency baseline (current stabilized set):

- `Microsoft.Extensions.Logging*` aligned on 10.0.x line
- `StreamHash` baseline at 1.11.2
- Avalonia and Dock packages kept on compatibility-aligned train used by current UI stack
- vcpkg baseline pinned at commit `c3867e714dd3a51c272826eea77267876517ed99` (2026.03.18 release)

## Upgrade Cadence Policy

1. Prefer modern stable versions by default.
2. Upgrade in small compatibility-preserving batches.
3. After each batch, require:

- Release build success
- Native tests pass
- Managed tests pass
- Benchmark checkpoint without unacceptable regression

4. For risky or high-churn upgrades:

- Use dedicated issue
- Include rollback plan
- Gate merge on CI and smoke-launch evidence

## Rollback Criteria

Rollback or pin freeze is required when any of the following occurs:

1. Deterministic startup/runtime regression on supported platforms.
2. CI release workflow failure that is attributable to dependency drift.
3. Test regressions without immediate safe fix.
4. Significant performance regression in benchmark checkpoints.

## Evidence and Traceability

1. Warning baseline and strict regression gate state are tracked in `build.yml` and Epic 22 validation docs.
2. Runtime dependency validation artifacts are captured for Windows/Linux/AppImage/macOS publish surfaces.
3. Session logs for modernization actions are recorded under `~docs/session-logs/`.
