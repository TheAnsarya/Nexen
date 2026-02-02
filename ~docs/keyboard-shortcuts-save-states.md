# Keyboard Shortcuts - Save State Changes

This document describes the keyboard shortcut changes for the Infinite Save States feature.

## New Default Bindings

| Key | Action | Description |
| ----- | -------- | ------------- |
| **F1** | Open Save State Picker | Opens a visual grid showing all save states for the current ROM |
| **Shift+F1** | Quick Save | Creates a new save state with timestamp naming |

## Old vs New Behavior

### F1 Key

| Version | Behavior |
| --------- | ---------- |
| **Before** | Load save state from slot 1 |
| **After** | Open save state picker overlay |

### Shift+F1 Key

| Version | Behavior |
| --------- | ---------- |
| **Before** | Varies (select slot or unused) |
| **After** | Save new timestamped state |

## Picker UI Shortcuts

When the save state picker is open:

| Key | Action |
| ----- | -------- |
| Arrow keys | Navigate between saves |
| Enter | Load selected save state |
| Delete | Delete selected save (with confirmation) |
| Escape | Close picker without loading |
| Page Up/Down | Navigate pages (if many saves) |

## Legacy Slot-Based Shortcuts

The following shortcuts are still available but **unbound by default** in new installations:

| Shortcut | Description |
| ---------- | ------------- |
| `SelectSaveSlot1-10` | Select a specific slot |
| `MoveToNextStateSlot` | Cycle to next slot |
| `MoveToPreviousStateSlot` | Cycle to previous slot |
| `SaveState` | Save to current slot |
| `LoadState` | Load from current slot |
| `SaveStateSlot1-10` | Save directly to slot N |
| `LoadStateSlot1-10` | Load directly from slot N |

Users who prefer the slot-based workflow can rebind these in **Options → Preferences → Shortcut Keys**.

## Migration for Existing Users

**Your existing keybindings are preserved.** These changes only affect:

1. Fresh installations
2. Users who reset their configuration
3. Users who manually update their shortcuts

If you prefer the old F1 = "Load Slot 1" behavior, you can:

1. Go to **Options → Preferences → Shortcut Keys**
2. Find `LoadStateSlot1` and bind it to F1
3. Optionally bind `OpenSaveStatePicker` to a different key

## Menu Shortcuts

The menu structure also reflects these changes:

```text
File
├── Quick Save		  Shift+F1
├── Browse Saves...	 F1
├── Save State		  →
│   ├── Slot 1
│   ├── Slot 2
│   ├── ...
│   ├── Slot 10
│   └── Save to File...
└── Load State		  →
	├── Slot 1
	├── Slot 2
	├── ...
	├── Slot 10
	├── Auto-save
	└── Load from File...
```
