Nexen v1.4.33 is a comprehensive quality-and-stability release featuring per-game data migration improvements, CI modernization, WonderSwan timing validation hardening, codebase cleanup, and performance benchmarking infrastructure. This release also closes out the v1.4.32 performance triage and GameDataManager migration epic.

## Highlights

- **Per-game data migration prompt** — ROM load now prompts before migrating save states and battery saves to per-game folders, preventing unexpected file moves.
- **CI modernization** — GitHub Actions workflows now force Node 24 runtime, eliminating all deprecated Node.js runtime warnings.
- **WonderSwan timing validation** — Hardened timing gate tests with multi-event ordering checks for improved WS/WSC emulation confidence.
- **TODOv2 marker triage** — All stale `TODOv2` markers triaged into issue-scoped `TODO(#issue)` references for better tracking.
- **Performance benchmark infrastructure** — Hotpath benchmark artifacts stabilized with repeatable scripts and evidence-backed triage.
- **SteamOS documentation rewrite** — Complete rewrite of the SteamOS/Steam Deck guide with Nexen-specific instructions.
- **Pause behavior hardening** — Menu/config auto-pause suppressed during ROM load transitions for seamless game startup.

## Fixed

- **Per-game migration now requires user confirmation** (#1274)
- ROM load checks for pending save state and battery save migrations.
- Displays a confirmation dialog before moving files to per-game folders.
- Prevents unexpected file reorganization on first load after update.

- **Menu/config pause suppressed during ROM load** (#1290, #1300)
- Auto-pause in menu/config dialogs no longer fires during ROM load transitions.
- Fixes intermittent pause-on-startup when "Pause when in menu" was enabled.

- **Pansy.Core package-only enforcement** (#1298, #1302)
- CI now validates Pansy.Core is consumed as a NuGet package, not a project reference.
- UI resource prep step added to package-only validation lane.

- **UTF-8 stream compatibility and test warning noise** (#1297)
- Restored UTF-8 stream compatibility for cross-platform builds.
- Reduced test warning noise from StringUtilities size_t assertions.

- **GitHub comment formatting** (#1297)
- Fixed escaped newline sequences appearing as literal `\n` in published issue comments.

## Changed

- **GitHub Actions Node 24 runtime** (#1303)
- All CI workflows now set `FORCE_JAVASCRIPT_ACTIONS_TO_NODE24: true`.
- Eliminates all deprecated Node.js 16/20 runtime warnings.

- **TODOv2 to issue-scoped TODO triage** (#1281)
- All 20 files with stale `TODOv2` markers converted to `TODO(#issue)` format.
- Each marker now references its tracking issue for traceability.

- **SteamOS guide rewrite** (#1282, #1283)
- Complete rewrite of `SteamOS.md` with Nexen-specific installation and configuration.
- Linked from main README documentation section.

- **First-party warning reduction** (#1297)
- Batch reduction of first-party compiler warnings.
- CI windows warning delta gated on first-party signal only.

- **Pansy decoupling plans** (#1298, #1299)
- Added decoupling plans for separating Pansy.Core dependency.

## Added

- **WonderSwan timing gate hardening** (#1285, #1287, #1076)
- Multi-event ordering assertions in WS timing gate tests.
- Improved validation of frame-level event sequencing.

- **Hotpath benchmark stabilization** (#1218)
- Repeatable `run-memory-hotpath-benchmarks.ps1` script with labeled, timestamped JSON output.
- GBA ProcessInternalCycle distribution coverage benchmarks.
- Evidence-backed performance triage documentation.

- **Per-game migration count API** (#1274)
- `GetPendingMigrationCount()` for pre-migration enumeration.
- Localized confirmation dialog `ConfirmGameDataMigration`.

## Performance

- **Hotpath branch-prediction triage** (#1216)
- Real emulator-path microbenchmarks for GBA and NES memory hotpaths.
- `[[likely]]`/`[[unlikely]]` annotations tested and discarded based on measured evidence.
- Documented approach for future PGO/profiling-guided optimization.

## Validation Summary

- Release x64 build: success (zero warnings)
- C++ tests: 3690 passed, 0 failed
- Managed tests: 1444 passed, 0 failed
- Total: 5134 tests passed

## Downloads

### Windows

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-Windows-x64-v1.4.33.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Windows-x64-v1.4.33.exe) | Single-file, recommended |
| **Native AOT** | [Nexen-Windows-x64-AoT-v1.4.33.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Windows-x64-AoT-v1.4.33.exe) | Faster startup |

### Linux

| Build | Download | Notes |
|-------|----------|-------|
| **AppImage x64** | [Nexen-Linux-x64-v1.4.33.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-x64-v1.4.33.AppImage) | Recommended |
| **AppImage ARM64** | [Nexen-Linux-ARM64-v1.4.33.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-ARM64-v1.4.33.AppImage) | Raspberry Pi, etc. |
| Binary x64 (clang) | [Nexen-Linux-x64-v1.4.33.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-x64-v1.4.33.tar.gz) | Tarball, requires SDL2 |
| Binary x64 (gcc) | [Nexen-Linux-x64-gcc-v1.4.33.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-x64-gcc-v1.4.33.tar.gz) | Tarball, requires SDL2 |
| Binary ARM64 (clang) | [Nexen-Linux-ARM64-v1.4.33.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-ARM64-v1.4.33.tar.gz) | Tarball, requires SDL2 |
| Binary ARM64 (gcc) | [Nexen-Linux-ARM64-gcc-v1.4.33.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-ARM64-gcc-v1.4.33.tar.gz) | Tarball, requires SDL2 |
| Native AOT x64 | [Nexen-Linux-x64-AoT-v1.4.33.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-x64-AoT-v1.4.33.tar.gz) | Faster startup |

### macOS (Apple Silicon)

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-macOS-ARM64-v1.4.33.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-macOS-ARM64-v1.4.33.zip) | App bundle |
| ~~Native AOT~~ | *Temporarily unavailable* | .NET 10 ILC compiler bug |