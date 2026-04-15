# Issue #1297 Warning Inventory And CI Gate Strategy

## Context

Issue: [#1297](https://github.com/TheAnsarya/Nexen/issues/1297)

After v1.4.32 release stabilization, local clean Release builds still report a high warning baseline, mostly from vendored third-party utility code and test dependencies.

## Goals

- Produce an actionable warning inventory grouped by source area and warning code.
- Land at least one low-risk warning reduction in first-party code.
- Clarify CI warning-gate behavior for third-party sources versus first-party sources.

## Inventory Snapshot (Windows Release clean build)

Primary warning concentration:

- `Utilities/Audio/stb_vorbis.cpp` (many `C4244`, `C4456`, `C4457`, `C4701`)
- `Utilities/Audio/ymfm/*` (many `C4244`, `C4100`)
- `vcpkg_installed/.../gtest.h` (`C4389`, `C4018` in test targets)
- Additional isolated warnings in legacy/vendor files (`blip_buf`, `md5`, `NTSC`, `ZmbvCodec`, `SevenZip/Precomp.c`)

## Low-Risk Batch Landed

File: `Core.Benchmarks/Lua/LuaHookBench.cpp`

- Removed unused local `hasMemWriteHooks` in `BM_LuaHook_EmulatorGuard_NoScript`.
- This removes a first-party `C4189` warning with zero runtime behavior impact.

## CI Warning-Gate Strategy Clarification

Current workflow (`.github/workflows/build.yml`) enforces regression gates against warning baselines.

Recommended strategy for #1297 execution:

1. Keep strict regression gates for first-party code (`Core`, `UI`, `Benchmarks`, `Tests`).
2. Maintain explicit baselines for third-party vendored code warnings while triage/migration is ongoing.
3. Prefer low-risk fixes in first-party files first; avoid invasive algorithmic edits in vendored code without dedicated validation passes.
4. For stubborn vendored warning clusters, evaluate per-file suppression or compile options scoped to those files only, with documentation and rationale.

### Implemented In This Batch

- Windows warning workflow now produces both:
	- full warning report (`build-warning-report.txt`)
	- first-party filtered warning report (`build-warning-report-firstparty.txt`)
- Windows warning delta/regression gate now counts filtered first-party warnings.
- Vendor-heavy warning paths are excluded from gate counting (while retained in full diagnostics artifact).

This keeps regression enforcement strict for first-party code while preventing unrelated third-party warning churn from masking useful signal.

## Next Batches

- Batch A: Continue first-party warning sweep (`Core.Benchmarks`, `Core.Tests`, `UI`) for unused locals and narrow conversion casts.
- Batch B: Third-party policy pass (decide suppress vs patch) for `stb_vorbis`, `ymfm`, and gtest warnings.
- Batch C: Re-run warning baselines and update CI baseline constants only when justified and documented.
