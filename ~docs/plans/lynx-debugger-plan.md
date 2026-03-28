# Lynx Debugger Completeness — Plan

> **Status: ✅ COMPLETE** — All items resolved across sessions 01–04.

## Overview

Ensure the Atari Lynx debugger has all system-specific features fully implemented, tested,
and benchmarked. Verify VRAM/sprite/palette viewer UI and core integration are correct.

## Audit Summary

### Core C++ (42 files in Core/Lynx/)

**Debugger subsystem (16 files):** ALL IDebugger methods implemented.

- `LynxDebugger.h/.cpp` — Stepping, breakpoints, callstack, CDL
- `LynxPpuTools.h/.cpp` (516 lines) — Palette, sprite, tilemap (framebuffer) viewers
- `LynxEventManager.h/.cpp` — 11 event categories
- `LynxTraceLogger.h/.cpp` — 65C02 trace logging
- `LynxDisUtils.h/.cpp` — Full 256-opcode disassembly table
- `LynxAssembler.h/.cpp` — 65C02 assembler
- `DummyLynxCpu.h/.cpp` — Predictive breakpoints

### .NET UI (4 dedicated + 20+ shared dispatch points)

- `LynxStatusView.axaml/.axaml.cs` — CPU status panel
- `LynxStatusViewModel.cs` — Reads LynxCpuState + LynxPpuState
- `LynxRegisterViewer.cs` (217 lines) — 5 tabs: CPU, Mikey, Suzy, Math, APU
- `LynxEventViewerConfigView.axaml/.axaml.cs` — Event viewer config sidebar (session-03)

All shared viewmodel dispatch points have Lynx cases:
DebuggerWindowViewModel, RegisterViewerWindowViewModel, EventViewerViewModel,
TileViewerViewModel, TilemapViewerViewModel, MemoryMappingViewModel,
TraceLoggerViewModel, SpritePreviewModel, RefreshTimingViewModel,
DebuggerOptionsViewModel, DefaultLabelHelper, etc.

### InteropDLL

Generic DebugApi model — uses CpuType/ConsoleType dispatch. System-specific export:
`SetLynxConfig(LynxConfig)`. No gap.

## Tile Viewer — ✅ RESOLVED (Session-04)

**Fixed in commit b1b05615 — #948**

The Lynx tile viewer previously used `TileFormat::Bpp4` (SNES planar 4bpp) which was wrong.
The Lynx framebuffer uses 4bpp packed nibbles (high-nibble-first), which exactly matches
`TileFormat::WsBpp4Packed`.

`GetTileView()` is non-virtual in the `PpuTools` base class — platforms don't override it,
they only declare supported `TileFormat` values. The base class handles all decoding via
`GetTilePixelColor<format>()` templates.

**Changes:**

- `TileViewerViewModel.cs`: Changed `TileFormat.Bpp4` → `TileFormat.WsBpp4Packed`
- `TileViewerViewModel.cs`: Added VRAM preset alongside ROM preset
- `LynxPpuTools.h`: Removed TODO, added WsBpp4Packed explanation comment

**Note:** Lynx sprites in cart ROM use variable-length bitstream encoding (literal/packed
mode) — the standard tile viewer can't decode those. VRAM viewing works correctly.

## Tests & Benchmarks — ✅ ALL EXIST

### C++ Tests

- ✅ `Core.Tests/Lynx/LynxPpuToolsTests.cpp` — ~60+ tests (palette, tilemap, sprite)
- ✅ `Core.Tests/Lynx/LynxDisUtilsTests.cpp` — ~60+ tests (opcode decoding, classification)
- ✅ 1021 total Lynx C++ tests across 38 suites — all passing

### C++ Benchmarks

- ✅ `Core.Benchmarks/Lynx/LynxPpuToolsBench.cpp` — 9 benchmarks
- ✅ `Core.Benchmarks/Lynx/LynxDisUtilsBench.cpp` — 6 benchmarks

### .NET Tests

- ✅ `Tests/Debugger/ViewModels/LynxRegisterViewerTests.cs` — 25 tests
- ✅ `Tests/Debugger/ViewModels/LynxEventViewerConfigTests.cs` — 8 tests (session-03)
- ✅ `Tests/Debugger/ViewModels/LynxTileViewerTests.cs` — 6 tests (session-04)

## Performance Improvements — ✅ ANALYZED

### Applied (Session-03, commit 2ce1e14c — #958)

- **Cart bitmask address wrapping** — Replaced modulo with bitmask in LynxCart
- **Suzy pixel write** — Removed redundant bounds check in WriteSpritePixel

### Analyzed and Deferred (Session-04)

The following were analyzed but not applied — all are debug-only tools with negligible
impact on runtime performance:

1. **`[[likely]]`/`[[unlikely]]` on DecodeSpriteLine** — Debug tool, not hot path
2. **Framebuffer memcpy optimization** — Already tight (palette lookup per pixel)
3. **Sprite decode buffer (128×512)** — Large but not perf-critical (debug only)

## Acceptance Criteria — ✅ ALL MET

1. ✅ All C++ tests pass (1021 Lynx tests, 38 suites)
2. ✅ All .NET tests pass (1314+ including Lynx-specific)
3. ✅ Benchmarks compile and run (15 Lynx benchmarks)
4. ✅ Tile viewer fixed (WsBpp4Packed + VRAM preset)
5. ✅ Event viewer config UI created
6. ✅ Performance improvements applied where justified
7. ✅ Zero new warnings in build output
