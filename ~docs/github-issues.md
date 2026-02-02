# Pansy Export - GitHub Issues

**Created:** 2026-01-19 18:48 UTC  
**Last Updated:** 2026-01-27 10:00 UTC  
**Repository:** TheAnsarya/Nexen  
**Branch:** pansy-export

## Issues to Create

### Issue #1: 🎯 Pansy Metadata Export - Phase 1 Implementation

**Status:** ✅ COMPLETED  
**Labels:** enhancement, debugger, pansy, phase-1  
**Milestone:** Pansy Integration v1.0  
**Created:** 2026-01-19 18:48 UTC  
**Closed:** 2026-01-19 18:43 UTC

**Description:**
Implement core Pansy binary format export functionality for Nexen debugger.

**Requirements:**

- [x] Binary format writer (Pansy v1.0 specification)
- [x] Platform ID mapping for all Nexen systems
- [x] Symbol export (CodeLabel → binary)
- [x] Comment export (per-address annotations)
- [x] Code/Data map export (CDL flags)
- [x] Jump target extraction
- [x] Subroutine entry point export
- [x] Debugger menu integration
- [x] Auto-export configuration option
- [x] Localization strings

**Implementation:**

- File: `UI/Debugger/Labels/PansyExporter.cs` (369 lines)
- Commit: 6fd99def
- Branch: pansy-export

**Sections Exported:**

1. CODE_DATA_MAP (0x0001) - CDL flags as byte array
2. SYMBOLS (0x0002) - Address → name mappings
3. COMMENTS (0x0003) - Address → comment text
4. JUMP_TARGETS (0x0007) - Branch destinations
5. SUB_ENTRY_POINTS (0x0008) - Function entry points

**Testing:** Deferred to Issue #2 (Phase 2)

---

### Issue #1.5: 🔒 Background CDL Recording & ROM Hash Verification - Phase 1.5

**Status:** ✅ COMPLETED  
**Labels:** enhancement, reliability, phase-1.5, critical  
**Milestone:** Pansy Integration v1.0  
**Completed:** 2026-01-26 09:00 UTC  
**Commits:** `38dfccf7`, `e95a7858`

**Description:**
Two critical reliability features for Pansy export:

1. Background CDL recording without requiring debugger window
2. ROM CRC32 verification to prevent Pansy file corruption

**Background CDL Recording - Completed:**

- [x] Enable CDL recording without opening debugger
- [x] Auto-start recording on ROM load (if config enabled)
- [x] Periodic auto-save (configurable interval, default 5 min)
- [x] Save on ROM unload
- [x] Background timer with thread-safe UI updates
- [x] New file: `BackgroundPansyExporter.cs` (160 lines)
- [x] Hook into MainWindow GameLoaded/EmulationStopped

**ROM Hash Verification - Completed:**

- [x] Calculate actual ROM CRC32 using IEEE polynomial
- [x] Store CRC32 in Pansy file header (was placeholder 0)
- [x] Verify CRC before updating existing Pansy file
- [x] Create separate CRC-suffixed files for ROM mismatches
- [x] Custom CRC32 implementation (System.IO.Hashing unavailable)
- [x] PansyHeader record for parsing existing files

**Configuration Options Added:**

- `BackgroundCdlRecording` (bool, default: false)
- `AutoSaveIntervalMinutes` (int, default: 5)
- `SavePansyOnRomUnload` (bool, default: true)

**Files Modified:**

- `UI/Debugger/Labels/PansyExporter.cs` - Added CRC32 calculation (~90 lines)
- `UI/Debugger/Labels/BackgroundPansyExporter.cs` - NEW (160 lines)
- `UI/Config/IntegrationConfig.cs` - Added 3 config properties
- `UI/MainWindow.axaml.cs` - Added hooks for ROM load/unload

**UI Polish (Completed 2026-01-27):**

- [x] Add UI config checkboxes in DebuggerConfigWindow.axaml
- [x] Add localization strings for new options:
	- `chkBackgroundCdlRecording`
	- `lblAutoSaveInterval`
	- `lblMinutes`
	- `chkSavePansyOnRomUnload`

---

### Issue #2: 🧪 Pansy Export Testing & Validation - Phase 2

**Status:** ⏳ TODO  
**Labels:** testing, validation, phase-2  
**Milestone:** Pansy Integration v1.0  
**Assignee:** TBD  
**Estimate:** 8 hours  
**Dependencies:** Issue #1 (completed)

**Description:**
Create comprehensive test suite for Pansy export functionality.

**Tasks:**

- [ ] Create test ROM with known symbols/comments
- [ ] Export .pansy file from Nexen
- [ ] Verify with Pansy CLI: `pansy info test.pansy`
- [ ] Validate all sections present and correct
- [ ] Test all supported platforms (NES, SNES, GB, GBA, etc.)
- [ ] Verify roundtrip: Nexen → Pansy → Nexen import (future)
- [ ] Test edge cases: empty labels, Unicode, long comments
- [ ] Performance testing with large ROMs (4MB+)
- [ ] Memory leak testing (auto-export enabled)
- [ ] Create unit tests (if feasible with Nexen architecture)

**Acceptance Criteria:**

- All platforms export valid .pansy files
- Pansy CLI successfully reads all exported files
- No data loss or corruption
- Performance impact < 5% on debugger operations
- Documentation updated with test results

**Files to Create:**

- `~manual-testing/pansy/test-roms/` - Test ROM collection
- `~manual-testing/pansy/expected-outputs/` - Known-good .pansy files
- `~docs/pansy-testing-guide.md` - Testing procedures

---

### Issue #3: 📊 Add Memory Regions Export - Phase 3

**Status:** ✅ COMPLETED  
**Labels:** enhancement, phase-3  
**Milestone:** Pansy Integration v1.1  
**Estimate:** 4 hours  
**Dependencies:** Issue #2 (testing complete)  
**Completed:** 2026-01-28 11:00 UTC

**Description:**
Export named memory regions to Pansy format (PRG-ROM, CHR-ROM, SRAM, etc.)

**Tasks:**

- [x] Add MEMORY_REGIONS section (0x0009)
- [x] Export region name, start, end, type, flags
- [x] Map Nexen memory types to Pansy region types
- [x] Add system memory maps for all consoles
- [x] Add UI checkbox: "Include memory regions"
- [x] Test with multi-mapper ROMs

**Binary Format:**

```text
Section 0x0009: MEMORY_REGIONS
[uint32 count]
[for each region]:
  [uint32 startAddress]
  [uint32 endAddress]
  [byte type] (0=Unknown, 1=Code, 2=Data, 3=RAM, 4=IO, 5=ROM, 6=SaveRam, 7=WorkRam, 8=VideoRam)
  [byte memType]
  [uint16 flags]
  [uint16 nameLength]
  [UTF-8 name string]
```

---

### Issue #4: 🔗 Add Cross-Reference Export - Phase 3

**Status:** ✅ COMPLETED  
**Labels:** enhancement, phase-3  
**Milestone:** Pansy Integration v1.1  
**Estimate:** 6 hours  
**Dependencies:** Issue #2  
**Completed:** 2026-01-28 11:00 UTC

**Description:**
Export cross-references (who calls what) to Pansy format.

**Tasks:**

- [x] Add CROSS_REFS section (0x0005)
- [x] Extract cross-references from CDL data
- [x] Support types: Call, Jump, Read, Write, Branch
- [x] Add source/destination addresses
- [x] Handle indirect calls/jumps
- [x] Performance optimization (large ROM handling)
- [x] Add UI option: "Include cross-references"
- [x] Document format specification

**Binary Format:**

```text
Section 0x0005: CROSS_REFS
[uint32 count]
[for each xref]:
  [uint32 fromAddress]
  [uint32 toAddress]
  [byte type] (1=Call, 2=Jump, 3=Read, 4=Write, 5=Branch)
  [byte memTypeFrom]
  [byte memTypeTo]
  [byte flags]
```

---

### Issue #5: ⚡ Performance Optimization - Phase 4

**Status:** ✅ COMPLETED  
**Labels:** performance, optimization, phase-4  
**Milestone:** Pansy Integration v1.2  
**Estimate:** 8 hours  
**Dependencies:** Issue #3, Issue #4  
**Completed:** 2026-01-28 11:00 UTC

**Description:**
Optimize export performance for large ROMs and frequent auto-exports.

**Tasks:**

- [x] Add GZip compression option
- [x] Implement async file writing
- [x] Buffered streams for large files
- [x] Add PansyExportOptions class
- [x] Add UI option for compression
- [ ] Profiling and bottleneck identification
- [ ] Caching for repeated exports
- [ ] Progress dialog for large exports
- [ ] Benchmark suite (1KB, 1MB, 4MB ROMs)

**Acceptance Criteria:**

- Export time < 100ms for 1MB ROM
- Export time < 500ms for 4MB ROM
- No UI blocking during export
- Memory usage < 2× ROM size

---

### Issue #6: 🎨 UI Enhancements - Phase 5

**Status:** ⏳ TODO  
**Labels:** ui, enhancement, phase-5  
**Milestone:** Pansy Integration v1.3  
**Estimate:** 6 hours  
**Dependencies:** Issue #5

**Description:**
Improve user experience for Pansy export feature.

**Tasks:**

- [ ] Add progress dialog with cancel button
- [ ] Show export statistics (sections, size, time)
- [ ] Recent exports list with quick re-export
- [ ] Export presets (full, symbols-only, minimal)
- [ ] Drag-and-drop .pansy file import (if supported)
- [ ] Export history log
- [ ] Tooltips and help text
- [ ] Keyboard shortcuts (Ctrl+Shift+E)

**UI Mockups:**

```text
┌─────────────────────────────────────────┐
│ Export Pansy Metadata				   │
├─────────────────────────────────────────┤
│ □ Symbols		   (123 labels)		│
│ □ Comments		  (45 annotations)	│
│ □ Code/Data Map	 (256 KB)			│
│ □ Jump Targets	  (89 branches)	   │
│ □ Subroutines	   (34 functions)	  │
│ ☑ Memory Regions	(5 regions)		 │
│ ☑ Cross-References  (456 xrefs)		 │
│										 │
│ Preset: [Full Export ▼]				│
│										 │
│ Output: test.pansy		  [Browse...]│
│										 │
│		   [Export]  [Cancel]			│
└─────────────────────────────────────────┘
```

---

### Issue #7: 🔄 Diff and Merge Support - Phase 6

**Status:** ⏳ TODO  
**Labels:** advanced, phase-6  
**Milestone:** Pansy Integration v2.0  
**Estimate:** 12 hours  
**Dependencies:** Issue #6

**Description:**
Add diff/merge capabilities for .pansy files.

**Tasks:**

- [ ] Compare two .pansy files and show differences
- [ ] Merge non-conflicting changes
- [ ] Conflict resolution UI
- [ ] Three-way merge support
- [ ] Export diff as patch file
- [ ] Integration with Pansy CLI diff tools
- [ ] Collaboration workflow documentation

**Use Cases:**

- Merge labels from multiple debugging sessions
- Compare before/after ROM modifications
- Team collaboration on disassembly projects

---

### Issue #8: 📥 Import Pansy Files - Phase 6

**Status:** ⏳ TODO  
**Labels:** import, advanced, phase-6  
**Milestone:** Pansy Integration v2.0  
**Estimate:** 10 hours  
**Dependencies:** Issue #7

**Description:**
Import .pansy files back into Nexen debugger.

**Tasks:**

- [ ] Parse Pansy binary format
- [ ] Convert symbols to CodeLabel objects
- [ ] Import comments to label manager
- [ ] Import CDL data (code/data flags)
- [ ] Import memory regions
- [ ] Import cross-references
- [ ] Conflict resolution (existing vs imported labels)
- [ ] Undo/redo support
- [ ] Backup before import

**Import Options:**

- Replace all labels
- Merge (keep existing + add new)
- Selective import (choose sections)

---

### Issue #9: 📦 Batch Export - Phase 6

**Status:** ⏳ TODO  
**Labels:** batch, automation, phase-6  
**Milestone:** Pansy Integration v2.0  
**Estimate:** 4 hours  
**Dependencies:** Issue #8

**Description:**
Export multiple ROMs to Pansy format in batch mode.

**Tasks:**

- [ ] Command-line interface for batch export
- [ ] Folder scanning for ROM files
- [ ] Automatic debugger startup/shutdown
- [ ] Parallel processing (multi-threading)
- [ ] Error handling and logging
- [ ] Progress reporting
- [ ] Summary report (successes/failures)

**Example Usage:**

```bash
Nexen.exe --batch-export --input "C:\ROMs\*.nes" --output "C:\Pansy"
```

---

### Issue #10: 📖 Documentation & Community - Phase 7

**Status:** ⏳ TODO  
**Labels:** documentation, community, phase-7  
**Milestone:** Pansy Integration v2.0  
**Estimate:** 8 hours  
**Dependencies:** Issue #9

**Description:**
Complete documentation and prepare for upstream PR.

**Tasks:**

- [ ] User guide with screenshots/videos
- [ ] Developer API documentation
- [ ] File format specification (formal spec)
- [ ] Tutorial: First Pansy export
- [ ] FAQ document
- [ ] Troubleshooting guide
- [ ] Release notes
- [ ] Submit PR to SourMesen/Nexen
- [ ] Community announcement (Reddit, Discord)
- [ ] Example .pansy files repository

**Documentation Files:**

- `docs/pansy-user-guide.md`
- `docs/pansy-api-reference.md`
- `docs/pansy-file-format.md`
- `docs/pansy-tutorial.md`
- `docs/pansy-faq.md`

---

### Issue #11: 📁 Folder-Based Debug Storage - Phase 7.5a

**Status:** ✅ COMPLETED  
**Labels:** enhancement, storage, phase-7.5  
**Milestone:** Pansy Integration v2.0  
**Estimate:** 6 hours  
**Dependencies:** Issue #5 (Performance)  
**Completed:** 2026-01-27 13:46 UTC  
**Commit:** `794d74c7`

**Description:**
Implement folder-based storage for debug data. Each ROM gets a dedicated folder containing Pansy, MLB, and CDL files.

**Folder Structure:**

```text
[Nexen Data]/Debug/[RomName_CRC32]/
├── metadata.pansy	 # Universal format
├── labels.mlb		 # Native Nexen labels
├── coverage.cdl	   # Code Data Logger
├── config.json		# Per-ROM config
└── history/		   # Version history (optional)
```

**Tasks:**

- [x] Create `DebugFolderManager` class
- [x] Implement folder naming convention (sanitize special chars)
- [x] Create folder on first save
- [x] Migrate existing auto-save to folder structure
- [x] Add config option for debug folder location
- [x] Support custom folder paths

**Files:**

- NEW: `UI/Debugger/Labels/DebugFolderManager.cs` (414 lines)
- MOD: `UI/Config/IntegrationConfig.cs`
- MOD: `UI/Debugger/Labels/PansyExporter.cs`

---

### Issue #12: 🔄 MLB File Sync - Phase 7.5b

**Status:** ✅ COMPLETED  
**Labels:** enhancement, sync, phase-7.5  
**Milestone:** Pansy Integration v2.0  
**Estimate:** 4 hours  
**Dependencies:** Issue #11  
**Completed:** 2026-01-27 13:46 UTC  
**Commit:** `794d74c7`

**Description:**
Keep MLB (Nexen Label) files in sync with Pansy files.

**Tasks:**

- [x] Export MLB alongside Pansy file
- [x] Import MLB when loading ROM
- [x] Detect MLB changes and update Pansy
- [x] Round-trip test: Pansy → MLB → Pansy
- [x] Handle label format differences
- [x] Preserve multi-byte labels

**Files:**

- MOD: `UI/Debugger/Labels/DebugFolderManager.cs`
- MOD: `UI/Debugger/Labels/PansyExporter.cs`
- [ ] Detect MLB changes and update Pansy
- [ ] Round-trip test: Pansy → MLB → Pansy
- [ ] Handle label format differences
- [ ] Preserve multi-byte labels

**Files:**

- MOD: `UI/Debugger/Labels/MesenLabelFile.cs`
- MOD: `UI/Debugger/Labels/PansyExporter.cs`
- MOD: `UI/Debugger/Labels/DebugFolderManager.cs`

---

### Issue #13: 📊 CDL File Sync - Phase 7.5c

**Status:** ✅ COMPLETED  
**Labels:** enhancement, sync, phase-7.5  
**Milestone:** Pansy Integration v2.0  
**Estimate:** 4 hours  
**Dependencies:** Issue #11  
**Completed:** 2026-01-27 13:46 UTC  
**Commit:** `794d74c7`

**Description:**
Keep CDL (Code Data Logger) files in sync with Pansy files.

**CDL Flags:**

```cpp
0x01 = Code
0x02 = Data
0x04 = JumpTarget
0x08 = SubEntryPoint
```

**Tasks:**

- [x] Export CDL alongside Pansy file
- [x] Auto-load CDL if config enabled
- [x] Sync CDL flags to/from Pansy CODE_DATA_MAP
- [x] Handle different ROM sizes
- [x] Verify flag mapping accuracy
- [x] Support platform-specific CDL formats

**Files:**

- MOD: `UI/Debugger/Labels/DebugFolderManager.cs`
- MOD: `UI/Debugger/Labels/PansyExporter.cs`

---

### Issue #14: 📝 DBG Integration - Phase 7.5d

**Status:** ⏳ TODO  
**Labels:** enhancement, import, phase-7.5  
**Milestone:** Pansy Integration v2.0  
**Estimate:** 6 hours  
**Dependencies:** Issue #12, Issue #13

**Description:**
Import DBG files (ca65, cc65, WLA-DX) into Pansy format, preserving source references.

**Tasks:**

- [ ] Import DBG files to Pansy format
- [ ] Preserve source file references
- [ ] Import symbol definitions
- [ ] Import scope information
- [ ] Handle assembler-specific formats
- [ ] Create DbgToPansyConverter utility

**Supported Formats:**

- ca65/cc65 DBG
- WLA-DX symbols
- RGBDS symbols
- SDCC symbols

**Files:**

- MOD: `UI/Debugger/Integration/DbgImporter.cs`
- NEW: `UI/Debugger/Integration/DbgToPansyConverter.cs`

---

### Issue #15: 🔗 Bidirectional Sync Manager - Phase 7.5e

**Status:** ⏳ TODO  
**Labels:** enhancement, sync, advanced, phase-7.5  
**Milestone:** Pansy Integration v2.0  
**Estimate:** 8 hours  
**Dependencies:** Issue #12, Issue #13, Issue #14

**Description:**
Manage bidirectional sync between Pansy and native Nexen files.

**Tasks:**

- [ ] Detect file changes (FileSystemWatcher)
- [ ] Conflict resolution UI dialog
- [ ] Merge strategies (ours, theirs, merge)
- [ ] Change notification system
- [ ] Undo/redo support
- [ ] Sync status indicator in UI
- [ ] Lock files during sync to prevent corruption

**Files:**

- NEW: `UI/Debugger/Labels/SyncManager.cs`
- NEW: `UI/Debugger/Windows/SyncConflictDialog.axaml`
- NEW: `UI/Debugger/Windows/SyncConflictDialog.axaml.cs`

---

## Issue Summary

| Phase | Issues | Total Hours | Status |
| ------- | -------- | ------------- | -------- |
| Phase 1 | #1 | 12h | ✅ Complete |
| Phase 1.5 | #1.5 | 4h | ✅ Complete |
| Phase 2 | #2 | 8h | ⏳ TODO |
| Phase 3 | #3, #4 | 10h | ✅ Complete |
| Phase 4 | #5 | 8h | ✅ Complete |
| Phase 5 | #6 | 6h | ⏳ TODO |
| Phase 6 | #7, #8, #9 | 26h | ⏳ TODO |
| Phase 7 | #10 | 8h | ⏳ TODO |
| Phase 7.5a/b/c | #11, #12, #13 | 14h | ✅ Complete |
| Phase 7.5d/e | #14, #15 | 14h | ⏳ TODO |
| **Total** | **16** | **110h** | 50% |

---

**Document Created:** 2026-01-19 18:48 UTC  
**Last Updated:** 2026-01-27 14:30 UTC  
**Repository:** <https://github.com/TheAnsarya/Nexen>
