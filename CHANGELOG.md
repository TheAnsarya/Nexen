# Changelog

All notable changes to Nexen are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.9] - 2026-03-29

### Added

- **WonderSwan issue tracking** — Created 11 WonderSwan/WSC issues for accuracy, cart features, test coverage (#1086-#1096)
- **Channel F PansyImporter support** — Added Channel F (0x1f) and missing platform mappings to PansyImporter (#1008)

### Fixed

- **First-party compiler warnings eliminated** — Removed unused variables (ChannelFEventManager, EmuApiWrapper), replaced unsafe `strncpy` with `strncpy_s` in LynxConsole, added `(void)` casts for `[[nodiscard]]` returns in Emulator and MovieRecorder
- **Missing final newlines** — Added final newlines to CamstudioCodec.cpp and RawCodec.h

---

## [1.4.5] - 2026-07-25

### Added

- **GSU HiROM memory mapping** — SuperFX coprocessor now supports HiROM cartridge layouts (upstream Mesen2 PR #89) (#1061)
	- HiROM banks $40-$6f, $72-$7d mapped with 64KB page increments, mirrors at $c0-$ef, $f2-$ff
	- RAM override at $70-$71 for HiROM carts
	- GSU-side linear 64KB-per-bank addressing preserved
- **Channel F TAS editor support** — Full controller layout with 8 buttons (→, ←, Bk, Fw, ↺, ↻, Pl, Ps) (#1063)
- **Channel F movie converter** — Added `SystemType.ChannelF` for movie format conversion (#1063)

### Fixed

- **GSU accuracy improvements** — Four fixes from upstream Mesen2 PR #90 (#1062):
	- STOP instruction now flushes pixel cache before halting
	- R14 ROM stall guard prevents hangs when program bank > $5f
	- RomDelay/RamDelay underflow prevention (explicit conditional instead of `std::min`)
	- GO register ($301f) properly resets wait flags, pending delays, and clamps CycleCount
- **Channel F tab localization** — Replaced hardcoded "Channel F" text with localized resource key (#1063)

---

## [1.4.4] - 2026-03-27

### Fixed

- **Linux SIGSEGV crash on startup** — Bundle ICU 72.1 via `Microsoft.ICU.ICU4C.Runtime` NuGet package instead of relying on system ICU (#1039)
	- Fixes `locale_get_default_76()` crash in `libicuuc.so.76` on KUbuntu 25.10 and other Linux distros with incompatible system ICU
	- App-local ICU configured via `runtimeconfig.template.json` with `System.Globalization.AppLocalIcu`
	- Full internationalization/multi-language support preserved (not using InvariantGlobalization)

## [1.4.0] - 2026-07-24

### Added

- **Pansy.Core Library Integration** — Full integration with shared Pansy.Core NuGet library (#586)
	- PansyExporter uses `Pansy.Core` enum types (`SymbolType`, `CommentType`, `CrossRefType`, `MemoryRegionType`)
	- `LabelMergeEngine` integrated for intelligent label deduplication and conflict resolution
	- PansyImporter rewritten to use `PansyLoader` for spec-compliant file reading
- **Pansy CPU State Section** — Export per-address CPU mode flags (#578, #579, #580)
	- SNES: IndexMode8/MemoryMode8 from CDL flags per code byte
	- GBA: Thumb mode from CDL flags per code byte
	- 9-byte entry format: Address(4) + Flags(1) + DataBank(1) + DirectPage(2) + CpuMode(1)
	- 21 correctness tests and benchmarks (SpanWrite: 2.1x faster, 50% less memory)
- **Pansy Data Types Section** — Export structured data annotations from labels and CDL (#582, #584)
- **Pansy Hot Reload** — Automatic re-import of Pansy files when changed on disk (#582)
- **Atari Lynx File Format** — `.atari-lynx` format reader/writer with auto-detection (#571-#577)
	- Full format specification, 50 unit tests
	- Cart address shift protocol and HLE boot support
	- ROM file format conversion libraries (.lnx/.lyx/.o/.atari-lynx)
- **TAS Infrastructure Tests** — 57 undo/redo and clipboard operation tests (#563)
- **Save State Manager Benchmarks** — GreenzoneManager capture/lookup/invalidation benchmarks (#564)

### Changed

- **PansyExporter cleanup** — Removed 128 lines of dead code (4 superseded methods), added `FLAG_HAS_CROSS_REFS` (#587)
- **Pansy spec alignment** — Removed count prefixes and extra bytes from section builders (#47)

### Performance

- **NES memory read fast-path** — Avoids virtual dispatch (1.84x faster) (#556)
- **Async save state writes** — Background thread for save state compression (#557-#560)
- **SnesPpuState field reordering** — Cache locality optimization (#525)
- **Rewind interval scaling** — Scale recording interval with emulation speed (#423)
- **Branch prediction hints** — `[[likely]]`/`[[unlikely]]` on hottest unannotated paths (#553)
- **Constexpr expansion** — Pure computation functions marked constexpr (#554)
- **`[[nodiscard]]` attributes** — Applied to critical query methods (#555)

### Fixed

- **Lynx sprite viewer** — Fixed packed mode and tilemap flip mode (#504, #505)
- **Lynx Timer 7 cascade** — Implemented Timer 7 → Audio Channel 0 cascade link (#496)
- **Lynx SYSCTL1 bank strobe** — Implemented bank strobe and cart write path (#499)

---

## [1.3.1] - 2026-03-04

### Fixed

- **Game Boy crash on ROM load** — Fixed null pointer dereference in `DebuggerRequest` copy constructor (#552)
	- Root cause: `std::make_unique` materializes a temporary and calls the copy constructor, unlike `reset(new T(...))` which benefits from C++17 mandatory copy elision
	- Copy constructor accessed `_emu->_debugger` without null-checking `_emu`
	- Affected all ROM loads (NES, SNES, GB, GBA, etc.) when debugger block count > 0
- **Cross-platform CI build failures** — Fixed `strncpy_s` and incomplete type with `unique_ptr` (#550)
	- `strncpy_s` → `strncpy` for Linux/macOS compatibility
	- Added explicit destructors for classes using `unique_ptr` with forward-declared types

### Changed

- **Pansy export/import spec alignment** — Full binary compatibility with Pansy v1.0 spec (#539–#546)
	- Fixed header format, section IDs, platform IDs, cross-ref types, memory region types
	- Fixed compression to use DEFLATE instead of GZip
	- Added DRAWN/READ/INDIRECT CDL flags and fixed SNES CDL collision
	- Added symbol/comment types and metadata section support
	- Fixed importer to match exporter format
	- Optimized cross-ref builder performance
- **Defensive startup** — Added try-catch in MainWindow `ApplyConfig` to prevent startup crashes
- **Internal version bump**: 2.2.0 → 2.2.1

### Performance

- **SNES PPU window mask precomputation** — Precompute window masks once per scanline render call instead of per-pixel (#523)
	- Added `PrecomputeWindowMasks()` called from `RenderScanline()` after `_drawEndX` is set
	- Replaces per-pixel `ProcessMaskWindow` template switch with `bool[6][256]` array lookups
	- Eliminates redundant window count calculation at each call site (sprites, tilemap, mode7, color math)

---

## [1.3.0] - 2026-07-23

### Performance

#### Phase 16.6 — Temp String & Table Elimination

- Eliminated temporary string allocations in TraceLogger, DebugStats, AudioPlayerHud
- Converted runtime-initialized arrays to `constexpr` lookup tables
- Added `reserve()` calls for known-size containers

#### Phase 16.7 — Branch Prediction Hints

- Applied `[[likely]]`/`[[unlikely]]` attributes across 9 CPU/PPU/memory hot-path files
- NES/SNES/GB/GBA/SMS/PCE/WS CPU cores, PPU renderers, memory handlers

#### Phase 16.8 — Constexpr LUT + Copy Elimination

- Base64 decode table: `constexpr` namespace-scope LUT (8.8x faster CheckSignature)
- NTSC filter, FDS audio, VS System: `static constexpr` member arrays
- VirtualFile, FolderUtilities: pass-by-const-ref, `string_view` params (66.3x faster extension set lookup)

#### Phase 16.9 — String Allocation Elimination

- Serializer::UpdatePrefix: in-place append instead of repeated concatenation
- FolderUtilities::CombinePath: `reserve()` + `append()` (eliminates intermediate allocations)
- PcmReader::LoadSamples: pre-reserve sample vector
- DebugStats: `std::format` replaces `stringstream` (3.8x faster)
- **Finding**: MSVC `std::format` is 3.4x slower than `operator+` for short strings — only use to replace `stringstream`

#### Phase 16.10 — Stringstream Elimination

- MD5/SHA1: hex digest via `std::format` (replaces `stringstream` + `setfill`/`setw`)
- RomLoader, NsfLoader: CRC/address formatting via `std::format`
- SaveStateManager: timestamp formatting (×3 sites)
- MessageManager, Debugger, ScriptingContext: `GetLog()` join via `std::format`
- AudioPlayerHud: `reserve()` for output string

### Changed

- Internal version bump: 2.1.1 → 2.2.0
- Missing trailing newlines added to header files

## [1.2.0] - 2026-03-01

### Added

- **Atari Lynx Emulation** — Complete new emulation core (not a Handy fork)
	- 65C02 CPU core with full instruction set and hardware bug emulation
	- Mikey: timer system, interrupts, display DMA, palette, audio
	- Suzy: sprite engine with all BPP modes, quadrant rendering, stretch/tilt, hardware math, collision
	- 4-channel LFSR audio with stereo panning and timer-driven sample generation
	- UART/ComLynx serial port with full register set
	- EEPROM support (93C46/66/86) with auto-detection from LNX header
	- RSA bootloader decryption
	- Boot ROM support with HLE fallback
	- Cart banking with LNX format support
	- MAPCTL memory overlays (ROM, vectors, Mikey, Suzy)
	- Debugger: disassembler, trace logger, event manager, effective address, sprite viewer
	- UI: config panel, input mapping, piano roll layout, movie converter
	- Pansy metadata export for Lynx platform
	- ROM database with ~80 No-Intro entries
	- 67 performance benchmarks, 84+ unit tests, TAS integration tests

- **Upstream Mesen2 Fixes** — 8 bug fixes from open Mesen2 PRs
	- PR #87: SNES DMA uint8_t overflow (could miscalculate cycle overhead)
	- PR #82: NES GetInternalOpenBus returning wrong bus value
	- PR #86: SNES CX4 cache/timing (bus delay, preload, IRQ flag, cycle compare)
	- PR #80: SNES hi-res blend pixel pairing (was blurring entire screen)
	- PR #31: NES NTSC filter dot crawl phase, Bisqwit RGB-YIQ matrix, RGB PPU emphasis
	- PR #74: SNES ExLoRom mapping + S-DD1 map mode fix
	- PR #76: Lua crash fix for non-string error objects
	- PR #85: Linux FontConfig typeface caching for Memory View performance

- **Game Package Export** — Export ROM + metadata as `.nexen-pack.zip`
- **Memory-based SaveState API** — Greenzone uses in-memory states (no temp files)
- **TAS Improvements**
	- Joypad interrupt detection for movie playback
	- InsertFrames/DeleteFrames Lua API
	- SortedSet greenzone lookups, bulk operations
	- O(1) single-frame UpdateFrames optimization
	- FormattedText caching in piano roll
	- SeekToFrameAsync batching (replaced Task.Delay with Task.Yield)
- **Movie Converter Fixes** — BK2/FM2/SMV/LSMV import correctness

### Performance

192 commits of optimization work across 13 phases:

- **Phase 7-9: Core Pipeline**
	- FastBinary positional serializer for run-ahead (eliminates string key overhead)
	- Rewind system: audio ring buffer, O(1) memory tracking, compression direct-write
	- Audio pipeline: ReverbFilter ring buffer, SDL atomics
	- Profiler: cached pointers (6-9x), callstack ring buffer (2x)
	- Lightweight CDL, VideoDecoder frame skip, SoundMixer turbo audio
	- DebugHud: flat buffer pixel tracking (replaces unordered_map)
	- Pansy auto-save off UI thread, ROM CRC32 caching

- **Phase 10-13: Hot Path Audit**
	- Move semantics for RenderedFrame/VideoDecoder
	- HUD string copy elimination, DrawStringCommand move
	- ExpressionEvaluator const ref (9x string copy elimination)
	- ClearFrameEvents swap, GBA pendingIrqs swap-and-pop O(1)
	- GetPortStates buffer reuse (per-frame alloc elimination)
	- TraceLogger persistent buffers, ScriptingContext const ref
	- NesSoundMixer timestamps reserve, selective zeroing
	- SystemHud snprintf, DebugStats stringstream reuse
	- ShortcutKeyHandler move pressedKeys
	- MovieRecorder/NexenMovie const ref, StepBack emplace_back

- **Lynx-Specific Performance**
	- NZ flag lookup table, LFSR branchless popcount
	- Timer mask optimization, video filter branch elimination
	- Branchless flags, branch hints, unsigned bounds checks
	- Cached GetCycleCount, removed redundant null checks

### Fixed

- **43 compiler warnings** across C++ and C# codebase
- **Lynx emulation** — 100+ bug fixes from comprehensive audit:
	- IRQ firing, timer prescaler units, MAPCTL bit assignments
	- Sprite engine: collision buffer, quadrant rendering, pixel format
	- Math engine: register addresses, signed multiply
	- Audio: tick rate, volume, timer cascade, TimerDone blocking
	- Cart: banking, RCART registers, page parameter
	- UART: 3 bug fixes for serial communication
	- DISPCTL flip mode, PBKUP register, 2-byte NOP opcodes

### Testing

- **1495 tests** across 100 test suites (up from 659 in v1.1.0)
	- 65C02 CPU exhaustive instruction logic tests
	- Lynx: hardware reference correctness tests (63), math unit (16), sprite (50+), fuzz (17)
	- TAS: integration tests, movie converter tests, truncation benchmarks
	- Upstream fix verification tests

---

## [1.1.4] - 2026-02-13

### Fixed

- **CI/CD**: Fixed Windows artifact upload with explicit output directory
	- Added `-o build/publish` to dotnet publish command
	- SolutionDir issue caused output to go outside repo root on CI runners

### Changed

- **CI/CD**: Release asset filenames now include version numbers
	- e.g., `Nexen-Windows-x64-v1.1.4.exe` instead of `Nexen-Windows-x64.exe`
	- Makes it clear which version a downloaded file is
- **CI/CD**: Linux binary releases now distributed as `.tar.gz` tarballs
	- Includes LICENSE file in each tarball
	- AppImages remain as standalone `.AppImage` files
- **README**: Updated download links to use versioned filenames
- **Copilot Instructions**: Added release process documentation with mandatory README update steps

---

## [1.1.3] - 2026-02-13

### Fixed

- **CI/CD**: Fixed Windows artifact upload paths (unsuccessful)
	- Corrected path from `build/TmpReleaseBuild/Nexen.exe` to actual publish output
	- Windows x64 and Windows AOT builds now upload artifacts correctly

---

## [1.1.2] - 2026-02-13

### Fixed

- **CI/CD**: Disabled macOS Native AOT builds temporarily
	- .NET 10 ILC compiler crashes with segfault on macOS ARM64
	- All other builds (Windows, Linux, macOS Standard) now publish correctly
	- See [issue #238](https://github.com/TheAnsarya/Nexen/issues/238) for details

### Changed

- **README**: Updated download links and macOS AOT availability notice

---

## [1.1.1] - 2026-02-12

### Fixed

- **CI/CD**: Fixed GitHub Actions build workflow for all platforms
  - Windows: Fixed native library packaging from NuGet cache
  - macOS: Made code signing optional when certificate not configured
  - macOS: Fixed libc++ linking via NEXENFLAGS environment variable
  - Linux AppImage: Fixed `--self-contained true` syntax (was `--no-self-contained false`)

---

## [1.1.0] - 2026-02-12

### Performance

Major performance optimization pass across all CPU emulation cores and UI rendering.

#### C++ Emulator Core

- **Branchless CPU Flag Operations** - Replaced branching conditionals with branchless equivalents
  - NES: CMP, ADD, BIT, ASL/LSR, ROL/ROR (7 operations)
  - SNES: CMP, Add8/16, Sub8/16, shifts, TestBits (10 operations)
  - GB: SetFlagState branchless implementation
  - PCE: CMP, ASL/LSR, ROL/ROR, BIT, TSB/TRB/TST (10 operations)
  - SMS: Merged ClearFlag in UpdateLogicalOpFlags (3 calls → 1 AND mask)

- **[[likely]]/[[unlikely]] Annotations** - Branch prediction hints on hot-path conditionals
  - All CPU Exec() IRQ/NMI/halt/stopped checks annotated

- **PPU Rendering Optimizations**
  - SNES ApplyBrightness: constexpr 512-byte LUT (33% faster)
  - SNES ConvertToHiRes: cached pixel read (38% faster)
  - SNES RenderBgColor: hoisted _cgram[0] out of per-pixel loop
  - SNES DefaultVideoFilter: row pointer hoisting in all 3 loops
  - GB/GBA video filter: flat loop (7% faster)
  - GB PPU: cached GameboyConfig (160x reduction in config access)
  - SMS VDP DrawPixel: cached buffer offset

- **Utility Library**
  - HexUtilities: constexpr nibble LUT for FromHex, eliminated 256-entry vector<string>
  - StringUtilities: string_view for Split, const ref for Trim/CopyToBuffer
  - Equalizer: vector→std::array+span (10.1x faster)

#### C# UI Allocation Reduction

Eliminated thousands of per-frame GC allocations in debugger UI:

- BreakpointBar: 6 brush/pen → ColorHelper cached lookups
- DisassemblyViewer: static ForbidDashStyle + BlackPen1 (−150 allocs/frame)
- CodeLineData ByteCodeStr: string.Create with hex LUT (−400 allocs/refresh)
- PaletteSelector: ColorHelper brush caching (−256 allocs/frame)
- PianoRollControl: 17 per-frame brush/pen/typeface → static readonly
- HexEditor: pre-allocated string[256] hex lookup table
- HexEditorDataProvider: static string[128] printable ASCII cache
- Utf8Utilities: Marshal.PtrToStringUTF8 + ArrayPool<byte>
- GetScriptLog: pooled buffer
- GetSaveStatePreview: ArrayPool
- PictureViewer: cached Pen/Brush/Typeface
- GreenzoneManager: pre-sized MemoryStream
- MemorySearch: direct loop

### Testing

- **659 tests** (up from 421 in v1.0.0)
  - Exhaustive 256×256 comparison tests for CPU flag operations
  - Branching vs branchless equivalence tests
  - Video filter correctness tests
  - Hex formatting tests

### Documentation

- Updated session logs documenting optimization work
- Phase 2-6 optimization plans in `~docs/plans/`

---

## [1.0.1] - 2026-02-07

### Fixed

- **Critical: Release build startup failure** - Fixed missing NexenCore.dll in Dependencies.zip
  - The embedded Dependencies.zip was not including NexenCore.dll, causing "Dll was not found" error on startup
  - Added [scripts/package-dependencies.ps1](scripts/package-dependencies.ps1) helper script for reliable release builds
  - Disabled problematic PreBuildWindows MSBuild target that was failing with dotnet CLI

### Build System

- Improved release build workflow documentation
- Added package-dependencies.ps1 script for Windows release packaging

---

## [1.0.0] - 2026-02-02

Initial public release of Nexen.

### Added

#### TAS Editor

- **Piano Roll View** - Visual timeline for frame-by-frame input editing
- **Greenzone System** - Automatic savestates for instant seeking and rerecording
- **Input Recording** - Record gameplay with Append, Insert, and Overwrite modes
- **Branches** - Save and compare alternate movie versions
- **Lua Scripting** - Automate TAS workflows with comprehensive API
- **Multi-Format Support** - Import/export BK2, FM2, SMV, LSMV, VBM, GMV, NMV
- **Rerecord Tracking** - Monitor optimization progress

#### Save States

- **Infinite Save States** - Unlimited timestamped saves per game
- **Visual Picker** - Grid view with screenshots and timestamps
- **Per-Game Organization** - Saves stored in `SaveStates/{RomName}/` directories
- **Quick Save/Load** - Shift+F1 to save, F1 to browse

#### Debugging

- **🌼 Pansy Export** - Export disassembly metadata to universal format
- **File Sync** - Bidirectional sync with external label files

#### Technical

- **C++23 Modernization** - Smart pointers, std::format, ranges, [[likely]]/[[unlikely]]
- **Unit Tests** - 421+ tests using Google Test framework
- **Benchmarks** - Performance benchmarks with Google Benchmark
- **Doxygen Documentation** - Generated API documentation

### Platforms

- Windows (x64)
- Linux (x64, ARM64, AppImage)
- macOS (Intel x64, Apple Silicon ARM64)

---

## Version History

Nexen is based on [Mesen](https://github.com/SourMesen/Mesen2) by Sour.

For the original Mesen changelog, see the [upstream repository](https://github.com/SourMesen/Mesen2/releases).
