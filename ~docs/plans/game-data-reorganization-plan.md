# Game Data Folder Reorganization Plan

**Issue:** #167 | **Created:** 2026-02-09

## Current State

Game data is scattered across multiple sibling folders in the Nexen home directory:

```
Documents/Nexen/
├── SaveStates/       ← Save states ({romName}_{slot}.nexen-save)
├── Saves/            ← Battery saves (.sav) — managed by C++ Core
├── Debug/            ← CDL, MLB, Pansy files
├── Debugger/         ← Workspace JSON, CDL files
├── Movies/           ← Movie recordings (.nexen-movie)
├── Screenshots/      ← PNG screenshots
├── Wave/             ← Audio recordings
├── AVI/              ← Video recordings
├── HdPacks/          ← HD texture packs
├── Lua/              ← Lua scripts
├── Firmware/         ← System firmware
├── Logs/             ← Log files
└── settings.json     ← Config file
```

### Key Path Construction Points

| Data Type | Current Path | Source File |
|-----------|-------------|-------------|
| Save states | `SaveStates/{romName}_{slot}.nexen-save` | SaveStateManager.cs |
| Battery saves | `Saves/` (C++ Core handles) | ConfigManager.cs |
| Debug workspace | `Debugger/{romName}.json` | DebugWorkspaceManager.cs |
| CDL files | `Debug/{romName}.cdl` | DebugWorkspaceManager.cs |
| Pansy (legacy) | `Debug/{romName}.pansy` | PansyExporter.cs |
| Pansy (folder) | `Debug/{romName}_{crc32}/metadata.pansy` | DebugFolderManager.cs |
| Movies | `Movies/{romName}.nexen-movie` | MovieRecordInfo.cs |
| Screenshots | `Screenshots/{romName}_{timestamp}.png` | ConfigManager.cs |

## Proposed Structure

```
Documents/Nexen/
├── GameData/
│   └── {System}/
│       └── {RomName}_{CRC32}/
│           ├── save-states/
│           │   ├── slot-01.nexen-save
│           │   ├── slot-02.nexen-save
│           │   └── auto.nexen-save
│           ├── debug/
│           │   ├── metadata.pansy
│           │   ├── labels.mlb
│           │   ├── coverage.cdl
│           │   ├── workspace.json
│           │   └── history/
│           ├── saves/
│           │   └── game.sav
│           ├── movies/
│           │   └── {timestamp}.nexen-movie
│           └── screenshots/
│               └── {timestamp}.png
├── Firmware/         ← Shared, not per-game
├── HdPacks/          ← Shared, not per-game
├── Lua/              ← Shared, not per-game
├── Logs/             ← Shared, not per-game
└── settings.json
```

## Impact Analysis

### High Impact (Core path changes)
1. **ConfigManager.cs** — All folder property getters need GameData-aware paths
2. **SaveStateManager.cs** — Save/load state path construction
3. **DebugFolderManager.cs** — Already uses `{romName}_{crc32}` pattern (closest to target)
4. **DebugWorkspaceManager.cs** — Workspace JSON path
5. **C++ Core (InteropDLL)** — Battery save paths passed from C# to C++

### Medium Impact (Path consumers)
6. **PansyExporter.cs** — Pansy file paths
7. **MovieRecordInfo.cs** — Movie file paths
8. **SaveStateViewer** — Thumbnail/browse paths
9. **CheatList** — Cheat file paths
10. **HistoryViewer** — Export paths

### Low Impact (Already organized or shared)
11. **Screenshots** — Just redirect to per-game folder
12. **Wave/AVI** — Recording paths
13. **Firmware/HdPacks/Lua** — Stay as shared folders

### C++ Core Interface
The C++ Core receives folder paths from C# via:
- `EmuApi.InitDll()` — passes ConfigManager paths
- `ConfigApiWrapper` — sends folder config to Core
- Core constructs `.sav` paths internally from the base `SaveFolder`

**This means we need to either:**
- a) Change how Core resolcts save paths (requires C++ changes)
- b) Set SaveFolder per-ROM before load (simpler, set dynamically)

## Migration Strategy

### Phase 1: Infrastructure
- Create `GameDataManager` service that resolves per-game folder paths
- Takes ROM info (name, CRC32, system) and returns the game data root
- All path helpers delegate to this service

### Phase 2: New Files Use New Paths
- New save states go to `GameData/{System}/{RomName}_{CRC32}/save-states/`
- New debug files go to `GameData/{System}/{RomName}_{CRC32}/debug/`
- Backwards-compatible: check old paths if new path doesn't exist

### Phase 3: Migration
- On ROM load, if old-style files exist, offer to migrate
- Background migration: copy files from old folders to new structure
- Keep old files as backup initially

### Phase 4: Cleanup
- Remove old scattered folders after user confirmation
- Update user-facing folder picker dialogs

## Risks

1. **C++ Core battery saves** — Most complex; Core constructs paths internally
2. **Backwards compatibility** — Users have existing data in old structure
3. **Cross-platform paths** — Linux/macOS path differences
4. **User-overridden paths** — Some users customize SaveFolder, DebugFolder, etc.
5. **ROM identification** — Need reliable CRC32 at the time of folder resolution

## Estimated Effort

| Phase | Effort | Risk |
|-------|--------|------|
| Infrastructure (GameDataManager) | 3-4 hours | Low |
| Save States migration | 2-3 hours | Medium |
| Debug files migration | 1-2 hours | Low (DebugFolderManager is close) |
| Battery saves (C++ Core) | 4-6 hours | High |
| Movies/Screenshots | 1 hour | Low |
| Migration UI/logic | 2-3 hours | Medium |
| Testing | 2-3 hours | — |
| **Total** | **~15-22 hours** | Medium-High |
