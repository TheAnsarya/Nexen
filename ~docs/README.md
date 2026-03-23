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

## Platform Parity Research

| Document | Description |
| ---------- | ------------- |
| [Research Home](research/README.md) | Entry point for deep research artifacts |
| [Platform Parity Program Index](research/platform-parity/README.md) | Atari 2600 and Genesis research tree with source links |
| [Atari 2600 Breakdown](research/platform-parity/atari-2600/README.md) | Subsystem-by-subsystem Atari 2600 documentation |
| [Genesis Breakdown](research/platform-parity/genesis/README.md) | Comparative Genesis emulator architecture research |
| [Compatibility Path](research/platform-parity/compatibility/sonic1-jurassic-park.md) | Sonic 1 and Jurassic Park execution milestones |

## 📁 Subfolders

| Folder | Description |
| -------- | ------------- |
| [modernization/](modernization/) | C++ modernization tracking and roadmaps |
| [testing/](testing/) | Test plans and benchmarking documentation |
| [plans/](plans/) | Planning documents for future features |
| [research/](research/) | Source-cited platform-parity and subsystem research |
| [session-logs/](session-logs/) | Development session logs |
| [chat-logs/](chat-logs/) | AI conversation logs |

## 📋 Project Tracking

| Document | Description |
| ---------- | ------------- |
| [GitHub Issues](github-issues.md) | Issue tracking notes |
| [Keyboard Shortcuts](keyboard-shortcuts-save-states.md) | Save state shortcut design |
| [Q2 Future Work Program](plans/future-work-program-2026-q2.md) | Milestone-driven execution plan for Epic #673 and sub-issues |
| [Q3 Platform Parity Research Program](plans/platform-parity-research-program-2026-q3.md) | Multi-session research and execution plan for #704-#707 |
| [Scaffold Future-Work Backlog](plans/platform-parity-scaffold-future-work-backlog.md) | Deferred issue tree for converting Atari/Genesis scaffold phases into production implementation work |
| [Atari 2600 Feasibility and Harness Plan](plans/atari-2600-feasibility-and-harness-plan.md) | Spike architecture boundaries, risk register, and harness milestones |
| [Genesis Architecture and Incremental Plan](plans/genesis-architecture-and-incremental-plan.md) | Phase plan, risk register, and benchmark/correctness gates for Genesis |
| [Atari CPU/RIOT Bring-Up Skeleton Plan](plans/atari-2600-cpu-riot-bringup-skeleton.md) | Issue-backed implementation scaffold for Atari CPU and RIOT boundaries (#697) |
| [Atari TIA Timing Spike Harness Plan](plans/atari-2600-tia-timing-spike-harness.md) | Deterministic TIA timing checkpoints and smoke harness contract (#698) |
| [Atari Mapper Order and Regression Matrix](plans/atari-2600-mapper-order-and-regression-matrix.md) | Mapper priority and regression matrix for staged cartridge support (#699) |
| [Atari 2600 Production Readiness Matrix](testing/atari2600-production-readiness-matrix.md) | Consolidated build/test/TAS/debugger/export validation checklist for Atari 2600 |
| [Genesis M68000 Boundary and Bring-Up Plan](plans/genesis-m68000-boundary-and-bringup.md) | M68000 contract and phased bring-up scaffold (#700) |
| [Genesis VDP DMA Milestone Plan](plans/genesis-vdp-dma-milestones-and-harness.md) | VDP timing and DMA checkpoint matrix for staged execution (#701) |
| [Genesis Z80 Audio Integration Staging](plans/genesis-z80-audio-integration-staging.md) | Phased Z80/YM2612/SN76489 integration and risk controls (#702) |
| [Platform Parity Benchmark and Correctness Gates](plans/platform-parity-benchmark-and-correctness-gates.md) | Cross-phase quality gates and evidence framework (#703) |
| [Atari 2600 + Genesis Parity Tracker](plans/atari2600-genesis-parity-tracker.md) | Active multi-phase checklist, issue linkage, and closure criteria for parity execution (#750) |
| [Platform Parity Source Index](research/platform-parity/source-index.md) | External hardware and emulator code references used by research docs |
| [User Future Work Index](../docs/FUTURE-WORK.md) | Public roadmap entry for active and upcoming tracks |
| [User Tutorials Index](../docs/TUTORIALS.md) | Step-by-step user and contributor workflow tutorials |

GitHub issue template for gate-driven phase execution:

- `.github/ISSUE_TEMPLATE/platform-parity-phase-gate.yml`

## 📊 Current Status

### Feature Status

| Feature | Status | Notes |
| --------- | -------- | ------- |
| C++23 Modernization | ✅ Complete | Smart pointers, ranges, format |
| Unit Tests | ✅ Complete | 2790 tests (1633 C++, 826 .NET, 331 MovieConverter) |
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
