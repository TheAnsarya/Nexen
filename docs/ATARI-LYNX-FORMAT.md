# `.atari-lynx` File Format Specification

**Version:** 0.1.0
**Status:** Draft
**Author:** Nexen Project

## Overview

The `.atari-lynx` format is a container format for Atari Lynx ROM data with rich
metadata. It replaces the limited LNX format (64-byte header, ASCII-only names)
and the metadata-less LYX/O formats with a modern, extensible design.

## Goals

- **Preserve all metadata** from LNX headers plus new fields (country, language,
  hardware model, orientation, handedness, multiplayer support)
- **UTF-8 strings** for international game titles and manufacturers
- **Integrity verification** via CRC32 of raw ROM data
- **Forward-compatible** via version field and TLV custom fields
- **Simple to parse** — fixed 32-byte header, optional metadata section

## File Extension

`.atari-lynx`

The extension is intentionally long and descriptive to avoid collision with
common programming extensions like `.o`.

## Format Layout

```text
+---------------------------+
| Fixed Header (32 bytes)   |
+---------------------------+
| ROM Data (variable)       |
+---------------------------+
| Metadata Section (opt.)   |
+---------------------------+
```

## Fixed Header (32 bytes)

All multi-byte integers are **little-endian**.

| Offset | Size | Type     | Field             | Description                                              |
|--------|------|----------|-------------------|----------------------------------------------------------|
| 0x00   | 8    | char[8]  | Magic             | `"LYNXROM\0"` (null-terminated, 8 bytes)                 |
| 0x08   | 1    | uint8    | VersionMajor      | Major version (0 for v0.1.0)                             |
| 0x09   | 1    | uint8    | VersionMinor      | Minor version (1 for v0.1.0)                             |
| 0x0a   | 1    | uint8    | VersionPatch      | Patch version (0 for v0.1.0)                             |
| 0x0b   | 1    | uint8    | Flags             | Bit 0: compressed, Bit 1: has-metadata-section           |
| 0x0c   | 4    | uint32   | RomSize           | Size of raw ROM data in bytes                            |
| 0x10   | 4    | uint32   | RomCrc32          | CRC32 of the raw ROM data                                |
| 0x14   | 4    | uint32   | TotalFileSize     | Total size of the entire file in bytes                   |
| 0x18   | 4    | uint32   | MetadataOffset    | Byte offset of metadata section from file start (0=none) |
| 0x1c   | 4    | uint32   | Reserved          | Must be 0                                                |

### Magic Bytes

The magic is always the 8-byte sequence: `4C 59 4E 58 52 4F 4D 00` (`"LYNXROM\0"`).
This distinguishes `.atari-lynx` files from LNX files (which start with `"LYNX"`).

### Version

The version uses semantic versioning (major.minor.patch). Readers should reject
files with a major version they don't understand. Minor/patch version changes are
backward-compatible.

### Flags

| Bit | Name                  | Description                                        |
|-----|-----------------------|----------------------------------------------------|
| 0   | FLAG_COMPRESSED       | ROM data is DEFLATE compressed (reserved, unused)  |
| 1   | FLAG_HAS_METADATA     | File has a metadata section at MetadataOffset      |
| 2-7 | Reserved              | Must be 0                                          |

## ROM Data

Immediately follows the 32-byte header. Contains raw ROM data (identical to
LYX format — no LNX header, just the game code/data). Size is specified by
the `RomSize` header field.

## Metadata Section

Present when `FLAG_HAS_METADATA` is set in Flags. Located at the byte offset
specified by `MetadataOffset`.

### Metadata Header

| Offset | Size | Type     | Field           | Description                              |
|--------|------|----------|-----------------|------------------------------------------|
| 0x00   | 4    | char[4]  | SectionMagic    | `"META"` (0x4d 0x45 0x54 0x41)           |
| 0x04   | 4    | uint32   | SectionSize     | Size of section data after this field    |

### Metadata Fields

Fields follow sequentially after the metadata header. All strings are
length-prefixed (uint16 length + UTF-8 data, no null terminator).

| Order | Size     | Type            | Field            | Description                                      |
|-------|----------|-----------------|------------------|--------------------------------------------------|
| 1     | 2+N      | string          | CartName         | Game title (UTF-8)                               |
| 2     | 2+N      | string          | Manufacturer     | Publisher/manufacturer (UTF-8)                   |
| 3     | 1        | uint8           | Rotation         | 0=none, 1=left, 2=right                         |
| 4     | 2        | uint16          | Bank0PageSize    | Bank 0 size in 256-byte pages (LE)               |
| 5     | 2        | uint16          | Bank1PageSize    | Bank 1 size in 256-byte pages (LE)               |
| 6     | 1        | uint8           | EepromType       | 0=none, 1=93C46, 2=93C56, 3=93C66, 4=93C76, 5=93C86 |
| 7     | 1        | uint8           | EepromFlags      | Bit 0: SD mode, Bit 1: 8-bit word organization   |
| 8     | 1        | uint8           | HardwareModel    | 0=auto, 1=Lynx I, 2=Lynx II                     |
| 9     | 2        | uint16          | YearOfRelease    | Year (LE), 0=unknown                             |
| 10    | 1        | uint8           | PlayerCount      | Number of players (1-4+), 0=unknown              |
| 11    | 1        | uint8           | Handedness       | 0=right, 1=left, 2=ambidextrous                 |
| 12    | 1        | uint8           | ComLynxSupport   | 0=no, 1=yes (serial multiplayer)                 |
| 13    | 2        | uint16          | CountryCode      | ISO 3166-1 numeric (LE), 0=unknown               |
| 14    | 2+N      | string          | Language         | ISO 639-1 code (e.g., "en"), empty if unknown    |
| 15    | 1        | uint8           | CustomFieldCount | Number of TLV custom fields following            |
| 16    | variable | CustomField[]   | CustomFields     | TLV-encoded custom fields                        |

### Custom Field (TLV)

| Size | Type   | Field | Description                    |
|------|--------|-------|--------------------------------|
| 2    | uint16 | Tag   | Custom field identifier (LE)   |
| 2    | uint16 | Size  | Data length in bytes (LE)      |
| N    | bytes  | Data  | Field data                     |

### String Encoding

All strings use the format: `uint16_le length` followed by `length` bytes of
UTF-8 data. Empty strings have length 0 and no data bytes.

## Conversion from LNX

When converting from LNX format:

1. Strip the 64-byte LNX header
2. ROM data = remaining bytes after header
3. Map LNX fields to metadata:
   - Cart name (bytes 10-41) → CartName (re-encode as UTF-8)
   - Manufacturer (bytes 42-57) → Manufacturer
   - Rotation (byte 58) → Rotation
   - Bank 0 pages (bytes 4-5) → Bank0PageSize
   - Bank 1 pages (bytes 6-7) → Bank1PageSize
   - EEPROM (byte 60, bits 0-3) → EepromType
   - EEPROM (byte 60, bits 6-7) → EepromFlags
4. Compute CRC32 of ROM data
5. Fill remaining metadata with defaults (0/empty)

## Conversion from LYX/O

When converting from headerless formats:

1. ROM data = entire file contents
2. Compute CRC32 → look up in game database for metadata
3. Infer bank sizes from ROM size:
   - Bank0PageSize = romSize / 256
   - Bank1PageSize = 0
4. Fill metadata from database if available, defaults otherwise

## Compatibility Notes

- Files with only the 32-byte header + ROM data (no metadata section) are valid
- Readers must tolerate unknown flags (bits 2-7) being set
- Readers must skip unknown custom field tags
- The format is NOT backward-compatible with LNX (different magic)

## Example

A 256 KB ROM (California Games) with metadata:

```text
Header (32 bytes):
  4C 59 4E 58 52 4F 4D 00   ; "LYNXROM\0"
  00 01 00                   ; version 0.1.0
  02                         ; flags: has-metadata
  00 00 04 00                ; ROM size: 262144 (0x40000)
  xx xx xx xx                ; CRC32 of ROM data
  xx xx xx xx                ; total file size
  20 00 04 00                ; metadata offset: 0x40020 (32 + 262144)
  00 00 00 00                ; reserved

ROM Data (262144 bytes):
  [raw game data]

Metadata Section:
  4D 45 54 41                ; "META"
  xx xx xx xx                ; section size
  10 00                      ; cart name length: 16
  43 61 6C 69 66 6F 72 6E   ; "Californ"
  69 61 20 47 61 6D 65 73   ; "ia Games"
  05 00                      ; manufacturer length: 5
  41 74 61 72 69             ; "Atari"
  00                         ; rotation: none
  00 04                      ; bank 0 pages: 1024
  00 00                      ; bank 1 pages: 0
  00                         ; EEPROM: none
  00                         ; EEPROM flags: none
  02                         ; hardware: Lynx II
  E5 07                      ; year: 1989
  04                         ; players: 4
  02                         ; handedness: ambidextrous
  01                         ; ComLynx: yes
  00 00                      ; country: unknown
  02 00 65 6E                ; language: "en"
  00                         ; custom fields: 0
```
