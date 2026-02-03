# Nexen

<p align="center">
  <img src="UI/Resources/icon.png" alt="Nexen" width="128"/>
</p>

<p align="center">
  <strong>A powerful multi-system emulator for Windows, Linux, and macOS</strong>
</p>

<p align="center">
  <a href="https://github.com/TheAnsarya/Nexen/actions/workflows/build.yml">
    <img src="https://github.com/TheAnsarya/Nexen/actions/workflows/build.yml/badge.svg" alt="Build Status"/>
  </a>
  <a href="https://github.com/TheAnsarya/Nexen/releases">
    <img src="https://img.shields.io/github/v/release/TheAnsarya/Nexen?include_prereleases" alt="Latest Release"/>
  </a>
  <a href="LICENSE">
    <img src="https://img.shields.io/badge/license-GPL--3.0-blue.svg" alt="License"/>
  </a>
</p>

---

## 📥 Downloads

### Latest Release (v1.0.0)

Download pre-built binaries from the [Releases page](https://github.com/TheAnsarya/Nexen/releases/latest).

| Platform | Architecture | Status | Download |
|----------|--------------|--------|----------|
| **Windows** | x64 | ✅ | [Nexen-v1.0.0-Windows-x64.zip](https://github.com/TheAnsarya/Nexen/releases/latest/download/Nexen-v1.0.0-Windows-x64.zip) |
| **Linux** | x64 | ❌ | *Coming soon* |
| **Linux** | x64 AppImage | ❌ | *Coming soon* |
| **Linux** | ARM64 | ❌ | *Coming soon* |
| **Linux** | ARM64 AppImage | ❌ | *Coming soon* |
| **macOS** | Intel (x64) | ❌ | *Coming soon* |
| **macOS** | Apple Silicon | ❌ | *Coming soon* |

> ℹ️ **Note:** Currently only Windows x64 is available. Linux and macOS builds are in progress.

### Development Builds

Automated builds from the latest code are available as artifacts from [GitHub Actions](https://github.com/TheAnsarya/Nexen/actions/workflows/build.yml). Click on a successful workflow run and download artifacts from the bottom of the page.

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

## ✨ Key Features

### 🕹️ Emulation

- High-accuracy emulation across all supported systems
- Cycle-accurate CPU and PPU emulation
- Full coprocessor support (Super FX, SA-1, DSP, etc.)
- Comprehensive mapper/chip support
- Netplay multiplayer support

### 💾 Save States

- **Infinite Save States** - Unlimited timestamped saves per game
- **Visual Picker** - Grid view with screenshots and timestamps
- **Quick Save/Load** - Shift+F1 to save, F1 to browse
- **Per-Game Organization** - Saves organized by ROM

### 🎮 TAS Editor

- **Piano Roll View** - Visual timeline for frame-by-frame editing
- **Greenzone System** - Instant seeking with automatic savestates
- **Input Recording** - Capture and edit gameplay
- **Branches** - Compare alternate strategies
- **Multi-Format Import/Export** - BK2, FM2, SMV, LSMV, VBM, GMV, NMV
- **Lua Scripting** - Automate TAS workflows

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

##  Quick Start

### Windows

1. Download and extract the ZIP file
2. Run `Nexen.exe`
3. **File → Open** to load a ROM

### Linux

1. Download the AppImage
2. Make it executable: `chmod +x Nexen.AppImage`
3. Run: `./Nexen.AppImage`

### macOS

1. Download and extract the ZIP file
2. Move `Nexen.app` to Applications
3. Right-click → Open (first time, to bypass Gatekeeper)

## 📖 Documentation

| Document | Description |
| ---------- | ------------- |
| [Compiling](COMPILING.md) | Build from source |
| [TAS Editor Manual](docs/TAS-Editor-Manual.md) | Complete TAS workflow guide |
| [TAS Developer Guide](docs/TAS-Developer-Guide.md) | TAS system architecture |
| [Movie Format](docs/NEXEN_MOVIE_FORMAT.md) | NMV file format specification |
| [Release Guide](docs/RELEASE.md) | Creating releases |
| [API Documentation](docs/README.md) | Generated API docs |

### Developer Documentation

| Document | Description |
| ---------- | ------------- |
| [Architecture Overview](~docs/ARCHITECTURE-OVERVIEW.md) | System design |
| [C++ Development Guide](~docs/CPP-DEVELOPMENT-GUIDE.md) | Coding standards |
| [Pansy Integration](~docs/pansy-export-index.md) | Metadata export system |
| [Profiling Guide](~docs/PROFILING-GUIDE.md) | Performance profiling |

## ⌨️ Keyboard Shortcuts

### General

| Shortcut | Action |
| ---------- | -------- |
| F1 | Browse Save States |
| Shift+F1 | Quick Save (Timestamped) |
| F2-F7 | Save to Slot 2-7 |
| Shift+F2-F7 | Load from Slot 2-7 |
| Escape | Pause/Resume |
| Alt+Enter | Toggle Fullscreen |

### TAS Editor

| Shortcut | Action |
| ---------- | -------- |
| Space | Play/Pause |
| F | Frame Advance |
| R | Frame Rewind |
| Ctrl+R | Toggle Recording |
| Insert | Insert Frame |
| Delete | Delete Frame |
| Ctrl+G | Go to Frame |
| Ctrl+B | Create Branch |

## 🔗 Related Projects

| Project | Description |
| --------- | ------------- |
| [🌼 Pansy](https://github.com/TheAnsarya/pansy) | Universal disassembly metadata format |
| [🌺 Peony](https://github.com/TheAnsarya/peony) | Multi-system disassembler |
| [🌸 Poppy](https://github.com/TheAnsarya/poppy) | Multi-system assembler |

## 🏗️ Building from Source

### Requirements

| Component | Version |
| ----------- | --------- |
| C++ | C++23 |
| .NET SDK | 10.0+ |
| SDL2 | Latest (Linux/macOS) |

See [COMPILING.md](COMPILING.md) for detailed instructions.

## 📜 License

Nexen is dual-licensed:

- **Base Code (GPL v3):** Original code from [Mesen](https://github.com/SourMesen/Mesen2) by Sour
- **Nexen Additions (Unlicense):** All new features and modifications

See [LICENSE](LICENSE) and [LICENSE-UNLICENSE](LICENSE-UNLICENSE) for details.

## 🙏 Acknowledgments

Nexen builds upon the excellent work of:

- **[Mesen](https://github.com/SourMesen/Mesen2)** by Sour - The original emulator codebase
- The emulation community for documentation and research
- Contributors and testers

---

<p align="center">
  Made with ❤️ for the retro gaming community
</p>
