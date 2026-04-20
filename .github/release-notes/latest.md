# Nexen v1.4.36 -- SNES Performance Blitz & TAS Editor Optimization

> **22 commits** | **13+ issues addressed** | **4,313 tests passing** (3,690 C++ + 623 .NET) | **Zero warnings**

Nexen v1.4.36 is a deep performance optimization release targeting the SNES emulation core and TAS editor. Four phases of SNES PPU optimizations deliver faster rendering through pointer hoisting, early exits, and attribute hints. The TAS editor receives major allocation reduction, frame ViewModel reuse, and a contiguous memory buffer for movie input data. A critical CDL recorder race condition fix rounds out the stability improvements.

---

## Highlights

| Feature | Details |
|---------|---------|
| :video_game: **SNES PPU Phase 1-4** | Four phases of PPU rendering optimization: memory dispatch inlining, window mask precompute skip, color math pointer hoist, HiRes pointer arithmetic |
| :zap: **TAS Editor Allocation Reduction** | Lazy string creation, dirty-check updates, button object reuse, stackalloc for hot paths |
| :recycle: **Frame ViewModel Reuse** | SyncFrameViewModels eliminates per-update ViewModel churn in the piano roll |
| :floppy_disk: **Contiguous Movie Buffer** | Movie input data stored in contiguous memory buffer instead of `vector<string>` |
| :bug: **CDL Race Fix** | Use-after-free race condition in CDL recorder Stop()/InitDebugger() eliminated |
| :bar_chart: **Benchmark Suite** | New 60K/300K frame benchmarks for Movie/TAS performance regression tracking |

---

## :rocket: Performance

### SNES PPU Phase 4 - #1381

- `ApplyColorMath` pointer hoist + `[[likely]]` attribute on fast path
- `PrecomputeWindowMasks` switch hoist to avoid redundant dispatch
- `ConvertToHiRes` pointer arithmetic optimization

### SNES PPU Phase 3 - #1376

- `PrecomputeWindowMasks` skip memset when masks unchanged
- `RenderBgColor` computation hoist out of inner loop
- Sprite priority early-exit for empty priority slots

### SNES Phase 2 - #1366

- Multiple hot-path SNES performance improvements across rendering pipeline

### SNES Memory Dispatch + CPU Inlining - #1361

- Memory access dispatch optimization for faster read/write operations
- CPU method inlining for critical execution paths

### TAS Editor Batch Updates - #1340

- `RangeObservableCollection.AddRange` for batch MarkerEntries insertion
- ViewModel allocation reduction: lazy strings, dirty-check, button reuse, stackalloc
- Remove unnecessary `ToList()` in CellsPainted
- Fix duplicate SyncFrameViewModels calls

### TAS Frame ViewModel Reuse - #1360

- Reuse `TasFrameViewModel` objects in `UpdateFrames` via `SyncFrameViewModels`
- Eliminates per-update allocation churn in the piano roll

### TAS ViewModel Caching - #1359

- Cache `P1Input`/`P2Input`/`Background` properties in `TasFrameViewModel`
- Avoid redundant property recalculation on every access

### Movie Input Contiguous Buffer - #1358

- Replace `vector<string>` with contiguous buffer for movie input data
- Improved cache locality and reduced allocation pressure

### Benchmark Suite - #1308

- New 60K and 300K frame benchmarks for Movie/TAS operations
- Enables regression tracking for future performance work

---

## :wrench: Fixed

### Movie/TAS Pass 3 - #1312

- InputFrame buffer management improvements
- FDS roundtrip correctness fix
- HasInput accuracy fix
- Undo helper robustness
- NexenMovie parameter handling

### CDL Recorder Use-After-Free Race - #1356

- Fixed race condition in CDL recorder `Stop()` and `InitDebugger()` that could cause use-after-free crashes

---

## :clipboard: Issues Addressed

| Issue | Description | Type |
|-------|-------------|------|
| #1381 | SNES PPU Phase 4 optimizations | perf |
# Nexen v1.4.36 -- Pansy Jump Graph, OSD Improvements & SNES Performance

> **11 commits** | **8 issues addressed** | **4,313 tests passing** (3,690 C++ + 623 .NET) | **Zero warnings**

Nexen v1.4.36 ships the Pansy Jump Graph export suite, bringing true one-source-many-target cross-reference serialization and conditional branch fallthrough edges to the `.pansy` metadata format. The UI gains Open ROM and Verified Games shortcut buttons on the game selection screen, and the OSD now scales its font with the HUD buffer and shows notifications for auto-save and recent-play saves. Two targeted performance improvements cover the TAS editor marker refresh and SNES Mode7 rendering.

---

## Highlights

| Feature | Details |
|---------|---------|
| :globe_with_meridians: **Multi-Target Pansy XRefs** | One source address serializes 2+ targets; backward compatible with existing Pansy readers |
| :twisted_rightwards_arrows: **Branch Fallthrough XRefs** | Conditional branch fallthrough edges added to the Pansy jump graph |
| :bug: **Pansy CDM Flags Fix** | JumpTarget/SubEntry/Opcode/Drawn/Read/Indirect bits no longer collide with platform-specific flags |
| :card_index_dividers: **Game Selection Buttons** | Open ROM and Verified Games action buttons added to the game selection screen |
| :capital_abcd: **OSD Font Scaling** | Bitmap OSD font now scales with the HUD buffer size |
| :bell: **OSD Save Notifications** | Notifications shown on OSD for auto-save and recent-play saves |
| :zap: **SNES Mode7 LargeMap** | Mode7 LargeMap templating + window mask hoisting + `[[likely]]` on Exec hot path |
| :zap: **RefreshMarkerEntries** | Cache-and-filter optimization skips unchanged TAS editor marker entries |

---

## :globe_with_meridians: Pansy Jump Graph Export

### Multi-Target XRef Export - #1399

- True one-source-many-target Pansy export contract implemented end-to-end
- A single source address can now serialize 2 or more outgoing targets without heuristic guessing
- Both the legacy `CrossReference` section and the grouped `MultiTargetCrossReference` section are written, maintaining full backward compatibility
- New tests pass in Nexen, Pansy, and Peony

### Conditional Branch Fallthrough XRefs - #1396

- Explicit source-to-target jump graph entries written for indirect and multi-target control flows
- Conditional branch fallthrough edges included alongside taken-branch edges
- Deduplication ensures deterministic output

### Pansy CDM Flag Fix - #1395

- `CODE_DATA_MAP` flag bits (`JumpTarget`, `SubEntry`, `Opcode`, `Drawn`, `Read`, `Indirect`) are now correctly preserved
- Platform-specific bits (SNES/GBA mode state) stored in `SECTION_CPU_STATE` only and no longer collide with standard CDM bits
- Roundtrip mapping tests verify all standard flags survive a full write-read cycle

---

## :card_index_dividers: UI Improvements

### Game Selection Screen Action Buttons - #1390

- **Open ROM** button added directly to the game selection screen for quick access
- **Verified Games** button added for browsing the curated verified game list
- Both buttons are positioned for easy access without navigating menus

### OSD Font Scaling - #1388

- The bitmap OSD font now scales proportionally with the HUD buffer size
- Ensures readability across different output resolutions and window sizes

### OSD Save Notifications - #1389

- Auto-save events now display an OSD notification message
- Recent-play save events also display an OSD notification
- Notifications use the scaled OSD font

---

## :rocket: Performance

### RefreshMarkerEntries Cache-and-Filter - #1392

- Cache the current marker set and skip entries that have not changed
- Reduces redundant UI update work in the TAS editor piano roll on every marker refresh

### SNES Mode7 LargeMap Templating - #1391

- Template the Mode7 LargeMap rendering path to eliminate branching in the inner loop
- Window mask computation hoisted out of per-pixel work
- `[[likely]]` attribute added to the `Exec` hot path for better branch prediction hints

---

## :clipboard: Issues Addressed

| Issue | Description | Type |
|-------|-------------|------|
| #1399 | Multi-target Pansy XRef export contract | feat |
| #1396 | Conditional branch fallthrough XRefs | feat |
| #1395 | Pansy CDM flag mapping fix | fix |
| #1392 | RefreshMarkerEntries cache-and-filter | perf |
| #1391 | SNES Mode7 LargeMap + window mask hoist | perf |
| #1390 | Game selection screen action buttons | feat |
| #1388 | OSD font scaling | feat |
| #1389 | OSD save notifications | feat |

---

## :white_check_mark: Validation Summary

| Check | Result |
|-------|--------|
| Release x64 build | :white_check_mark: Success (zero warnings) |
| C++ tests (Google Test) | :white_check_mark: 3,690 passed, 0 failed |
| .NET tests (xUnit v3) | :white_check_mark: 623 passed, 0 failed |
| **Total** | **4,313 tests passed** |

---

## :inbox_tray: Downloads

### Windows

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-Windows-x64-v1.4.36.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.36/Nexen-Windows-x64-v1.4.36.exe) | Single-file, recommended |
| **Native AOT** | [Nexen-Windows-x64-AoT-v1.4.36.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.36/Nexen-Windows-x64-AoT-v1.4.36.exe) | Faster startup |

### Linux

| Build | Download | Notes |
|-------|----------|-------|
| **AppImage x64** | [Nexen-Linux-x64-v1.4.36.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.36/Nexen-Linux-x64-v1.4.36.AppImage) | Recommended |
| **AppImage ARM64** | [Nexen-Linux-ARM64-v1.4.36.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.36/Nexen-Linux-ARM64-v1.4.36.AppImage) | Raspberry Pi 4/5 |
| Binary x64 (clang) | [Nexen-Linux-x64-v1.4.36.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.36/Nexen-Linux-x64-v1.4.36.tar.gz) | Requires SDL2 |
| Binary x64 (gcc) | [Nexen-Linux-x64-gcc-v1.4.36.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.36/Nexen-Linux-x64-gcc-v1.4.36.tar.gz) | Requires SDL2 |
| Binary ARM64 (clang) | [Nexen-Linux-ARM64-v1.4.36.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.36/Nexen-Linux-ARM64-v1.4.36.tar.gz) | Requires SDL2 |
| Binary ARM64 (gcc) | [Nexen-Linux-ARM64-gcc-v1.4.36.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.36/Nexen-Linux-ARM64-gcc-v1.4.36.tar.gz) | Requires SDL2 |
| Native AOT x64 | [Nexen-Linux-x64-AoT-v1.4.36.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.36/Nexen-Linux-x64-AoT-v1.4.36.tar.gz) | Faster startup |

### macOS (Apple Silicon)

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-macOS-ARM64-v1.4.36.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.36/Nexen-macOS-ARM64-v1.4.36.zip) | App bundle |

---

**Full Changelog:** https://github.com/TheAnsarya/Nexen/compare/v1.4.35...v1.4.36
