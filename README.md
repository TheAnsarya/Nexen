# Nexen

<p align="center">
  <img src="~assets/Nexen%20icon.png" alt="Nexen" width="128"/>
</p>

<p align="center">
  <strong>A powerful multi-system emulator for Windows, Linux, and macOS</strong><br/>
  <em>High-accuracy emulation &bull; TAS editing &bull; Advanced debugging &bull; Pansy metadata export</em>
</p>

<p align="center">
  <a href="https://github.com/TheAnsarya/Nexen/releases">
    <img src="https://img.shields.io/github/v/release/TheAnsarya/Nexen?include_prereleases" alt="Latest Release"/>
  </a>
  <a href="LICENSE">
    <img src="https://img.shields.io/badge/license-Unlicense%20%2B%20GPL--3.0-blue.svg" alt="License"/>
  </a>
</p>

<p align="center">
  💬 We've started using <a href="https://github.com/TheAnsarya/Nexen/discussions"><strong>GitHub Discussions</strong></a> — join us to discuss progress, share ideas, and suggest changes!
</p>

---

## Overview

Nexen is a multi-system emulator based on [Mesen2](https://github.com/SourMesen/Mesen2) with significant enhancements: a full-featured TAS editor with undo/redo and greenzone support, an infinite save state system with visual picker, a ZIP-based movie format with multi-format import/export, 🌼 Pansy metadata export for integration with the Flower Toolchain disassembly pipeline, and active development of Sega Genesis, Atari 2600, and Fairchild Channel F support.

---

## 📥 Downloads

Download pre-built binaries from the [Releases page](https://github.com/TheAnsarya/Nexen/releases/latest).

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
| ~~Native AOT~~ | _Temporarily unavailable_ | .NET 10 ILC compiler bug |

> **Notes:**
>
> - Linux requires SDL2 (`sudo apt install libsdl2-2.0-0`)
> - macOS: Right-click → Open on first launch to bypass Gatekeeper
> - macOS Intel (x64) builds are no longer provided
> - macOS Native AOT is disabled due to a [.NET 10 ILC compiler crash](https://github.com/TheAnsarya/Nexen/issues/238)

---

## 🎮 Supported Systems

| System | CPU | Status |
|--------|-----|--------|
| NES / Famicom | MOS 6502 | ✅ Full |
| SNES / Super Famicom | WDC 65816 | ✅ Full |
| Game Boy / Game Boy Color | Sharp LR35902 | ✅ Full |
| Game Boy Advance | ARM7TDMI | ✅ Full |
| Sega Master System / Game Gear | Zilog Z80 | ✅ Full |
| Sega Genesis / Mega Drive | Motorola 68000 | 🔄 In Progress |
| PC Engine / TurboGrafx-16 | HuC6280 | ✅ Full |
| WonderSwan / WonderSwan Color | NEC V30MZ | ✅ Full |
| Atari 2600 | MOS 6507 | 🔄 In Progress |
| Atari Lynx | WDC 65SC02 | ✅ Full |
| Fairchild Channel F | Fairchild F8 | 🔄 In Progress |

---

## ✨ Features

### Emulation

- **High-accuracy** cycle-accurate CPU and PPU emulation across all systems
- Full coprocessor support (Super FX, SA-1, DSP, Cx4, SDD-1, OBC-1, S-RTC, BS-X)
- Comprehensive mapper and memory controller support
- Netplay multiplayer with rollback

### Save States

| Feature | Description |
|---------|-------------|
| **Infinite Saves** | Unlimited timestamped saves per game |
| **Visual Picker** | Grid view with screenshots and timestamps (Shift+F1) |
| **Quick Save** | F1 to save, Shift+F1 to browse |
| **Designated Slots** | 3 dedicated slots (F2/F3/F4 load, Shift+F2/F3/F4 save) |
| **Recent Play** | 36 rolling checkpoints at 5-minute cadence (about 3 hours) |
| **Auto-Save Log** | Periodic timestamped progress entries (no overwrite) |
| **Per-Game** | Saves organized by ROM hash |

### Movie System

- **Nexen Movie Format (.nexen-movie)** — ZIP-based with JSON metadata, input log, savestate, and SRAM
- **Recording & Playback** — Frame-by-frame with rerecording support
- **Multi-Format Import/Export** — BK2 (BizHawk), FM2 (FCEUX), SMV (Snes9x), LSMV (lsnes), VBM (VBA), GMV (Gens)
- **Multi-System TAS** — Full TAS support for all emulated systems including Channel F

### TAS Editor

- **Piano Roll** — Visual timeline for frame-by-frame editing with batch paint
- **Full Undo/Redo** — Every operation (toggle, paint, insert, delete, clear, fork) is undoable
- **Greenzone** — Instant seeking with automatic savestates
- **Input Recording** — Capture, insert, and overwrite modes
- **Branches** — Fork, compare, and load alternate strategies
- **Lua Scripting** — Automate TAS workflows with full undo integration

### Debugging

| Tool | Description |
|------|-------------|
| **Disassembler** | Full CPU disassembly with labels and navigation |
| **Memory Viewer** | Hex editor with search and watch |
| **Breakpoints** | Execution, read, and write breakpoints |
| **Trace Logger** | CPU execution logging with filtering |
| **Code/Data Logger** | Track ROM coverage (CDL) |
| **Profiler** | Callstack-based CPU profiling |
| **🌼 Pansy Export** | Export symbols, CDL, cross-refs, and source maps to universal metadata format |

### Video & Audio

- Multiple shader and filter options (NTSC, PAL, HQ2x, etc.)
- Integer scaling and aspect ratio correction
- Per-channel volume control
- HUD overlays for debugging
- Audio and video recording

---

## Quick Start

### Windows

1. Download [Nexen-Windows-x64-v1.4.33.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Windows-x64-v1.4.33.exe)
2. Run the executable (no installation needed)
3. **File → Open** to load a ROM

### Linux

1. Download [Nexen-Linux-x64-v1.4.33.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-Linux-x64-v1.4.33.AppImage)
2. `chmod +x Nexen-Linux-x64-v1.4.33.AppImage`
3. `./Nexen-Linux-x64-v1.4.33.AppImage`

> For non-AppImage builds, install SDL2 first: `sudo apt install libsdl2-2.0-0`

### macOS

1. Download [Nexen-macOS-ARM64-v1.4.33.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.33/Nexen-macOS-ARM64-v1.4.33.zip)
2. Extract and move `Nexen.app` to Applications
3. Right-click → Open (first launch only, to bypass Gatekeeper)

---

## 🧭 User Guides

| Guide | Description |
|-------|-------------|
| [Save States Guide](docs/Save-States.md) | Quick save, designated slots, and visual picker workflows |
| [Rewind Guide](docs/Rewind.md) | Rewind usage and route iteration workflows |
| [Movie System Guide](docs/Movie-System.md) | Record, playback, import/export, and deterministic playback |
| [TAS Editor Manual](docs/TAS-Editor-Manual.md) | Complete frame-by-frame TAS editing workflow |
| [Debugging Guide](docs/Debugging.md) | Disassembler, memory viewer, breakpoints, trace, and CDL |
| [Video and Audio Guide](docs/Video-Audio.md) | Rendering and audio configuration |

---

## ⌨️ Keyboard Shortcuts

### General

| Shortcut | Action |
|----------|--------|
| F1 | Quick Save (Timestamped) |
| Shift+F1 | Browse Save States |
| F2/F3/F4 | Load Designated Slots 1-3 |
| Shift+F2/F3/F4 | Save Designated Slots 1-3 |
| Ctrl+S | Quick Save (Alt) |
| Ctrl+Shift+S | Save State to File |
| Ctrl+L | Load State from File |
| Escape | Pause/Resume |
| Tab | Fast Forward |
| Backspace | Rewind |
| F11 | Toggle Fullscreen |
| F12 | Screenshot |

### TAS Editor

| Shortcut | Action |
|----------|--------|
| Escape | Pause/Resume Playback |
| \` (backtick) | Frame Advance |
| Backspace | Frame Rewind |
| Insert | Insert Frame |
| Delete | Delete Frame |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Ctrl+G | Go to Frame |
| Ctrl+B | Create Branch |

---

## 🌷 Flower Toolchain Integration

Nexen is the **play & debug** stage of the Flower Toolchain — an integrated pipeline for playing, debugging, disassembling, editing, and rebuilding retro games:

```text
🎮 Play & Debug (Nexen) → 🌺 Disassemble (Peony) → ✏️ Edit (Pansy UI) → 🌸 Build (Poppy) → ✅ Verify (Game Garden)
```

| Stage | Tool | Purpose |
|-------|------|---------|
| 1. Play & Debug | **Nexen** | Run games, export CDL + symbols + source maps via 🌼 Pansy |
| 2. Disassemble | [🌺 Peony](https://github.com/TheAnsarya/peony) | ROM → annotated `.pasm` source code |
| 3. Edit & Document | [🌼 Pansy](https://github.com/TheAnsarya/pansy) UI | View and edit disassembly metadata |
| 4. Build | [🌸 Poppy](https://github.com/TheAnsarya/poppy) | `.pasm` source → ROM |
| 5. Verify | [🌱 Game Garden](https://github.com/TheAnsarya/game-garden) | Roundtrip byte-identical rebuild verification |

---

## 📖 Documentation

### User Documentation

| Document | Description |
|----------|-------------|
| [Documentation Index](docs/README.md) | Entry point for all user-facing docs |
| [Manual Testing Hub](docs/manual-testing/README.md) | Ranked manual release validation workflows |
| [Compiling](COMPILING.md) | Build from source |
| [SteamOS / Steam Deck](SteamOS.md) | Running Nexen on SteamOS and Game Mode |
| [Performance Guide](docs/PERFORMANCE.md) | Performance strategy and references |
| [Release Guide](docs/RELEASE.md) | Creating releases |
| [Changelog](CHANGELOG.md) | Version history |

### Systems Documentation

| System | Guide |
|--------|-------|
| All Systems | [Systems Index](docs/systems/README.md) |
| NES / Famicom | [NES Guide](docs/systems/NES.md) |
| SNES / Super Famicom | [SNES Guide](docs/systems/SNES.md) |
| Game Boy / GBC | [GB Guide](docs/systems/GB.md) |
| Game Boy Advance | [GBA Guide](docs/systems/GBA.md) |
| Sega Master System / GG | [SMS Guide](docs/systems/SMS.md) |
| Sega Genesis / Mega Drive | _Coming soon_ |
| PC Engine / TG-16 | [PCE Guide](docs/systems/PCE.md) |
| WonderSwan / WSC | [WS Guide](docs/systems/WS.md) |
| Atari 2600 | _Coming soon_ |
| Atari Lynx | [Lynx Guide](docs/systems/Lynx.md) |
| Fairchild Channel F | [Channel F Guide](docs/systems/ChannelF.md) |

### Developer Documentation

| Document | Description |
|----------|-------------|
| [Architecture Overview](~docs/ARCHITECTURE-OVERVIEW.md) | System design |
| [C++ Development Guide](~docs/CPP-DEVELOPMENT-GUIDE.md) | Coding standards |
| [Developer Tasks](DEVELOPER-TASKS.md) | What to work on next |
| [Code Documentation Style](~docs/CODE-DOCUMENTATION-STYLE.md) | C++ documentation style guide |
| [Build and Run](~docs/BUILD-AND-RUN.md) | Building and running Nexen |
| [Profiling Guide](~docs/PROFILING-GUIDE.md) | Performance profiling |
| [ASan Guide](~docs/ASAN-GUIDE.md) | AddressSanitizer configuration |
| [Movie & TAS Subsystem](~docs/MOVIE-TAS.md) | Movie recording and TAS internals |
| [TAS Algorithm Reference](~docs/algorithms/tas-algorithms.md) | TAS data structure and undo system design |
| [Audio Subsystem](~docs/AUDIO-SUBSYSTEM.md) | Audio processing and resampling |
| [Video Rendering](~docs/VIDEO-RENDERING.md) | Video rendering pipeline |
| [Input Subsystem](~docs/INPUT-SUBSYSTEM.md) | Input and controller handling |
| [Debugger Subsystem](~docs/DEBUGGER.md) | Debugger internals |
| [Utilities Library](~docs/UTILITIES-LIBRARY.md) | Shared utility library |
| [Pansy Integration](~docs/pansy-export-index.md) | Metadata export system |
| [Channel F Master Plan](~docs/plans/channel-f-master-program-plan.md) | Fairchild Channel F roadmap |
| [Channel F Implementation Prompts](~docs/plans/channel-f-implementation-prompts.md) | Ordered multi-session execution prompt pack |
| [Channel F Source Index](~docs/research/channel-f-source-index.md) | External references mapped to implementation work |

### Emulation Core Internals

| Document | Description |
|----------|-------------|
| [NES Core](~docs/NES-CORE.md) | NES/Famicom emulation core |
| [SNES Core](~docs/SNES-CORE.md) | SNES/Super Famicom emulation core |
| [GB/GBA Core](~docs/GB-GBA-CORE.md) | Game Boy and GBA emulation core |
| [SMS/PCE/WS Core](~docs/SMS-PCE-WS-CORE.md) | SMS, PCE, and WS core |
| [Lynx Core](~docs/LYNX-CORE.md) | Atari Lynx core |

---

## 🏗️ Building from Source

### Requirements

| Component | Version | Notes |
|-----------|---------|-------|
| **C++ Compiler** | C++23 (MSVC v145+, Clang 18+, GCC 13+) | `/std:c++latest` |
| **.NET SDK** | 10.0+ | [Download](https://dotnet.microsoft.com/download) |
| **SDL2** | 2.0+ | Linux/macOS only |

Nexen resolves `Pansy.Core` from NuGet; you do not need a sibling `pansy` repo checked out next to Nexen.

### Windows

1. Install Visual Studio 2026 with "Desktop development with C++" workload
2. Install .NET 10 SDK
3. Open `Nexen.sln` and build Release x64

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential clang-18 libc++-18-dev libsdl2-dev
# Build
make -j$(nproc)
dotnet publish -c Release UI/UI.csproj
```

See [COMPILING.md](COMPILING.md) for detailed instructions.

---

## Markdown Quality Automation

Use these scripts to validate and benchmark markdown structure policy checks (`MD022`, `MD031`, `MD032`, `MD047`):

- `scripts/test-markdown-policy.ps1`
- `scripts/benchmark-markdown-policy.ps1`

Example:

```powershell
pwsh -File scripts/test-markdown-policy.ps1
pwsh -File scripts/benchmark-markdown-policy.ps1 -Runs 5
```

---

## 🔗 Related Projects

| Project | Description |
|---------|-------------|
| [🌼 Pansy](https://github.com/TheAnsarya/pansy) | Universal disassembly metadata format and toolkit |
| [🌺 Peony](https://github.com/TheAnsarya/peony) | Multi-system disassembler (ROM → `.pasm` source) |
| [🌸 Poppy](https://github.com/TheAnsarya/poppy) | Multi-system assembler (`.pasm` → ROM) |
| [🌱 Game Garden](https://github.com/TheAnsarya/game-garden) | Games disassembly, editing, and recompilation |

---

## 📜 License

Nexen is dual-licensed:

- **Nexen Additions (Unlicense):** All new features, modifications, and additions created for this fork are released into the public domain under [The Unlicense](https://unlicense.org).
- **Base Code (GPL v3):** The original [Mesen](https://github.com/SourMesen/Mesen2) code by Sour remains under the GNU General Public License v3.

See [LICENSE](LICENSE) for full details.

---

## 🙏 Acknowledgments

Nexen builds upon the excellent work of:

- **[Mesen](https://github.com/SourMesen/Mesen2)** by Sour — The original emulator codebase
- The emulation community for documentation and research
- Contributors and testers

---

<p align="center">
  Made with ❤️ for the retro gaming community
</p>
