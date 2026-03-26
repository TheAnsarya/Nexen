# Atari Lynx Debug and TAS Manual Test

Use this checklist to validate Atari Lynx debugger, TAS/movie, and core emulation functionality for release.

## Preconditions

- Release build available.
- Atari Lynx ROM (`.lnx` or headerless `.o`/`.lyx`) loaded.
- Optionally: Lynx boot ROM (`lynxboot.img`, 512 bytes) in firmware directory.
- A clean movie workspace path prepared.
- Recommended test ROMs: California Games (landscape), Klax (right rotation), a homebrew EEPROM title.

## Stage 1: ROM Loading & Boot

### 1.1 LNX Header ROM

1. Load a `.lnx` ROM file.
2. Verify no crash or error dialog.
3. Verify the game title appears in the title bar.
4. Verify game starts and runs.

Expected result:
- ROM loads cleanly with correct metadata.

### 1.2 Headerless ROM

1. Load a headerless `.o` or `.lyx` ROM.
2. Verify it loads without crashing.
3. Verify game database auto-detects rotation and EEPROM type.

Expected result:
- Headerless format is handled transparently.

### 1.3 HLE Boot (No Boot ROM)

1. Ensure no boot ROM file is present.
2. Load a commercial game.
3. Verify the game starts (may skip FUJI logo).

Expected result:
- HLE boot initializes CPU state and jumps to entry point.

### 1.4 Real Boot ROM

1. Place `lynxboot.img` in the firmware directory.
2. Load a commercial game.
3. Verify the Atari FUJI boot animation plays.
4. Verify the game starts after the boot sequence.

Expected result:
- Boot ROM executes correctly and hands off to the game.

### 1.5 Reset

1. While a game is running, trigger Reset.
2. Verify a full power cycle occurs (no soft reset on Lynx).
3. Verify the game restarts cleanly.

Expected result:
- Power cycle behavior matches real hardware.

## Stage 2: Display & Input Validation

### 2.1 Display Resolution and Rotation

1. Load a landscape game — verify 160x102 output.
2. Load a right-rotated game (Klax) — verify 102x160 output, correct orientation.
3. Toggle Auto Rotate in config — verify rotation switches.

Expected result:
- Screen orientation matches game metadata.

### 2.2 Color Adjustments

1. Open Video Settings.
2. Adjust Brightness, Contrast, Hue, Saturation.
3. Reset to defaults.

Expected result:
- Adjustments have visible effect, defaults restore original palette.

### 2.3 Input

1. Map all 9 buttons: Up, Down, Left, Right, A, B, Option1, Option2, Pause.
2. Verify all respond correctly in-game.
3. Enable Turbo A/B — verify rapid-fire works.
4. Toggle input display overlay — verify "UDLRabOoP" layout appears.

Expected result:
- All buttons functional, overlay updates in real-time.

## Stage 3: Audio Validation

### 3.1 Audio Playback

1. Load a game with music and sound effects.
2. Verify audio is audible, with no crackling or silence.
3. Verify stereo separation with headphones.

Expected result:
- 4-channel LFSR audio plays correctly in stereo.

### 3.2 Volume Control

1. Adjust system volume — verify audio responds.
2. Mute — verify complete silence.
3. Adjust per-channel volume in Lynx config — verify effect.

Expected result:
- Volume controls function correctly.

## Stage 4: Save Data & Save States

### 4.1 EEPROM Save/Load

1. Load an EEPROM game (Growing Ties, Ynxa).
2. Make progress that triggers a save.
3. Close and reopen the game.
4. Verify saved progress is restored.
5. Delete the `.eeprom` file, reload — verify clean slate, no crash.

Expected result:
- EEPROM persistence works across sessions.

### 4.2 Save States

1. Create a quick save during gameplay.
2. Advance game state.
3. Load the quick save — verify exact restoration.
4. Test multiple save slots.
5. Close Nexen, reopen, load save state — verify cross-session persistence.

Expected result:
- Save states capture and restore complete system state.

## Stage 5: Debugger UI Validation

### 5.1 Disassembly and Breakpoint Flow

1. Open debugger and disassembly panel.
2. Verify WDC 65C02 opcodes shown (lowercase).
3. Set a breakpoint on a known game loop address.
4. Run until breakpoint — verify pause at correct address.
5. Step one instruction — verify PC update and register changes.

Expected result:
- Disassembly is accurate, breakpoints hit exact addresses.

### 5.2 Register Viewer Panels

1. Verify **CPU** panel shows A, X, Y, SP, PC, P (NVDIZC flags).
2. Verify **Mikey** panel shows timer values, IRQ flags, display address.
3. Verify **Suzy** panel shows sprite engine registers.
4. Verify **APU** panel shows audio channel data (not all zeros during gameplay).
5. Verify **Cart** panel shows bank and address info.
6. Verify **EEPROM** panel shows chip type and serial state.

Expected result:
- All subsystem panels display live, accurate data.

### 5.3 Memory Viewer

1. Select **LynxWorkRam** — verify 64 KB visible.
2. Select **LynxPrgRom** — verify ROM data visible.
3. Select **LynxBootRom** — verify boot ROM data if loaded.
4. Edit a RAM byte — verify change reflected in-game.

Expected result:
- All memory types accessible and editable.

### 5.4 Trace and Event Viewer

1. Start trace logging, run 120 frames, stop.
2. Verify trace output contains expected Lynx tags and ordering.
3. Open event viewer — verify stability during run/pause cycles.

Expected result:
- Trace and event data are stable and ordered.

## Stage 6: TAS UI Validation

### 6.1 Movie Recording

1. Start recording a new movie.
2. Play the game for ~5 seconds.
3. Stop recording.
4. Verify movie file created.

Expected result:
- Movie captures all 9 input buttons per frame at 75 fps.

### 6.2 Movie Playback

1. Load the recorded movie.
2. Play back — verify inputs replay identically.
3. Verify per-frame input uses "UDLRabOoP" key format.

Expected result:
- Deterministic playback produces identical on-screen results.

### 6.3 Frame Advance and Rewind

1. During playback, use frame advance.
2. Verify single-frame stepping works correctly.
3. Use rewind — verify game state reverts accurately.

Expected result:
- Frame-accurate TAS editing works deterministically.

### 6.4 BK2 Roundtrip

1. Export a movie to BK2 format.
2. Re-import the BK2 file.
3. Compare playback — verify no input data loss.

Expected result:
- BK2 format preserves all Lynx input data.

## Stage 7: Edge Cases

### 7.1 Rapid ROM Switching

1. Load a game, then immediately load a different game.
2. Repeat several times.

Expected result:
- No crash, memory leak, or state corruption.

### 7.2 Invalid ROM

1. Try loading a non-Lynx file renamed to `.lnx`.
2. Try loading an empty file with `.lnx` extension.

Expected result:
- Graceful error handling, no crash.

### 7.3 Fast Forward

1. Enable fast-forward/turbo mode.
2. Verify accelerated speed, no audio crashes.

Expected result:
- Turbo mode works without stability issues.

## Pass Criteria

All Critical and High ranked steps in [Manual Step Priority Ranking](manual-step-priority-ranking.md) must pass.

## Evidence Logging

Record:

- Build identifier.
- Test ROM used.
- Step IDs executed.
- Pass or fail results.
- Issue links for any failures.
