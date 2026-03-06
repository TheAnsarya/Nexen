# Nexen Labels Format — Design & Migration Plan

## Overview

Replace the legacy MLB (Mesen Label Binary) text format with a new binary `.nexen-labels` format as the **native** label storage format. MLB becomes a legacy import/export-only format, accessible from a dedicated conversion menu.

### Why Replace MLB?

| Problem | Impact |
|---------|--------|
| **No file header / magic bytes** | Can't detect corruption or wrong file type |
| **No version field** | Can't evolve the format without breaking old files |
| **No ROM metadata** | Labels not tied to specific ROM → silent misapplication |
| **CodeLabelFlags lost** | AutoJumpLabel flag silently dropped on round-trip |
| **Plain text** | Larger files, no compression, slow parsing for large label sets |
| **No integrity check** | Can't detect truncation or corruption |
| **Hex address only** | No structured encoding; parsing is regex/split-based |

---

## Format Specification: `.nexen-labels` v1.0

### File Structure

```text
┌─────────────────────────────────────────────┐
│ Header (32 bytes, fixed)                    │
├─────────────────────────────────────────────┤
│ ROM Metadata Section                        │
├─────────────────────────────────────────────┤
│ Label Data Section                          │
├─────────────────────────────────────────────┤
│ Footer (8 bytes)                            │
└─────────────────────────────────────────────┘
```

### Header (32 bytes)

| Offset | Size | Type   | Description |
|--------|------|--------|-------------|
| 0x00   | 8    | ASCII  | Magic: `NXLABEL\0` |
| 0x08   | 2    | UInt16 | Format version (0x0100 = v1.0) |
| 0x0a   | 1    | Byte   | Platform / ConsoleType enum value |
| 0x0b   | 1    | Byte   | Flags (bit 0 = compressed, bits 1-7 reserved) |
| 0x0c   | 4    | UInt32 | ROM CRC32 (standard big-endian via RomHashService) |
| 0x10   | 4    | UInt32 | Label count |
| 0x14   | 4    | UInt32 | Data section offset (from file start) |
| 0x18   | 4    | UInt32 | Data section size (uncompressed) |
| 0x1c   | 4    | UInt32 | Reserved (0x00000000) |

All multi-byte values are **little-endian** (matches .NET `BinaryWriter` default).

### ROM Metadata Section (variable)

Immediately after header. Contains UTF-8 strings for ROM identification:

| Field | Encoding |
|-------|----------|
| ROM name | Length-prefixed UTF-8 (UInt16 length + bytes) |
| ROM filename | Length-prefixed UTF-8 (UInt16 length + bytes) |

### Label Data Section

If header flag bit 0 is set, this section is GZip-compressed. Otherwise raw.

Each label entry:

| Size | Type   | Description |
|------|--------|-------------|
| 4    | UInt32 | Address |
| 4    | UInt32 | Length (byte count, default 1) |
| 2    | UInt16 | MemoryType enum value |
| 2    | UInt16 | CodeLabelFlags enum value |
| 2+N  | String | Label name (UInt16 length + UTF-8 bytes) |
| 2+N  | String | Comment (UInt16 length + UTF-8 bytes) |

### Footer (8 bytes)

| Offset | Size | Type   | Description |
|--------|------|--------|-------------|
| 0x00   | 4    | UInt32 | CRC32 of everything before footer |
| 0x04   | 4    | ASCII  | End marker: `NXLE` |

### Design Decisions

1. **Binary over JSON/text** — Labels can be 10,000+ entries. Binary is ~3-5x smaller, ~10x faster to parse.
2. **CRC32 integrity** — Detect corruption/truncation. Same CRC32 as StreamHash uses.
3. **ROM CRC32 in header** — Can warn if labels don't match loaded ROM.
4. **Version field** — Future versions can add fields after reserved bytes.
5. **GZip optional** — Large label sets (>1000) benefit; small sets don't need it.
6. **Flags preserved** — `CodeLabelFlags` serialized in every entry. No more data loss.
7. **Little-endian** — Matches BinaryWriter default, no byte-swapping needed.

---

## Migration Plan

### Phase 1: Implement Format (Core)

**New file: `NexenLabelFormat.cs`** — Binary reader/writer for `.nexen-labels`

- `Write(string path, List<CodeLabel> labels, RomInfo romInfo, bool compress = false)`
- `Read(string path) → (List<CodeLabel> labels, NexenLabelHeader header)`
- `ReadHeader(string path) → NexenLabelHeader` (quick peek without full parse)
- `ValidateRomMatch(string path, RomInfo romInfo) → bool`

**Modify: `NexenLabelFile.cs`** — Becomes the unified entry point

- `Import(path, showResult)` checks extension:
  - `.nexen-labels` → delegates to `NexenLabelFormat.Read()`
  - `.mlb` → uses existing text parser (legacy)
- `Export(path)` always writes `.nexen-labels` binary format
- New: `ExportLegacyMlb(path)` for explicit MLB export
- New: `ImportLegacyMlb(path, showResult)` for explicit MLB import

### Phase 2: Update Storage Layer

**Modify: `DebugFolderManager.cs`**

| Change | Before | After |
|--------|--------|-------|
| Filename constant | `MlbFilename = "labels.mlb"` | `LabelsFilename = "labels.nexen-labels"` |
| Path method | `GetMlbPath()` | `GetLabelsPath()` |
| Export | Calls `NexenLabelFile.Export(mlbPath)` | Calls `NexenLabelFile.Export(labelsPath)` |
| Import | Reads `MlbFilename` | Reads `LabelsFilename`, falls back to `labels.mlb` for migration |
| Doc comments | References MLB | References nexen-labels |

**Modify: `SyncManager.cs`**

| Change | Before | After |
|--------|--------|-------|
| File relevance | `".mlb"` | `".nexen-labels"` (also accept `.mlb` for migration) |
| Watch comment | "Pansy, MLB, and CDL" | "Pansy, Labels, and CDL" |
| Auto-import | `case ".mlb"` | `case ".nexen-labels"` |
| Force sync | `GetMlbPath()` | `GetLabelsPath()` |
| Config check | `config.AutoLoadMlbFiles` | `config.AutoLoadLabelFiles` |

### Phase 3: Update Config & UI

**Modify: `IntegrationConfig.cs`**

| Before | After | Note |
|--------|-------|------|
| `AutoLoadMlbFiles` | `AutoLoadLabelFiles` | Rename property |
| `SyncMlbFiles` | `SyncLabelFiles` | Rename property |

**Modify: `DebuggerConfigWindow.axaml`**

| Before | After |
|--------|-------|
| `{Binding Integration.AutoLoadMlbFiles}` | `{Binding Integration.AutoLoadLabelFiles}` |
| `{l:Translate chkAutoLoadMlbFiles}` | `{l:Translate chkAutoLoadLabelFiles}` |
| `{Binding Integration.SyncMlbFiles}` | `{Binding Integration.SyncLabelFiles}` |
| `{l:Translate chkSyncMlbFiles}` | `{l:Translate chkSyncLabelFiles}` |

**Modify: `resources.en.xml`**

| Before | After |
|--------|-------|
| `chkAutoLoadMlbFiles` → ".MLB files (Nexen label file)" | `chkAutoLoadLabelFiles` → ".nexen-labels files (Nexen label file)" |
| `chkSyncMlbFiles` → "Sync MLB label files" | `chkSyncLabelFiles` → "Sync label files" |

### Phase 4: Update File Loading

**Modify: `DebugWorkspaceManager.cs`**

- `AutoLoadMlbFiles` → `AutoLoadLabelFiles` in auto-load check
- Keep `MesenLabelExt` fallback in `LoadSupportedFile` switch for drag-drop/import
- `LoadNexenLabelFile` accepts both extensions (already does)

**Modify: `FileDialogHelper.cs`**

- Keep `MesenLabelExt = "mlb"` for legacy import dialogs
- Import filter already includes both `*.nexen-labels` and `*.mlb` ✓
- Export default: `NexenLabelExt` only (no `.mlb` in save dialog)

### Phase 5: Update Converter

**Modify: `DbgToPansyConverter.cs`**

- Rename `NexenMlb` enum → `NexenLabels`
- `.mlb` case → still detected, maps to `NexenLabels` (legacy import)
- Add `.nexen-labels` case
- Rename `ImportNexenMlb()` → `ImportNexenLabels()`

### Phase 6: Legacy Import/Export Menu

Add menu entries for legacy format conversion:

- **Debug menu → "Export Labels as Legacy MLB..."** — Calls `NexenLabelFile.ExportLegacyMlb()`
- **Debug menu → "Import Legacy MLB File..."** — Calls `NexenLabelFile.ImportLegacyMlb()`

These are for interop with other Mesen-based emulators.

---

## File Change Inventory

| File | Changes |
|------|---------|
| **NEW** `UI/Debugger/Labels/NexenLabelFormat.cs` | Binary format reader/writer (~200 lines) |
| `UI/Debugger/Labels/NexenLabelFile.cs` | Extension-based dispatch, legacy MLB methods |
| `UI/Debugger/Labels/CodeLabel.cs` | Add `WriteBinary(BinaryWriter)` / `ReadBinary(BinaryReader)` |
| `UI/Debugger/Labels/DebugFolderManager.cs` | Rename constants/methods, migration fallback |
| `UI/Debugger/Labels/SyncManager.cs` | Update extension checks, config property names |
| `UI/Config/IntegrationConfig.cs` | Rename 2 properties |
| `UI/Debugger/Windows/DebuggerConfigWindow.axaml` | Update 4 bindings + translate keys |
| `UI/Localization/resources.en.xml` | Update 2 control strings |
| `UI/Debugger/Utilities/DebugWorkspaceManager.cs` | Update config property references, doc comments |
| `UI/Debugger/Integration/DbgToPansyConverter.cs` | Rename enum + method, add extension case |
| `UI/Utilities/FileDialogHelper.cs` | Keep both extensions (already good) |
| `Tests/Debugger/Labels/DbgToPansyConverterTests.cs` | Update enum references |

**Total: 1 new file, 11 modified files**

---

## Backward Compatibility

1. **Import**: `.nexen-labels` AND `.mlb` both importable (auto-detected by extension)
2. **Auto-load**: Tries `.nexen-labels` first, falls back to `.mlb` in same folder
3. **Debug folder**: Reads `labels.mlb` if `labels.nexen-labels` doesn't exist (migration)
4. **Export**: Always writes `.nexen-labels`. Legacy MLB only via explicit menu action.
5. **Config migration**: Old `AutoLoadMlbFiles=true` → new `AutoLoadLabelFiles=true` (same default)

---

## GitHub Issues

- **Epic**: `[Epic 199] Nexen Labels Binary Format`
- `[199.1]` Implement NexenLabelFormat binary reader/writer
- `[199.2]` Add binary serialization to CodeLabel
- `[199.3]` Migrate DebugFolderManager from MLB to nexen-labels
- `[199.4]` Migrate SyncManager from MLB to nexen-labels  
- `[199.5]` Rename IntegrationConfig MLB properties
- `[199.6]` Update UI (AXAML bindings, localization strings)
- `[199.7]` Update DebugWorkspaceManager and DbgToPansyConverter
- `[199.8]` Add legacy MLB import/export menu entries
