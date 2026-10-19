# Savestate Manual Test

## Setup

1. Build the Release application and launch Nexen. [X] Pass
2. Load a game that can be safely restarted and has visible on-screen changes. [X] Pass
3. Open Preferences and confirm autosave is enabled with a short interval for testing. [X] Pass

## Timestamped Saves

1. Press `F1` twice, waiting at least one second between saves. [X] Pass
2. Press `Shift+F1` to open the save-state picker. [X] Pass
3. Confirm both saves appear newest first, with screenshots, timestamps, and `Manual` badges. [X] Pass
4. Select an older save after changing the game state. Confirm it restores the earlier state and closes the picker. [X] Pass
5. Reopen the picker, right-click one save, choose Delete, and confirm that entry disappears. [X] Pass
6. With no timestamped saves for a game, open the picker and confirm the game resumes instead of staying paused. [X] Pass
7. Pause the game before opening the picker; close it without selecting a save and confirm it remains paused. [ ] Fail (don't know if we want this, we have a pause button in the load)

## Autosave Preferences

1. In Preferences, enable autosave and set the interval to one minute. [X] Pass [X] Pass
2. Play for slightly longer than one minute. Open the picker and confirm a timestamped save appears with an `Auto-save` badge and screenshot.
3. Change the interval while the game is running and confirm subsequent autosaves use the new interval.
4. Disable autosave before its next pending interval expires. Confirm no delayed autosave appears.
5. Re-enable autosave and confirm it resumes. Verify the setting persists after restarting Nexen.

## Recent Play

1. Leave autosave enabled and play for at least five minutes.
2. Confirm a `Recent play` save appears in the picker with its screenshot and timestamp.
3. Continue playing through additional five-minute intervals; confirm only the newest five Recent Play saves are retained.
4. Load a Recent Play save and confirm the picker closes and emulation resumes.

## Quick Save / Quick Load (Retry Slot)

1. Press `F2` (File > Quick Save (Retry Slot)) at a hard spot, play on, then press `F3` (Quick Load). Confirm the state restores instantly with no screen shown.
2. Confirm the old `Save State Menu` and `Load State Menu` entries are gone from the File menu and Preferences > Shortcut Keys.
3. Confirm numbered slot shortcuts (old `F1`..`F7` loads, `Shift+F1`..`F7` saves) no longer do anything.

## Game Select

1. In File, confirm `Game Select` is directly below `Open`.
2. With a game running, click it. Confirm the game/savestate select screen appears, the game pauses, and the `Paused` badge shows.
3. Close it with the X, and confirm the game resumes. Choose a different game and confirm it starts running, not paused.

## Picker Details

1. Confirm the `Paused` badge shows a real label (not `[StateGrid:SaveStatePaused]`).
2. Confirm `Manual` (blue), `Auto-save` (green), and `Recent play` (orange) badges have distinct colors.
3. With no saves for a game, press `Shift+F1`. Confirm a "No save states found" message shows and the game keeps running.
4. Open a ROM from File > Open and confirm it is not paused on start.

## Result

- [X] Pass
- [ ] Fail
**
Notes:
