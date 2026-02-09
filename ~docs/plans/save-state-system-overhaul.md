# Save State System Overhaul

## Overview

Complete overhaul of the Nexen save state system - **removing the legacy numbered slot system** and replacing it with a modern file-based approach with clear categories.

**Epic Issue**: #172
**Related Issues**: #173, #174, #175, #176, #177, #178

## ⚠️ MAJOR CHANGE: Removing Slot System

The numbered save slot system (slots 1-10 with F1-F10) is being **completely removed**. This frees up F1-F10 keyboard shortcuts for other uses and simplifies the save state architecture.

## Goals

1. **REMOVE** numbered save slot system
2. **Implement** single Designated Save (F4/Shift+F4)
3. **Keep** Quick Save system (Ctrl+S timestamped)
4. **Implement** Recent Play rotating queue (5-min intervals)
5. **Add** SaveStateOrigin tracking with visual badges
6. **Free up** F1-F10 shortcuts for other uses

---

## New Save State Architecture

### 1. Designated Save (Single Slot)

The primary "quick load" mechanism:

- **F4** = Load the designated save file
- **Shift+F4** = Save to designated slot
- User can set which file is designated via picker
- Origin: `Save` (green badge)

### 2. Quick Save (Timestamped Files)

For creating named save points:

- **Ctrl+S** = Create timestamped save
- Saves to: `{rom}-{timestamp}.mss`
- Browse via File → Browse Save States
- Origin: `Save` (green badge)

### 3. Recent Play Queue (Auto, Rotating)

Automatic gameplay capture:

- Every 5 minutes (configurable)
- 12 saves = 1 hour retention
- FIFO rotation (oldest replaced)
- Origin: `Recent` (red badge)
- Browse via File → Recent Play

### 4. Auto Save (System)

Long-term recovery:

- Every 20-30 minutes (configurable)
- Single auto-save file per ROM
- Origin: `Auto` (blue badge)

---

## SaveStateOrigin Enum

```csharp
public enum SaveStateOrigin : byte
{
	Auto = 0,   // Blue badge - System auto-saves
	Save = 1,   // Green badge - User-initiated saves
	Recent = 2, // Red badge - Recent play queue
	Lua = 3     // Yellow badge - Lua script saves
}
```

### Badge Colors (Bootstrap 5)

| Origin | Color | Hex Code | Text Color |
|--------|-------|----------|------------|
| Auto   | Blue  | #0d6efd  | White      |
| Save   | Green | #198754  | White      |
| Recent | Red   | #dc3545  | White      |
| Lua    | Yellow| #ffc107  | Black      |

---

## File Format Changes

### nexen-save Header Extension

Add origin byte after existing header fields:

```
Offset | Size | Field
-------|------|-------
...    | ...  | (existing fields)
+N     | 1    | SaveStateOrigin (uint8_t)
```

### Backwards Compatibility

- Existing saves without origin default to `Save` (1)
- Version check in header for new field
- Old versions ignore new field

---

## Keyboard Shortcuts (New)

| Shortcut | Action |
|----------|--------|
| F4 | Load Designated Save |
| Shift+F4 | Save to Designated Slot |
| Ctrl+S | Quick Save (timestamped) |
| F1-F3, F5-F12 | **Available for other uses** |

---

## Configuration Options

### PreferencesConfig

```csharp
// Designated Save
public string DesignatedSavePath { get; set; } = "";

// Auto Save (20-30 min)
public bool EnableAutoSave { get; set; } = true;
public uint AutoSaveInterval { get; set; } = 20; // minutes

// Recent Play Queue (5 min)
public bool EnableRecentPlaySaves { get; set; } = true;
public uint RecentPlayInterval { get; set; } = 5; // minutes
public uint RecentPlayRetentionCount { get; set; } = 12; // saves
```

---

## Menu Structure

### File Menu

```
File
├── ...
├── Designated Save
│   ├── Load (F4)
│   ├── Save (Shift+F4)
│   └── Set Designated Save...
├── Quick Save (Ctrl+S)
├── Browse Save States...
├── Recent Play
│   ├── Load Most Recent
│   ├── ────────────────
│   ├── 5 min ago - World 3-1
│   ├── 10 min ago - World 2-4
│   ├── ...
│   └── Browse Recent Play...
├── ...
```

### Settings Menu

```
Settings
├── ...
├── Auto Save
│   ├── ✓ Auto Save (20 min)
│   ├── ✓ Recent Play Saves (5 min)
│   ├── ────────────────
│   ├── ✓ Auto Save Pansy Data
│   └── ✓ Save Pansy on ROM Unload
├── ────────────────
└── Preferences
```

---

## UI Components

### SaveStateBadge Control

```xml
<Border Classes="save-state-badge auto">
	<TextBlock Text="AUTO"/>
</Border>
```

### Badge Styles

```xml
<Style Selector="Border.save-state-badge">
	<Setter Property="CornerRadius" Value="10"/>
	<Setter Property="Padding" Value="6,2"/>
</Style>
<Style Selector="Border.save-state-badge.auto">
	<Setter Property="Background" Value="#0d6efd"/>
</Style>
<Style Selector="Border.save-state-badge.save">
	<Setter Property="Background" Value="#198754"/>
</Style>
<Style Selector="Border.save-state-badge.recent">
	<Setter Property="Background" Value="#dc3545"/>
</Style>
<Style Selector="Border.save-state-badge.lua">
	<Setter Property="Background" Value="#ffc107"/>
</Style>
```

---

## Implementation Order

1. **Phase 1**: SaveStateOrigin enum and file format (#173)
2. **Phase 2**: Recent Play queue infrastructure (#174)
3. **Phase 3**: Designated Save system (#175)
4. **Phase 4**: Recent Play UI (#176)
5. **Phase 5**: Origin badges (#177)
6. **Phase 6**: Remove legacy slot system (#178)

---

## C++ Core Changes

### SaveStateManager.h

```cpp
enum class SaveStateOrigin : uint8_t {
	Auto = 0,
	Save = 1,
	Recent = 2,
	Lua = 3
};

class SaveStateManager {
public:
	// File-based saves (no numbered slots)
	void SaveToFile(const string& path, SaveStateOrigin origin);
	void LoadFromFile(const string& path);
	
	// Designated save
	void SaveDesignated();
	void LoadDesignated();
	void SetDesignatedPath(const string& path);
	
	// Quick save (timestamped)
	void QuickSave(); // Creates {rom}-{timestamp}.mss
	
	// Recent play queue
	void ProcessRecentPlaySave();
	static constexpr uint32_t RecentPlayCount = 12;
	
	// Auto save
	void ProcessAutoSave();
	
	SaveStateOrigin GetStateOrigin(const string& path);
};
```

---

## Files to Remove/Modify

### Remove Slot-Based Code
- Remove F1-F10 save/load shortcuts
- Remove slot selection menus
- Remove numbered slot UI
- Remove `SaveState(uint32_t slot, ...)` overloads

### Modify
- `SaveStateManager` - File-based only
- `MainMenuViewModel` - New menu structure
- `ShortcutHandler` - F4/Shift+F4 shortcuts
- `PreferencesConfig` - Designated save path

---

## Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| Removing slots breaks user workflows | High | Clear migration docs |
| File format change breaks saves | High | Version check, defaults |
| Performance from frequent saves | Medium | Async I/O, compression |

---

## Timeline

- **Week 1**: #173 (enum/format), #178 (remove slots)
- **Week 2**: #174 (recent play), #175 (designated)
- **Week 3**: #176 (UI), #177 (badges)

---

## Testing Checklist

- [ ] F4/Shift+F4 work correctly
- [ ] Ctrl+S creates timestamped saves
- [ ] Recent play queue rotates correctly
- [ ] Origin tracking persists across save/load
- [ ] Backwards compatibility with old saves
- [ ] Badges display correctly in all themes
- [ ] F1-F10 shortcuts are freed up
- [ ] Old slot-based code is removed
