# Nexen v1.0.0 Manual Testing Checklist

> **Purpose:** Comprehensive testing checklist for all Nexen features implemented since forking from Mesen2.
> **Date Created:** 2026-02-02
> **Target Version:** v1.0.0

---

## How to Use This Checklist

### Marking Tests

Work through each section sequentially. Mark tests using one of these methods:

| Status | Markdown Syntax | Rendered | When to Use |
|--------|-----------------|----------|-------------|
| Not tested | `- [ ]` | - [ ] | Default state |
| **Passed** | `- [x]` | - [x] | Test passed |
| **Failed** | `- [!]` or note | - [!] | Test failed - add details below |
| **Skipped** | `- [-]` or `- [~]` | - [-] | Intentionally skipped |

### Examples

**Unchecked (not yet tested):**
```markdown
- [ ] ROM loads without error
- [ ] Graphics display correctly
```

**Checked (test passed):**
```markdown
- [x] ROM loads without error
- [x] Graphics display correctly
```

**Mixed results:**
```markdown
- [x] ROM loads without error
- [!] Graphics display correctly - FAILED: sprite flicker on frame 1200
- [-] PAL mode test - SKIPPED: no PAL ROM available
```

### Recording Failures

When a test fails:
1. Mark it with `[!]` or leave `[ ]` and add a note
2. Add details inline: `- [!] Test name - FAILED: brief description`
3. Also record in the "Issues Found" section at the bottom with full reproduction steps

### Fill-in Fields

Some tests have blank fields like `______`. Fill these in during testing:
```markdown
Before: - Note current frame: ______
After:  - Note current frame: **1523**
```

---

## Table of Contents

1. [Test Environment Setup](#1-test-environment-setup)
2. [TAS Editor Testing](#2-tas-editor-testing)
3. [Save State System Testing](#3-save-state-system-testing)
4. [Movie Converter Testing](#4-movie-converter-testing)
5. [Pansy Export Testing](#5-pansy-export-testing)
6. [Keyboard Shortcuts Testing](#6-keyboard-shortcuts-testing)
7. [UI/UX Testing](#7-uiux-testing)
8. [Multi-System Testing](#8-multi-system-testing)
9. [Performance Testing](#9-performance-testing)
10. [Build & Platform Testing](#10-build--platform-testing)

---

## 1. Test Environment Setup

> **Goal:** Prepare your testing environment with all required ROMs and a clean Nexen installation.

### 1.1 Required Test ROMs

**Instructions:** Gather at least one ROM for each system. Store them in a dedicated test folder (e.g., `C:\NexenTest\ROMs\`).

| System | Recommended Test ROM | Your ROM | ✓ |
|--------|---------------------|----------|---|
| NES | Super Mario Bros. or Mega Man 2 | | [ ] |
| SNES | Super Mario World or Chrono Trigger | | [ ] |
| Game Boy | Pokemon Red/Blue or Tetris | | [ ] |
| Game Boy Color | Pokemon Crystal or Zelda DX | | [ ] |
| Game Boy Advance | Metroid Fusion or Pokemon Emerald | | [ ] |
| Master System | Sonic the Hedgehog | | [ ] |
| Game Gear | Sonic Triple Trouble | | [ ] |
| PC Engine | Bonk's Adventure | | [ ] |
| WonderSwan | Any title | | [ ] |

### 1.2 Clean Installation

**Instructions:** Perform these steps to ensure a fresh test environment.

1. **Close Nexen** if it's running
2. **Delete settings folder:**
   - Windows: `%APPDATA%\Nexen\` or `%LOCALAPPDATA%\Nexen\`
   - Linux: `~/.config/Nexen/`
   - macOS: `~/Library/Application Support/Nexen/`
3. **Restart Nexen** - Verify it opens with default settings
4. **Record your test system:**
   - OS: ____________________
   - CPU: ____________________
   - RAM: ____________________
   - GPU: ____________________

- [ ] Clean installation verified - Nexen starts with fresh settings

### 1.3 Pre-Test Checklist

- [ ] Screen recording software ready (OBS, ShareX, etc.) for bug reproduction
- [ ] Test ROM folder accessible
- [ ] Notepad/text editor open for notes
- [ ] At least 30 minutes of uninterrupted testing time

---

## 2. TAS Editor Testing

> **Goal:** Verify all TAS Editor functionality including frame editing, playback, recording, and the greenzone system.

### 2.1 Opening the TAS Editor

**Instructions:** Open a game and then open the TAS Editor.

1. **Launch Nexen**
2. **File → Open** and select an NES ROM (e.g., Super Mario Bros.)
3. **Wait** for the game to start playing
4. **Tools → TAS Editor** (or press the TAS Editor shortcut if configured)

**Expected Result:** TAS Editor window opens alongside the main emulator window.

- [ ] TAS Editor window opens without crash
- [ ] Window title shows "TAS Editor" or similar
- [ ] Menu bar visible with: File, Edit, Playback, Recording, View, Tools
- [ ] Toolbar visible with playback buttons (Play, Pause, Stop, Frame Advance)
- [ ] Status bar at bottom shows frame count (should show "Frame: 0" or similar)
- [ ] Frame list/grid is visible in the main area

### 2.2 Frame List View

**Instructions:** Examine the frame list and test basic navigation.

1. **Look at the frame list** - Should show at least frame 0
2. **Use mouse wheel** to scroll up and down
3. **Click on frame 50** - Frame should become selected/highlighted
4. **Hold Shift, click frame 100** - Frames 50-100 should be selected
5. **Look at columns** - Should see frame number and input columns (P1, maybe P2)

- [ ] Frame 0 visible in list
- [ ] Mouse wheel scrolls the frame list smoothly
- [ ] Single click selects a frame (visual highlight)
- [ ] Shift+click selects a range of frames
- [ ] Frame number column displays correctly
- [ ] Player 1 input columns display (buttons like A, B, Start, etc.)

### 2.3 Input Editing - Button Toggle

**Instructions:** Test toggling individual buttons on and off.

1. **Navigate to frame 10** in the frame list
2. **Find the "A" button column** for Player 1
3. **Click the cell** at frame 10, column A
4. **Observe:** The cell should change appearance (color/icon) to indicate "ON"
5. **Click the same cell again**
6. **Observe:** The cell should return to "OFF" state

Repeat for each button:

| Button | Toggle ON works | Toggle OFF works | ✓ |
|--------|-----------------|------------------|---|
| A | | | [ ] |
| B | | | [ ] |
| Start | | | [ ] |
| Select | | | [ ] |
| Up | | | [ ] |
| Down | | | [ ] |
| Left | | | [ ] |
| Right | | | [ ] |

- [ ] All button toggles work correctly
- [ ] Visual feedback clearly shows ON vs OFF state

### 2.4 Input Editing - D-Pad Combinations

**Instructions:** Test diagonal and opposite direction inputs.

1. **Go to frame 20**
2. **Set Up + Right** both ON
3. **Play the movie** - Character should move diagonally up-right (if applicable)
4. **Go to frame 30**
5. **Set Up + Down** both ON (opposite directions)
6. **Note the behavior:** Some games allow this, others don't

- [ ] Diagonal input (Up+Right) can be set
- [ ] Diagonal input (Up+Left) can be set
- [ ] Diagonal input (Down+Right) can be set
- [ ] Diagonal input (Down+Left) can be set
- [ ] Opposite directions (Up+Down) behavior noted: ____________
- [ ] Opposite directions (Left+Right) behavior noted: ____________

### 2.5 Frame Operations - Insert

**Instructions:** Test inserting blank frames into the movie.

1. **Note the current movie length** (shown in status bar or frame list)
   - Starting length: ______ frames
2. **Click on frame 50** to select it
3. **Edit → Insert Frame** (or Ctrl+Shift+I)
4. **Observe:** A new blank frame should appear at position 50
5. **Check movie length** - Should be +1 from before
   - New length: ______ frames
6. **Undo** (Ctrl+Z) - Frame should be removed

- [ ] Insert Frame adds a new frame at selection
- [ ] Movie length increases by 1
- [ ] Inserted frame is blank (no inputs)
- [ ] Undo removes the inserted frame

**Test edge cases:**

7. **Select frame 0** and insert - Should insert at beginning
8. **Select last frame** and insert - Should insert at end

- [ ] Insert at frame 0 works
- [ ] Insert at last frame works

### 2.6 Frame Operations - Delete

**Instructions:** Test deleting frames from the movie.

1. **Note current movie length:** ______ frames
2. **Click on frame 50** to select it
3. **Press Delete key** (or Edit → Delete Frame)
4. **Observe:** Frame 50 should be removed
5. **Check movie length** - Should be -1 from before
6. **Undo** (Ctrl+Z) - Frame should be restored

- [ ] Delete removes the selected frame
- [ ] Movie length decreases by 1
- [ ] Undo restores the deleted frame

**Test multiple deletion:**

7. **Select frames 60-70** (click 60, Shift+click 70)
8. **Press Delete**
9. **Verify** 11 frames were removed

- [ ] Multiple frame deletion works
- [ ] Correct number of frames removed

### 2.7 Frame Operations - Copy/Paste

**Instructions:** Test copying and pasting frames.

1. **Go to frame 20** and set inputs: A=ON, Right=ON
2. **Select frame 20**
3. **Ctrl+C** (Copy)
4. **Select frame 100**
5. **Ctrl+V** (Paste)
6. **Check frame 100** - Should have A=ON, Right=ON

- [ ] Copy (Ctrl+C) works without error
- [ ] Paste (Ctrl+V) inserts copied frame
- [ ] Pasted frame has correct inputs

**Test Cut:**

7. **Select frame 20**
8. **Ctrl+X** (Cut)
9. **Verify** frame 20 is removed
10. **Select frame 50**
11. **Ctrl+V** (Paste)
12. **Verify** inputs appear at frame 50

- [ ] Cut (Ctrl+X) removes the frame
- [ ] Paste after cut works correctly

### 2.8 Undo/Redo System

**Instructions:** Verify the undo/redo functionality thoroughly.

1. **Perform these actions in order:**
   - Toggle A button at frame 10
   - Toggle B button at frame 20
   - Delete frame 30
   - Insert frame at 40
2. **Press Ctrl+Z four times**
3. **Verify:** All changes are undone (frame 30 restored, etc.)
4. **Press Ctrl+Y four times** (or Ctrl+Shift+Z)
5. **Verify:** All changes are redone

- [ ] Each Ctrl+Z undoes one action
- [ ] Multiple undos work in sequence
- [ ] Ctrl+Y redoes undone actions
- [ ] Redo restores exact state

### 2.9 Playback Controls

**Instructions:** Test movie playback functionality.

**Setup:** Create a simple test movie first:
1. Set frame 0: Start=ON
2. Set frames 10-20: Right=ON
3. Set frame 30: A=ON

**Test Play:**
4. **Go to frame 0**
5. **Click Play button** (or Playback → Play)
6. **Watch the game** - It should start, move right, then jump

- [ ] Play button starts playback
- [ ] Inputs from movie are applied to game
- [ ] Frame counter advances during playback

**Test Pause:**
7. **Click Pause button** during playback
8. **Verify** game freezes, frame counter stops

- [ ] Pause button stops playback
- [ ] Game state is frozen

**Test Stop:**
9. **Click Stop button**
10. **Verify** playback stops and returns to frame 0

- [ ] Stop button ends playback
- [ ] Returns to beginning of movie

### 2.10 Frame Advance/Rewind

**Instructions:** Test frame-by-frame navigation.

1. **Go to frame 50**
2. **Press Frame Advance** (F key or button)
3. **Verify** frame counter shows 51
4. **Press Frame Advance 5 more times**
5. **Verify** frame counter shows 56

- [ ] Frame Advance moves forward one frame
- [ ] Each press advances exactly one frame
- [ ] Game state updates on each advance

6. **Press Frame Rewind** (Shift+F or button)
7. **Verify** frame counter shows 55
8. **Press Frame Rewind 5 more times**
9. **Verify** frame counter shows 50

- [ ] Frame Rewind moves backward one frame
- [ ] Each press rewinds exactly one frame
- [ ] Game state updates (loads from greenzone)

### 2.11 Speed Control

**Instructions:** Test playback speed options.

1. **Start playback**
2. **Set speed to 0.25x** (Playback menu or dropdown)
3. **Observe:** Game runs at quarter speed

- [ ] 0.25x speed works (noticeably slow)

4. **Set speed to 0.5x**
- [ ] 0.5x speed works (half speed)

5. **Set speed to 1x**
- [ ] 1x speed works (normal)

6. **Set speed to 2x**
- [ ] 2x speed works (fast)

7. **Set speed to 4x**
- [ ] 4x speed works (very fast)

### 2.12 Seeking with Greenzone

**Instructions:** Test fast seeking using the greenzone savestate system.

1. **Go to frame 0**
2. **Play for about 5 seconds** to generate greenzone states
3. **Stop playback**
4. **Double-click frame 200** (or use seek function)
5. **Time how long it takes** to reach frame 200

- [ ] Seeking to frame 200 completes (doesn't hang)
- [ ] Seek time is reasonable (under 5 seconds with greenzone)

6. **Seek back to frame 100**
7. **Verify** game state at frame 100 is correct

- [ ] Backward seeking works
- [ ] Game state is accurate after seek

### 2.13 Piano Roll View

**Instructions:** Test the visual piano roll input editor.

1. **View → Piano Roll** (or toggle button)
2. **Verify** piano roll panel appears

- [ ] Piano roll view opens

3. **Look at the display:**
   - Rows = frames (time)
   - Columns = buttons (lanes)
   - Filled cells = button pressed

- [ ] Visual timeline displays correctly
- [ ] Button lanes are labeled

4. **Click a cell** in the piano roll to toggle input
- [ ] Cell click toggles button ON

5. **Click and drag** across multiple cells
- [ ] Drag painting works (multiple cells toggled)

6. **Try zoom in** (Ctrl+Mousewheel or zoom button)
- [ ] Zoom in makes cells larger

7. **Try zoom out**
- [ ] Zoom out makes cells smaller

8. **Look for the playback cursor** (vertical line at current frame)
- [ ] Playback cursor is visible

9. **Look for greenzone indication** (frames with savestates)
- [ ] Greenzone frames are visually marked

### 2.14 Recording Mode

**Instructions:** Test recording new inputs into the movie.

1. **Recording → Start Recording**
2. **Verify** status bar shows "RECORDING" or "REC"

- [ ] Recording indicator appears in status bar

3. **Start playback** and press buttons on your controller/keyboard
4. **Stop recording** after about 3 seconds
5. **Review the frames** - Your inputs should be recorded

- [ ] Controller inputs are captured during recording
- [ ] Inputs appear in the frame list

**Test Recording Modes:**

6. **Select "Append" mode** - New inputs add after current frame
- [ ] Append mode adds frames at end

7. **Select "Overwrite" mode** - New inputs replace existing frames
- [ ] Overwrite mode replaces existing inputs

8. **Select "Insert" mode** - New inputs insert at current position
- [ ] Insert mode inserts new frames

### 2.15 Rerecording

**Instructions:** Test the rerecording workflow (load state, continue recording).

1. **Note the rerecord count** in status bar: ______
2. **Go to frame 100**
3. **Load state** (greenzone) at frame 100
4. **Start recording** and add new inputs
5. **Stop recording**
6. **Check rerecord count** - Should have increased

- [ ] Rerecord count increments when re-recording
- [ ] Old inputs after frame 100 are replaced (or new branch created)

### 2.16 Branch Management

**Instructions:** Test creating and switching between movie branches.

1. **Recording → Create Branch**
2. **Name the branch** "Test Branch 1"
3. **Verify** branch appears in the branches list

- [ ] Create Branch adds new branch
- [ ] Branch is named correctly

4. **Make some input changes** to this branch
5. **Recording → Create Branch** again ("Test Branch 2")
6. **Switch to "Test Branch 1"** by double-clicking it

- [ ] Can switch between branches
- [ ] Each branch has its own inputs

7. **Delete "Test Branch 2"**
- [ ] Branch deletion works

### 2.17 Greenzone System Details

**Instructions:** Verify the greenzone savestate system is working.

1. **Look at the TAS Editor status bar** for greenzone info:
   - Savestates: ______ (should be > 0 after playing)
   - Memory: ______ MB

- [ ] Greenzone savestate count displayed
- [ ] Memory usage displayed

2. **Play the movie for 60 seconds** to generate many states
3. **Check greenzone count again:** ______

- [ ] Savestates are captured automatically during playback

4. **Seek backward 1000 frames**
5. **Time the seek:** ______ seconds

- [ ] Greenzone enables fast backward seeking

### 2.18 Multi-Controller Support

**Instructions:** Test editing inputs for multiple players.

1. **Load a 2-player game** (e.g., Super Mario Bros. for NES allows 2P)
2. **Look for P2 column** in the frame list

- [ ] Player 2 columns visible (if game supports)

3. **Edit P2 inputs** at frame 50
- [ ] P2 input editing works

4. **Change controller layout** (if option exists)
   - Try NES, then SNES layout
- [ ] Layout changes button columns appropriately

### 2.19 File Operations

**Instructions:** Test saving and loading TAS movies.

1. **File → New Movie**
2. **Confirm** you want to discard changes (if prompted)

- [ ] New Movie creates empty movie
- [ ] Prompts to save if unsaved changes exist

3. **Add some inputs** (at least 10 frames)
4. **File → Save** (or Ctrl+S)
5. **Choose a location** and name: `test_movie.nmv`

- [ ] Save dialog appears
- [ ] Movie saves without error
- [ ] File created on disk

6. **File → New Movie** (discard current)
7. **File → Open Movie** and select `test_movie.nmv`

- [ ] Open dialog appears
- [ ] Movie loads correctly
- [ ] Inputs are preserved

8. **File → Save As** and save as `test_movie_copy.nmv`
- [ ] Save As creates a copy

9. **Close TAS Editor** window
10. **If prompted to save** - Click "Don't Save"

- [ ] Close prompts for unsaved changes

---

## 3. Save State System Testing

> **Goal:** Verify the infinite savestate browser system and all save state functionality.
>
> ⚠️ **Important:** Nexen uses an **infinite savestate browser** system, NOT numbered slots like traditional emulators. The F1/Shift+F1 shortcuts open the savestate browser rather than saving/loading to numbered slots.

### 3.1 Infinite Savestate Browser

**Background:** Unlike emulators with fixed slots (Slot 1-10), Nexen allows unlimited savestates organized in a browser interface with thumbnails, timestamps, and names.

#### 3.1.1 Creating a Savestate

**Instructions:** Create a savestate using the keyboard shortcut.

1. **Load a game** (any NES or SNES ROM)
2. **Play for about 10-15 seconds** until you reach a memorable point
3. **Press Shift+F1** to create a savestate

**Expected Result:** A savestate is created and added to the browser. You may see:
- A brief on-screen confirmation message
- The savestate browser opens showing your new state
- A thumbnail preview is captured

- [ ] Shift+F1 creates a new savestate
- [ ] Confirmation message or visual feedback appears
- [ ] No crash or error occurs

4. **Continue playing** for another 10 seconds
5. **Press Shift+F1 again** to create a second savestate

- [ ] Second savestate is created separately (not overwriting first)
- [ ] Both savestates exist independently

#### 3.1.2 Opening the Savestate Browser

**Instructions:** Open the savestate browser to view your savestates.

1. **Press F1** to open the savestate browser

**Expected Result:** A browser window or panel opens showing your savestates.

- [ ] F1 opens the savestate browser (NOT instant-load)
- [ ] Browser window displays list of savestates
- [ ] Your previously created savestates are visible

2. **Look at the browser interface:**
   - List or grid of savestates
   - Thumbnail images
   - Timestamps or creation dates
   - State names (if applicable)

- [ ] Savestates are displayed in a list or grid
- [ ] Thumbnail previews are visible
- [ ] Creation time/date is shown
- [ ] Interface is responsive and not frozen

#### 3.1.3 Loading a Savestate from Browser

**Instructions:** Load a savestate from the browser.

1. **With browser open**, find your first savestate (the older one)
2. **Double-click** the savestate (or select and press Enter/Load button)

**Expected Result:** Game jumps back to that exact moment.

- [ ] Double-click loads the savestate
- [ ] Game returns to saved position/state
- [ ] Video displays correctly (no corruption)
- [ ] Audio continues correctly (no glitches)

3. **Close the browser** (Escape key or close button)

- [ ] Browser closes without issue

#### 3.1.4 Delete a Savestate

**Instructions:** Delete a savestate from the browser.

1. **Open the browser** (F1)
2. **Select one of your test savestates** (click once to select)
3. **Press Delete key** or use the delete button/menu option
4. **Confirm deletion** if prompted

- [ ] Savestate is removed from the list
- [ ] Other savestates are not affected
- [ ] Disk space is freed (file deleted)

#### 3.1.5 Rename a Savestate

**Instructions:** Give a savestate a custom name.

1. **Open the browser** (F1)
2. **Right-click a savestate** (or use context menu)
3. **Select "Rename"** or similar option
4. **Enter a name:** "Test State - Boss Fight"
5. **Press Enter** to confirm

- [ ] Rename option is available
- [ ] Custom name is saved
- [ ] Name displays in browser

### 3.2 Savestate Verification

#### 3.2.1 Game State Accuracy

**Instructions:** Verify that savestates perfectly restore game state.

1. **Load Super Mario Bros.** (or similar game)
2. **Play until you have:**
   - Specific score (note it): ______
   - Specific number of lives: ______
   - Specific position in level
   - A power-up (mushroom/fire flower)
3. **Create a savestate** (Shift+F1)
4. **Continue playing:** Get hit, lose a life, get more points
5. **Load the savestate** (F1 → select → load)
6. **Verify:**
   - Score matches noted value
   - Lives match noted value
   - Position is exact
   - Power-up is restored

- [ ] Score exactly restored: ______
- [ ] Lives exactly restored: ______
- [ ] Position exactly restored
- [ ] Power-up state restored
- [ ] Music/sound continues correctly

#### 3.2.2 Audio State

**Instructions:** Verify audio doesn't glitch after loading.

1. **Play a game with music** (Super Mario Bros, Zelda, etc.)
2. **Create savestate during music**
3. **Continue playing for 30 seconds**
4. **Load the savestate**
5. **Listen carefully** for:
   - Music is correct
   - No audio pops/clicks
   - No missing channels
   - Sound effects work

- [ ] Music continues correctly after load
- [ ] No audio artifacts (pops, clicks)
- [ ] All audio channels present

### 3.3 Thumbnail System

#### 3.3.1 Thumbnail Capture

**Instructions:** Verify thumbnails accurately represent the savestate.

1. **Load a game** with distinct visual scenes
2. **Navigate to a recognizable screen** (title screen, specific level, boss)
3. **Create a savestate** (Shift+F1)
4. **Open the browser** (F1)
5. **Look at the thumbnail** for your new state

- [ ] Thumbnail shows correct screen content
- [ ] Thumbnail is recognizable (not corrupted)
- [ ] Thumbnail resolution is usable (can identify location)

#### 3.3.2 Thumbnail Generation Performance

**Instructions:** Check that thumbnail generation doesn't lag.

1. **During active gameplay**, create a savestate (Shift+F1)
2. **Note any hesitation or freeze** in the game
3. **Repeat 5 times** in quick succession

- [ ] Savestate creation is near-instant (< 0.5 seconds)
- [ ] No noticeable gameplay interruption
- [ ] Multiple quick saves don't cause issues

### 3.4 Per-Game Organization

**Instructions:** Verify savestates are organized per-game.

1. **Load Game A** (e.g., Super Mario Bros.)
2. **Create a savestate** (Shift+F1)
3. **Close Game A**
4. **Load Game B** (e.g., Mega Man 2)
5. **Create a savestate** (Shift+F1)
6. **Open the browser** (F1)

- [ ] Only Game B's savestates are shown
- [ ] Game A's savestates are NOT mixed in

7. **Close Game B**
8. **Reload Game A**
9. **Open the browser** (F1)

- [ ] Game A's savestates are available again
- [ ] Game B's savestates are NOT shown

### 3.5 Edge Cases

#### 3.5.1 Many Savestates

**Instructions:** Test behavior with many savestates.

1. **Create 20 savestates** in rapid succession (press Shift+F1 every few seconds)
2. **Open the browser** (F1)

- [ ] Browser handles 20+ savestates
- [ ] Scrolling/navigation works
- [ ] No excessive memory usage

#### 3.5.2 Large Savestates (SNES)

**Instructions:** Test savestates with larger-memory systems.

1. **Load an SNES game** (they have larger RAM)
2. **Create a savestate**
3. **Load the savestate**

- [ ] SNES savestates work correctly
- [ ] No noticeable delay compared to NES

---

## 4. Movie Converter Testing

> **Goal:** Test importing and exporting TAS movies in various formats.

### 4.1 FM2 Format (FCEUX NES Movies)

**Instructions:** Test importing an FCEUX movie.

**Preparation:** Obtain a test FM2 file from TASVideos.org or FCEUX recordings.

1. **Load the NES ROM** that the FM2 was recorded on
2. **Open TAS Editor** (Tools → TAS Editor)
3. **File → Import Movie**
4. **Select the FM2 file**
5. **Observe import progress**

- [ ] Import dialog accepts FM2 files
- [ ] Import completes without error
- [ ] Frame count matches original: ______ frames

6. **Play the imported movie**
7. **Watch for 30 seconds** or until completion

- [ ] Inputs sync correctly with game
- [ ] No desync (game follows expected path)

**Export Test:**

8. **File → Export Movie → FM2 format**
9. **Save as test_export.fm2**

- [ ] Export dialog appears
- [ ] File is created

10. **Re-import the exported file**
- [ ] Roundtrip: Original → Export → Import matches

### 4.2 SMV Format (Snes9x Movies)

**Instructions:** Test importing a Snes9x movie.

1. **Load the SNES ROM** that the SMV was recorded on
2. **File → Import Movie**
3. **Select the SMV file**

- [ ] Import completes without error
- [ ] Inputs sync correctly during playback

**Export Test:**

4. **File → Export Movie → SMV format**
- [ ] Export creates valid SMV file

### 4.3 BK2 Format (BizHawk Movies)

**Instructions:** Test importing a BizHawk movie.

1. **Load the appropriate ROM**
2. **File → Import Movie**
3. **Select the BK2 file**

- [ ] BK2 import works
- [ ] Playback syncs correctly

### 4.4 VBM Format (VisualBoyAdvance Movies)

**Instructions:** Test importing a VBA movie.

1. **Load the GBA/GB ROM**
2. **File → Import Movie**
3. **Select the VBM file**

- [ ] VBM import works
- [ ] Playback syncs correctly

### 4.5 Nexen Native Format (.nmv)

**Instructions:** Test the native movie format.

1. **Create a new movie** in TAS Editor
2. **Add some inputs** (at least 100 frames)
3. **Add metadata:**
   - Author name
   - Comments/description
4. **File → Save As → test_native.nmv**

- [ ] NMV file is created

5. **Close TAS Editor**
6. **Reopen TAS Editor**
7. **File → Open → test_native.nmv**

- [ ] Movie loads correctly
- [ ] All inputs preserved
- [ ] Metadata preserved (author, comments)
- [ ] Rerecord count preserved

### 4.6 Error Handling

#### 4.6.1 Corrupt File

**Instructions:** Test import of invalid file.

1. **Create a text file** named `fake.fm2` with random content
2. **Attempt to import** fake.fm2

- [ ] Error message displayed (not crash)
- [ ] Error message is helpful ("Invalid file format" or similar)

#### 4.6.2 Wrong ROM

**Instructions:** Test importing movie for different ROM.

1. **Load ROM A**
2. **Try to import** a movie made for ROM B

- [ ] Warning or error about ROM mismatch
- [ ] Option to proceed anyway or abort

---

## 5. Pansy Export Testing

> **Goal:** Test exporting disassembly metadata in Pansy format for use with external tools.

### 5.1 Basic Export

**Instructions:** Export Pansy metadata for a running game.

1. **Load an NES game**
2. **Open the Debugger** (Debug → Debugger, or Tools → Debugger)
3. **Add some labels:**
   - Find the NMI handler address
   - Right-click → Add Label → Name it "NMI_Handler"
   - Repeat for another address → Name it "ResetVector"
4. **Add some comments:**
   - Find an instruction
   - Right-click → Add Comment → "This loads the player position"
5. **Debug → Export Pansy Metadata** (or similar menu option)
6. **Choose save location:** `test_export.pansy`

- [ ] Export menu option exists
- [ ] File dialog appears
- [ ] File is created without error

### 5.2 Verify Exported Contents

**Instructions:** Check the exported file contains correct data.

1. **Open test_export.pansy** in a text editor or hex viewer
2. **Look for:**
   - "PNSY" magic bytes at start (if binary format)
   - Or JSON/text content (if text format)
3. **Search for your labels:**
   - "NMI_Handler" should appear
   - "ResetVector" should appear
4. **Search for your comment:**
   - "player position" text should appear

- [ ] File has correct header/format
- [ ] Labels are present in export
- [ ] Comments are present in export
- [ ] Addresses are correct

### 5.3 Platform-Specific Export

**Instructions:** Test export for different systems.

| System | Test ROM | Export Works | Correct CPU Format | ✓ |
|--------|----------|--------------|-------------------|---|
| NES | Any | | 6502 addresses | [ ] |
| SNES | Any | | 65816 addresses | [ ] |
| Game Boy | Any | | LR35902 addresses | [ ] |
| GBA | Any | | ARM7 addresses | [ ] |

For each system:
1. Load a ROM for that system
2. Add at least one label
3. Export Pansy metadata
4. Verify the export includes the label

### 5.4 CDL Integration

**Instructions:** Test that Code/Data Logger information is exported.

1. **Load an NES game**
2. **Enable Code/Data Logger** (Debug → Code/Data Logger)
3. **Play the game for 60 seconds** to build CDL data
4. **Check CDL status** - Should show logged code and data regions
5. **Export Pansy metadata**
6. **Examine the export** for code/data region markers

- [ ] CDL is active during play
- [ ] Export includes code region markers
- [ ] Export includes data region markers

---

## 6. Keyboard Shortcuts Testing

> **Goal:** Verify all keyboard shortcuts work correctly.
>
> ⚠️ **Important:** F1/Shift+F1 are for the **infinite savestate browser**, not numbered slots.

### 6.1 Global Emulation Shortcuts

**Instructions:** Test each shortcut with a game running.

| Shortcut | Action | How to Test | ✓ |
|----------|--------|-------------|---|
| F5 | Run/Pause | Press while running → game pauses. Press again → resumes | [ ] |
| F6 | Reset | Press → game resets to title screen | [ ] |
| Esc | Pause | Press → emulation pauses | [ ] |
| F12 | Screenshot | Press → screenshot saved (check screenshots folder) | [ ] |

**Detailed Test for F5:**
1. **Load a game** and let it run
2. **Press F5** - Game should freeze (pause)
3. **Press F5 again** - Game should resume
- [ ] F5 toggles pause correctly

**Detailed Test for F6:**
1. **Play a game** past the title screen
2. **Press F6** - Game should reset
- [ ] F6 resets to beginning

**Detailed Test for F12:**
1. **Navigate to a recognizable screen**
2. **Press F12** - Screenshot should be captured
3. **Check the screenshots folder** (usually in Nexen's data directory)
- [ ] F12 saves screenshot
- [ ] Screenshot file exists

### 6.2 Savestate Shortcuts (Infinite Browser)

> ⚠️ **Note:** Unlike traditional emulators with F1-F10 = Slot 1-10, Nexen uses an infinite savestate browser.

| Shortcut | Action | How to Test | ✓ |
|----------|--------|-------------|---|
| Shift+F1 | Create Savestate | Press → new savestate created, added to browser | [ ] |
| F1 | Open Savestate Browser | Press → browser window opens showing all savestates | [ ] |

**Detailed Test for Shift+F1:**
1. **Load a game** and play for 10 seconds
2. **Press Shift+F1**
3. **Expected:** A new savestate is created
4. **Press F1** to open browser
5. **Verify:** New savestate appears in the list

- [ ] Shift+F1 creates new savestate (not overwriting)
- [ ] Savestate appears in browser with thumbnail
- [ ] Each Shift+F1 press creates a NEW savestate

**Detailed Test for F1:**
1. **Press F1** with a game running
2. **Expected:** Savestate browser window/panel opens
3. **The browser shows:**
   - List of all savestates for current game
   - Thumbnails
   - Timestamps
4. **Select a savestate** and load it

- [ ] F1 opens the savestate browser
- [ ] F1 does NOT instant-load a "slot 1" state
- [ ] Can browse and select from multiple savestates

### 6.3 TAS Editor Shortcuts

**Instructions:** Test each shortcut with TAS Editor open.

| Shortcut | Action | ✓ |
|----------|--------|---|
| Ctrl+N | New movie | [ ] |
| Ctrl+O | Open movie | [ ] |
| Ctrl+S | Save movie | [ ] |
| Ctrl+Shift+S | Save As | [ ] |
| Ctrl+Z | Undo | [ ] |
| Ctrl+Y | Redo | [ ] |
| Ctrl+C | Copy frames | [ ] |
| Ctrl+V | Paste frames | [ ] |
| Ctrl+X | Cut frames | [ ] |
| Delete | Delete frames | [ ] |
| Space | Play/Pause | [ ] |
| F | Frame advance | [ ] |
| Shift+F | Frame rewind | [ ] |

**Detailed Tests:**

**Ctrl+Z / Ctrl+Y Test:**
1. **Select frame 10**, toggle A button ON
2. **Press Ctrl+Z** - A button should toggle OFF (undone)
3. **Press Ctrl+Y** - A button should toggle ON (redone)
- [ ] Undo/Redo work correctly

**Space Bar Test:**
1. **Go to frame 0**
2. **Press Space** - Playback starts
3. **Press Space** - Playback pauses
- [ ] Space toggles play/pause

**Frame Advance Test (F key):**
1. **Note current frame number:** ______
2. **Press F** - Frame advances by 1
3. **Press F five more times**
4. **Verify frame is now:** original + 6
- [ ] F advances one frame each press

### 6.4 Debugger Shortcuts

**Instructions:** Test with debugger open.

| Shortcut | Action | ✓ |
|----------|--------|---|
| F9 | Toggle breakpoint | [ ] |
| F10 | Step over | [ ] |
| F11 | Step into | [ ] |
| Shift+F11 | Step out | [ ] |

**Detailed Test for F9:**
1. **Open Debugger** (Debug → Debugger)
2. **Click on an instruction line**
3. **Press F9** - Breakpoint marker should appear
4. **Press F9 again** - Breakpoint should be removed
- [ ] F9 toggles breakpoints

### 6.5 Shortcut Conflicts

**Instructions:** Verify no shortcuts are accidentally duplicated.

1. **Go to settings/preferences**
2. **Look for keyboard shortcut configuration**
3. **Check for any conflicts** (same key assigned to multiple actions)

- [ ] No conflicting shortcuts found
- [ ] Each shortcut has exactly one action

---

## 7. UI/UX Testing

> **Goal:** Verify the user interface is responsive and functional.

### 7.1 Main Window

**Instructions:** Test main window behavior.

**Resizing:**
1. **Drag a corner** of the window to resize
2. **Make it very small** (300x200 pixels)
3. **Make it very large** (fullscreen)
4. **Verify** game display scales appropriately

- [ ] Window resizes smoothly
- [ ] Game display scales with window
- [ ] No UI elements cut off or overlapping

**Minimize/Maximize:**
1. **Click minimize** button
2. **Click taskbar** to restore
3. **Click maximize** button
4. **Click maximize** again to restore

- [ ] Minimize works
- [ ] Restore works
- [ ] Maximize fills screen
- [ ] Un-maximize restores size

### 7.2 Menu System

**Instructions:** Click through each menu.

| Menu | Opens Without Error | All Items Clickable | ✓ |
|------|--------------------|--------------------|---|
| File | | | [ ] |
| Edit | | | [ ] |
| View | | | [ ] |
| Tools | | | [ ] |
| Debug | | | [ ] |
| Help | | | [ ] |

**Test each menu item** - Click it and verify it does something (opens dialog, toggles option, etc.)

- [ ] All menu items respond to click
- [ ] No disabled items that should be enabled
- [ ] No enabled items that cause crash

### 7.3 Toolbar

**Instructions:** Test toolbar buttons.

1. **Hover over each button** - Tooltip should appear
2. **Click each button** - Action should execute

- [ ] All buttons have tooltips
- [ ] All buttons perform their action
- [ ] Button states update (enabled/disabled as appropriate)

### 7.4 Status Bar

**Instructions:** Check status bar information.

1. **Load a game**
2. **Look at status bar** at bottom of window
3. **Verify it shows:**
   - Frame counter
   - FPS
   - System name (NES, SNES, etc.)

- [ ] Frame counter visible and updating
- [ ] FPS display visible
- [ ] System name shown

### 7.5 Dialogs

**Instructions:** Test common dialogs.

**Open File Dialog:**
1. **File → Open ROM**
2. **Navigate folders**
3. **Select a file**
4. **Click Open**

- [ ] Open dialog appears
- [ ] Can navigate folders
- [ ] Can select files
- [ ] File opens correctly

**Settings Dialog:**
1. **Edit → Preferences** (or Tools → Settings)
2. **Browse through tabs/sections**
3. **Change a setting** (e.g., video scale)
4. **Click OK/Apply**
5. **Reopen dialog** - Setting should be saved

- [ ] Settings dialog opens
- [ ] Can browse all sections
- [ ] Changes are saved
- [ ] Changes take effect

---

## 8. Multi-System Testing

> **Goal:** Verify each supported system works correctly.

### 8.1 NES Testing

**Instructions:** Complete test with an NES game.

1. **File → Open** and select an NES ROM (.nes file)
2. **Wait for game to load**

- [ ] ROM loads without error
- [ ] Title screen displays

3. **Play for 60 seconds:**
   - Press Start to begin
   - Use D-pad to move
   - Press A/B buttons
   - Listen to music and sound effects

- [ ] Graphics display correctly (no corruption)
- [ ] Input is responsive (controls work)
- [ ] Audio plays correctly (music, SFX)
- [ ] No crashes during gameplay

4. **Create a savestate** (Shift+F1) then load it (F1 → select)
- [ ] Savestate works

5. **Open TAS Editor** and record a few inputs
- [ ] TAS Editor works with NES

### 8.2 SNES Testing

**Instructions:** Complete test with an SNES game.

1. **Load an SNES ROM** (.sfc or .smc)
2. **Play for 60 seconds**

- [ ] ROM loads
- [ ] Graphics correct (no Mode 7 issues, no sprite corruption)
- [ ] All controller buttons work (A, B, X, Y, L, R, Start, Select)
- [ ] Audio correct (SPC700 music)

3. **Test enhancement chips** (if you have games with them):
   - Super FX (Star Fox, Yoshi's Island)
   - SA-1 (Super Mario RPG)
   - DSP (Mario Kart)

- [ ] Super FX game works (if tested): ____________
- [ ] SA-1 game works (if tested): ____________
- [ ] DSP game works (if tested): ____________

### 8.3 Game Boy Testing

**Instructions:** Complete test with a Game Boy game.

1. **Load a GB ROM** (.gb)
2. **Verify:**
   - 4-shade grayscale displays
   - Controls work
   - Audio works

- [ ] GB game loads and plays correctly

### 8.4 Game Boy Color Testing

1. **Load a GBC ROM** (.gbc)
2. **Verify:**
   - Colors display correctly
   - GBC-exclusive game works

- [ ] GBC game loads and plays correctly
- [ ] Colors are accurate

### 8.5 Game Boy Advance Testing

1. **Load a GBA ROM** (.gba)
2. **Verify:**
   - Graphics render (sprites, backgrounds, effects)
   - L/R trigger buttons work
   - Audio works

- [ ] GBA game loads and plays correctly
- [ ] L/R buttons functional

### 8.6 Master System Testing

1. **Load an SMS ROM** (.sms)
2. **Verify playback**

- [ ] SMS game loads and plays correctly

### 8.7 PC Engine Testing

1. **Load a PCE ROM** (.pce)
2. **Verify playback**

- [ ] PCE game loads and plays correctly

### 8.8 WonderSwan Testing

1. **Load a WS ROM** (.ws or .wsc)
2. **Test both orientations** (vertical/horizontal)

- [ ] WonderSwan game loads and plays correctly
- [ ] Orientation modes work

---

## 9. Performance Testing

> **Goal:** Verify Nexen runs efficiently without performance issues.

### 9.1 Frame Rate

**Instructions:** Monitor frame rate during gameplay.

1. **Enable FPS display** (View → Show FPS, or status bar)
2. **Load an NES game** (60 FPS target)
3. **Play for 2 minutes** watching FPS

- [ ] Maintains 60 FPS (NTSC) consistently
- [ ] No frame drops during normal gameplay
- [ ] FPS counter visible

4. **Load a PAL game** if available (50 FPS target)
- [ ] Maintains 50 FPS (PAL)

5. **Test fast forward:**
   - Hold the fast forward key
   - Note the maximum speed achieved: ______ FPS

- [ ] Fast forward works
- [ ] Speed is noticeably faster than 60 FPS

### 9.2 Memory Usage

**Instructions:** Monitor memory over extended session.

1. **Open Task Manager** (Windows: Ctrl+Shift+Esc)
2. **Find Nexen** in the process list
3. **Note initial memory:** ______ MB
4. **Play a game for 10 minutes**
5. **Note memory after 10 min:** ______ MB
6. **Create 10 savestates**
7. **Note memory after savestates:** ______ MB

- [ ] Memory doesn't grow excessively (< 500MB increase over session)
- [ ] No obvious memory leak (constant growth)

### 9.3 Startup Time

1. **Close Nexen completely**
2. **Start a stopwatch**
3. **Launch Nexen**
4. **Stop when main window appears**
5. **Time:** ______ seconds

- [ ] Startup is reasonable (< 5 seconds)

### 9.4 Large Movie Performance

**Instructions:** Test with a long TAS movie.

1. **Open TAS Editor**
2. **Load or create a movie** with 100,000+ frames
3. **Scroll through the frame list**
4. **Seek to frame 50,000**

- [ ] Frame list scrolls smoothly
- [ ] Seeking completes (doesn't hang)
- [ ] UI remains responsive

---

## 10. Build & Platform Testing

> **Goal:** Verify Nexen works on all target platforms.

### 10.1 Windows x64

**Instructions:** Test on Windows.

1. **Download** Nexen-Windows-x64.zip from release
2. **Extract** to a folder
3. **Run** Nexen.exe

- [ ] Executable runs without error
- [ ] No missing DLL errors
- [ ] All features work

**Windows Version Testing:**
- [ ] Windows 10: Works
- [ ] Windows 11: Works

### 10.2 Linux x64

**Instructions:** Test on Linux.

1. **Download** Nexen-Linux-x64.AppImage
2. **Make executable:** `chmod +x Nexen-Linux-x64.AppImage`
3. **Run:** `./Nexen-Linux-x64.AppImage`

- [ ] AppImage runs
- [ ] No missing library errors
- [ ] All features work

### 10.3 Linux ARM64

**Instructions:** Test on ARM Linux (Raspberry Pi, etc.).

1. **Download** Nexen-Linux-ARM64.AppImage
2. **Make executable:** `chmod +x Nexen-Linux-ARM64.AppImage`
3. **Run:** `./Nexen-Linux-ARM64.AppImage`

- [ ] AppImage runs on ARM64
- [ ] Performance acceptable

### 10.4 macOS Intel (x64)

**Instructions:** Test on Intel Mac.

1. **Download** Nexen-macOS-x64.zip
2. **Extract** (double-click zip)
3. **Right-click** the app → Open (bypass Gatekeeper)
4. **Confirm** the security dialog

- [ ] App launches
- [ ] Gatekeeper warning can be bypassed
- [ ] All features work

### 10.5 macOS Apple Silicon (ARM64)

**Instructions:** Test on M1/M2/M3 Mac.

1. **Download** Nexen-macOS-ARM64.zip
2. **Extract** and run
3. **Verify** it runs natively (not via Rosetta)

- [ ] App launches natively
- [ ] Performance is good
- [ ] All features work

---

## Test Results Summary

### Overall Status

| Section | Total Tests | Pass | Fail | Not Tested |
|---------|-------------|------|------|------------|
| 1. Environment Setup | | | | |
| 2. TAS Editor | | | | |
| 3. Save States | | | | |
| 4. Movie Converter | | | | |
| 5. Pansy Export | | | | |
| 6. Keyboard Shortcuts | | | | |
| 7. UI/UX | | | | |
| 8. Multi-System | | | | |
| 9. Performance | | | | |
| 10. Build/Platform | | | | |
| **TOTAL** | | | | |

### Critical Issues Found

> List any bugs that prevent core functionality from working.

1.
2.
3.

### Non-Critical Issues Found

> List any minor bugs or polish issues.

1.
2.
3.

### Notes and Observations

> Additional notes from testing.

---

## Test Sign-Off

| Tester | Date | Sections Tested | Result |
|--------|------|-----------------|--------|
| | | | |

---

*Document version: 1.2.0*
*Last updated: 2026-02-02*
*Major revision: Added checkbox usage instructions; corrected savestate shortcuts; removed duplicate sections*
