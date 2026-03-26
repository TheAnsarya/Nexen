# Lynx Debugger Completeness — Plan

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

All shared viewmodel dispatch points have Lynx cases:
DebuggerWindowViewModel, RegisterViewerWindowViewModel, EventViewerViewModel,
TileViewerViewModel, TilemapViewerViewModel, MemoryMappingViewModel,
TraceLoggerViewModel, SpritePreviewModel, RefreshTimingViewModel,
DebuggerOptionsViewModel, DefaultLabelHelper, etc.

### InteropDLL

Generic DebugApi model — uses CpuType/ConsoleType dispatch. System-specific export:
`SetLynxConfig(LynxConfig)`. No gap.

## 1 Known TODO

**File:** `LynxPpuTools.h` line 21
**Task:** "Tile viewer: Decode sprite pixel data from cart ROM as tiles"

Currently the tile viewer only reports `{ TileFormat::Bpp4 }` as supported format.
The Lynx has no tile hardware — sprites use variable-width packed pixel data from
cart ROM. A meaningful tile viewer would decode raw cart ROM data as 4bpp tiles
for inspection.

### Implementation Plan

In `LynxPpuTools`, add `GetTileView()` override that:

1. Reads cart ROM data from the specified source offset
2. Decodes consecutive bytes as 4bpp packed tiles (8×8 each, 32 bytes per tile)
3. Renders into the tile view output buffer using the current palette
4. Supports configurable tile width (1–4 bpp) matching the `TileFormat::Bpp4` etc. formats

## Gaps Found

### Test Gaps

**Missing C++ test file:** `LynxPpuToolsTests.cpp`

- Palette viewer: GetPaletteInfo RGB444 packing, color count, palette editing
- Tilemap viewer: Framebuffer rendering, flip mode, pixel-level tile info
- Sprite viewer: SCB chain walk, sprite decode, visibility, reload flags
- Sprite preview compositing: Back-to-front ordering, flip rendering

**Missing C++ test file:** `LynxDisUtilsTests.cpp`

- Opcode size by addressing mode (all 256 opcodes)
- Instruction classification: unconditional/conditional jump, JSR, RTS/RTI
- CDL flag generation
- Effective address calculation for each addressing mode

**Missing .NET test file:** `LynxRegisterViewerTests.cs`

- 5 tabs returned (CPU, Mikey, Suzy, Math, APU)
- CPU tab: A, X, Y, SP, PC, PS flags, IRQ, NMI, StopState
- Mikey tab: IRQ, Display, SERCTL, UART, 8 timers
- Suzy tab: Sprite state, SPRSYS flags, Joystick, Switches
- Math tab: ABCD, EFGH, JKLM, NP, status flags
- APU tab: MSTEREO, MPAN, 4 channels

### Benchmark Gaps

**Missing:** `LynxPpuToolsBench.cpp`

- Framebuffer rendering throughput (normal + flip modes)
- Sprite chain walk + decode
- Sprite preview compositing
- Palette info extraction
- DecodeSpriteLine per-line throughput

**Missing:** `LynxDisUtilsBench.cpp`

- Opcode classification throughput (all 256 opcodes)
- Effective address calculation throughput

### Performance Opportunities

1. **`[[likely]]`/`[[unlikely]]` on hot paths** — DecodeSpriteLine bit-level loops,
   SCB chain walk termination checks
2. **Framebuffer direct rendering** — Current GetTilemap does safe byte-by-byte;
   could use `memcpy`-based batch for non-flip mode
3. **Sprite decode buffer** — `uint8_t linePixels[128][512]` on stack = 64KB;
   consider reducing max line count or using smaller buffer

## New Files

### C++ Tests

- `Core.Tests/Lynx/LynxPpuToolsTests.cpp` (~50 tests)
- `Core.Tests/Lynx/LynxDisUtilsTests.cpp` (~40 tests)

### C++ Benchmarks

- `Core.Benchmarks/Lynx/LynxPpuToolsBench.cpp` (~8 benchmarks)
- `Core.Benchmarks/Lynx/LynxDisUtilsBench.cpp` (~4 benchmarks)

### .NET Tests

- `Tests/Debugger/ViewModels/LynxRegisterViewerTests.cs` (~25 tests)

## Acceptance Criteria

1. All new C++ tests pass
2. All existing 878+ Lynx C++ tests still pass
3. All .NET tests pass
4. Benchmarks compile and run
5. Tile viewer TODO implemented (basic 4bpp decode)
6. Performance improvements applied where benchmarks justify
7. Zero new warnings in build output
