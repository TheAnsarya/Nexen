# Live Language Switch Manual Test

Use this checklist to verify that language changes apply immediately at runtime (no restart) across the main UI, debugger UI, and tool windows.

## Preconditions

- Build a Release x64 binary and launch Nexen.
- Have at least one ROM available to load.
- Confirm language resources exist for English (`en`), Spanish (`es`), and Japanese (`ja`).

## Runtime Switch Checklist

1. Start in English and load a ROM.
2. Open Preferences and set UI Language to Spanish.
3. Verify there is no restart prompt and changes apply immediately.
4. Confirm top-level main menu labels update (`File`, `Game`, `Settings`, `Tools`, `Debug`, `Help`).
5. Open `Settings -> Speed -> Speed Slider Prototype...` and verify title/body/buttons are translated.
6. Open `Tools -> TAS Editor` and verify `Create New Movie` label is translated.
7. If running an FDS title, open `Game -> Select Disk` and verify `Disk {n} Side {A/B}` is translated.
8. Switch to Japanese and repeat checks from steps 4-7.
9. Switch back to English and verify all labels return to English.

## Debugger Window Checks

1. Open the debugger window while language is English.
2. Change language to Spanish from Preferences.
3. Verify debugger window title and debugger menu labels update without reopening the app.
4. Change language to Japanese and confirm the same behavior.

## Pass Criteria

- No restart is required for language changes.
- Main menu, debugger window, and speed slider prototype window all update at runtime.
- No stale English labels remain in tested surfaces after switching to `es` or `ja`.
- Switching back to English restores English labels immediately.

## Notes

- If any label does not update, capture the exact menu path/window and current language code.
- Record failures in the active session log with screenshots when possible.
