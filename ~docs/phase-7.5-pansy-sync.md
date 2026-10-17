# Phase 7.5: Pansy Sync & Unified Debug Data Storage

**Created:** 2026-01-27  
**Status:** Audited and reconciled (2026-03-16)  
**Branch:** master

## Overview

Create a unified debug data storage system where each ROM has a dedicated folder containing:

1. **Pansy file** (`.pansy`) - Universal metadata format
2. **Nexen Label file** (`.nexen-labels`, legacy `.mlb` fallback) - Native Nexen labels
3. **CDL file** (`.cdl`) - Code Data Logger data
4. **Debug info** (`.dbg`) - Extended debug information

These files are kept in sync bidirectionally:

- Changes to MLB/CDL update the Pansy file
- Pansy file can be imported back to restore MLB/CDL data

## Data Mapping: Pansy ↔ Nexen Formats

### Pansy Format Capabilities

| Pansy Section | Description | Nexen Equivalent |
| --------------- | ------------- | ------------------ |
| CODE_DATA_MAP | Code/Data flags | CDL file (`.cdl`) |
| SYMBOLS | Labels with addresses | MLB file (`.mlb`) |
| COMMENTS | Per-address comments | MLB file comments |
| JUMP_TARGETS | Branch destinations | CDL JumpTarget flag |
| SUB_ENTRY_POINTS | Function entries | CDL SubEntryPoint flag |
| MEMORY_REGIONS | Named regions | No native equivalent |
| CROSS_REFS | Call graph | Derived from analysis |
| DATA_BLOCKS | Data tables | No native equivalent |
| BOOKMARKS | User bookmarks | Future: bookmark storage |
| WATCH_ENTRIES | Watch expressions | Future: watch storage |

### Nexen Format Details

#### MLB (Nexen Label File)

```text
Format: MemoryType:Address[-EndAddress]:Label[:Comment]
Example: NesPrgRom:8000:Reset:Main entry point
Example: NesInternalRam:0010-001F:PlayerData
```

Contains:

- Symbol names (labels)
- Memory type/address
- Optional comments
- Multi-byte label ranges

#### CDL (Code Data Logger)

```text
Format: Raw byte array, one byte per ROM byte
Flags:
  0x01 = Code
  0x02 = Data  
  0x04 = JumpTarget
  0x08 = SubEntryPoint
```

Contains:

- Code/Data classification
- Jump targets
- Subroutine entry points

#### DBG (Debug Files from ca65, cc65, etc.)

External debug info format - Nexen can import but doesn't export.

## Folder Structure

```text
[Nexen Data Directory]/
├── Debug/
│   ├── [RomName_CRC32]/           # Per-ROM folder
│   │   ├── metadata.pansy       # Pansy universal format
│   │   ├── labels.nexen-labels  # Nexen native labels (legacy labels.mlb still supported)
│   │   ├── coverage.cdl           # Code Data Logger
│   │   ├── config.json         # Per-ROM config
│   │   └── history/               # Optional: version history
│   │      ├── 2026-01-27_120000.pansy
│   │      └── ...
│   └── [AnotherRom_CRC32]/
│      └── ...
```

### Naming Convention

Folder name: `{RomBaseName}_{CRC32}`

- `Super Mario Bros. (USA)_A2B3C4D5/`
- `Zelda - A Link to the Past (USA)_12345678/`

## Sync Mechanism

### Export Flow (Nexen → Pansy)

```text
┌────────────────────────────────────────────────────────────────┐
│                     Export Trigger                            │
│  (Manual export, Auto-save timer, ROM unload, Label change)   │
└─────────────────────────┬──────────────────────────────────────┘
                          │
                          ▼
┌────────────────────────────────────────────────────────────────┐
│                  Collect Current State                        │
│  - LabelManager.GetAllLabels()                                 │
│  - DebugApi.GetCdlData()                                     │
│  - DebugApi.GetBreakpoints()  (future)                         │
│  - DebugApi.GetWatchList()    (future)                         │
└─────────────────────────┬──────────────────────────────────────┘
                          │
                          ▼
┌────────────────────────────────────────────────────────────────┐
│              Write to ROM Folder                            │
│  1. Create folder if not exists                               │
│  2. Write metadata.pansy                                     │
│  3. Export labels.mlb (native backup)                       │
│  4. Export coverage.cdl (native backup)                       │
│  5. Update config.json with timestamp                       │
└────────────────────────────────────────────────────────────────┘
```

### Import Flow (Pansy → Nexen)

```text
┌────────────────────────────────────────────────────────────────┐
│                     Import Trigger                            │
│  (Manual import, ROM load with AutoLoad, User request)        │
└─────────────────────────┬──────────────────────────────────────┘
                          │
                          ▼
┌────────────────────────────────────────────────────────────────┐
│                  Load Pansy File                            │
│  - Read header, verify CRC32                                 │
│  - Parse all sections                                       │
└─────────────────────────┬──────────────────────────────────────┘
                          │
                          ▼
┌────────────────────────────────────────────────────────────────┐
│             Apply to Nexen State                            │
│  - SYMBOLS → LabelManager.SetLabels()                       │
│  - COMMENTS → CodeLabel.Comment                               │
│  - CODE_DATA_MAP → DebugApi.SetCdlData()                     │
│  - (Optionally restore breakpoints, watches)                 │
└────────────────────────────────────────────────────────────────┘
```

## Audit Results (2026-03-16)

The original "Issue #11-#15" labels in this document were planning identifiers, not accurate GitHub issue references. They conflicted with real early repository issue numbers and are removed here.

### Workstream 7.5a: Folder-Based Debug Storage

Status: Complete

- [x] `DebugFolderManager` implemented
- [x] Folder naming convention implemented (`{RomName}_{CRC32}` via `GameDataManager`)
- [x] Folder creation on first save implemented
- [x] Auto-export path uses folder storage when enabled
- [x] Config option for debug folder location implemented (`DebugFolderPath`)
- [x] ROM name sanitization for folder safety implemented

Implemented files:

- `UI/Debugger/Labels/DebugFolderManager.cs`
- `UI/Config/IntegrationConfig.cs`
- `UI/Debugger/Labels/BackgroundPansyExporter.cs`

### Workstream 7.5b: Label File Sync (.nexen-labels / legacy .mlb)

Status: Complete

- [x] Label export alongside Pansy implemented (`labels.nexen-labels`)
- [x] Legacy `.mlb` fallback import supported
- [x] Auto-load label import on ROM load implemented
- [x] External label file change detection path implemented via `SyncManager`

Implemented files:

- `UI/Debugger/Labels/NexenLabelFile.cs`
- `UI/Debugger/Labels/DebugFolderManager.cs`
- `UI/Debugger/Labels/SyncManager.cs`

### Workstream 7.5c: CDL Sync

Status: Complete

- [x] CDL export alongside Pansy implemented (`coverage.cdl`)
- [x] Auto-load CDL on ROM load implemented (config-gated)
- [x] External CDL change import implemented via `SyncManager`
- [x] ROM-size validation on import implemented

Implemented files:

- `UI/Debugger/Labels/DebugFolderManager.cs`
- `UI/Debugger/Labels/SyncManager.cs`

### Workstream 7.5d: DBG Integration

Status: Complete

- [x] DBG importer support implemented
- [x] Conversion pipeline to Pansy implemented
- [x] Multiple debug format detection/import paths implemented (`.dbg`, `.sym`, `.elf`, `.cdb`, label files)

Implemented files:

- `UI/Debugger/Integration/DbgImporter.cs`
- `UI/Debugger/Integration/DbgToPansyConverter.cs`

### Workstream 7.5e: Bidirectional Sync Manager

Status: Core complete, advanced conflict UX deferred

- [x] File change detection implemented (`FileSystemWatcher`)
- [x] Change queue + debounce processing implemented
- [x] Change notification and sync status events implemented
- [x] Manual force sync path implemented (`ForceSyncAsync`)
- [ ] Conflict resolution dialog UI not implemented
- [ ] Merge-strategy UX (ours/theirs/merge) not implemented
- [ ] Undo/redo workflow not implemented

Implemented files:

- `UI/Debugger/Labels/SyncManager.cs`
- `UI/Debugger/Labels/PansyFileWatcher.cs`

Removed stale planning entry:

- `UI/Debugger/Windows/SyncConflictDialog.axaml` (never added in repository)

## Configuration (Current)

```csharp
// IntegrationConfig.cs (current Phase 7.5-related options)
public sealed class IntegrationConfig : BaseConfig<IntegrationConfig> {
    [Reactive] public bool UseFolderStorage { get; set; } = true;
    [Reactive] public bool SyncLabelFiles { get; set; } = true;
    [Reactive] public bool SyncCdlFiles { get; set; } = true;
    [Reactive] public bool KeepVersionHistory { get; set; } = false;
    [Reactive] public int MaxHistoryEntries { get; set; } = 10;
    [Reactive] public string DebugFolderPath { get; set; } = "";

    [Reactive] public bool EnableFileWatching { get; set; } = false;
    [Reactive] public bool AutoReloadOnExternalChange { get; set; } = true;
}
```

## Success Criteria (Reconciled)

1. [x] Each ROM has dedicated debug folder
2. [x] Pansy metadata export uses folder storage when enabled
3. [x] Label and CDL companion files export/import with config gates
4. [x] External file watching and re-import pipeline exists
5. [x] Legacy `.mlb` compatibility retained while native `.nexen-labels` is primary
6. [ ] Interactive conflict resolution/merge UI (deferred)
