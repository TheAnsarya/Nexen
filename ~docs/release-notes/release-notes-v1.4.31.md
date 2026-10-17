# Nexen v1.4.31 — WonderSwan Timing, Avalonia Fixes & Benchmark Infrastructure

> **15 commits** | **5,134 tests passing** (3,690 C++ + 1,444 .NET) | **Zero warnings**

Nexen v1.4.31 focuses on WonderSwan PPU timing accuracy, Avalonia UI stability fixes, benchmark infrastructure for evidence-based performance work, and codebase hygiene improvements.

---

## Highlights

| Feature | Details |
|---------|---------|
| **WonderSwan timing fixes** | Corrected WS VTOTAL frame timing, low-VTOTAL frame finalization, and WSC PPU short-frame handling aligned with ares behavior (#1076) |
| **Avalonia binding fix** | Fixed enum-to-int tab SelectedIndex binding and NavigateTo robustness (#1278) |
| **InitializeComponent cleanup** | Removed manual InitializeComponent shadowing that conflicted with generated code (#1223, #1279) |
| **Benchmark infrastructure** | Added SNES PPU, Genesis VDP, Debugger, GBA DMA, and save state compression benchmarks |
| **EditorConfig charset** | Set charset to utf-8-bom for consistency across the project (#1283) |

---

## Fixed

- **WonderSwan VTOTAL frame timing aligned with ares** — corrected WS VTOTAL frame finalization behavior to match ares reference implementation (#1076)
- **WonderSwan low-VTOTAL frame finalize** — fixed low-VTOTAL frame finalization and added parity tests (#1076)
- **WSC PPU VTOTAL modulo revert** — reverted WSC PPU VTOTAL modulo change and fixed short-frame handling (#1076)
- **Avalonia enum-to-int tab SelectedIndex binding** — used ReflectionBinding for enum-to-int tab SelectedIndex and hardened NavigateTo (#1278)
- **InitializeComponent shadowing** — removed manual InitializeComponent that was shadowing XAML-generated code (#1223, #1279)
- **Stale NuGet Package Restore TODO** — removed stale TODO from .gitignore (#1280)
- **CHANGELOG date errors** — fixed 3 incorrect dates in CHANGELOG and untracked build_output.txt (#1280)

## Changed

- **EditorConfig charset** — changed charset to utf-8-bom for consistency (#1283)
- **Warning stabilization** — stabilized remaining warnings for clean v1.4.31 release (#1284)

## Performance

- **New benchmark suite** — added SNES PPU, Genesis VDP, Debugger, GBA DMA, and save state compression benchmarks for evidence-based performance triage

---

## Issues Referenced

| Issue | Title | Type |
|-------|-------|------|
| #1076 | WonderSwan/WSC timing and PPU accuracy | fix |
| #1223 | InitializeComponent shadowing | fix |
| #1278 | Enum-to-int tab binding | fix |
| #1279 | InitializeComponent generated code conflict | fix |
| #1280 | Stale .gitignore TODO and CHANGELOG dates | fix |
| #1283 | EditorConfig charset consistency | fix |
| #1284 | Warning stabilization for release | chore |

---

## Validation Summary

| Check | Result |
|-------|--------|
| Release x64 build | Success (zero warnings) |
| C++ tests (Google Test) | 3,690 passed, 0 failed |
| .NET tests (xUnit v3) | 1,444 passed, 0 failed |
| **Total** | **5,134 tests passed** |

---

## Downloads

### Windows

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-Windows-x64-v1.4.31.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.31/Nexen-Windows-x64-v1.4.31.exe) | Single-file, recommended |
| **Native AOT** | [Nexen-Windows-x64-AoT-v1.4.31.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.31/Nexen-Windows-x64-AoT-v1.4.31.exe) | Faster startup, larger binary |

### Linux

| Build | Download | Notes |
|-------|----------|-------|
| **AppImage x64** | [Nexen-Linux-x64-v1.4.31.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.31/Nexen-Linux-x64-v1.4.31.AppImage) | Recommended for most users |
| **AppImage ARM64** | [Nexen-Linux-ARM64-v1.4.31.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.31/Nexen-Linux-ARM64-v1.4.31.AppImage) | Raspberry Pi 4/5, etc. |
| Binary x64 (clang) | [Nexen-Linux-x64-v1.4.31.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.31/Nexen-Linux-x64-v1.4.31.tar.gz) | Requires SDL2 |
| Binary x64 (gcc) | [Nexen-Linux-x64-gcc-v1.4.31.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.31/Nexen-Linux-x64-gcc-v1.4.31.tar.gz) | Requires SDL2 |
| Binary ARM64 (clang) | [Nexen-Linux-ARM64-v1.4.31.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.31/Nexen-Linux-ARM64-v1.4.31.tar.gz) | Requires SDL2 |
| Binary ARM64 (gcc) | [Nexen-Linux-ARM64-gcc-v1.4.31.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.31/Nexen-Linux-ARM64-gcc-v1.4.31.tar.gz) | Requires SDL2 |
| Native AOT x64 | [Nexen-Linux-x64-AoT-v1.4.31.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.31/Nexen-Linux-x64-AoT-v1.4.31.tar.gz) | Faster startup |

### macOS (Apple Silicon)

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-macOS-ARM64-v1.4.31.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.31/Nexen-macOS-ARM64-v1.4.31.zip) | App bundle |
| ~~Native AOT~~ | *Temporarily unavailable* | .NET 10 ILC compiler bug |

---

**Full Changelog:** https://github.com/TheAnsarya/Nexen/compare/v1.4.30...v1.4.31
