# Manual Testing Plan: Nexen Pansy Exporter Phase 1.5 Features

## Background CDL Recording

- [ ] Enable "Background CDL Recording" in DebuggerConfigWindow
- [ ] Load a ROM and run emulation without opening the debugger
- [ ] Confirm CDL data is recorded in the background
- [ ] Check that disabling the option stops background recording

## ROM CRC32 Verification

- [ ] Export a .PANSY file and verify CRC32 is written
- [ ] Corrupt the ROM and attempt export; confirm warning or error is shown
- [ ] Confirm valid CRC32 prevents export of corrupted data

## Auto-Save Interval

- [ ] Set "Auto-save interval" to various values (1-60 minutes)
- [ ] Confirm .PANSY file is auto-saved at correct intervals
- [ ] Change interval during runtime and verify new timing

## Save on ROM Unload

- [ ] Enable "Save .PANSY file when ROM is closed"
- [ ] Load and close a ROM; confirm .PANSY file is saved
- [ ] Disable option and confirm no file is saved on unload

## UI Verification

- [ ] Confirm all new config options appear in DebuggerConfigWindow
- [ ] Check localization strings for all new options

## Edge Cases

- [ ] Test with unsupported ROM types
- [ ] Test with large ROMs and long emulation sessions
- [ ] Confirm no crashes or data loss

---

**Log all results in session log. Attach screenshots for UI tests.**
