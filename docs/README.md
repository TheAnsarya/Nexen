# Nexen Documentation

This directory contains user-facing documentation for Nexen.

## User Guides

| Document | Description |
|---|---|
| [Save States Guide](Save-States.md) | Quick save, designated slots, and visual state browser workflows |
| [Rewind Guide](Rewind.md) | Rewind usage and frame-level iteration workflows |
| [Movie System Guide](Movie-System.md) | Record, playback, import/export, and deterministic movie workflows |
| [Debugging Guide](Debugging.md) | Disassembler, memory viewer, breakpoints, trace, and CDL usage |
| [Video and Audio Guide](Video-Audio.md) | Rendering and audio configuration workflows |
| [TAS Editor Manual](TAS-Editor-Manual.md) | Complete TAS workflow guide |
| [Tutorials](TUTORIALS.md) | Step-by-step user and contributor workflows |
| [Future Work Index](FUTURE-WORK.md) | Active roadmap tracks and planned improvement programs |
| [TAS Developer Guide](TAS-Developer-Guide.md) | Technical internals for TAS extensions |
| [TAS Architecture](TAS_ARCHITECTURE.md) | TAS and movie system architecture |
| [Channel F Architecture](CHANNELF-ARCHITECTURE.md) | Channel F core, debugger, TAS, and persistence integration overview |
| [Movie Format Spec](NEXEN_MOVIE_FORMAT.md) | .nexen-movie file format specification |
| [Manual Testing Hub](manual-testing/README.md) | Ranked and repeatable manual release validation workflows |
| [Performance Guide](PERFORMANCE.md) | Consolidated performance references and validation workflow |
| [Archive Stack](ARCHIVE-STACK.md) | Runtime/archive tooling matrix for ZIP and 7z support |
| [Release Guide](RELEASE.md) | Release and publishing workflow |

## Manual Testing

| Document | Description |
|---|---|
| [Manual Testing Hub](manual-testing/README.md) | Starting manual instructions and execution order |
| [Atari 2600 Debug and TAS Manual Test](manual-testing/atari2600-debug-and-tas-manual-test.md) | End-to-end debugger, TAS, and movie validation workflow |
| [Channel F Debug and TAS Manual Test](manual-testing/channelf-debug-and-tas-manual-test.md) | End-to-end Channel F debugger, TAS, movie, and persistence validation workflow |
| [Manual Step Priority Ranking](manual-testing/manual-step-priority-ranking.md) | Release impact ranking for manual validation steps |
| [Developer Testing and Manual Validation Index](../~docs/testing/README.md) | Full manual and automated testing tree |

## Libraries

| Document | Description |
|---|---|
| [MovieConverter Library](../MovieConverter/README.md) | TAS movie format conversion library — read/write Nexen, BK2, LSMV, FM2, SMV; import VBM, GMV |

## Developer Tasks

| Task Document | Description |
|---|---|
| [Screenshot Capture Checklist](screenshots/CAPTURE-CHECKLIST.md) | Capture and verify every screenshot anchor used by user guides |
| [Atari 2600 Emulation Roadmap](../~docs/plans/long-term-missing-platforms.md) | Feasibility, architecture constraints, and implementation staging for Atari 2600 |
| [Atari 2600 Feasibility and Harness Plan](../~docs/plans/atari-2600-feasibility-and-harness-plan.md) | Detailed spike scope, measurable harness milestones, and issue decomposition for Atari 2600 |
| [Sega Genesis Emulation Roadmap](../~docs/plans/long-term-missing-platforms.md) | Feasibility, architecture constraints, and implementation staging for Genesis/Mega Drive |
| [Sega Genesis Architecture and Incremental Plan](../~docs/plans/genesis-architecture-and-incremental-plan.md) | Detailed phase plan, risks, and benchmark/correctness gates for Genesis support |
| [Platform Parity Research Index](../~docs/research/platform-parity/README.md) | Source-cited Atari 2600 and Genesis research tree with subsystem breakdowns |
| [Atari 2600 Research Breakdown](../~docs/research/platform-parity/atari-2600/README.md) | Tiny-section Atari 2600 subsystem docs and execution checklist |
| [Genesis Research Breakdown](../~docs/research/platform-parity/genesis/README.md) | Comparative Genesis subsystem docs with emulator code analysis |
| [Sonic 1 and Jurassic Park Compatibility Path](../~docs/research/platform-parity/compatibility/sonic1-jurassic-park.md) | Title-targeted compatibility milestones and harness gates |
| [Performance Improvement Plan](../~docs/plans/performance-improvement-plan.md) | Multi-phase emulation and audio performance optimization roadmap |
| [Hot Path Optimization Plan](../~docs/plans/hot-path-optimization-plan.md) | CPU/PPU hot-path micro-optimization plan and benchmark strategy |
| [Debugger Performance Optimization Plan](../~docs/plans/debugger-performance-optimization.md) | Debugger pipeline overhead reduction and benchmark-driven fixes |
| [Save State System Overhaul](../~docs/plans/save-state-system-overhaul.md) | Save-state architecture and reliability improvement planning |
| [TAS UI Comprehensive Plan](../~docs/plans/tas-ui-comprehensive-plan.md) | TAS editor and workflow modernization roadmap |
| [UI Modernization Plan](../~docs/plans/ui-modernization-plan.md) | Broader UI modernization and consistency tasks |
| [Documentation Restructure Plan](../~docs/plans/documentation-restructure-plan.md) | Documentation hierarchy and feature-first navigation roadmap |
| [Q2 2026 Future Work Program](../~docs/plans/future-work-program-2026-q2.md) | Milestones and acceptance criteria for platform-parity planning and execution |
| [Q3 2026 Platform Parity Research Program](../~docs/plans/platform-parity-research-program-2026-q3.md) | Multi-session research and execution cadence for issues #704-#707 |
| [Scaffold Future-Work Backlog](../~docs/plans/platform-parity-scaffold-future-work-backlog.md) | Deferred Atari/Genesis issue tree for post-scaffold implementation fill-in |
| [Harness Drift Comparator](HARNESS-DRIFT-COMPARATOR.md) | Baseline/candidate artifact comparison script and CI integration guide |
| [Epic 16 Closure Matrix](../~docs/plans/platform-parity-epic16-closure-matrix.md) | Issue-to-evidence traceability matrix for Epic 16 parity work |
| [Epic 16 Weekly Cadence](../~docs/plans/platform-parity-epic16-weekly-cadence.md) | Week-by-week execution cadence and triage checklist |

## Systems Documentation

| Document | Description |
|---|---|
| [Systems Index](systems/README.md) | Entry point for all system pages |
| [NES](systems/NES.md) | NES and Famicom usage and platform overview |
| [SNES](systems/SNES.md) | SNES and Super Famicom usage and platform overview |
| [GB](systems/GB.md) | Game Boy and Game Boy Color usage and platform overview |
| [GBA](systems/GBA.md) | Game Boy Advance usage and platform overview |
| [SMS](systems/SMS.md) | Sega Master System and Game Gear usage and platform overview |
| [Genesis](systems/Genesis.md) | Sega Genesis and Mega Drive usage, TAS parity, and benchmark runbook |
| [PCE](systems/PCE.md) | PC Engine and TurboGrafx-16 usage and platform overview |
| [WS](systems/WS.md) | WonderSwan and WonderSwan Color usage and platform overview |
| [Lynx](systems/Lynx.md) | Atari Lynx landing page and deep links |
| [ChannelF](systems/ChannelF.md) | Fairchild Channel F usage, TAS, and debugging overview |
| Atari 2600 | *Coming soon* |
| [Channel F](systems/ChannelF.md) | Fairchild Channel F landing page and deep links |

## Lynx Deep-Dive Documents

| Document | Description |
|---|---|
| [Lynx Emulation](LYNX-EMULATION.md) | Hardware and subsystem overview |
| [Lynx Accuracy Status](LYNX-ACCURACY.md) | Per-subsystem accuracy tracking |
| [Lynx Testing Guide](LYNX-TESTING.md) | Testing and benchmark workflows |
| [Lynx Hardware Bugs](LYNX-HARDWARE-BUGS.md) | Hardware quirk and bug behavior references |
| [Atari Lynx ROM Format](ATARI-LYNX-FORMAT.md) | .atari-lynx format specification |

## API Documentation

API documentation is generated using Doxygen from source comments.

### Generating Documentation

```bash
# From repository root
doxygen Doxyfile

# Output: docs/api/html/index.html
```

### Prerequisites

- Doxygen 1.9.0+
- Graphviz (optional, for diagrams)

### Installation

Windows:

- [Doxygen Downloads](https://www.doxygen.nl/download.html)
- [Graphviz Downloads](https://graphviz.org/download/)

macOS:

```bash
brew install doxygen graphviz
```

Linux:

```bash
sudo apt install doxygen graphviz
```

## Developer Documentation

For development and contribution documentation, see:

| Location | Description |
|---|---|
| [~docs/](../~docs/README.md) | Developer documentation index |
| [~docs/ARCHITECTURE-OVERVIEW.md](../~docs/ARCHITECTURE-OVERVIEW.md) | System architecture |
| [~docs/CPP-DEVELOPMENT-GUIDE.md](../~docs/CPP-DEVELOPMENT-GUIDE.md) | C++23 coding guide |
| [~docs/pansy-export-index.md](../~docs/pansy-export-index.md) | Pansy export docs |

## Related Files

| File | Description |
|---|---|
| [COMPILING.md](../COMPILING.md) | Build instructions |
| [FORMATTING.md](../FORMATTING.md) | Code style guide |
| [Doxyfile](../Doxyfile) | Doxygen configuration |
