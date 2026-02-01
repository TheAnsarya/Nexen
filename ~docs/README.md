# Nexen Development Documentation

This folder contains development documentation for Nexen contributions and customizations.

## 📖 Core Documentation

| Document | Description |
|----------|-------------|
| [Architecture Overview](ARCHITECTURE-OVERVIEW.md) | High-level architecture and design |
| [Code Documentation Style](CODE-DOCUMENTATION-STYLE.md) | Documentation standards |
| [C++ Development Guide](CPP-DEVELOPMENT-GUIDE.md) | C++23 coding practices |
| [Profiling Guide](PROFILING-GUIDE.md) | Performance profiling |
| [ASan Guide](ASAN-GUIDE.md) | AddressSanitizer memory debugging |

## 🎮 Emulation Core Documentation

| Document | Description |
|----------|-------------|
| [NES Core](NES-CORE.md) | 6502 CPU, PPU, APU emulation |
| [SNES Core](SNES-CORE.md) | 65816 CPU, PPU, SPC700, coprocessors |
| [GB/GBA Cores](GB-GBA-CORE.md) | LR35902, ARM7TDMI emulation |
| [SMS/PCE/WS Cores](SMS-PCE-WS-CORE.md) | Z80, HuC6280, V30MZ emulation |
| [Debugger Subsystem](DEBUGGER.md) | Breakpoints, CDL, scripting |
| [Utilities Library](UTILITIES-LIBRARY.md) | Common utilities reference |

## 🎛️ Peripheral System Documentation

| Document | Description |
|----------|-------------|
| [Input Subsystem](INPUT-SUBSYSTEM.md) | Controllers, input handling |
| [Audio Subsystem](AUDIO-SUBSYSTEM.md) | Audio mixing, effects, recording |
| [Video Rendering](VIDEO-RENDERING.md) | Filters, HUD, recording |
| [Movie/TAS System](MOVIE-TAS.md) | Movie recording, TAS features |

## 🌼 Pansy Export Feature

The Pansy export feature enables exporting and importing debug metadata in a universal format.

| Document | Description |
|----------|-------------|
| **[📚 Documentation Index](pansy-export-index.md)** | Start here! |
| [User Guide](pansy-export-user-guide.md) | End-user documentation |
| [Tutorials](pansy-export-tutorials.md) | Step-by-step workflows |
| [API Reference](pansy-export-api.md) | Developer API docs |
| [Developer Guide](pansy-export-developer-guide.md) | Contributing guide |

### Design Documents

| Document | Description |
|----------|-------------|
| [Integration Design](pansy-integration.md) | Original design |
| [Roadmap](pansy-roadmap.md) | Future plans |
| [Phase 7.5 Sync](phase-7.5-pansy-sync.md) | Sync feature design |

## 📁 Other Documentation

| Folder | Description |
|--------|-------------|
| [DiztinGUIsh Integration](DiztinGUIsh_Integration/) | Integration with DiztinGUIsh |
| [Modernization](modernization/) | C++ modernization notes |
| [Testing](testing/) | Test documentation |
| [Plans](plans/) | Planning documents |
| [Session Logs](session-logs/) | Development session logs |
| [Chat Logs](chat-logs/) | AI chat conversation logs |

## 🔗 GitHub

| Document | Description |
|----------|-------------|
| [GitHub Issues](github-issues.md) | Issue tracking notes |

## 📊 Project Status

### Branch: `cpp-modernization`

| Metric | Value |
|--------|-------|
| Unit Tests | 421 tests |
| Benchmarks | 224 benchmarks |
| Documentation Files | 13 core docs |

### Completed Epics

- ✅ [Epic 15] Unit Testing Infrastructure
- ✅ [Epic 17] Code Documentation and Comments
- ✅ [Epic 18] Additional System Documentation

### Branch: `pansy-export`

- ✅ All 7 phases implemented
- ✅ 152 tests passing
- ✅ Full documentation complete
- 🔄 Ready for review and testing
