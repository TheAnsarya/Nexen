# WonderSwan FF2 Fire Magic — Manual Testing Instructions

## Issue

[#1076](https://github.com/TheAnsarya/Nexen/issues/1076) — WSC Final Fantasy 2 fire magic causes video instability during the spell animation when targeting a single enemy.

## Root Cause Analysis

The WS PPU's VTOTAL register (port $16, `LastScanline`) controls frame timing. When a game sets VTOTAL to a value less than 144 (the visible screen height), it creates a "short frame" effect commonly used for raster effects.

**Previous behavior (bug):** Nexen terminated frames immediately when `Scanline > LastScanline`, creating frames with fewer total scanlines than expected. This disrupted game timing because:

- Frame duration was shorter (fewer CPU cycles per frame)
- VBlank IRQ never fired (game lost sync)
- Image rendering stopped prematurely

**Fixed behavior:** Matching ares, the frame now always runs to `max(144, LastScanline + 1)` scanlines. The rendering Y coordinate wraps via modulo (`renderY = Scanline % (LastScanline + 1)`), which repeats the displayed image. VBlank is inhibited for short VTOTAL (matching hardware behavior).

## Test Setup

### ROM

- **Game:** Final Fantasy II (WSC)
- **Platform:** WonderSwan Color
- **ROM location:** `C:\~reference-roms\ws\` (if available)
- **Alternative:** Any WS/WSC game that manipulates VTOTAL mid-frame

### Prerequisites

1. Build Nexen Release x64
2. Load FF2 WSC ROM
3. Start a new game or load a save near a fire-based battle spell

## Test Scenarios

### Test 1: Fire Magic Single Target

1. Enter a battle (random encounter or boss)
2. Select a character with Fire magic
3. Cast Fire on a **single enemy** (this is the specific case that triggers the issue)
4. **Watch the animation carefully** — the fire effect should display smoothly without:
	- Screen flickering/tearing
	- Horizontal line artifacts
	- Partial frame corruption
	- Momentary white flashes
	- Shifted/repeated image sections

### Test 2: Fire Magic Multi-Target

1. Enter a battle with multiple enemies
2. Cast Fire targeting **all enemies**
3. Verify the animation renders correctly (this case may work even without the fix)

### Test 3: Other Spell Animations

Test other WSC spell effects that might use VTOTAL manipulation:

- Ice magic
- Thunder magic
- Cure/healing spells
- Status effect spells
- Any visually complex spell animation

### Test 4: General Gameplay

1. Play through several battles
2. Navigate menus and the overworld
3. Enter/exit towns
4. Verify no regressions in normal rendering

### Test 5: Other WS/WSC Games

Games that may exercise VTOTAL manipulation:

- **Digimon** series (WSC) — complex visual effects
- **Gunpey** (WSC) — screen transitions
- **Klonoa** (WSC) — parallax scrolling effects
- **Rockman/Megaman** (WSC) — boss transitions
- Any game with split-screen or raster effects

## What to Look For

### Visual Indicators of the Bug

- **Flickering:** Rapid alternation between normal and corrupted frames
- **Tearing:** Horizontal split where top/bottom halves are misaligned
- **White flashes:** Entire frame or partial frame going white momentarily
- **Missing content:** Parts of the screen not rendering (blank/white)
- **Timing issues:** Animation running too fast or skipping frames

### Debugging with Event Viewer

1. Open **Debug → Event Viewer** while in a battle
2. Look for abnormally short frames (fewer than 159 scanlines)
3. Check if VBlank events are firing consistently
4. Monitor VTOTAL register writes ($16) during spell animations

### Using the Debugger

1. Set a breakpoint on port $16 write (`WsPpu::WritePort`, case 0x16)
2. Note the VTOTAL values being written during the fire animation
3. Compare with normal gameplay VTOTAL (should be 158)
4. If the game writes VTOTAL < 144, the fix is relevant

## Expected Results (Post-Fix)

- Fire magic animation renders smoothly with no artifacts
- Frame timing remains consistent (no speedup during animations)
- All spell effects display correctly
- No regressions in normal gameplay
- Event viewer shows consistent frame lengths even during effects

## Technical Details

### Key Code Changes

- [WsPpu.h](../../Core/WS/WsPpu.h) — `Exec()`: Uses `_prevRenderY` for framebuffer position; sprite copy conditioned on `LastScanline >= 144`
- [WsPpu.cpp](../../Core/WS/WsPpu.cpp) — `ProcessHblank()`: Computes `renderY = Scanline % (LastScanline + 1)`
- [WsPpu.cpp](../../Core/WS/WsPpu.cpp) — `ProcessEndOfScanline()`: Frame boundary at `max(ScreenHeight, LastScanline)`, VBlank inhibited for short VTOTAL

### Cross-Emulator Reference

See [ws-ppu-cross-emulator-verification.md](../ws-ppu-cross-emulator-verification.md) for detailed comparison with ares, MAME, and Mednafen.
