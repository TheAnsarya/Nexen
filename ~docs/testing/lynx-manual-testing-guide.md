# 🎮 Atari Lynx Manual Testing Guide

> **Last updated:** 2026-02-20 | **Platform:** Nexen (Mesen2 fork) | **Branch:** `features-atari-lynx`

This guide covers manual testing procedures for the Atari Lynx emulation in Nexen. Use it to verify functionality after code changes.

---

## Prerequisites

- **Nexen** built and running (Release x64)
- At least one commercial `.lnx` ROM for testing
- Optionally: Lynx boot ROM (`lynxboot.img`, 512 bytes)
- Optionally: homebrew ROMs for EEPROM and rotation testing

### Recommended Test ROMs

| Game | Rotation | EEPROM | Players | CRC32 | Notes |
|------|----------|--------|---------|-------|-------|
| California Games | None | None | 4 | — | Good general test |
| Chip's Challenge | None | None | 1 | — | Classic puzzle game |
| Klax | Right | None | 1 | — | Tests right rotation |
| Awesome Golf | Right | None | 1 | — | Tests right rotation |
| Growing Ties (HB) | None | 93C46 | 1 | — | Tests EEPROM save |
| Ynxa (HB) | None | 93C46 | 1 | — | Tests EEPROM save |

---

## 1. ROM Loading & Boot

### 1.1 LNX Header Parsing

- [ ] Load a `.lnx` ROM file
- [ ] Verify no crash or error dialog
- [ ] Check console log for header info (bank sizes, name, manufacturer)
- [ ] Verify the game title appears correctly in the title bar

### 1.2 Headerless ROM (.o / .lyx)

- [ ] Load a headerless ROM (rename a `.lnx` and strip the 64-byte header)
- [ ] Verify it loads without crashing
- [ ] Check that game database auto-detects rotation/EEPROM if applicable

### 1.3 HLE Boot (No Boot ROM)

- [ ] Remove/rename the boot ROM file
- [ ] Load a commercial game
- [ ] Verify the game starts and runs (may skip FUJI logo)
- [ ] Check console log shows `HLE boot: PC=$XXXX`
- [ ] Verify display output is correct (not garbled)

### 1.4 Real Boot ROM

- [ ] Place `lynxboot.img` in the firmware directory
- [ ] Load a commercial game
- [ ] Verify the Atari FUJI boot animation plays
- [ ] Verify the game starts after the boot sequence

### 1.5 Reset

- [ ] While a game is running, trigger Reset
- [ ] Verify it performs a full power cycle (reload)
- [ ] Verify the game restarts cleanly

---

## 2. Display & Video

### 2.1 Native Resolution

- [ ] Load a landscape game (no rotation)
- [ ] Verify output is 160×102 pixels at the correct aspect ratio
- [ ] Verify colors look correct (Lynx uses 4096-color / 12-bit RGB palette)

### 2.2 Screen Rotation — Left (90° CCW)

- [ ] Load a left-rotated game (if available)
- [ ] Verify output is 102×160 (portrait mode, rotated left)
- [ ] Verify game content is correctly oriented

### 2.3 Screen Rotation — Right (90° CW)

- [ ] Load a right-rotated game (Klax, Awesome Golf, NFL Football, etc.)
- [ ] Verify output is 102×160 (portrait mode, rotated right)
- [ ] Verify game content is correctly oriented
- [ ] Verify input mapping is correct for rotated play

### 2.4 Color Adjustments

- [ ] Open Video Settings
- [ ] Adjust **Brightness** — verify visible effect on Lynx output
- [ ] Adjust **Contrast** — verify visible effect
- [ ] Adjust **Hue** — verify visible effect
- [ ] Adjust **Saturation** — verify visible effect
- [ ] Reset all to default — verify output returns to normal

### 2.5 Frame Rate

- [ ] Enable FPS counter overlay
- [ ] Verify it shows approximately 75 fps (Lynx native rate ~75.0 Hz)

---

## 3. Audio

### 3.1 Basic Audio Playback

- [ ] Load a game with music (e.g., California Games, Chip's Challenge)
- [ ] Verify music and sound effects are audible
- [ ] Verify no crackling, popping, or silence

### 3.2 Stereo Separation

- [ ] Use headphones/stereo output
- [ ] Load a game with stereo audio
- [ ] Verify left/right channel separation is correct

### 3.3 Volume Control

- [ ] Adjust system volume settings
- [ ] Verify audio volume responds correctly
- [ ] Mute audio — verify complete silence

---

## 4. Input / Controls

### 4.1 D-Pad

- [ ] Map Up, Down, Left, Right to keyboard/gamepad
- [ ] Verify all 4 directions work in-game
- [ ] Verify diagonals work (two directions simultaneously)

### 4.2 Face Buttons

- [ ] Map A and B buttons
- [ ] Verify both respond correctly in-game

### 4.3 Option Buttons

- [ ] Map Option 1 (L) and Option 2 (R)
- [ ] Verify both trigger correct in-game actions

### 4.4 Pause

- [ ] Map Pause (Start)
- [ ] Press Pause in-game — verify the game pauses
- [ ] Press again — verify unpause

### 4.5 Input Display

- [ ] Toggle the input display overlay
- [ ] Verify the 35×24 pixel overlay appears showing D-pad, face buttons, options, pause
- [ ] Press each button and verify the overlay updates in real-time

### 4.6 Turbo

- [ ] Enable Turbo A and/or Turbo B
- [ ] Verify rapid-fire behavior in-game

---

## 5. Save Data (EEPROM)

### 5.1 EEPROM Save

- [ ] Load a game with EEPROM support (Growing Ties, Ynxa)
- [ ] Make in-game progress that triggers a save
- [ ] Close the game
- [ ] Verify an `.eeprom` file was created in the save directory

### 5.2 EEPROM Load

- [ ] With the `.eeprom` file present, reload the game
- [ ] Verify saved progress is restored

### 5.3 EEPROM Without File

- [ ] Delete the `.eeprom` file
- [ ] Reload the game
- [ ] Verify the game starts with default/empty save data
- [ ] Verify no crash

### 5.4 LNX Header EEPROM Detection

- [ ] Load a `.lnx` ROM with EEPROM byte set (byte 60)
- [ ] Check console log shows EEPROM type detection

### 5.5 Database EEPROM Detection

- [ ] Load a headerless ROM for a known EEPROM game
- [ ] Check console log shows `EEPROM auto-detected from database`

---

## 6. Save States

### 6.1 Quick Save/Load

- [ ] During gameplay, create a quick save
- [ ] Advance the game state (move, change screen, etc.)
- [ ] Load the quick save
- [ ] Verify the game returns to the exact saved state

### 6.2 Multiple Save Slots

- [ ] Create saves in different slots
- [ ] Load each — verify each restores the correct state

### 6.3 Save State After EEPROM Change

- [ ] In an EEPROM game, save state
- [ ] Make in-game save progress
- [ ] Load the save state
- [ ] Verify EEPROM state was correctly restored (or not, depending on design)

### 6.4 Cross-Session Save States

- [ ] Save state, close Nexen
- [ ] Reopen Nexen, load the same ROM
- [ ] Load the save state — verify it restores correctly

---

## 7. Debugger

### 7.1 CPU State

- [ ] Open the CPU debugger
- [ ] Verify it shows WDC 65C02 registers (A, X, Y, SP, PC, P)
- [ ] Step through instructions — verify registers update correctly
- [ ] Verify the disassembly view shows valid 65C02 opcodes

### 7.2 Memory Viewer

- [ ] Open Memory Viewer
- [ ] Select **LynxWorkRam** — verify 64 KB of RAM is visible
- [ ] Select **LynxPrgRom** — verify ROM data is visible
- [ ] Select **LynxBootRom** — verify boot ROM data if loaded
- [ ] Edit a RAM byte — verify the change is reflected in-game

### 7.3 Subsystem State Panels

- [ ] Verify **Mikey** state shows timer values, IRQ flags, display address
- [ ] Verify **Suzy** state shows sprite engine registers
- [ ] Verify **PPU** state shows frame count, scanline, display address
- [ ] Verify **APU** state shows audio channel data (not all zeros)
- [ ] Verify **Cart** state shows bank and address info
- [ ] Verify **EEPROM** state shows chip type and serial state

### 7.4 Breakpoints

- [ ] Set a breakpoint at an address
- [ ] Run the emulator — verify it breaks at the correct address
- [ ] Verify CPU state is valid at the breakpoint
- [ ] Continue execution — verify the game resumes

---

## 8. Sprites & Collision

### 8.1 Sprite Rendering

- [ ] Load a sprite-heavy game (any action game)
- [ ] Verify sprites are rendered correctly (no missing, misaligned, or corrupted sprites)
- [ ] Verify sprite priority/layering is correct

### 8.2 Collision Detection

- [ ] Play an action game that uses hardware collision (most Lynx games)
- [ ] Verify player/enemy collisions are detected correctly
- [ ] Verify projectile collisions work
- [ ] Verify no phantom collisions or missed collisions

### 8.3 Math Coprocessor

- [ ] Games use Suzy's 16×16→32 multiply and 32/16 divide
- [ ] Verify no visual glitches from incorrect math (sprite scaling/rotation artifacts)

---

## 9. ComLynx / UART

### 9.1 UART Registers

- [ ] In the debugger, verify SERCTL and SERDAT registers are accessible
- [ ] Verify UART state shows in Mikey state panel

> **Note:** Multi-player ComLynx networking is not yet implemented. UART registers exist for compatibility but no link cable emulation is available.

---

## 10. Edge Cases & Regression Tests

### 10.1 Rapid ROM Switching

- [ ] Load a game, then immediately load a different game
- [ ] Repeat several times
- [ ] Verify no crash, memory leak, or state corruption

### 10.2 Invalid ROM

- [ ] Try loading a non-Lynx file (e.g., a `.nes` ROM renamed to `.lnx`)
- [ ] Verify graceful error handling (no crash)

### 10.3 Zero-Size ROM

- [ ] Try loading an empty file with `.lnx` extension
- [ ] Verify graceful error handling

### 10.4 Large ROM

- [ ] If available, test with the largest Lynx ROM (512 KB)
- [ ] Verify it loads and runs correctly

### 10.5 Fast Forward

- [ ] Enable fast-forward/turbo mode
- [ ] Verify the game runs at accelerated speed
- [ ] Verify audio handles the speedup without crashes

### 10.6 TAS Recording

- [ ] Record a short movie (.bk2 format)
- [ ] Play back the movie — verify inputs replay correctly
- [ ] Verify per-frame input uses "UDLRabOoP" key format (9 chars)

---

## Known Issues & Limitations

| Issue | Status | Notes |
|-------|--------|-------|
| ComLynx multi-player | Not implemented | UART registers exist but no link cable emulation |
| Cart shift register addressing | Needs research | May affect some bank-switched games |
| CpuCyclesPerFrame ~0.18% fast | Known | Integer truncation in timing constant (53235 vs 53333) |
| Duplicate APU state in LynxState | Design issue | Both `Mikey.Apu` and top-level `Apu` exist |
| MAPCTL write to RAM | Minor | Hardware register also writes to RAM backing |

---

## Test Results Template

Copy this for recording results:

```text
Date: YYYY-MM-DD
Build: <commit hash>
Tester: <name>

| Section | Pass | Fail | Skip | Notes |
|---------|------|------|------|-------|
| 1. ROM Loading | | | | |
| 2. Display | | | | |
| 3. Audio | | | | |
| 4. Input | | | | |
| 5. EEPROM | | | | |
| 6. Save States | | | | |
| 7. Debugger | | | | |
| 8. Sprites | | | | |
| 9. UART | | | | |
| 10. Edge Cases | | | | |
```

---

## Revision History

| Date | Changes |
|------|---------|
| 2025-06 | Initial guide created — covers all Lynx subsystems |
