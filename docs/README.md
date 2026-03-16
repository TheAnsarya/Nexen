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
| [TAS Developer Guide](TAS-Developer-Guide.md) | Technical internals for TAS extensions |
| [TAS Architecture](TAS_ARCHITECTURE.md) | TAS and movie system architecture |
| [Movie Format Spec](NEXEN_MOVIE_FORMAT.md) | .nexen-movie file format specification |
| [Performance Guide](PERFORMANCE.md) | Consolidated performance references and validation workflow |
| [Release Guide](RELEASE.md) | Release and publishing workflow |

## Systems Documentation

| Document | Description |
|---|---|
| [Systems Index](systems/README.md) | Entry point for all system pages |
| [NES](systems/NES.md) | NES and Famicom usage and platform overview |
| [SNES](systems/SNES.md) | SNES and Super Famicom usage and platform overview |
| [GB](systems/GB.md) | Game Boy and Game Boy Color usage and platform overview |
| [GBA](systems/GBA.md) | Game Boy Advance usage and platform overview |
| [SMS](systems/SMS.md) | Sega Master System and Game Gear usage and platform overview |
| [PCE](systems/PCE.md) | PC Engine and TurboGrafx-16 usage and platform overview |
| [WS](systems/WS.md) | WonderSwan and WonderSwan Color usage and platform overview |
| [Lynx](systems/Lynx.md) | Atari Lynx landing page and deep links |

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
