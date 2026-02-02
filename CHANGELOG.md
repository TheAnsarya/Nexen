# Changelog

All notable changes to Nexen are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

#### TAS Editor

- **Piano Roll View** - Visual timeline for frame-by-frame input editing
- **Greenzone System** - Automatic savestates for instant seeking and rerecording
- **Input Recording** - Record gameplay with Append, Insert, and Overwrite modes
- **Branches** - Save and compare alternate movie versions
- **Lua Scripting** - Automate TAS workflows with comprehensive API
- **Multi-Format Support** - Import/export BK2, FM2, SMV, LSMV, VBM, GMV, NMV
- **Rerecord Tracking** - Monitor optimization progress

#### Save States

- **Infinite Save States** - Unlimited timestamped saves per game
- **Visual Picker** - Grid view with screenshots and timestamps
- **Per-Game Organization** - Saves stored in `SaveStates/{RomName}/` directories
- **Quick Save/Load** - Shift+F1 to save, F1 to browse

#### Debugging

- **🌼 Pansy Export** - Export disassembly metadata to universal format
- **File Sync** - Bidirectional sync with external label files

#### Technical

- **C++23 Modernization** - Smart pointers, std::format, ranges, [[likely]]/[[unlikely]]
- **Unit Tests** - 421+ tests using Google Test framework
- **Benchmarks** - Performance benchmarks with Google Benchmark
- **Doxygen Documentation** - Generated API documentation

### Changed

- Updated to .NET 10 and C++23
- Improved memory management with smart pointers
- Enhanced code documentation with XML comments

### Fixed

- Various memory leaks identified via AddressSanitizer
- Performance improvements in hot paths

## [1.0.0] - TBD

Initial public release of Nexen.

### Platforms

- Windows (x64)
- Linux (x64, ARM64, AppImage)
- macOS (Intel x64, Apple Silicon ARM64)

---

## Version History

Nexen is based on [Mesen](https://github.com/SourMesen/Mesen2) by Sour.

For the original Mesen changelog, see the [upstream repository](https://github.com/SourMesen/Mesen2/releases).
