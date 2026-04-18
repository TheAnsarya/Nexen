# Nexen v1.4.1

A major feature and stability release with a **complete TAS Editor**, **movie format system**, **80+ performance optimizations**, **300+ new tests**, and **CI fixes across all platforms**.

## TAS Editor (New)

The TAS Editor is now fully functional with a rich feature set:

- **Piano Roll View** -- Visual timeline for frame-by-frame editing with batch paint and multi-selection
- **Full Undo/Redo System** -- Every operation (toggle, paint, insert, delete, clear, fork) is undoable with O(1) incremental updates
- **Greenzone System** -- Instant seeking with automatic savestates and visual greenzone indicator
- **Branches** -- Fork, compare, and load alternate TAS strategies
- **Input Recording** -- Capture, insert, and overwrite modes with branch support
- **Lua Scripting** -- Automate TAS workflows via `TasLuaApi` with full undo integration
- **UI Dialogs** -- GoToFrame, Seek, Comment, Greenzone settings, Branch manager
- **Multi-Selection & Context Menu** -- Bulk editing with right-click actions
- **Search** -- Find specific inputs in the piano roll
- **Metadata Display** -- View movie metadata alongside the editor

## Movie System (New)

Full movie recording, playback, and format conversion:

- **Nexen Movie Format (.nexen-movie)** -- ZIP-based format with JSON metadata, input log, savestate, and SRAM
- **Movie Recording & Playback** -- Record/replay inputs frame-by-frame with rerecording support
- **Multi-Format Import/Export:**
  - **BK2** (BizHawk) -- Read/Write
  - **FM2** (FCEUX) -- Read/Write
  - **LSMV** (lsnes) -- Read/Write
  - **SMV** (Snes9x) -- Read
  - **VBM** (VisualBoyAdvance) -- Read
  - **GMV** (Gens) -- Read
- **Bug Fixes:** LSMV stream leak, BK2 command-field parse bug, HistoryViewer time >59min

## Performance (80+ Optimizations)

Extensive hot-path optimizations across the C++ and .NET codebases:

### C++ Core

- `std::format` for controller text state, no-alloc parsing, `reserve()` pre-allocation
- `std::erase`, `ranges::contains`, `C++20 contains()` modernization
- Zero-copy `stringstream::view()`, `ends_with`, `RewindData::str()` optimization
- Eliminate redundant recursive locks in `SetTextState`/`GetTextState`/`InvertBit`
- `SaveStateManager` buffer reuse, `CheatManager` const-correctness
- `ControllerHub` const ref, `GetControlDevices` const ref, `nodiscard` audit
- Pre-compute `HexEditor` format string, `std::endl` removal
- `NotificationManager` send snapshot and registration lock overhead reduction

### .NET / TAS

- `InputFrame` span-based parsing with `StringBuilder` and `OrdinalIgnoreCase`
- `FrozenDictionary`/`FrozenSet` for static readonly collections
- `TryGetValue` pattern replacing `ContainsKey` + indexer double-lookups
- `ZipReader` direct extraction, `PianoRoll` allocation removal
- `MovieData` CRC32 batched buffer, `InputFrame.Clone` skip-init
- O(1) `StateLoaded`, property notification batching, LINQ elimination
- Incremental `ObservableCollection` updates for Paste and Fork
- Zero-alloc PianoRoll cache eviction in render path

## Testing (+300 New Tests)

Test count grew from ~2400 to **2790** (1633 C++ + 826 .NET + 331 MovieConverter):

- 75 TasLuaApi tests + fix lazy property init + fix branch creation bug
- 89 format roundtrip tests for InputFrame parsing
- 79 ViewModel integration tests for TasEditorViewModel
- 61 API doc generation + guard tests
- 58 ViewModel branch, controller layout, and frame navigation tests
- 30 StringUtilities and ControlDeviceState tests
- 29 C++ SetTextState/GetTextState round-trip tests
- 24 C++ TAS/movie tests (ControllerHub, RewindData, movie format)
- 23 greenzone, clipboard, clone, and frame operation tests
- 14 TAS recording mode and undo/redo state machine tests

## CI / Build Fixes

All CI builds were previously broken. Three separate issues were diagnosed and fixed:

1. **Pansy.Core project reference** -- Added `git clone` of the Pansy repo to all CI jobs (the UI project references `Pansy.Core` via a sibling directory path)
2. **`std::jthread` to `std::thread`** -- `libc++` (used by clang on Linux) doesn't support `std::jthread`; replaced with `std::thread` (the `stop_token` parameter was unused)
3. **HdNesPack template destructors** -- Added explicit destructor template instantiations for `HdNesPack<1>` through `HdNesPack<10>` (clang linker couldn't find them)

**All 10 CI jobs now pass:** Windows (x64, AoT), Linux gcc (x64, ARM64), Linux clang (x64, ARM64, AoT), AppImage (x64, ARM64), macOS ARM64.

## Documentation

- Comprehensive README overhaul with accurate download links, expanded documentation tables
- New Movie System section documenting all supported formats
- New Emulation Core Documentation section (NES, SNES, GB/GBA, SMS/PCE/WS, Lynx)
- Expanded Developer Documentation (14 docs covering all subsystems)
- TAS algorithm reference documentation with undo system design

## Supported Systems

NES, SNES, Game Boy / GBC, Game Boy Advance, Sega Master System / Game Gear, PC Engine / TurboGrafx-16, WonderSwan / WSC, Atari Lynx

---

## Downloads

| Platform | File | Size |
|----------|------|------|
| **Windows x64** | [Nexen-Windows-x64-v1.4.1.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.1/Nexen-Windows-x64-v1.4.1.exe) | 31 MB |
| **Windows x64 (AOT)** | [Nexen-Windows-x64-AoT-v1.4.1.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.1/Nexen-Windows-x64-AoT-v1.4.1.exe) | 70 MB |
| **Linux x64 AppImage** | [Nexen-Linux-x64-v1.4.1.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.1/Nexen-Linux-x64-v1.4.1.AppImage) | 78 MB |
| **Linux ARM64 AppImage** | [Nexen-Linux-ARM64-v1.4.1.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.1/Nexen-Linux-ARM64-v1.4.1.AppImage) | 74 MB |
| Linux x64 (clang) | [Nexen-Linux-x64-v1.4.1.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.1/Nexen-Linux-x64-v1.4.1.tar.gz) | 20 MB |
| Linux x64 (gcc) | [Nexen-Linux-x64-gcc-v1.4.1.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.1/Nexen-Linux-x64-gcc-v1.4.1.tar.gz) | 18 MB |
| Linux ARM64 (clang) | [Nexen-Linux-ARM64-v1.4.1.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.1/Nexen-Linux-ARM64-v1.4.1.tar.gz) | 20 MB |
| Linux ARM64 (gcc) | [Nexen-Linux-ARM64-gcc-v1.4.1.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.1/Nexen-Linux-ARM64-gcc-v1.4.1.tar.gz) | 18 MB |
| Linux x64 (AOT) | [Nexen-Linux-x64-AoT-v1.4.1.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.1/Nexen-Linux-x64-AoT-v1.4.1.tar.gz) | 36 MB |
| **macOS ARM64** | [Nexen-macOS-ARM64-v1.4.1.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.1/Nexen-macOS-ARM64-v1.4.1.zip) | 61 MB |

> **Note:** Linux tarball builds require SDL2 (`sudo apt install libsdl2-2.0-0`). macOS: Right-click then Open on first launch.

**Full Changelog:** https://github.com/TheAnsarya/Nexen/compare/v1.4.0...v1.4.1
