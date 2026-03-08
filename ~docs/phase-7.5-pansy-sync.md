# Phase 7.5: Pansy Sync & Unified Debug Data Storage

**Created:** 2026-01-27  
**Status:** Planning → Implementation  
**Branch:** pansy-export

## Overview

Create a unified debug data storage system where each ROM has a dedicated folder containing:

1. **Pansy file** (`.pansy`) - Universal metadata format
2. **Nexen Label file** (`.mlb`) - Native Nexen labels
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
│   │   ├── labels.mlb           # Nexen native labels
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

## Implementation Plan

### Issue #11: 📁 Folder-Based Debug Storage - Phase 7.5a

**Estimate:** 6 hours

**Tasks:**

- [ ] Create `DebugFolderManager` class
- [ ] Implement folder naming convention
- [ ] Create folder on first save
- [ ] Migrate existing auto-save to folder structure
- [ ] Add config option for debug folder location
- [ ] Handle special characters in ROM names

**Files:**

- NEW: `UI/Debugger/Labels/DebugFolderManager.cs`
- MOD: `UI/Config/IntegrationConfig.cs`
- MOD: `UI/Debugger/Labels/BackgroundPansyExporter.cs`

### Issue #12: 🔄 MLB Sync - Phase 7.5b

**Estimate:** 4 hours

**Tasks:**

- [ ] Export MLB alongside Pansy file
- [ ] Import MLB when loading ROM (existing)
- [ ] Detect MLB changes and update Pansy
- [ ] Round-trip test: Pansy → MLB → Pansy
- [ ] Handle label format differences

**Files:**

- MOD: `UI/Debugger/Labels/NexenLabelFile.cs`
- MOD: `UI/Debugger/Labels/PansyExporter.cs`

### Issue #13: 📊 CDL Sync - Phase 7.5c

**Estimate:** 4 hours

**Tasks:**

- [ ] Export CDL alongside Pansy file
- [ ] Auto-load CDL if config enabled
- [ ] Sync CDL flags to/from Pansy CODE_DATA_MAP
- [ ] Handle different ROM sizes
- [ ] Verify flag mapping accuracy

**Files:**

- NEW: `UI/Debugger/Labels/CdlFileManager.cs`
- MOD: `UI/Debugger/Labels/PansyExporter.cs`

### Issue #14: 📝 DBG Integration - Phase 7.5d

**Estimate:** 6 hours

**Tasks:**

- [ ] Import DBG files to Pansy format
- [ ] Preserve source file references
- [ ] Import symbol definitions
- [ ] Import scope information
- [ ] Handle assembler-specific formats (ca65, WLA-DX, etc.)

**Files:**

- MOD: `UI/Debugger/Integration/DbgImporter.cs`
- NEW: `UI/Debugger/Integration/DbgToPansyConverter.cs`

### Issue #15: 🔗 Bidirectional Sync Manager - Phase 7.5e

**Estimate:** 8 hours

**Tasks:**

- [ ] Detect file changes (FileSystemWatcher)
- [ ] Conflict resolution UI
- [ ] Merge strategies (ours, theirs, merge)
- [ ] Change notification system
- [ ] Undo/redo support
- [ ] Sync status indicator in UI

**Files:**

- NEW: `UI/Debugger/Labels/SyncManager.cs`
- NEW: `UI/Debugger/Windows/SyncConflictDialog.axaml`

## Configuration

```csharp
// IntegrationConfig.cs additions
public class IntegrationConfig : BaseConfig<IntegrationConfig> {
    // Existing...
    
    // Phase 7.5: Sync options
    [Reactive] public bool UseFolderStorage { get; set; } = true;
    [Reactive] public bool SyncMlbFiles { get; set; } = true;
    [Reactive] public bool SyncCdlFiles { get; set; } = true;
    [Reactive] public bool KeepVersionHistory { get; set; } = false;
    [Reactive] public int MaxHistoryEntries { get; set; } = 10;
    [Reactive] public string DebugFolderPath { get; set; } = "";  // Empty = default location
}
```

## API Surface

### DebugFolderManager

```csharp
public static class DebugFolderManager {
    // Get the debug folder for a ROM
    public static string GetRomDebugFolder(RomInfo romInfo);
    
    // Ensure folder exists
    public static void EnsureFolderExists(RomInfo romInfo);
    
    // Get paths for specific files
    public static string GetPansyPath(RomInfo romInfo);
    public static string GetMlbPath(RomInfo romInfo);
    public static string GetCdlPath(RomInfo romInfo);
    
    // Export all files
    public static void ExportAllFiles(RomInfo romInfo, MemoryType memType);
    
    // Import from folder
    public static void ImportFromFolder(RomInfo romInfo);
}
```

### SyncManager

```csharp
public class SyncManager : IDisposable {
    public event EventHandler<SyncEventArgs>? OnSyncRequired;
    public event EventHandler<ConflictEventArgs>? OnConflictDetected;
    
    public void StartWatching(string folderPath);
    public void StopWatching();
    
    public SyncResult SyncToNexen(string pansyPath);
    public SyncResult SyncFromNexen(string pansyPath);
    
    public ConflictResolution ResolveConflict(Conflict conflict);
}
```

## Testing Plan

1. **Unit Tests:**
   - Folder naming sanitization
   - Path generation for various ROMs
   - MLB/CDL round-trip accuracy

2. **Integration Tests:**
   - Full export/import cycle
   - Multi-ROM folder isolation
   - Concurrent access handling

3. **Manual Tests:**
   - Load ROM → Add labels → Close → Reload → Verify
   - External MLB edit → Sync → Verify in Nexen
   - Conflict scenario testing

## Timeline

| Task | Estimate | Phase |
| ------ | ---------- | ------- |
| Folder Storage | 6h | 7.5a |
| MLB Sync | 4h | 7.5b |
| CDL Sync | 4h | 7.5c |
| DBG Integration | 6h | 7.5d |
| Sync Manager | 8h | 7.5e |
| Testing | 4h | 7.5f |
| **Total** | **32h** |  |

## Success Criteria

1. ✅ Each ROM has dedicated debug folder
2. ✅ Pansy file contains all MLB data
3. ✅ Pansy file contains all CDL data
4. ✅ MLB/CDL files exported alongside Pansy
5. ✅ Changes to any file sync to others
6. ✅ No data loss in round-trip
7. ✅ Works with all Nexen-supported platforms
