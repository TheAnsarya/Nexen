# TAS/Movie Manual Testing Guide

Comprehensive manual testing checklist for all TAS (Tool-Assisted Speedrun) and movie functionality in Nexen.

---

## Prerequisites

- A ROM file loaded and running (NES, SNES, GB, GBA, SMS, PCE, or WS)
- Nexen built in Release mode
- Access to sample movie files in various formats (optional but recommended)

---

## 1. Movie Recording

### 1.1 Record from Power-On

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Load a ROM | Game starts normally | |
| 2 | **Tools > Record Movie** | Record movie dialog appears | |
| 3 | Select "Record from Power On" | Start type set to power-on | |
| 4 | Enter author name | Author field filled | |
| 5 | Click "Record" | Movie recording starts, status bar shows "Recording" | |
| 6 | Play game for ~5 seconds | Game responds to input normally | |
| 7 | **Tools > Stop Recording** | Movie saved, file appears at chosen path | |
| 8 | Verify file exists | `.nexen-movie` file created | |

### 1.2 Record from Save State

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Load a ROM and play for 30+ seconds | Game state established | |
| 2 | **Tools > Record Movie** | Record movie dialog appears | |
| 3 | Select "Record from Save State" | Start type set to savestate | |
| 4 | Click "Record" | Recording starts from current state | |
| 5 | Play game for ~5 seconds | Input recorded from current position | |
| 6 | Stop recording | Movie file includes embedded savestate | |

### 1.3 Record from SRAM

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Load a game with battery save (e.g., RPG) | Game loads existing save | |
| 2 | Record movie with "Record from SRAM" | SRAM data embedded in movie | |
| 3 | Stop recording | Movie file includes battery data | |

### 1.4 Multi-Controller Recording

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Configure 2 controllers in settings | Both controllers active | |
| 2 | Record a movie | Both controller inputs recorded | |
| 3 | Verify movie file contains multi-port input | Input.txt has pipe-separated multi-port data | |

---

## 2. Movie Playback

### 2.1 Basic Playback

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | **Tools > Play Movie** | File picker appears | |
| 2 | Select a `.nexen-movie` file | Movie loads | |
| 3 | Playback begins from appropriate start point | Game plays back recorded input | |
| 4 | Visual output matches original recording | Frame-perfect playback | |
| 5 | Movie reaches end | "Playback finished" shown | |

### 2.2 Playback Controls

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | During playback, press **Pause** | Emulation pauses | |
| 2 | Press **Resume** | Emulation resumes playback | |
| 3 | Press **Frame Advance** (F key in TAS editor) | Advances exactly one frame | |
| 4 | Press **Frame Rewind** (R key in TAS editor) | Rewinds exactly one frame | |

### 2.3 Playback from Save State

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Play a movie recorded from savestate | Savestate loaded first | |
| 2 | Playback matches recording | Deterministic replay from savestate | |

---

## 3. TAS Editor

### 3.1 Opening and Basic UI

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | **Tools > TAS Editor** | TAS Editor window opens | |
| 2 | Observe piano roll | Timeline with button lanes visible | |
| 3 | Observe frame list | Frame numbers listed on left | |
| 4 | Observe toolbar | Play/Pause/Record/Save buttons present | |

### 3.2 Piano Roll Interaction

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Click a cell in the piano roll | Button toggled for that frame | |
| 2 | Click and drag across cells | Multiple frames painted | |
| 3 | Right-click to start selection | Selection range started | |
| 4 | Drag right-click | Selection range extends | |
| 5 | Scroll mouse wheel | Horizontal scroll through frames | |
| 6 | Ctrl+Scroll | Zoom in/out (0.25x to 4.0x) | |

### 3.3 Frame Operations

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Select a frame range | Frames highlighted with selection overlay | |
| 2 | **Insert** key | Blank frames inserted at selection | |
| 3 | **Delete** key | Selected frames removed | |
| 4 | **Ctrl+C** | Selection copied to clipboard | |
| 5 | **Ctrl+V** | Clipboard frames pasted at cursor | |
| 6 | **Ctrl+X** | Selection cut to clipboard | |
| 7 | **Ctrl+Z** | Undo last operation | |
| 8 | **Ctrl+Y** | Redo last undone operation | |

### 3.4 Navigation

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | **Left/Right** arrow keys | Scroll one frame | |
| 2 | **Page Up / Page Down** | Jump 10 frames | |
| 3 | **Home** | Jump to frame 0 | |
| 4 | **End** | Jump to last frame | |
| 5 | **Space** | Toggle playback | |
| 6 | **F** key | Frame advance | |
| 7 | **R** key | Frame rewind | |

### 3.5 Playback Integration

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Press Space to play | Emulation plays through movie frames | |
| 2 | Playback position indicator moves | Red line tracks current frame | |
| 3 | Status bar updates | Shows "Frame N / Total" | |
| 4 | UI updates every 10 frames | Smooth updates without flicker | |

### 3.6 Input Interrupt

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Enable "Interrupt on Input" | Setting active | |
| 2 | Start playback | Movie plays | |
| 3 | Press a keyboard/controller button | Interrupt dialog appears | |
| 4 | Choose Fork/Edit/Continue | Appropriate action taken | |

---

## 4. Movie File Operations

### 4.1 Save/Load

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | In TAS Editor: **Ctrl+S** | Save file dialog (first save) or saved to current path | |
| 2 | Save movie as `.nexen-movie` | File written successfully | |
| 3 | Close and reopen TAS Editor | Previous movie cleared | |
| 4 | **Ctrl+O** to open saved file | Movie loaded correctly | |
| 5 | Verify all frames present | Frame count matches | |
| 6 | Verify controller input preserved | Button presses intact | |

### 4.2 Format Import/Export

Test each supported format for import:

| Format | Extension | Source Emulator | Import | Export |
|--------|-----------|-----------------|--------|--------|
| Nexen | `.nexen-movie` | Nexen | | |
| BK2 | `.bk2` | BizHawk | | |
| FM2 | `.fm2` | FCEUX (NES) | | |
| SMV | `.smv` | Snes9x (SNES) | | |
| LSMV | `.lsmv` | lsnes (SNES) | | |
| VBM | `.vbm` | VBA (GBA) | | |
| GMV | `.gmv` | Gens (Genesis) | | |
| MMO | `.mmo` | Legacy format | | |

For each format:

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Import movie file | File loads without errors | |
| 2 | Verify header info | Author, game name, system type parsed | |
| 3 | Verify frame count | Matches expected count | |
| 4 | Check first/last frame input | Button states match expected | |
| 5 | Play imported movie | Playback works (if matching ROM loaded) | |

---

## 5. Movie File Format

### 5.1 ZIP Structure Verification

The `.nexen-movie` format is a ZIP archive containing:

| File | Required | Description |
|------|----------|-------------|
| `Input.txt` | Yes | Pipe-separated input data, one line per frame |
| `GameSettings.txt` | Yes | Key-value settings (version, system, region, etc.) |
| `SaveState.nexen-save` | No | Embedded savestate (for record-from-state) |
| `Battery.*` | No | Battery/SRAM data (for record-from-SRAM) |
| `MovieInfo.txt` | No | Author, description, timestamps |
| `PatchData.*` | No | IPS/BPS patch data |

### 5.2 Manual Text Editing of Movie File

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Rename `.nexen-movie` to `.zip` | Can open in ZIP tool (7-Zip, etc.) | |
| 2 | Extract `Input.txt` | Text file with pipe-separated lines | |
| 3 | View `Input.txt` format | Lines start with `\|`, ports separated by `\|` | |
| 4 | Edit a frame (change button) | Modified text saved | |
| 5 | Repack ZIP and rename back | Modified movie file | |
| 6 | Play modified movie | Changes reflected in playback | |

### 5.3 Input.txt Format

Each line represents one frame of input:

```
|BYsSUDLRAXLR|............
```

Button positions (SNES example):

| Position | Button |
|----------|--------|
| 1 | B |
| 2 | Y |
| 3 | Select (s) |
| 4 | Start (S) |
| 5 | Up |
| 6 | Down |
| 7 | Left |
| 8 | Right |
| 9 | A |
| 10 | X |
| 11 | L |
| 12 | R |

- Pressed button: letter shown
- Not pressed: `.` (dot)

### 5.4 GameSettings.txt Format

Key-value pairs, one per line:

```
NexenVersion 2.2.0
MovieFormatVersion 2
GameName Super Mario World
Author TASer
SystemType Snes
Region NTSC
ConsoleType Snes
```

---

## 6. Rewind System

### 6.1 Basic Rewind

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Play game for 30+ seconds | Rewind buffer populated | |
| 2 | Hold Rewind button | Game rewinds smoothly | |
| 3 | Release Rewind | Game resumes from rewound position | |
| 4 | Audio plays backward during rewind | Audio plays in reverse (or muted) | |

### 6.2 Rewind Buffer Management

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Check Settings > Emulation > Rewind | Buffer size configurable | |
| 2 | Set buffer to minimum (e.g., 10 seconds) | Only 10 seconds rewindable | |
| 3 | Play for 60 seconds | Oldest states evicted | |
| 4 | Try to rewind past 10 seconds | Stops at buffer limit | |

### 6.3 Rewind + Save States

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Play game, create save state | State saved in slot | |
| 2 | Continue playing for 10 seconds | More states buffered | |
| 3 | Load save state | State loaded, rewind buffer adjusted | |
| 4 | Rewind from loaded state | Works correctly | |

---

## 7. Fast-Forward

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Hold Fast-Forward button | Game runs at increased speed | |
| 2 | Release Fast-Forward | Returns to normal speed | |
| 3 | Fast-Forward during movie playback | Playback advances quickly | |
| 4 | Fast-Forward + Rewind interact | Clean transitions between modes | |

---

## 8. Configuration

### 8.1 TAS Settings

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Settings > Emulation > Rewind buffer size | Configurable in seconds | |
| 2 | Settings > Input > Controller mapping | Controllers configurable | |
| 3 | TAS Editor > Interrupt on Input toggle | Setting persists | |

### 8.2 Movie Settings Dialog

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Author name persistence | Remembered between sessions | |
| 2 | Output file path | Remembers last directory | |
| 3 | Start type radio buttons | Power On / Save State / SRAM | |

---

## 9. Edge Cases

### 9.1 Empty/Minimal Movies

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Record and immediately stop | 0-frame movie file created | |
| 2 | Play 0-frame movie | Playback immediately finishes | |
| 3 | Open empty movie in TAS Editor | Editor shows empty timeline | |

### 9.2 Very Large Movies

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Record 10+ minute movie | Recording stable, no memory issues | |
| 2 | Play back long movie | Playback completes correctly | |
| 3 | Open long movie in TAS Editor | UI remains responsive | |
| 4 | Scroll through all frames | No lag or rendering artifacts | |
| 5 | Clone/copy large movie data | Operation completes | |

### 9.3 Corrupt/Invalid Files

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Load a non-movie ZIP file | Error message, no crash | |
| 2 | Load a truncated movie file | Graceful error handling | |
| 3 | Load a movie with missing Input.txt | Error message about missing data | |
| 4 | Load a movie for wrong system | Incompatibility warning | |
| 5 | Load movie with version mismatch | Version incompatible message | |

### 9.4 Cross-Platform

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Record NES movie | NES input format used | |
| 2 | Record SNES movie | SNES input format (12 buttons) | |
| 3 | Record GB movie | GB input format (8 buttons) | |
| 4 | Record GBA movie | GBA input format | |
| 5 | Record SMS movie | SMS input format | |
| 6 | Record WS movie | WS input format | |

---

## 10. Performance Verification

### 10.1 Recording Performance

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Record while monitoring frame rate | Steady 60 FPS during recording | |
| 2 | Record for 5+ minutes | No gradual slowdown | |
| 3 | Record with multiple controllers | No additional frame drops | |

### 10.2 Playback Performance

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Play movie while monitoring frame rate | Steady 60 FPS during playback | |
| 2 | Play 100K+ frame movie | No slowdown at end | |
| 3 | Frame advance rapidly | Responsive, no stuttering | |

### 10.3 TAS Editor Performance

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Load large movie (100K frames) | Opens quickly | |
| 2 | Scroll rapidly through piano roll | Smooth rendering | |
| 3 | Zoom in/out rapidly | No rendering lag | |
| 4 | Paint input across 100+ frames | Responsive painting | |
| 5 | Undo/Redo rapidly | Instant response | |

### 10.4 Rewind Performance

| # | Step | Expected Result | Pass |
|---|------|-----------------|------|
| 1 | Rewind at 60 FPS | No frame drops | |
| 2 | Rewind through XOR delta states | Seamless reconstruction | |
| 3 | Memory usage during long session | Stable, within buffer limits | |

---

## 11. Regression Tests

After any TAS/movie code change, verify:

| # | Check | Expected Result | Pass |
|---|-------|-----------------|------|
| 1 | All C++ tests pass | 1548+ tests pass | |
| 2 | All .NET tests pass | 474+ tests pass | |
| 3 | All MovieConverter tests pass | 218+ tests pass | |
| 4 | Record → Play round-trip | Identical playback | |
| 5 | Save → Load round-trip | All data preserved | |
| 6 | Format conversion round-trip | Data integrity maintained | |
| 7 | Rewind XOR compression | Lossless state reconstruction | |

---

## Algorithms Reference

See [TAS Algorithm Documentation](algorithms/tas-algorithms.md) for detailed algorithm descriptions.

Key algorithms tested:

- **XOR Delta Compression**: Full state every 30 frames, XOR deltas between
- **Ring Buffer Audio**: Circular buffer for rewind audio samples
- **Text Protocol**: Pipe-delimited controller state format
- **Greenzone Tracking**: Verified frame range for editing safety
