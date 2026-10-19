# Savestate Manual Test

## Setup

1. Build the Release application and launch Nexen.
2. Load a game that can be safely restarted and has visible on-screen changes.
3. Open Preferences and confirm autosave is enabled with a short interval for testing.

## Timestamped Saves

1. Press `F1` twice, waiting at least one second between saves.
2. Press `Shift+F1` to open the save-state picker.
3. Confirm both saves appear newest first, with screenshots, timestamps, and `Manual` badges.
4. Select an older save after changing the game state. Confirm it restores the earlier state and closes the picker.
5. Reopen the picker, right-click one save, choose Delete, and confirm that entry disappears.
6. With no timestamped saves for a game, open the picker and confirm the game resumes instead of staying paused.
7. Pause the game before opening the picker; close it without selecting a save and confirm it remains paused.

## Autosave Preferences

1. In Preferences, enable autosave and set the interval to one minute.
2. Play for slightly longer than one minute. Open the picker and confirm a timestamped save appears with an `Auto-save` badge and screenshot.
3. Change the interval while the game is running and confirm subsequent autosaves use the new interval.
4. Disable autosave before its next pending interval expires. Confirm no delayed autosave appears.
5. Re-enable autosave and confirm it resumes. Verify the setting persists after restarting Nexen.

## Recent Play

1. Leave autosave enabled and play for at least five minutes.
2. Confirm a `Recent play` save appears in the picker with its screenshot and timestamp.
3. Continue playing through additional five-minute intervals; confirm only the newest five Recent Play saves are retained.
4. Load a Recent Play save and confirm the picker closes and emulation resumes.

## Designated Slots

1. From File, open the Save State slot screen. Save to a numbered slot and confirm the slot shows a screenshot.
2. Change the game state, open the Load State slot screen, and load that slot.
3. Confirm the saved state is restored and the picker closes.
4. Verify existing slot-select, save-slot, and load-slot shortcuts still select and use the expected numbered slots.

## Result

- [ ] Pass
- [ ] Fail

Notes:
