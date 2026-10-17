# Save States Guide

## Overview

Nexen supports quick save workflows, Open Save State Picker navigation, designated slots, and per-game organization.

It also maintains two background save streams:

- Recent Play checkpoints every 5 minutes in a rolling 36-slot window (about 3 hours)
- Auto Save progress log entries every configured interval (timestamped, not overwritten)

## Common Workflows

### Quick Save and Quick Load

1. Press `F1` to create a Quick Save (Timestamped).
2. Press `Shift+F1` to run Open Save State Picker.
3. Select a state thumbnail and load it.

### Designated Slots

1. Press `Shift+F2`, `Shift+F3`, or `Shift+F4` to save to designated slots 1-3.
2. Press `F2`, `F3`, or `F4` to load designated slots 1-3.

### File-Based States

1. Use `Ctrl+Shift+S` to run Save State to File.
2. Use `Ctrl+L` to run Load State from File.

## GUI Tips

- Use the visual picker to compare multiple branches of play.
- Use timestamped entries for rapid iteration while testing.
- Keep per-game save sets to avoid cross-ROM confusion.

## Panel Walkthrough (Screenshot Anchors)

### Walkthrough A: Quick Save and Visual Browser

1. In the main window, press `F1` to create a Quick Save (Timestamped). Expected result: a new timestamped save entry appears for the current ROM session.
2. Press `Shift+F1` to run Open Save State Picker. Expected result: the save picker opens and displays available save entries.
3. In the save picker grid, select a thumbnail with the target timestamp. Expected result: the selected entry is highlighted and metadata updates to match selection.
4. Confirm load from the selected entry. Expected result: emulation resumes from the selected saved frame.

| Step | Panel | Screenshot Anchor ID | Capture Target |
|---|---|---|---|
| 1 | Main Emulation Window | `save-states-a-01-main-window` | `docs/screenshots/save-states/a-01-main-window.png` |
| 2 | Open Save State Picker (Grid View) | `save-states-a-02-browser-grid` | `docs/screenshots/save-states/a-02-browser-grid.png` |
| 3 | Open Save State Picker (Selected Entry) | `save-states-a-03-selected-entry` | `docs/screenshots/save-states/a-03-selected-entry.png` |

### Walkthrough B: Designated Slot Workflow

1. Press `Shift+F2`/`Shift+F3`/`Shift+F4` from gameplay to store current state in the selected designated slot. Expected result: that slot updates to the current frame/state.
2. Continue play, then press `F2`/`F3`/`F4` to load the selected designated slot. Expected result: the ROM state reloads from that slot.
3. Verify game state returns to the exact checkpoint. Expected result: position, timer, and immediate game context match the saved checkpoint.

| Step | Panel | Screenshot Anchor ID | Capture Target |
|---|---|---|---|
| 1 | Main Emulation Window (Before Slot Save) | `save-states-b-01-before-slot-save` | `docs/screenshots/save-states/b-01-before-slot-save.png` |
| 2 | Main Emulation Window (After Slot Load) | `save-states-b-02-after-slot-load` | `docs/screenshots/save-states/b-02-after-slot-load.png` |

## Related Links

- [Documentation Index](README.md)
- [Rewind Guide](Rewind.md)
- [Movie System Guide](Movie-System.md)
