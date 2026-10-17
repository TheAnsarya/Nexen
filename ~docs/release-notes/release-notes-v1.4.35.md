# Nexen v1.4.35 — SNES Performance Blitz & TAS Editor Optimization

> **22 commits** | **13+ issues addressed** | **4,313 tests passing** (3,690 C++ + 623 .NET) | **Zero warnings**

Nexen v1.4.35 is a deep performance optimization release targeting the SNES emulation core and TAS editor. Four phases of SNES PPU optimizations deliver faster rendering through pointer hoisting, early exits, and attribute hints. The TAS editor receives major allocation reduction, frame ViewModel reuse, and a contiguous memory buffer for movie input data. A critical CDL recorder race condition fix rounds out the stability improvements.

---

## Highlights

| Feature | Details |
|---------|---------|
| **SNES PPU Phase 1-4** | Four phases of PPU rendering optimization: memory dispatch inlining, window mask precompute skip, color math pointer hoist, HiRes pointer arithmetic |
| **TAS Editor Allocation Reduction** | Lazy string creation, dirty-check updates, button object reuse, stackalloc for hot paths |
| **Frame ViewModel Reuse** | SyncFrameViewModels eliminates per-update ViewModel churn in the piano roll |
| **Contiguous Movie Buffer** | Movie input data stored in contiguous memory buffer instead of vector\<string\> |
| **CDL Race Fix** | Use-after-free race condition in CDL recorder Stop()/InitDebugger() eliminated |
| **Benchmark Suite** | New 60K/300K frame benchmarks for Movie/TAS performance regression tracking |

---

## Performance

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

## Fixed

### Movie/TAS Pass 3 - #1312

- InputFrame buffer management improvements
- FDS roundtrip correctness fix
- HasInput accuracy fix
- Undo helper robustness
- NexenMovie parameter handling

### CDL Recorder Use-After-Free Race - #1356

- Fixed race condition in CDL recorder `Stop()` and `InitDebugger()` that could cause use-after-free crashes

---

## Issues Addressed

| Issue | Description | Type |
|-------|-------------|------|
| #1381 | SNES PPU Phase 4 optimizations | perf |
| #1376 | SNES PPU Phase 3 optimizations | perf |
| #1366 | SNES Phase 2 optimizations | perf |
| #1361 | SNES memory dispatch + CPU inlining | perf |
| #1360 | TAS frame ViewModel reuse | perf |
| #1359 | TAS ViewModel caching | perf |
| #1358 | Movie input contiguous buffer | perf |
| #1340 | TAS editor allocation reduction + batch updates | perf |
| #1312 | Movie/TAS Pass 3 fixes | fix |
| #1308 | Movie/TAS benchmark suite | perf |
| #1356 | CDL recorder use-after-free race | fix |

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
| **Standard** | [Nexen-Windows-x64-v1.4.35.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.35/Nexen-Windows-x64-v1.4.35.exe) | Single-file, recommended |
| **Native AOT** | [Nexen-Windows-x64-AoT-v1.4.35.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.35/Nexen-Windows-x64-AoT-v1.4.35.exe) | Faster startup, larger binary |

### Linux

| Build | Download | Notes |
|-------|----------|-------|
| **AppImage x64** | [Nexen-Linux-x64-v1.4.35.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.35/Nexen-Linux-x64-v1.4.35.AppImage) | Recommended for most users |
| **AppImage ARM64** | [Nexen-Linux-ARM64-v1.4.35.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.35/Nexen-Linux-ARM64-v1.4.35.AppImage) | Raspberry Pi 4/5, etc. |
| Binary x64 (clang) | [Nexen-Linux-x64-v1.4.35.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.35/Nexen-Linux-x64-v1.4.35.tar.gz) | Requires SDL2 |
| Binary x64 (gcc) | [Nexen-Linux-x64-gcc-v1.4.35.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.35/Nexen-Linux-x64-gcc-v1.4.35.tar.gz) | Requires SDL2 |
| Binary ARM64 (clang) | [Nexen-Linux-ARM64-v1.4.35.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.35/Nexen-Linux-ARM64-v1.4.35.tar.gz) | Requires SDL2 |
| Binary ARM64 (gcc) | [Nexen-Linux-ARM64-gcc-v1.4.35.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.35/Nexen-Linux-ARM64-gcc-v1.4.35.tar.gz) | Requires SDL2 |
| Native AOT x64 | [Nexen-Linux-x64-AoT-v1.4.35.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.35/Nexen-Linux-x64-AoT-v1.4.35.tar.gz) | Faster startup |

### macOS (Apple Silicon)

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-macOS-ARM64-v1.4.35.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.35/Nexen-macOS-ARM64-v1.4.35.zip) | App bundle |

---

**Full Changelog:** [v1.4.34...v1.4.35](https://github.com/TheAnsarya/Nexen/compare/v1.4.34...v1.4.35)
