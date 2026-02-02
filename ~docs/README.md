# Nexen Developer Documentation

This directory contains internal development documentation for Nexen contributors and maintainers.

> **Note:** For user documentation, see [docs/](../docs/README.md).

## 📖 Core Documentation

| Document | Description |
| ---------- | ------------- |
| [Architecture Overview](ARCHITECTURE-OVERVIEW.md) | High-level system design and component interaction |
| [C++ Development Guide](CPP-DEVELOPMENT-GUIDE.md) | C++23 coding practices and standards |
| [Code Documentation Style](CODE-DOCUMENTATION-STYLE.md) | XML comment style for Doxygen |
| [Profiling Guide](PROFILING-GUIDE.md) | Performance profiling techniques |
| [ASan Guide](ASAN-GUIDE.md) | AddressSanitizer for memory debugging |

## 🎮 Emulation Core Documentation

| Document | Description |
| ---------- | ------------- |
| [NES Core](NES-CORE.md) | 6502 CPU, PPU, APU, mappers |
| [SNES Core](SNES-CORE.md) | 65816 CPU, PPU, SPC700, coprocessors |
| [GB/GBA Cores](GB-GBA-CORE.md) | LR35902, ARM7TDMI, PPU |
| [SMS/PCE/WS Cores](SMS-PCE-WS-CORE.md) | Z80, HuC6280, V30MZ |
| [Debugger Subsystem](DEBUGGER.md) | Breakpoints, CDL, scripting |
| [Utilities Library](UTILITIES-LIBRARY.md) | Common utility classes |

## 🎛️ Peripheral System Documentation

| Document | Description |
| ---------- | ------------- |
| [Input Subsystem](INPUT-SUBSYSTEM.md) | Controllers, input handling, polling |
| [Audio Subsystem](AUDIO-SUBSYSTEM.md) | Audio mixing, effects, recording |
| [Video Rendering](VIDEO-RENDERING.md) | Filters, HUD, shaders, recording |
| [Movie/TAS System](MOVIE-TAS.md) | Movie recording, TAS features |

## 🌼 Pansy Export Feature

The Pansy export feature enables exporting and importing debug metadata in a universal format compatible with the Peony disassembler and Poppy assembler.

### Getting Started

| Document | Description |
| ---------- | ------------- |
| **[📚 Documentation Index](pansy-export-index.md)** | Start here! Complete overview |
| [User Guide](pansy-export-user-guide.md) | End-user documentation |
| [Tutorials](pansy-export-tutorials.md) | Step-by-step workflows |

### Technical Reference

| Document | Description |
| ---------- | ------------- |
| [API Reference](pansy-export-api.md) | C# and C++ API documentation |
| [Developer Guide](pansy-export-developer-guide.md) | Contributing to Pansy export |

### Design Documents

| Document | Description |
| ---------- | ------------- |
| [Integration Design](pansy-integration.md) | Original design document |
| [Roadmap](pansy-roadmap.md) | Future plans and phases |
| [Phase 7.5 Sync](phase-7.5-pansy-sync.md) | File sync feature design |

## 📁 Subfolders

| Folder | Description |
| -------- | ------------- |
| [modernization/](modernization/) | C++ modernization tracking and roadmaps |
| [testing/](testing/) | Test plans and benchmarking documentation |
| [plans/](plans/) | Planning documents for future features |
| [session-logs/](session-logs/) | Development session logs |
| [chat-logs/](chat-logs/) | AI conversation logs |

## 📋 Project Tracking

| Document | Description |
| ---------- | ------------- |
| [GitHub Issues](github-issues.md) | Issue tracking notes |
| [Keyboard Shortcuts](keyboard-shortcuts-save-states.md) | Save state shortcut design |

## 📊 Current Status

### Feature Status

| Feature | Status | Notes |
| --------- | -------- | ------- |
| C++23 Modernization | ✅ Complete | Smart pointers, ranges, format |
| Unit Tests | ✅ Complete | 421 tests |
| Pansy Export | ✅ Complete | All 7 phases |
| Infinite Save States | ✅ Complete | Visual picker, timestamped |
| TAS Editor | ✅ Complete | Piano roll, greenzone, Lua |
| Documentation | ✅ Complete | User and developer docs |

### Branch Overview

| Branch | Purpose | Status |
| -------- | --------- | -------- |
| `master` | Stable releases | 🔒 Protected |
| `cpp-modernization` | C++ updates | ✅ Merged |
| `pansy-export` | Pansy integration | ✅ Merged |
| `feature/tas-movie-system` | TAS Editor | 🔄 Active |

## 🔗 External Links

| Resource | URL |
| ---------- | ----- |
| Repository | [github.com/TheAnsarya/Nexen](https://github.com/TheAnsarya/Nexen) |
| Issues | [GitHub Issues](https://github.com/TheAnsarya/Nexen/issues) |
| Actions | [CI/CD Builds](https://github.com/TheAnsarya/Nexen/actions) |
| Pansy | [github.com/TheAnsarya/pansy](https://github.com/TheAnsarya/pansy) |
| Peony | [github.com/TheAnsarya/peony](https://github.com/TheAnsarya/peony) |
| Poppy | [github.com/TheAnsarya/poppy](https://github.com/TheAnsarya/poppy) |
