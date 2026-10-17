# 💾 Infinite Save States - Design Document

**Feature Branch:** `save-state-rewrite`
**Status:** ✅ Implemented
**Created:** 2025-01-28
**Completed:** 2026-01-31

---

## 📋 Overview

Transform Nexen's save state system from a fixed 10-slot model to an infinite timestamped save state system with a visual picker UI.

### Goals

1. **Infinite save states** - No limit on number of saves per ROM ✅
2. **Timestamped naming** - Saves named with ROM + datetime for organization ✅
3. **Visual picker** - Grid-based UI similar to ROM selector for browsing saves ✅
4. **Keyboard shortcuts** - Shift+F1 saves, F1 opens picker ✅
5. **No auto-deletion** - User manages their saves manually ✅

### Non-Goals

- Cloud sync
- Save state compression optimization
- Cross-ROM state loading
- Auto-backup/versioning

---

## 🔧 Current Architecture

### File Structure

```text
SaveStates/
  └── {RomName}_1.mss	# Slot 1
  └── {RomName}_2.mss	# Slot 2
  └── ...
  └── {RomName}_10.mss   # Slot 10
  └── {RomName}_11.mss   # Auto-save slot
```

### Key Components

| File | Purpose |
| ------ | --------- |
| `Core/Shared/SaveStateManager.h` | Core save state logic |
| `Core/Shared/SaveStateManager.cpp` | Save/load implementation |
| `UI/Config/Shortcuts/EmulatorShortcut.cs` | Shortcut definitions |
| `UI/Utilities/ShortcutHandler.cs` | Shortcut action handlers |
| `UI/ViewModels/MainMenuViewModel.cs` | Menu structure |
| `UI/Controls/StateGrid.axaml(.cs)` | State picker grid UI |
| `UI/Controls/StateGridEntry.axaml(.cs)` | Individual state entry |

### Current Shortcuts

- `SelectSaveSlot1-10` - Select slot
- `MoveToNextStateSlot` - Next slot
- `MoveToPreviousStateSlot` - Previous slot
- `SaveState` - Save to current slot
- `LoadState` - Load from current slot
- `SaveStateSlot1-10` - Save to specific slot
- `LoadStateSlot1-10` - Load from specific slot
- `SaveStateDialog` - Open save UI
- `LoadStateDialog` - Open load UI
- `SaveStateToFile` / `LoadStateFromFile` - File dialog

---

## 🎯 New Architecture

### File Structure

```text
SaveStates/
  └── {RomName}/
	  └── {RomName}_2025-01-28_14-30-45.mss
	  └── {RomName}_2025-01-28_15-42-12.mss
	  └── {RomName}_2025-01-28_16-00-00.mss
	  └── ...
```

### Key Changes

#### 1. Save State File Naming

**Old:** `{RomName}_{SlotIndex}.mss`
**New:** `{RomName}_{YYYY-MM-DD}_{HH-mm-ss}.mss`

#### 2. Directory Structure

- Each ROM gets its own subdirectory
- Prevents file listing performance issues with many ROMs
- Easier to locate/backup saves for specific games

#### 3. Shortcut Behavior

| Shortcut | Old Behavior | New Behavior |
| ---------- | -------------- | -------------- |
| `F1` | Load from current slot | Open save state picker |
| `Shift+F1` | (unused or slot select) | Save new timestamped state |
| `SaveState` | Save to slot N | Save new timestamped state |
| `LoadState` | Load from slot N | Open picker |

#### 4. Picker UI Features

- Grid layout matching ROM selector ✅
- Each entry shows:
	- Screenshot thumbnail ✅
	- Date/time formatted with full date and time (e.g., "Today 1/31/2026 2:30 PM", "Yesterday 1/30/2026 5:45 PM") ✅
	- ROM name as title ✅
- Sorted by most recent first ✅
- Keyboard navigation (arrows, Enter to load, Escape to close) ✅
- Delete option (DEL key with confirmation dialog) ✅

---

## 📁 Implementation Plan

### Phase 1: Core Infrastructure (C++) ✅

1. **`SaveStateManager` Updates**
   - Add method: `GetTimestampedFilepath()` → generates datetime-based path ✅
   - Add method: `GetRomSaveStateDirectory()` → ensures ROM subdirectory exists ✅
   - Add method: `GetSaveStateList()` → lists all saves for current ROM ✅
   - Add method: `SaveTimestampedState()` → save with timestamp naming ✅
   - Add method: `DeleteSaveState()` → remove a specific save ✅
   - Add method: `GetSaveStateCount()` → count saves for current ROM ✅
   - Keep: Slot-based methods for backward compatibility ✅

2. **New `SaveStateInfo` struct** ✅

   ```cpp
   struct SaveStateInfo {
	   string filepath;
	   string romName;
	   time_t timestamp;
	   uint64_t fileSize;
   };
   ```

### Phase 2: API Layer ✅

1. **New Interop Methods** (`EmuApi.cs`)
   - `SaveTimestampedState()` → save with datetime name ✅
   - `GetSaveStateList()` → return list of saves for current ROM ✅
   - `GetSaveStateCount()` → count saves for current ROM ✅
   - `DeleteSaveState(filepath)` → remove a specific save ✅

### Phase 3: UI Components ✅

1. **Update `StateGrid`/`StateGridEntry`**
   - Support dynamic entry count (not fixed 10) ✅
   - Handle longer lists with pagination ✅
   - Improve date formatting for better readability ✅
   - Delete key with confirmation dialog ✅

2. **Update `RecentGamesViewModel`**
   - Add new mode: `SaveStatePicker` ✅
   - Load saves from ROM subdirectory ✅
   - Sort by timestamp descending ✅

3. **Add Delete Functionality**
   - DEL key handler ✅
   - Confirmation dialog ✅

### Phase 4: Shortcuts & Menu ✅

1. **Modify `EmulatorShortcut.cs`**
   - Add: `QuickSaveTimestamped` ✅
   - Add: `OpenSaveStatePicker` ✅
   - Keep: Legacy slot shortcuts for power users ✅

2. **Update `ShortcutHandler.cs`**
   - Handle new shortcuts ✅
   - Call new API methods ✅

3. **Update `MainMenuViewModel.cs`**
   - Update File menu save state section ✅
   - Add "Quick Save (Timestamped)" option ✅
   - Add "Browse Save States..." option ✅
   - Keep slot-based options in submenu for compatibility ✅

### Phase 5: Default Keybindings ✅

1. **Update `PreferencesConfig.cs`**
   - Set default: F1 → `OpenSaveStatePicker` ✅
   - Set default: Shift+F1 → `QuickSaveTimestamped` ✅
   - Slot 1 keybindings removed, F2-F7 retained for slots 2-7 ✅

---

## 🎮 User Experience

### Quick Save Flow (Shift+F1)

1. User presses Shift+F1
2. System saves state with timestamp
3. OSD shows "State saved at {time}"
4. User continues playing

### Load State Flow (F1)

1. User presses F1
2. Picker overlay appears (pauses emulation)
3. Shows grid of recent saves with screenshots
4. User navigates with arrows, selects with Enter
5. State loads, overlay closes, emulation resumes

### Delete State Flow

1. In picker, user highlights a state
2. Presses DEL
3. Confirmation dialog: "Are you sure you want to delete this save state? [timestamp]"
4. User confirms with Yes
5. State deleted, list refreshes

---

## 🔄 Backward Compatibility

### Migration Strategy

- **No automatic migration** - Old slot files remain accessible
- **Slot-based shortcuts still work** - SaveStateSlot1-10 unchanged
- **Legacy submenu** - File → Save State → Slots → (1-10)
- **LoadLastSession unchanged** - Uses separate recent games system

### Breaking Changes

- **Default F1 behavior changes** - Was "load slot 1", now "open picker"
- **Users must rebind** - If they want old behavior

---

## 🧪 Testing Plan

### Unit Tests

1. Timestamp filepath generation
2. Save state enumeration
3. Directory creation
4. Date/time formatting

### Integration Tests

1. Save and load roundtrip
2. Multiple saves accumulate correctly
3. Delete removes file and refreshes list
4. Picker shows correct screenshots

### Manual Tests

1. Keyboard navigation in picker
2. Performance with 100+ save states
3. Cross-platform path handling
4. UI responsiveness during save enumeration

---

## 📊 Estimated Effort

| Phase | Complexity | Estimate |
| ------- | ------------ | ---------- |
| Phase 1: Core Infrastructure | Medium | 3-4 hours |
| Phase 2: API Layer | Low | 1-2 hours |
| Phase 3: UI Components | High | 4-6 hours |
| Phase 4: Shortcuts & Menu | Medium | 2-3 hours |
| Phase 5: Default Keybindings | Low | 1 hour |
| Testing & Polish | Medium | 3-4 hours |
| **Total** |  | **14-20 hours** |

---

## 📝 Open Questions

1. **Maximum saves per ROM?** - Consider adding optional limit setting
2. **Auto-save slot?** - Keep as slot 11, or integrate into timestamped system?
3. **Save state categories?** - Future: Allow user-defined folders/tags?
4. **Thumbnail size?** - Current is adequate, but consider options

---

## 📚 References

- Current implementation: [SaveStateManager.h](../../../Core/Shared/SaveStateManager.h)
- UI components: [StateGrid.axaml.cs](../../../UI/Controls/StateGrid.axaml.cs)
- Shortcuts: [EmulatorShortcut.cs](../../../UI/Config/Shortcuts/EmulatorShortcut.cs)
