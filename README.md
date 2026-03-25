# Nexen

<p align="center">
  <img src="~assets/Nexen%20icon.png" alt="Nexen" width="128"/>
</p>

<p align="center">
  <strong>A powerful multi-system emulator for Windows, Linux, and macOS</strong>
</p>

<p align="center">
  <a href="https://github.com/TheAnsarya/Nexen/releases">
    <img src="https://img.shields.io/github/v/release/TheAnsarya/Nexen?include_prereleases" alt="Latest Release"/>
  </a>
  <a href="LICENSE">
    <img src="https://img.shields.io/badge/license-Unlicense%20%2B%20GPL--3.0-blue.svg" alt="License"/>
  </a>
</p>

---

## 📥 Downloads

Download pre-built binaries from the [Releases page](https://github.com/TheAnsarya/Nexen/releases/latest).

### Windows

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-Windows-x64-v1.4.2.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.2/Nexen-Windows-x64-v1.4.2.exe) | Single-file, recommended |
| **Native AOT** | [Nexen-Windows-x64-AoT-v1.4.2.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.2/Nexen-Windows-x64-AoT-v1.4.2.exe) | Faster startup |

### Linux

| Build | Download | Notes |
|-------|----------|-------|
| **AppImage x64** | [Nexen-Linux-x64-v1.4.2.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.2/Nexen-Linux-x64-v1.4.2.AppImage) | Recommended |
| **AppImage ARM64** | [Nexen-Linux-ARM64-v1.4.2.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.2/Nexen-Linux-ARM64-v1.4.2.AppImage) | Raspberry Pi, etc. |
| Binary x64 (clang) | [Nexen-Linux-x64-v1.4.2.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.2/Nexen-Linux-x64-v1.4.2.tar.gz) | Tarball, requires SDL2 |
| Binary x64 (gcc) | [Nexen-Linux-x64-gcc-v1.4.2.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.2/Nexen-Linux-x64-gcc-v1.4.2.tar.gz) | Tarball, requires SDL2 |
| Binary ARM64 (clang) | [Nexen-Linux-ARM64-v1.4.2.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.2/Nexen-Linux-ARM64-v1.4.2.tar.gz) | Tarball, requires SDL2 |
| Binary ARM64 (gcc) | [Nexen-Linux-ARM64-gcc-v1.4.2.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.2/Nexen-Linux-ARM64-gcc-v1.4.2.tar.gz) | Tarball, requires SDL2 |
| Native AOT x64 | [Nexen-Linux-x64-AoT-v1.4.2.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.2/Nexen-Linux-x64-AoT-v1.4.2.tar.gz) | Faster startup |

### macOS (Apple Silicon)

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-macOS-ARM64-v1.4.2.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.2/Nexen-macOS-ARM64-v1.4.2.zip) | App bundle |
| ~~Native AOT~~ | *Temporarily unavailable* | .NET 10 ILC compiler bug |

> ℹ️ **Notes:**
>
> - Linux requires SDL2 (`sudo apt install libsdl2-2.0-0`)
> - macOS: Right-click → Open on first launch to bypass Gatekeeper
> - macOS Intel (x64) builds are no longer provided
> - macOS Native AOT is disabled due to a [.NET 10 ILC compiler crash](https://github.com/TheAnsarya/Nexen/issues/238)
> - **Download links point to v1.4.2** — Updated on each release

---

## 🎮 Supported Systems

| System | CPU | Status |
| -------- | ----- | -------- |
| NES / Famicom | 6502 | ✅ Full |
| SNES / Super Famicom | 65816 | ✅ Full |
| Game Boy / Game Boy Color | LR35902 | ✅ Full |
| Game Boy Advance | ARM7TDMI | ✅ Full |
| Sega Master System / Game Gear | Z80 | ✅ Full |
| PC Engine / TurboGrafx-16 | HuC6280 | ✅ Full |
| WonderSwan / WonderSwan Color | V30MZ | ✅ Full |
| Atari Lynx | 65SC02 | ✅ Full |

## ✨ Key Features

### 🕹️ Emulation

- High-accuracy emulation across all supported systems
- Cycle-accurate CPU and PPU emulation
- Full coprocessor support (Super FX, SA-1, DSP, etc.)
- Comprehensive mapper/chip support
- Netplay multiplayer support

### 💾 Save States

- **Infinite Save States** - Unlimited timestamped saves per game
- **Visual Picker** - Grid view with screenshots and timestamps (Shift+F1)
- **Quick Save** - F1 to save, Shift+F1 to browse
- **Designated Slot** - F4 to load, Shift+F4 to save
- **Auto-Save** - Periodic quick saves and recent play saves
- **Per-Game Organization** - Saves organized by ROM

### 🎬 Movie System

- **Nexen Movie Format (.nexen-movie)** - ZIP-based format with JSON metadata, input log, savestate, and SRAM
- **Movie Recording & Playback** - Record/replay inputs frame-by-frame with rerecording support
- **Multi-Format Import/Export** - BK2 (BizHawk), FM2 (FCEUX), SMV (Snes9x), LSMV (lsnes), VBM (VisualBoyAdvance), GMV (Gens)
- **Multi-System TAS** - Full TAS support for NES, SNES, GB, GBA, SMS, PCE, WS, and Lynx

### 🎮 TAS Editor

- **Piano Roll View** - Visual timeline for frame-by-frame editing with batch paint
- **Full Undo/Redo** - Every operation (toggle, paint, insert, delete, clear, fork) is undoable
- **Greenzone System** - Instant seeking with automatic savestates
- **Fast Timeline Refresh** - Smooth frame navigation during long movies
- **Input Recording** - Capture, insert, and overwrite modes with branch support
- **Branches** - Fork, compare, and load alternate strategies
- **Lua Scripting** - Automate TAS workflows with full undo integration

### 🔧 Debugging

- **Disassembler** - Full CPU disassembly with labels
- **Memory Viewer** - Hex editor with search
- **Breakpoints** - Execution, read, and write breakpoints
- **Trace Logger** - CPU execution logging
- **Code/Data Logger (CDL)** - Track ROM usage
- **🌼 Pansy Export** - Export metadata to universal format

### 🎨 Video

- Multiple shader/filter options
- Integer scaling and aspect ratio correction
- NTSC and PAL filters
- HUD overlays for debugging

### 🔊 Audio

- High-quality audio resampling
- Per-channel volume control
- Audio recording (WAV)

## 🧭 Using Nexen

| Guide | Description |
|---|---|
| [Save States Guide](docs/Save-States.md) | Quick save, designated slots, and visual picker workflows |
| [Rewind Guide](docs/Rewind.md) | Rewind usage and route iteration workflows |
| [Movie System Guide](docs/Movie-System.md) | Record, playback, import/export, and deterministic playback workflows |
| [TAS Editor Manual](docs/TAS-Editor-Manual.md) | Complete frame-by-frame TAS editing workflow |
| [Debugging Guide](docs/Debugging.md) | Disassembler, memory viewer, breakpoints, trace, and CDL usage |
| [Video and Audio Guide](docs/Video-Audio.md) | Rendering and audio configuration workflows |

## Quick Start

### Windows

1. Download `Nexen-Windows-x64-v1.4.2.exe` from [Releases](https://github.com/TheAnsarya/Nexen/releases/tag/v1.4.2)
2. Run the executable (no installation needed)
3. **File → Open** to load a ROM

### Linux

1. Download `Nexen-Linux-x64-v1.4.2.AppImage`
2. Make executable: `chmod +x Nexen-Linux-x64-v1.4.2.AppImage`
3. Run: `./Nexen-Linux-x64-v1.4.2.AppImage`

> For non-AppImage builds, install SDL2 first: `sudo apt install libsdl2-2.0-0`

### macOS

1. Download `Nexen-macOS-ARM64-v1.4.2.zip`
2. Extract and move `Nexen.app` to Applications
3. Right-click → Open (first launch only, to bypass Gatekeeper)

## 📖 Documentation

| Document | Description |
|---|---|
| [Documentation Index](docs/README.md) | Entry point for user-facing docs |
| [Manual Testing Hub](docs/manual-testing/README.md) | Ranked manual release validation workflows |
| [Compiling](COMPILING.md) | Build from source |
| [Performance Guide](docs/PERFORMANCE.md) | Centralized performance strategy and references |
| [Release Guide](docs/RELEASE.md) | Creating releases |
| [API Documentation](docs/README.md) | Generated API docs |

### Systems Documentation

| System | Guide |
|---|---|
| All systems (index) | [Systems Index](docs/systems/README.md) |
| NES / Famicom | [NES Guide](docs/systems/NES.md) |
| SNES / Super Famicom | [SNES Guide](docs/systems/SNES.md) |
| Game Boy / Game Boy Color | [GB Guide](docs/systems/GB.md) |
| Game Boy Advance | [GBA Guide](docs/systems/GBA.md) |
| Sega Master System / Game Gear | [SMS Guide](docs/systems/SMS.md) |
| PC Engine / TurboGrafx-16 | [PCE Guide](docs/systems/PCE.md) |
| WonderSwan / WonderSwan Color | [WS Guide](docs/systems/WS.md) |
| Atari Lynx | [Lynx Guide](docs/systems/Lynx.md) |

### Developer Documentation

| Document | Description |
|---|---|
| [Architecture Overview](~docs/ARCHITECTURE-OVERVIEW.md) | System design |
| [C++ Development Guide](~docs/CPP-DEVELOPMENT-GUIDE.md) | Coding standards |
| [Developer Tasks and Priorities](DEVELOPER-TASKS.md) | What to work on next, with Atari/Genesis support checklist |
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

### Emulation Core Documentation

| Document | Description |
|---|---|
| [NES Core](~docs/NES-CORE.md) | NES/Famicom emulation core internals |
| [SNES Core](~docs/SNES-CORE.md) | SNES/Super Famicom emulation core internals |
| [GB/GBA Core](~docs/GB-GBA-CORE.md) | Game Boy and GBA emulation core internals |
| [SMS/PCE/WS Core](~docs/SMS-PCE-WS-CORE.md) | SMS, PCE, and WS core internals |
| [Lynx Core](~docs/LYNX-CORE.md) | Atari Lynx core internals |

## ⌨️ Keyboard Shortcuts

### General

| Shortcut | Action |
| ---------- | -------- |
| F1 | Quick Save (Timestamped) |
| Shift+F1 | Browse Save States |
| F4 | Load Designated Slot |
| Shift+F4 | Save Designated Slot |
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
| ---------- | -------- |
| Escape | Pause/Resume Playback |
| \` (backtick) | Frame Advance |
| Backspace | Frame Rewind |
| Insert | Insert Frame |
| Delete | Delete Frame |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Ctrl+G | Go to Frame |
| Ctrl+B | Create Branch |

## 🌷 Integrated Pipeline

Nexen is the **play & debug** stage of the **Flower Toolchain** — an integrated pipeline for playing, debugging, disassembling, editing, and rebuilding retro games:

| Stage | Tool | Nexen Role |
|-------|------|------------|
| 1. Play & Debug | **Nexen** | Run games, export CDL + symbols + CPU state via Pansy |
| 2. Disassemble | [Peony](https://github.com/TheAnsarya/peony) | — |
| 3. Edit & Document | Editor + [Pansy](https://github.com/TheAnsarya/pansy) UI | — |
| 4. Build | [Poppy](https://github.com/TheAnsarya/poppy) | — |
| 5. Verify | [Game Garden](https://github.com/TheAnsarya/game-garden) | Roundtrip byte-identical rebuild |

See the [Integrated Pipeline Master Plan](https://github.com/TheAnsarya/pansy/blob/main/~Plans/integrated-pipeline-master-plan.md) for architecture details.

## 🔗 Related Projects

| Project | Description |
| --------- | ------------- |
| [🌼 Pansy](https://github.com/TheAnsarya/pansy) | Universal disassembly metadata format |
| [🌺 Peony](https://github.com/TheAnsarya/peony) | Multi-system disassembler |
| [🌸 Poppy](https://github.com/TheAnsarya/poppy) | Multi-system assembler |
| [🌱 Game Garden](https://github.com/TheAnsarya/game-garden) | Games disassembly & recompilation |

## 🏗️ Building from Source

### Requirements

| Component | Version | Notes |
|-----------|---------|-------|
| **C++ Compiler** | C++23 (MSVC 19.40+, Clang 18+, GCC 13+) | `stdcpplatest` |
| **.NET SDK** | 10.0+ | [Download](https://dotnet.microsoft.com/download) |
| **SDL2** | 2.0+ | Linux/macOS only |

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

## Markdown Quality Automation

Use these scripts to validate and benchmark markdown structure policy checks (`MD022`, `MD031`, `MD032`, `MD047`):

- `scripts/test-markdown-policy.ps1`
- `scripts/benchmark-markdown-policy.ps1`

Example:

```powershell
pwsh -File scripts/test-markdown-policy.ps1
pwsh -File scripts/benchmark-markdown-policy.ps1 -Runs 5
```

## 📜 License

Nexen is dual-licensed:

- **Nexen Additions (Unlicense):** All new features, modifications, and additions created for this fork are released into the public domain under [The Unlicense](https://unlicense.org).
- **Base Code (GPL v3):** The original [Mesen](https://github.com/SourMesen/Mesen2) code by Sour remains under the GNU General Public License v3.

See [LICENSE](LICENSE) for full details.

## 🙏 Acknowledgments

Nexen builds upon the excellent work of:

- **[Mesen](https://github.com/SourMesen/Mesen2)** by Sour - The original emulator codebase
- The emulation community for documentation and research
- Contributors and testers

---

<p align="center">
  Made with ❤️ for the retro gaming community
</p>
