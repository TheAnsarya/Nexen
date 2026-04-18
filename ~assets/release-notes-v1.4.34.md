# Nexen v1.4.34 — TAS Editor Performance & Stability

> **25 commits** | **30+ issues resolved** | **4,313 tests passing** (3,690 C++ + 623 .NET) | **Zero warnings**

Nexen v1.4.34 is a major TAS editor and movie system release. It features comprehensive piano roll performance optimization, a full undo/redo system for recording and comments, selection system refactoring, game loading stability fixes, WonderSwan PPU accuracy improvements, and hardware-accelerated CRC32 for movie files.

---

## Highlights

| Feature | Details |
|---------|---------|
| **Piano roll virtualization** | Viewport culling, debounced prefetch, and visible-range-only updates reduce per-frame overhead dramatically |
| **Recording undo system** | Full undo/redo for TAS recording, comments, markers, and branch operations |
| **Selection system refactor** | `SortedSet`-based selection eliminates `Distinct()`/`OrderBy()` overhead (Epic 14) |
| **Game loading stability** | Cross-console load transitions serialized to prevent freeze/paused-start race conditions |
| **WonderSwan VBlank fix** | PPU VBlank and sprite copy now fire unconditionally at scanline 144 (#1076) |
| **Hardware CRC32** | Movie file checksums use hardware-accelerated CRC32 intrinsics |
| **Batch collection updates** | `RangeObservableCollection` for piano roll frames reduces notification spam |

---

## Fixed

### Game loading crash and selection screen — #1353, #1354

Fixed a crash triggered by specific game loading sequences and improved the selection screen behavior for smoother game startup.

### Cross-console load freeze/paused-start — #1344, #1345, #1346, #1347

Loading a ROM for a different console while another was running could cause a freeze or leave the emulator in a paused state. Load transitions are now serialized with a scoped gating mechanism that prevents race conditions between console teardown and initialization.

### Pansy background unload lifecycle — #1350

Instrumented and serialized the background Pansy metadata unload lifecycle to prevent use-after-free during console switches.

### Cross-console load transitions — #1349

Serialized cross-console load transitions with added diagnostics to detect ordering violations in the load pipeline.

### WonderSwan PPU VBlank fix — #1076

WS PPU VBlank interrupt and sprite copy now fire unconditionally at scanline 144, matching hardware behavior. Previously, certain conditions could suppress these events, causing rendering glitches in games like Final Fantasy II.

### Greenzone data race, GBA L/R decode, rerecord init — #1310

Fixed a greenzone data race in multi-threaded TAS playback, corrected GBA L/R button decode mapping, and initialized rerecord count properly on new movie creation. Also applied C++ `string_view` modernization.

### InputFrame truncation, FDS roundtrip, undo hysteresis — #1313

Fixed InputFrame truncation when controllers have fewer buttons than expected, FDS movie roundtrip fidelity, and undo stack hysteresis for smoother undo behavior. Improved `ControllerInput` completeness checks.

### OOM guard for Lua input combinations

Capped `GenerateInputCombinations` to 16 buttons to prevent out-of-memory from Lua scripts requesting combinatorial explosion on controllers with many inputs.

### Accuracy test build split — #1304

Split the accuracy test build into InteropDLL + Dependencies.zip + UI stages for faster CI feedback.

---

## Changed

### Selection system refactor (Epic 14) — #1334, #1335, #1336, #1337

Replaced the selection system's `List<int>` + `Distinct()` + `OrderBy()` pattern with a `SortedSet<int>` that maintains sorted uniqueness invariants automatically. This eliminates O(n log n) sorting on every selection operation and simplifies the selection API.

### Remove misleading AggressiveInlining

Removed `[MethodImpl(MethodImplOptions.AggressiveInlining)]` from large methods where it was counterproductive, and fixed a tautological ternary in the GMV converter.

---

## Added

### Recording undo system — #1323, #1325, #1326, #1327, #1328, #1329, #1330, #1331

Full undo/redo system for TAS recording operations including input edits, frame insertion/deletion, and branch operations. Includes thread safety guards, rerecord undo support, and proper state restoration on undo.

### Comment/marker undo + incremental refresh — #1314

Undo system for comment and marker operations with incremental `RefreshMarkerEntryAt()` updates instead of full-list rebuilds. Includes benchmarks and tests for the marker system.

### Hardware-accelerated CRC32 — #1309

Movie file CRC32 computation now uses hardware intrinsics (`System.Runtime.Intrinsics`) when available, with software fallback. Includes Movie/TAS tests and benchmarks.

### RangeObservableCollection for batch updates

New `RangeObservableCollection<T>` with `AddRange`/`RemoveRange` methods that fire a single `CollectionChanged` notification instead of per-item notifications. Used by the piano roll frame list. Ships with 25 unit tests.

### GitHub Discussions link

Added GitHub Discussions link to README for community engagement.

---

## Performance

### Piano roll visible-range culling — #1342

Unified the piano roll's visible range culling logic so that only frames within the viewport are updated during scroll/paint operations, dramatically reducing work for large movies.

### Piano roll prefetch debounce — #1341

Debounced piano roll prefetch requests and reduced paint-drag overhead to prevent redundant data fetches during rapid scrolling.

### Piano roll button lookup optimization — #1348

Optimized the hot path for controller button lookups in the piano roll, reducing per-cell rendering overhead.

### Multi-delete coalescing

Coalesced multi-delete operations into contiguous ranges — O(r*n) instead of O(k*n) — for significantly faster bulk frame deletion.

---

## Issues Resolved in This Release

| Issue | Title | Type |
|-------|-------|------|
| #1076 | WS PPU VBlank and sprite copy fix | fix |
| #1304 | Split accuracy test build | fix |
| #1305 | Update DEVELOPER-TASKS.md | docs |
| #1309 | Hardware-accelerated CRC32 + tests | perf |
| #1310 | Greenzone data race, GBA L/R, rerecord | fix |
| #1313 | InputFrame truncation, FDS roundtrip, undo | fix |
| #1314 | Comment/marker undo + incremental refresh | feat |
| #1323 | Recording undo system | feat |
| #1325-#1331 | Recording undo sub-issues | feat |
| #1334-#1337 | Selection system refactor (Epic 14) | refactor |
| #1339 | Continue in-flight work | chore |
| #1341 | Piano roll prefetch debounce | perf |
| #1342 | Piano roll visible-range culling | perf |
| #1344-#1347 | Cross-console load gating | fix |
| #1348 | Piano roll button lookup optimization | perf |
| #1349 | Cross-console load serialization | fix |
| #1350 | Pansy unload lifecycle | fix |
| #1353 | Game loading crash fix | fix |
| #1354 | Selection screen improvements | fix |

---

## Validation Summary

| Check | Result |
|-------|--------|
| Release x64 build | Success (zero warnings) |
| C++ tests (Google Test) | 3,690 passed, 0 failed |
| .NET tests (xUnit v3) | 623 passed, 0 failed |
| **Total** | **4,313 tests passed** |

---

## Downloads

### Windows

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-Windows-x64-v1.4.34.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.34/Nexen-Windows-x64-v1.4.34.exe) | Single-file, recommended |
| **Native AOT** | [Nexen-Windows-x64-AoT-v1.4.34.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.34/Nexen-Windows-x64-AoT-v1.4.34.exe) | Faster startup, larger binary |

### Linux

| Build | Download | Notes |
|-------|----------|-------|
| **AppImage x64** | [Nexen-Linux-x64-v1.4.34.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.34/Nexen-Linux-x64-v1.4.34.AppImage) | Recommended for most users |
| **AppImage ARM64** | [Nexen-Linux-ARM64-v1.4.34.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.34/Nexen-Linux-ARM64-v1.4.34.AppImage) | Raspberry Pi 4/5, etc. |
| Binary x64 (clang) | [Nexen-Linux-x64-v1.4.34.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.34/Nexen-Linux-x64-v1.4.34.tar.gz) | Requires SDL2 |
| Binary x64 (gcc) | [Nexen-Linux-x64-gcc-v1.4.34.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.34/Nexen-Linux-x64-gcc-v1.4.34.tar.gz) | Requires SDL2 |
| Binary ARM64 (clang) | [Nexen-Linux-ARM64-v1.4.34.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.34/Nexen-Linux-ARM64-v1.4.34.tar.gz) | Requires SDL2 |
| Binary ARM64 (gcc) | [Nexen-Linux-ARM64-gcc-v1.4.34.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.34/Nexen-Linux-ARM64-gcc-v1.4.34.tar.gz) | Requires SDL2 |
| Native AOT x64 | [Nexen-Linux-x64-AoT-v1.4.34.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.34/Nexen-Linux-x64-AoT-v1.4.34.tar.gz) | Faster startup |

### macOS (Apple Silicon)

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-macOS-ARM64-v1.4.34.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.34/Nexen-macOS-ARM64-v1.4.34.zip) | App bundle |

---

**Full Changelog:** https://github.com/TheAnsarya/Nexen/compare/v1.4.33...v1.4.34
