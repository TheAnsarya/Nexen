# Mesen (TheAnsarya Fork)

Mesen is a multi-system emulator (NES, SNES, Game Boy, Game Boy Advance, PC Engine, SMS/Game Gear, WonderSwan) for Windows, Linux and macOS.

**This is a modernization fork** focused on C++23, .NET 10, and 🌼 Pansy metadata integration.

## Fork Features

- **C++23 Modernization** - Smart pointers, std::format, ranges, [[likely]]/[[unlikely]]
- **🌼 Pansy Export** - Export disassembly metadata for use with Peony/Poppy toolchain
- **💾 Infinite Save States** - Timestamped saves with visual picker (see below)
- **Unit Tests** - Comprehensive CPU instruction tests (Google Test)
- **Documentation** - Doxygen API documentation

## 💾 Infinite Save States

This fork replaces the default F1/Shift+F1 behavior with an infinite timestamped save state system:

### Quick Reference

| Shortcut | Action |
|----------|--------|
| **Shift+F1** | Quick save (creates new timestamped save) |
| **F1** | Open save state picker |
| **DEL** (in picker) | Delete selected save (with confirmation) |
| **ESC** (in picker) | Close picker |
| **Arrow keys** (in picker) | Navigate saves |
| **Enter** (in picker) | Load selected save |

### Features

- **Unlimited saves** - Each save creates a new timestamped file (`{ROM}_{YYYY-MM-DD}_{HH-mm-ss}.mss`)
- **Per-ROM organization** - Saves stored in `SaveStates/{RomName}/` subdirectories
- **Visual picker** - Grid view with screenshots and timestamps
- **Full timestamps** - Always shows date and time (e.g., "Today 1/31/2026 2:30 PM")
- **Delete confirmation** - Prevents accidental deletion
- **Slot compatibility** - F2-F7 still work for slots 2-7 (legacy support)

### Menu Access

- **File → Quick Save (Timestamped)** - Save with timestamp
- **File → Browse Save States...** - Open visual picker
- **File → Save State → Slot N** - Legacy slot-based saves
- **File → Load State → Slot N** - Legacy slot-based loads

## Upstream

Original Mesen by Sour: [SourMesen/Mesen2](https://github.com/SourMesen/Mesen2)

## Releases

For stable releases, see the [upstream releases](https://github.com/SourMesen/Mesen2/releases).
For this fork's development builds, see [Actions](https://github.com/TheAnsarya/Mesen2/actions).

## Development Builds

[![Mesen](https://github.com/TheAnsarya/Mesen2/actions/workflows/build.yml/badge.svg)](https://github.com/TheAnsarya/Mesen2/actions/workflows/build.yml)

### Requirements

| Component | Version |
|-----------|---------|
| C++ | C++23 (`/std:c++latest`) |
| .NET | 10.0+ |
| Visual Studio | 2026 (v145 toolset) |
| SDL2 | Latest (Linux/macOS) |

## Compiling

See [COMPILING.md](COMPILING.md)

## License

This project uses a **dual-license** structure:

### Original Mesen Code (GPL v3)

All code from the original Mesen project by Sour is licensed under the GNU General Public License v3.
See [LICENSE-GPL](LICENSE) for the full GPL v3 text.

Copyright (C) 2014-2025 Sour

### Fork Modifications (The Unlicense)

All modifications, additions, and new code in this fork (TheAnsarya/Mesen2) are released under **The Unlicense** (public domain).
See [LICENSE-UNLICENSE](LICENSE-UNLICENSE) for details.

This includes:
- C++ modernization changes
- 🌼 Pansy metadata export feature
- Unit tests
- Documentation improvements

You are free to use the fork-specific changes without any restrictions.

## Related Projects

- [🌼 Pansy](https://github.com/TheAnsarya/pansy) - Disassembly metadata format
- [🌺 Peony](https://github.com/TheAnsarya/peony) - Multi-system disassembler
- [🌸 Poppy](https://github.com/TheAnsarya/poppy) - Multi-system assembler
