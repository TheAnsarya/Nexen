# 🎮 TAS Editor User Manual

Welcome to the Nexen TAS (Tool-Assisted Speedrun/Superplay) Editor! This comprehensive guide will help you create, edit, and perfect your TAS movies.

## Table of Contents

1. [Getting Started](#getting-started)
2. [User Interface Overview](#user-interface-overview)
3. [Opening and Saving Movies](#opening-and-saving-movies)
4. [Editing Frames](#editing-frames)
5. [Recording Input](#recording-input)
6. [Playback and Seek](#playback-and-seek)
7. [Greenzone System](#greenzone-system)
8. [Piano Roll View](#piano-roll-view)
9. [Branches and Rerecording](#branches-and-rerecording)
10. [Multi-System Support](#multi-system-support)
11. [Keyboard Shortcuts](#keyboard-shortcuts)
12. [Tips and Best Practices](#tips-and-best-practices)

---

## Getting Started

### What is TAS?

TAS (Tool-Assisted Speedrun/Superplay) is a technique for creating video game playthroughs by recording controller inputs one frame at a time. This allows for pixel-perfect precision, frame-perfect timing, and optimization that would be impossible for a human to execute in real-time.

### Opening the TAS Editor

1. Launch Nexen
2. Go to **Tools** → **TAS Editor** (or press `Ctrl+T`)
3. The TAS Editor window will open

### Creating Your First TAS

1. Load a ROM in the main Nexen window
2. Open the TAS Editor
3. Either:
	- **Start Recording**: Begin playing and the editor will capture your inputs
	- **Open Existing**: Load a movie file (`.nmv`, `.bk2`, `.fm2`, etc.)
	- **Start Fresh**: Begin editing an empty movie frame-by-frame

---

## User Interface Overview

The TAS Editor window is divided into several sections:

### Menu Bar

| Menu | Purpose |
| ------ | --------- |
| **File** | Open, Save, Import, Export movies |
| **Edit** | Undo, Redo, Cut, Copy, Paste, Insert/Delete frames |
| **View** | Go to Frame, Toggle Greenzone, Show Piano Roll |
| **Playback** | Play/Pause, Frame Advance, Speed Control |
| **Recording** | Start/Stop Recording, Rerecord, Branches |

### Toolbar

Quick access buttons for common actions:

- **Open/Save** - File operations
- **↶/↷** - Undo/Redo
- **+/-** - Insert/Delete frames
- **⏮/⏯/⏭** - Playback controls
- **●** - Recording indicator
- **GZ** - Greenzone toggle

### Frame List (Left Panel)

Shows all frames in the movie with:

- **Frame #** - Frame number (1-based)
- **P1 Input** - Player 1's button presses
- **P2 Input** - Player 2's button presses
- **Lag** - Lag frame indicator
- **Comment** - Notes and markers

Color coding:

- 🟢 **Green** - Greenzone (safe to rerecord from)
- 🔴 **Red** - Lag frame
- ⬜ **White** - Normal frame

### Properties Panel (Right Panel)

- **Frame Properties** - Current frame info
- **Controller Layout** - Switch between system layouts
- **Controller Buttons** - Click to toggle inputs
- **Movie Info** - Format, frame count, rerecords, author
- **Branches** - Saved alternate versions

### Piano Roll (Bottom Panel)

Visual timeline showing button presses across frames. Enable via **View** → **Show Piano Roll**.

### Status Bar

Shows current status, selected frame, total frames, rerecord count, and recording mode.

---

## Opening and Saving Movies

### Supported Formats

| Format | Extension | Read | Write | System |
| -------- | ----------- | ------ | ------- | -------- |
| Nexen Movie | `.nmv` | ✅ | ✅ | All |
| BizHawk | `.bk2` | ✅ | ✅ | All |
| FCEUX | `.fm2` | ✅ | ✅ | NES |
| Snes9x | `.smv` | ✅ | ✅ | SNES |
| lsnes | `.lsmv` | ✅ | ✅ | SNES |
| VBA | `.vbm` | ✅ | ⚠️ | GB/GBA |
| Gens | `.gmv` | ✅ | ⚠️ | Genesis |

### Opening a Movie

1. **File** → **Open** (or drag-and-drop)
2. Select a movie file
3. The movie loads and frames appear in the list

### Saving a Movie

1. **File** → **Save** (overwrites current file)
2. **File** → **Save As...** (save to new location)

### Importing from Other Formats

1. **File** → **Import...**
2. Select source movie file
3. Movie is converted and loaded (marked as unsaved)

### Exporting to Other Formats

1. **File** → **Export...**
2. Choose target format and location
3. Movie is converted and saved

---

## Editing Frames

### Selecting Frames

- **Click** a frame to select it
- The selected frame is highlighted
- Properties panel shows frame details

### Modifying Input

**Using Button Panel:**

1. Select a frame
2. Click buttons in the Controller panel
3. Buttons toggle on/off

**Using Keyboard:**

- Press button keys while frame is selected (configurable)

**Using Piano Roll:**

- Click cells to toggle buttons
- Drag to paint multiple frames

### Insert/Delete Frames

**Insert Frame:**

1. Select position
2. **Edit** → **Insert Frame(s)** or press `Insert`
3. New empty frame is inserted

**Delete Frame:**

1. Select frame(s)
2. **Edit** → **Delete Frame(s)** or press `Delete`

### Copy/Paste Frames

1. Select source frame(s)
2. **Edit** → **Copy** (`Ctrl+C`)
3. Select destination
4. **Edit** → **Paste** (`Ctrl+V`)

### Cut Frames

1. Select frame(s)
2. **Edit** → **Cut** (`Ctrl+X`)
3. Frames are removed and copied to clipboard

### Clear Input

1. Select frame
2. **Edit** → **Clear Input**
3. All buttons cleared on that frame

### Add Comments/Markers

1. Select frame
2. **Edit** → **Set Comment**
3. Enter your note (appears in frame list and piano roll)

---

## Recording Input

### Starting Recording

1. Ensure a movie is loaded or create new
2. **Recording** → **Start Recording**
3. Play the game normally
4. Your inputs are captured frame-by-frame

### Recording Modes

| Mode | Description |
| ------ | ------------- |
| **Append** | Add frames to end of movie |
| **Insert** | Insert frames at current position |
| **Overwrite** | Replace existing frames |

Change mode via **Recording** menu before starting.

### Stopping Recording

- **Recording** → **Stop Recording**
- Or click the recording button in toolbar
- Movie is paused and you can edit

### Auto Lag Detection

Recording automatically detects lag frames when input wasn't polled by the game.

---

## Playback and Seek

### Basic Playback

| Action | How |
| -------- | ----- |
| Play/Pause | **Playback** → **Play/Pause** or `Space` |
| Stop | **Playback** → **Stop** |
| Frame Advance | **Playback** → **Frame Advance** or `F` |
| Frame Rewind | **Playback** → **Frame Rewind** or `R` |

### Speed Control

Change playback speed via **Playback** menu:

- 0.25x (slow motion)
- 0.5x
- 1.0x (normal)
- 2.0x
- 4.0x (fast forward)

### Seeking to Frames

**Go to Frame:**

1. **View** → **Go to Frame...**
2. Enter frame number
3. Editor jumps to that frame

**Seek with Greenzone:**

1. **Playback** → **Seek to Frame...**
2. Uses greenzone savestates for instant seeking

---

## Greenzone System

### What is Greenzone?

The greenzone is a system of automatic savestates that enables:

- **Instant seeking** to any frame with a savestate
- **Frame-accurate rewind** without replay
- **Efficient rerecording** without starting from beginning

### How It Works

1. As you play/record, savestates are captured at intervals
2. Frames with savestates show as green in frame list
3. When you seek/rewind, nearest savestate is loaded
4. Remaining frames are replayed if needed

### Greenzone Settings

Access via **View** → **Greenzone Settings...**

| Setting | Default | Description |
| --------- | --------- | ------------- |
| Capture Interval | 60 | Frames between auto-captures |
| Max Savestates | 1000 | Memory limit (oldest are pruned) |
| Ring Buffer Size | 120 | Recent frames for instant rewind |
| Compression | On | Compress old states to save memory |

### Memory Usage

The status bar shows greenzone memory usage. If memory is high:

- Reduce Max Savestates
- Increase Capture Interval
- Enable Compression

### Invalidation

When you modify a frame, all greenzone states after that frame are invalidated (removed) since they're no longer valid.

---

## Piano Roll View

### Enabling Piano Roll

**View** → **Show Piano Roll**

The piano roll appears at the bottom of the window.

### Reading the Piano Roll

- **Horizontal axis**: Frames (time)
- **Vertical axis**: Buttons (lanes)
- **Blue cells**: Button pressed
- **Red line**: Current playback position
- **Orange markers**: Comments/markers

### Editing in Piano Roll

**Toggle Single Cell:**

- Click a cell to toggle that button on that frame

**Paint Multiple Cells:**

- Click and drag horizontally to paint a button across frames

**Zoom:**

- `Ctrl+Scroll` to zoom in/out
- Or `Ctrl++`/`Ctrl+-`

**Scroll:**

- Scroll wheel to scroll horizontally
- Arrow keys to navigate

### Selection

Right-click and drag to select a range of frames in the piano roll.

---

## Branches and Rerecording

### What is a Branch?

A branch is a saved copy of your entire movie that you can return to later. Useful for:

- Trying different strategies
- Keeping a backup before risky edits
- Comparing different approaches

### Creating a Branch

1. **Recording** → **Create Branch**
2. Branch is saved with timestamp
3. Appears in Branches list in right panel

### Loading a Branch

1. Select branch in Branches list
2. Double-click or right-click → Load
3. Current movie is replaced with branch contents

### Rerecording

Rerecording lets you "go back in time" and try a section again:

1. Select the frame to rerecord from
2. **Recording** → **Rerecord from Here**
3. Greenzone loads the savestate
4. Movie is truncated to that point
5. Recording starts automatically

The rerecord count in the status bar tracks how many times you've rerecorded.

---

## Multi-System Support

The TAS Editor supports all systems emulated by Nexen:

| System | Controller Layout | Buttons |
| -------- | ------------------ | --------- |
| NES | 8 buttons | A, B, Select, Start, D-Pad |
| SNES | 12 buttons | A, B, X, Y, L, R, Select, Start, D-Pad |
| Game Boy | 8 buttons | A, B, Select, Start, D-Pad |
| GBA | 10 buttons | A, B, L, R, Select, Start, D-Pad |
| Genesis | 11 buttons | A, B, C, X, Y, Z, Start, D-Pad |
| Master System | 6 buttons | 1, 2, D-Pad |
| PC Engine | 8 buttons | I, II, Select, Run, D-Pad |
| WonderSwan | 11 buttons | A, B, Start, X1-X4, Y1-Y4 |

### Changing Layout

1. Open Controller Layout dropdown in right panel
2. Select target system
3. Button panel updates accordingly

### Auto-Detection

When opening a movie, the editor auto-detects the correct layout from:

1. Movie's system type (most reliable)
2. Movie's source format (fallback)

---

## Keyboard Shortcuts

### Global

| Shortcut | Action |
| ---------- | -------- |
| `Ctrl+O` | Open movie |
| `Ctrl+S` | Save movie |
| `Ctrl+Shift+S` | Save As |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+C` | Copy |
| `Ctrl+X` | Cut |
| `Ctrl+V` | Paste |
| `Insert` | Insert frame |
| `Delete` | Delete frame |

### Playback

| Shortcut | Action |
| ---------- | -------- |
| `Space` | Play/Pause |
| `F` | Frame Advance |
| `R` | Frame Rewind |
| `Escape` | Stop |

### Navigation

| Shortcut | Action |
| ---------- | -------- |
| `Home` | Go to first frame |
| `End` | Go to last frame |
| `Page Up` | Jump back 100 frames |
| `Page Down` | Jump forward 100 frames |
| `Ctrl+G` | Go to frame... |

### Recording

| Shortcut | Action |
| ---------- | -------- |
| `Ctrl+R` | Toggle recording |
| `Ctrl+B` | Create branch |

---

## Tips and Best Practices

### For Beginners

1. **Start small** - Practice on short segments before full runs
2. **Use greenzone** - Keep it enabled for easy rerecording
3. **Save often** - Create branches before risky sections
4. **Watch tutorials** - Learn from existing TAS creators

### For Optimization

1. **Frame advance constantly** - Find frame-perfect inputs
2. **Test alternatives** - Use branches to compare strategies
3. **Study lag frames** - Minimize lag through input timing
4. **Check memory** - Watch RAM values for RNG manipulation

### For Workflow

1. **Organize branches** - Name them descriptively
2. **Add comments** - Mark important frames for later
3. **Backup regularly** - Export to multiple formats
4. **Track rerecords** - Lower is often better optimized

### Common Issues

| Issue | Solution |
| ------- | ---------- |
| Movie desyncs | Verify same ROM, check header |
| High memory usage | Increase greenzone interval |
| Slow seeking | Enable greenzone compression |
| Wrong buttons | Check controller layout matches system |

---

## Need Help?

- Check the [Nexen Documentation](../README.md)
- Report issues on [GitHub](https://github.com/TheAnsarya/Nexen/issues)
- Join the community discussions

Happy TASing! 🎮✨
