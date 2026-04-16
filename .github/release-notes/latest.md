# Nexen v1.4.33 ΓÇö Quality, Stability & Migration Improvements

> ≡ƒÅù∩╕Å **18 commits** | ≡ƒÉ¢ **12 issues resolved** | Γ£à **5,134 tests passing** (3,690 C++ + 1,444 .NET) | ΓÜá∩╕Å **Zero warnings**

Nexen v1.4.33 is a comprehensive quality-and-stability release that wraps up the v1.4.32 performance triage and GameDataManager migration epic. This release features per-game data migration improvements, CI modernization to Node 24, WonderSwan timing validation hardening, a complete SteamOS guide rewrite, codebase cleanup eliminating all stale `TODOv2` markers, and benchmark infrastructure for evidence-based performance work.

---

## Γ£¿ Highlights

| Feature | Details |
|---------|---------|
| ≡ƒùé∩╕Å **Per-game data migration prompt** | ROM load now prompts before migrating save states and battery saves to per-game folders ΓÇö no more unexpected file moves |
| ≡ƒñû **CI modernization (Node 24)** | GitHub Actions workflows force Node 24 runtime, eliminating all deprecated Node.js 16/20 warnings |
| ≡ƒÄ« **WonderSwan timing validation** | Hardened timing gate tests with multi-event ordering checks for improved WS/WSC emulation confidence |
| ≡ƒº╣ **TODOv2 marker triage** | All 20 files with stale `TODOv2` markers converted to issue-scoped `TODO(#issue)` references |
| ΓÜí **Performance benchmark infrastructure** | Hotpath benchmark artifacts stabilized with repeatable scripts and evidence-backed triage |
| ≡ƒôû **SteamOS guide rewrite** | Complete rewrite of the Steam Deck / SteamOS installation guide with Nexen-specific instructions |
| ΓÅ╕∩╕Å **Pause behavior fix** | Menu/config auto-pause suppressed during ROM load transitions for seamless game startup |

---

## ≡ƒÉ¢ Fixed

### Per-game migration now requires user confirmation ΓÇö #1274

The `GameDataManager` migration pipeline now checks for pending save state and battery save migrations before moving files. A confirmation dialog (`ConfirmGameDataMigration`) is displayed to the user, and a new `GetPendingMigrationCount()` API provides pre-migration enumeration. This prevents the jarring experience of files being silently reorganized on first load after an update.

### Menu/config pause suppressed during ROM load ΓÇö #1290, #1300

When "Pause when in menu" or "Pause when in config" was enabled, opening a ROM could trigger an unintended auto-pause during the load transition. The pause guard now correctly suppresses auto-pause events that fire during ROM load, ensuring seamless game startup.

### Pansy.Core package-only enforcement ΓÇö #1298, #1302

CI now validates that `Pansy.Core` is consumed strictly as a NuGet package (not a project reference), preventing accidental coupling between the Nexen and Pansy solutions. A UI resource prep step was added to the package-only validation lane to ensure the build completes end-to-end.

### UTF-8 stream compatibility and test warnings ΓÇö #1297

Restored UTF-8 stream compatibility for cross-platform builds and reduced test warning noise from `StringUtilities` `size_t` assertions. Also fixed escaped newline sequences (`\n`) appearing as literal text in published GitHub issue comments.

### COMPILING.md Pansy dependency documentation ΓÇö #1298, #1299

Added documentation for the Pansy.Core NuGet dependency and created decoupling plans for future separation.

---

## ≡ƒöä Changed

### GitHub Actions ΓåÆ Node 24 runtime ΓÇö #1303

All CI workflows now set `FORCE_JAVASCRIPT_ACTIONS_TO_NODE24: true` in the environment, forcing the GitHub Actions runner to use Node 24 for all JavaScript-based actions. This eliminates the deprecation warnings that were cluttering every CI run since GitHub announced the Node 16/20 sunset.

### TODOv2 ΓåÆ issue-scoped TODO triage ΓÇö #1281

All 20 files containing stale `TODOv2` markers (inherited from the Mesen2 era) have been triaged. Each marker was either:
- Converted to `TODO(#issue)` format with a tracking issue reference
- Resolved and removed if the work was already done
- Documented as intentional divergence from upstream

### SteamOS / Steam Deck guide ΓÇö #1282, #1283

The `SteamOS.md` guide was entirely stale Mesen2 content. It has been completely rewritten with:
- Nexen-specific download and installation instructions
- Desktop Mode and Game Mode configuration
- Controller mapping guidance
- Performance tips for Steam Deck hardware
- Linked from the main `README.md` documentation section

### First-party warning reduction ΓÇö #1297

Batch reduction of first-party compiler warnings across `Utilities/` and `Core.Tests/`. The CI windows warning delta is now gated on first-party signal only, preventing third-party header noise from blocking merges.

---

## Γ₧ò Added

### WonderSwan timing gate hardening ΓÇö #1285, #1287, #1076

Multi-event ordering assertions added to WS timing gate tests, providing improved validation of frame-level event sequencing. This work is part of the ongoing WonderSwan PPU accuracy investigation (#1076) and ensures that timing regressions are caught automatically.

### Hotpath benchmark stabilization ΓÇö #1216, #1218

- Repeatable `run-memory-hotpath-benchmarks.ps1` script with labeled, timestamped JSON output
- GBA `ProcessInternalCycle` distribution coverage benchmarks
- Evidence-backed performance triage documentation showing that `[[likely]]`/`[[unlikely]]` annotations had no measurable effect on the tested hotpaths
- Documented approach for future PGO/profiling-guided optimization

### Per-game migration count API ΓÇö #1274

New `GetPendingMigrationCount()` method and localized `ConfirmGameDataMigration` dialog resource for the migration prompt flow.

---

## ΓÜí Performance

### Hotpath branch-prediction triage ΓÇö #1216

Real emulator-path microbenchmarks were created for GBA and NES memory hotpaths to evaluate whether `[[likely]]`/`[[unlikely]]` C++20 attributes would improve performance. After rigorous benchmarking with `--benchmark_repetitions=3` and multiple runs:

- **Result:** No statistically significant improvement detected
- **Conclusion:** Modern compilers (MSVC v145, clang 18) already optimize these branches well
- **Next step:** PGO (Profile-Guided Optimization) is the recommended path for further hotpath gains

---

## ≡ƒôï Issues Resolved in This Release

| Issue | Title | Type |
|-------|-------|------|
| #1274 | Add migration flow for legacy scattered game data | fix |
| #1281 | Triage 21 stale TODOv2 comments from Mesen2 era | refactor |
| #1282 | SteamOS.md is entirely stale Mesen2 content | docs |
| #1283 | SteamOS guide rewrite | docs |
| #1285 | WS timing gate scaffold | test |
| #1287 | Expand WS timing scaffold to multi-event timeline | test |
| #1290 | Pause when in menu and config dialogs problem | fix |
| #1297 | Triage and reduce third-party warning debt | fix |
| #1298 | COMPILING.md does not mention Pansy dependency | fix |
| #1299 | Pansy decoupling plans | docs |
| #1300 | Pause suppression during ROM load | fix |
| #1302 | Enforce package-only Pansy.Core consumption | fix |
| #1303 | Enforce Node 24 for GitHub Actions | ci |

---

## Γ£à Validation Summary

| Check | Result |
|-------|--------|
| Release x64 build | Γ£à Success (zero warnings) |
| C++ tests (Google Test) | Γ£à 3,690 passed, 0 failed |
| .NET tests (xUnit v3) | Γ£à 1,444 passed, 0 failed |
| **Total** | **Γ£à 5,134 tests passed** |

---

## ≡ƒôÑ Downloads

### Windows

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-Windows-x64-v1.4.33.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Windows-x64-v1.4.33.exe) | Single-file, recommended |
| **Native AOT** | [Nexen-Windows-x64-AoT-v1.4.33.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Windows-x64-AoT-v1.4.33.exe) | Faster startup, larger binary |

### Linux

| Build | Download | Notes |
|-------|----------|-------|
| **AppImage x64** | [Nexen-Linux-x64-v1.4.33.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-x64-v1.4.33.AppImage) | Recommended for most users |
| **AppImage ARM64** | [Nexen-Linux-ARM64-v1.4.33.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-ARM64-v1.4.33.AppImage) | Raspberry Pi 4/5, etc. |
| Binary x64 (clang) | [Nexen-Linux-x64-v1.4.33.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-x64-v1.4.33.tar.gz) | Requires SDL2 |
| Binary x64 (gcc) | [Nexen-Linux-x64-gcc-v1.4.33.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-x64-gcc-v1.4.33.tar.gz) | Requires SDL2 |
| Binary ARM64 (clang) | [Nexen-Linux-ARM64-v1.4.33.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-ARM64-v1.4.33.tar.gz) | Requires SDL2 |
| Binary ARM64 (gcc) | [Nexen-Linux-ARM64-gcc-v1.4.33.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-ARM64-gcc-v1.4.33.tar.gz) | Requires SDL2 |
| Native AOT x64 | [Nexen-Linux-x64-AoT-v1.4.33.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-x64-AoT-v1.4.33.tar.gz) | Faster startup |

### macOS (Apple Silicon)

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-macOS-ARM64-v1.4.33.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-macOS-ARM64-v1.4.33.zip) | App bundle |
| ~~Native AOT~~ | *Temporarily unavailable* | .NET 10 ILC compiler bug |

---

**Full Changelog:** https://github.com/TheAnsarya/Nexen/compare/v1.4.32...v1.4.33
