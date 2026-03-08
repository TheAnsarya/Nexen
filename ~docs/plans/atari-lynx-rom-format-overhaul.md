# Atari Lynx ROM Format Overhaul — Master Plan

## Status: Multi-Session Implementation

## Overview

Comprehensive overhaul of Atari Lynx ROM handling in Nexen:

1. Add missing file extensions to Open File dialog
2. Create new `.atari-lynx` container format with rich metadata
3. Build conversion libraries for `.lnx`/`.lyx`/`.o` ↔ `.atari-lynx`
4. Fix Lynx emulation (black screen on load)
5. Full test coverage with real ROM samples

## Problem Statement

### File Extension Issues

- **Open File dialog** is completely missing Lynx file extensions
- `.lnx`, `.lyx`, `.o` are not listed as loadable formats
- `.o` is a common extension in programming — ambiguous
- `.lyx` is undocumented in TOSEC but used by newer homebrew

### Format Limitations

- **LNX format** (64-byte header): Has basic metadata (bank sizes, rotation, EEPROM)
  but lacks firmware info, orientation details, multiplayer support, country/language
- **LYX format**: Raw ROM data, no header at all — metadata is lost
- **`.o` format**: cc65 linker output, raw ROM data, no header

### Emulation Issues

- Loading a Lynx ROM currently results in a **black screen**
- Need to investigate: boot ROM requirement, HLE fallback, initialization

## Research Findings

### Current File Extension Distribution (TOSEC Collection)

| Extension | Count | Format |
|-----------|-------|--------|
| `.lnx`   | 596   | LNX header (64 bytes) + ROM data |
| `.lyx`   | 12    | Raw ROM data (no header) |
| `.o`     | 3     | Raw ROM data (cc65 output) |
| `.bin`   | 4     | BIOS firmware files |

### LNX Header Format (64 bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0-3    | 4    | Magic: "LYNX" |
| 4-5    | 2    | Bank 0 page size (in 256-byte pages, LE) |
| 6-7    | 2    | Bank 1 page size (in 256-byte pages, LE) |
| 8-9    | 2    | Version |
| 10-41  | 32   | Cart name (null-terminated) |
| 42-57  | 16   | Manufacturer name (null-terminated) |
| 58     | 1    | Rotation (0=none, 1=left, 2=right) |
| 59     | 1    | AUDIN (reserved) |
| 60     | 1    | EEPROM type (bits 0-3: chip, bit 6: SD, bit 7: 8-bit org) |
| 61-63  | 3    | Reserved |

### LYX/O: Raw ROM Data

- No header, first bytes are game code/data
- No way to store metadata (bank sizes, rotation, EEPROM type)
- Emulators must guess bank configuration from ROM size

### Existing Nexen Lynx Code

- `LynxConsole::GetSupportedExtensions()`: `{ ".lnx", ".o" }` — missing `.lyx`
- `FileDialogHelper.cs`: No Lynx entry at all in the filter list
- `LynxConsole::LoadRom()`: Handles LNX header and raw headerless ROMs
- `LynxGameDatabase`: CRC32 lookup for headerless ROMs

## Session Breakdown

### Session 1 (Current): Planning + Quick Fixes

1. ✅ Research and analyze all formats
2. Create this master plan document
3. Create GitHub epic and all sub-issues
4. **Quick fix**: Add `.lnx`/`.lyx`/`.o` to Open File dialog
5. **Quick fix**: Add `.lyx` to `GetSupportedExtensions()`
6. Copy test ROM files to test data directory
7. Investigate black screen issue

### Session 2: `.atari-lynx` Format Design & Implementation

1. Design the `.atari-lynx` binary format specification
2. Implement `AtariLynxFormat.h/cpp` — reader/writer in C++ core
3. Implement C# conversion library (for UI tools)
4. Unit tests for format reading/writing
5. Round-trip tests (lnx → atari-lynx → lnx)

### Session 3: Conversion & Integration

1. Implement LNX → atari-lynx converter
2. Implement LYX/O → atari-lynx converter (with metadata inference)
3. Implement atari-lynx → LNX converter (for compatibility)
4. Auto-conversion on file load in `LynxConsole::LoadRom()`
5. Save/update `.atari-lynx` files from UI
6. Integration tests with real ROMs

### Session 4: Lynx Emulation Fixes

1. Diagnose black screen issue
2. Fix boot ROM / HLE initialization
3. Verify ROM loading pipeline end-to-end
4. Test with multiple games
5. Fix any warnings found

### Session 5: Polish & Documentation

1. Comprehensive test suite
2. Benchmarks for conversion
3. Update documentation
4. Update README
5. Final cleanup

## `.atari-lynx` Format Design (v0.1.0)

### Magic & Header

```text
Offset  Size  Field
------  ----  -----
0x00    8     Magic: "LYNXROM\0"
0x08    1     Major version (0)
0x09    1     Minor version (1)
0x0a    1     Patch version (0)
0x0b    1     Flags (bit 0: compressed, bit 1: has-metadata-section)
0x0c    4     ROM data size (uint32, LE)
0x10    4     ROM CRC32 (of raw ROM data, no headers)
0x14    4     Total file size (uint32, LE)
0x18    4     Metadata section offset (uint32, LE; 0 if none)
0x1c    4     Reserved (0)
------  ----  -----
Total:  32 bytes (fixed header)
```

### Metadata Section (variable length, at MetadataSectionOffset)

```text
Offset  Size  Field
------  ----  -----
0x00    4     Section magic: "META"
0x04    4     Section length (uint32, LE; bytes after this field)
0x08    2     Cart name length (uint16, LE)
0x0a    N     Cart name (UTF-8, not null-terminated)
...     2     Manufacturer length (uint16, LE)
...     N     Manufacturer (UTF-8)
...     1     Rotation (0=none, 1=left, 2=right)
...     1     Orientation (0=horizontal, 1=vertical)
...     1     Firmware type (0=standard, 1=custom)
...     1     Left/Right hand mode (0=right, 1=left, 2=ambidextrous)
...     1     Two-player serial support (0=no, 1=ComLynx)
...     2     Country code (ISO 3166-1 numeric, LE; 840=US)
...     2     Language code length (uint16, LE)
...     N     Language (ISO 639-1, e.g., "en")
...     2     Bank 0 page size (uint16, LE; in 256-byte pages)
...     2     Bank 1 page size (uint16, LE)
...     1     EEPROM type (0=none, 1=93C46, 2=93C56, 3=93C66, 4=93C76, 5=93C86)
...     1     EEPROM flags (bit 0: SD mode, bit 1: 8-bit org)
...     1     Hardware model (0=auto, 1=Lynx I, 2=Lynx II)
...     2     Year of release (uint16, LE; 0=unknown)
...     1     Number of custom fields (uint8)
...     ...   Custom fields (TLV: uint16 tag, uint16 length, data)
```

### ROM Data

- Immediately after the 32-byte header
- Raw ROM data (same as LYX/O format — no LNX header)
- Size specified in header field

### Design Decisions

1. **UTF-8 strings** with explicit lengths (not null-terminated) — supports Unicode
2. **Version triplet** for future-proofing
3. **CRC32 of raw ROM** for integrity verification
4. **Metadata section** is optional — can store just ROM + header
5. **Custom fields** via TLV for extensibility
6. **No compression initially** — flag reserved for future use

## Sub-Issues

### Epic: Atari Lynx ROM Format Overhaul

**Phase 1 — File Extensions (Session 1):**

- [ ] Add `.lnx`/`.lyx`/`.o` to Open File dialog filter
- [ ] Add `.lyx` to `LynxConsole::GetSupportedExtensions()`
- [ ] Add Lynx entry in C# `FileDialogHelper.cs`

**Phase 2 — `.atari-lynx` Format (Session 2):**

- [ ] Write format specification document
- [ ] Implement C++ reader/writer (`AtariLynxFormat.h/cpp`)
- [ ] Implement C# reader/writer (for UI/tools)
- [ ] Unit tests for format I/O
- [ ] Round-trip tests

**Phase 3 — Conversion Libraries (Session 3):**

- [ ] LNX → atari-lynx converter
- [ ] LYX/O → atari-lynx converter
- [ ] atari-lynx → LNX converter
- [ ] Auto-convert on load in `LynxConsole::LoadRom()`
- [ ] UI save/update for atari-lynx files
- [ ] Integration tests with real ROMs

**Phase 4 — Emulation Fixes (Session 4):**

- [ ] Diagnose black screen on Lynx ROM load
- [ ] Fix boot ROM / HLE initialization
- [ ] Fix any warnings
- [ ] End-to-end testing

**Phase 5 — Polish (Session 5):**

- [ ] Comprehensive test suite
- [ ] Conversion benchmarks
- [ ] Documentation updates
- [ ] README updates

## Test ROM Files

### Files to Copy for Testing

From `D:\Roms\TOSEC\Atari\Lynx\`:

**LNX format (with header):**

- `Atari Lynx II Production Test v0.02 (1989)(Atari).lnx` — 262,208 bytes
- `256 Color Demo (199x)(PD).lnx` — ~12 KB (small)

**LYX format (headerless):**

- `Banana Ghost v0.1 (2021-12-11)(Bjorn).lyx` — 23,005 bytes
- `Black Pit (2021-12-12)(Fadest).lyx` — 35,668 bytes

**O format (headerless cc65):**

- `Banana Ghost v0.1 (2021-12-11)(Bjorn).o` — 22,804 bytes

**Destination:** `Core.Tests/TestData/Lynx/` (gitignored, not committed)

## Dependencies

- Requires: Existing Lynx emulation core
- Affects: `FileDialogHelper.cs`, `LynxConsole.h/cpp`, `LynxTypes.h`
- New files: `AtariLynxFormat.h/cpp`, test files, format spec doc
