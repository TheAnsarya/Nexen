# GitHub Issues for Movie Record/Replay Feature

This document contains the issue definitions for the movie recording enhancement feature branch `mesen-record-and-play-gameplay`.

---

## Issue #1: [EPIC] Movie Recording and TAS Feature Enhancements

**Labels:** enhancement, epic

### Description

Epic issue tracking all enhancements to Mesen2's movie recording and playback system. This includes:
- Adding standard TAS (Tool-Assisted Speedrun) features
- Importing movies from other emulators
- Improving the recording/playback UI
- Adding rerecord tracking

### Related Issues
- #TBD - Rerecord counter
- #TBD - Frame counter display
- #TBD - SMV import
- #TBD - Input display overlay
- etc.

### Background

Mesen2 already has a functional movie recording system in `Core/Shared/Movies/`. This epic tracks enhancements to bring it up to feature parity with other TAS-focused emulators like BizHawk, lsnes, and FCEUX.

### Acceptance Criteria
- [ ] All sub-issues completed
- [ ] Movie format documented
- [ ] Import/export tested with real movie files

---

## Issue #2: [ENHANCEMENT] Add rerecord counter to movie recording

**Labels:** enhancement

### Description

Add a rerecord counter that tracks how many times a savestate is loaded during movie recording. This is a standard TAS metric used to measure the effort put into creating a movie.

### Current Behavior
No rerecord count is tracked.

### Expected Behavior
- Increment counter each time a savestate is loaded during recording
- Store rerecord count in `GameSettings.txt` as `RerecordCount <number>`
- Display rerecord count in movie info dialog

### Implementation Notes
- Add `uint32_t _rerecordCount` to `MovieRecorder`
- Hook into `ProcessNotification` for `ConsoleNotificationType::StateLoaded`
- Add `RerecordCount` to `MovieKeys` namespace

### Files to Modify
- `Core/Shared/Movies/MovieRecorder.h`
- `Core/Shared/Movies/MovieRecorder.cpp`
- `Core/Shared/Movies/MovieTypes.h`
- `Core/Shared/Movies/MesenMovie.cpp` (for playback display)

---

## Issue #3: [ENHANCEMENT] Display frame counter during movie playback/recording

**Labels:** enhancement

### Description

Show the current frame number in the OSD (On-Screen Display) during movie recording and playback.

### Current Behavior
No frame counter is displayed during movie operations.

### Expected Behavior
- Display "Frame: X / Y" during playback (X = current, Y = total)
- Display "Frame: X (Recording)" during recording
- Allow user to toggle this display on/off

### Implementation Notes
- Add OSD element in video rendering
- Get frame count from `_controlManager->GetPollCounter()`
- Add preference setting `ShowMovieFrameCounter`

### Files to Modify
- `UI/` video overlay components
- `Core/Shared/Movies/MovieManager.h/.cpp`
- Settings/preferences

---

## Issue #4: [ENHANCEMENT] Add lag frame detection and counter

**Labels:** enhancement

### Description

Detect and count "lag frames" - frames where no input was polled by the game. This is important for TAS as it affects timing calculations.

### Current Behavior
Lag frames are not tracked.

### Expected Behavior
- Detect when a frame completes without polling input
- Increment lag counter
- Display lag count in movie info
- Optionally display "LAG" indicator on screen during lag frames

### Implementation Notes
- Track poll counter changes between frames
- If poll counter doesn't change, it's a lag frame
- Store in movie file as `LagFrameCount`

---

## Issue #5: [ENHANCEMENT] Support recording soft/hard resets in movies

**Labels:** enhancement

### Description

Allow recording of soft and hard resets within a movie, ensuring perfect reproducibility.

### Current Behavior
Resets are not recorded in the input stream.

### Expected Behavior
- Record soft reset (console reset)
- Record hard reset (power cycle)
- Playback correctly performs resets at recorded frames

### Input Format Extension
```
|CMD:RESET_SOFT|controller_data|
|CMD:RESET_HARD|controller_data|
```

### Implementation Notes
- Extend input format to support command prefixes
- Hook reset functions to record the action
- During playback, detect command and perform reset

### Files to Modify
- `Core/Shared/Movies/MovieRecorder.cpp`
- `Core/Shared/Movies/MesenMovie.cpp`

---

## Issue #6: [FEATURE] Import Snes9x SMV movie files

**Labels:** enhancement, feature

### Description

Add support for importing Snes9x movie files (.smv) to allow playing existing TAS movies.

### SMV Format Summary
- Binary header (32-64 bytes depending on version)
- UTF-16 metadata (author info)
- Optional ROM info block (CRC32, game name)
- Gzip-compressed savestate or SRAM
- Controller data (2 bytes per controller per frame)

### Input Mapping
SMV button order: `R L X A → ↓ ← Start Select Y B`
```
10 00 R
20 00 L
40 00 X
80 00 A
00 01 Right
00 02 Left
00 04 Down
00 08 Up
00 10 Start
00 20 Select
00 40 Y
00 80 B
```

### Implementation Notes
- Create `SmvMovie` class implementing `IMovie`
- Parse header to detect version (1.43, 1.51, 1.52)
- Handle savestate/SRAM differences
- Convert 2-byte input to Mesen button format
- Handle reset marker (FF FF)

### Acceptance Criteria
- [ ] Can load SMV v1.43 files
- [ ] Can load SMV v1.51 files
- [ ] Reset markers are handled
- [ ] Movie plays back correctly

---

## Issue #7: [FEATURE] Import lsnes LSMV movie files

**Labels:** enhancement, feature

### Description

Add support for importing lsnes movie files (.lsmv).

### LSMV Format Summary
- ZIP archive containing multiple files
- `systemid` - must be "lsnes-rr1"
- `gametype` - system type (snes_ntsc, snes_pal, etc.)
- `input` - text-based input log
- Various metadata files

### Input Format
```
F.|BYsSudlrAXLR
```
Where:
- F = frame start (or . for subframe)
- `|` = separator
- `BYsSudlrAXLR` = button states

### Implementation Notes
- Create `LsmvMovie` class implementing `IMovie`
- Parse ZIP archive
- Validate systemid
- Convert input format to Mesen format
- Handle multiple controller configurations

---

## Issue #8: [ENHANCEMENT] Add input display overlay during playback

**Labels:** enhancement

### Description

Display the currently pressed buttons on screen during movie playback for verification and entertainment.

### Expected Behavior
- Show controller representation on screen
- Highlight pressed buttons in real-time
- Configurable position (corner selection)
- Toggle on/off

### Implementation Notes
- Render controller overlay in video output
- Read input state from current frame
- Add preference for position and visibility

---

## Issue #9: [ENHANCEMENT] Improve movie recording dialog with metadata fields

**Labels:** enhancement

### Description

Enhance the movie recording dialog to capture more metadata.

### Current Dialog
- Filename
- Basic author field

### Proposed Dialog
- Filename (with browse button)
- Author name (multiple authors support)
- Description/comments (multiline)
- Recording mode selection:
  - Power on (clean state)
  - Power on (with save data)
  - Current state
- ROM verification display (SHA1)
- Estimated duration display

---

## Issue #10: [ENHANCEMENT] Show movie metadata during playback

**Labels:** enhancement

### Description

Display movie information when playback begins and on request.

### Information to Display
- Movie filename
- Author
- Description
- Rerecord count
- Total frames
- Estimated duration
- ROM hash (for verification)

### Implementation Notes
- Show info popup at playback start
- Add menu option to view movie info
- Add keyboard shortcut for quick info view

---

## Issue #11: [ENHANCEMENT] Read-only mode toggle for movie playback

**Labels:** enhancement

### Description

Add ability to toggle between read-only (strict playback) and read-write (continue recording) modes.

### Current Behavior
Playback is always read-only until end, then stops.

### Expected Behavior
- Default to read-only mode
- Allow toggle to read-write mode
- In read-write mode:
  - User input overwrites movie from current frame
  - Movie continues with new input
  - Rerecord counter increments

---

## Issue Priority Order

1. **High Priority (Phase 1)**
   - #2 Rerecord counter
   - #3 Frame counter display
   - #9 Recording dialog improvements

2. **Medium Priority (Phase 2)**
   - #6 SMV import
   - #8 Input display overlay
   - #10 Movie metadata display
   - #11 Read-only toggle

3. **Lower Priority (Phase 3)**
   - #4 Lag frame detection
   - #5 Reset recording
   - #7 LSMV import

---

## Creating Issues via GitHub CLI

```bash
# Epic issue
gh issue create --title "[EPIC] Movie Recording and TAS Feature Enhancements" \
  --body "Epic tracking all movie recording enhancements..." \
  --label "enhancement"

# Rerecord counter
gh issue create --title "[Enhancement] Add rerecord counter to movie recording" \
  --body "Add rerecord counter tracking..." \
  --label "enhancement"

# ... etc
```

---

## Notes

- All issues should reference this branch: `mesen-record-and-play-gameplay`
- Each issue should be completable independently where possible
- Testing with real TAS movies from TASVideos is essential
