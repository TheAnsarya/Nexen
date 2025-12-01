# TAS Epic Verification Checklist

This document provides a comprehensive manual verification checklist for the TAS (Tool-Assisted Speedrun) features implemented in Mesen2.

> **Epic Status:** COMPLETE  
> **Last Verified:** December 1, 2025  
> **All Issues:** #17-#25 CLOSED

---

## Pre-Verification Setup

### Build Requirements
1. [ ] Build MesenCore.dll (C++ Core) - `msbuild Mesen.sln /p:Configuration=Debug /p:Platform=x64`
2. [ ] Build Mesen.dll (C# UI) - `dotnet build UI/UI.csproj -c Debug`
3. [ ] Build MovieConverter.dll - `dotnet build MovieConverter/MovieConverter.csproj -c Debug`
4. [ ] Build MovieConverter.CLI - `dotnet build MovieConverter.CLI/MovieConverter.CLI.csproj -c Debug`

### Test ROMs Needed
- SNES ROM (any licensed game)
- NES ROM (for FM2 format testing)
- Gameboy ROM (optional)

---

## Feature Verification Checklist

### 1. Movie Recording & Playback (Pre-existing, enhanced)

#### Record a Movie
- [ ] Launch Mesen2 and load a SNES ROM
- [ ] Go to **Tools → Movies → Record**
- [ ] Choose a filename and click Save
- [ ] Verify "Recording" icon appears on screen (if ShowMovieIcons enabled)
- [ ] Play for 30+ seconds, making various inputs
- [ ] Go to **Tools → Movies → Stop** to stop recording
- [ ] Verify movie file (.mmo) was created in Movies folder

#### Play a Movie
- [ ] Load the same ROM
- [ ] Go to **Tools → Movies → Play**
- [ ] Select the recorded movie file
- [ ] Verify "Play" icon appears on screen
- [ ] Verify inputs are replayed correctly
- [ ] Movie should play to the end

---

### 2. Rerecording from Savestate (#21)

#### Basic Rerecording
- [ ] Start recording a new movie
- [ ] Play for 10 seconds
- [ ] Create a savestate (F5 or Shift+F1-F10)
- [ ] Play for another 10 seconds
- [ ] Load the savestate (F6 or F1-F10)
- [ ] Verify movie continues from savestate position
- [ ] Verify rerecord counter increments by 1
- [ ] Continue playing with different inputs
- [ ] Stop recording and save

#### Verify Movie Truncation
- [ ] The new inputs should replace the old ones after the savestate point
- [ ] Playing back should show the new sequence

---

### 3. TAS HUD Overlay (#22)

#### Frame Counter
- [ ] Go to **Options → Preferences → Advanced**
- [ ] Enable "Show frame counter"
- [ ] Verify frame count appears in top-right corner
- [ ] Verify counter increments each frame

#### Lag Counter
- [ ] Enable "Show lag counter" in Preferences
- [ ] Verify lag count appears below frame counter
- [ ] Play a game that has lag frames (loading screens, etc.)
- [ ] Verify lag counter increments during lag

#### Rerecord Counter
- [ ] Enable "Show rerecord counter (TAS)" in Preferences
- [ ] Start recording a movie
- [ ] Verify "Rerecord: 0" appears on screen
- [ ] Create and load a savestate during recording
- [ ] Verify counter increments to "Rerecord: 1"
- [ ] Repeat rerecording multiple times
- [ ] Verify counter accurately tracks rerecords

#### Lag Frame Visual Indicator
- [ ] Enable "Flash screen on lag frames (TAS)" in Preferences
- [ ] Play a game that has lag frames
- [ ] Verify red border flashes around screen on lag frames
- [ ] Toggle off and verify border no longer appears

---

### 4. Read-Only Mode & TAS Indicator (#17)

#### R/W Indicator Display
- [ ] Start recording a movie
- [ ] Verify "W" indicator appears (red background) next to record icon
- [ ] Stop recording and play the movie
- [ ] Verify "R" indicator appears (green background) next to play icon

#### Toggle Read-Only Mode
- [ ] While playing a movie, use TAS Toggle Read-Only shortcut
- [ ] Verify indicator changes from "R" to "W"
- [ ] Verify message "Mode: Read-Write" appears
- [ ] Toggle again, verify "R" returns with "Mode: Read-Only" message
- [ ] In Read-Write mode, inputs should modify the movie
- [ ] In Read-Only mode, inputs should be ignored

---

### 5. TAS Keyboard Shortcuts (#24)

#### Configure Shortcuts
- [ ] Go to **Options → Preferences → Shortcuts**
- [ ] Find TAS shortcuts in the list:
  - Toggle Rerecord Counter
  - Toggle Lag Frame Indicator (TAS)
  - TAS Frame Advance
  - TAS Previous Frame
  - TAS Toggle Read-Only
- [ ] Assign keyboard shortcuts to each

#### Test Frame Advance
- [ ] Load a game and pause (Pause key)
- [ ] Press TAS Frame Advance shortcut
- [ ] Verify game advances exactly one frame
- [ ] Press multiple times, verify frame-by-frame advancement

#### Test Previous Frame
- [ ] Enable rewind buffer in Preferences
- [ ] Play a game for a few seconds
- [ ] Press TAS Previous Frame shortcut
- [ ] Verify game rewinds (note: currently uses 10-second rewind)

---

### 6. Movie Import/Export (#23)

#### Import External Movies
- [ ] Go to **Tools → Movies → Import Movie...**
- [ ] Select an SMV file (Snes9x format)
- [ ] Verify conversion dialog appears
- [ ] Select destination for converted .mmo file
- [ ] Verify movie plays correctly after import

#### Export to External Formats
- [ ] Record or load a Mesen movie (.mmo)
- [ ] Go to **Tools → Movies → Export Movie...**
- [ ] Select the source .mmo file
- [ ] Choose export format (SMV, LSMV, FM2, BK2)
- [ ] Select destination path
- [ ] Click Export
- [ ] Verify file is created in target format
- [ ] (Optional) Load exported file in target emulator to verify

---

### 7. MovieConverter CLI Tool (#19)

#### Test Command-Line Conversion
```powershell
# Navigate to bin folder
cd bin\Debug

# List supported formats
.\MesenMovieConverter.exe list-formats

# Convert SMV to MMO
.\MesenMovieConverter.exe convert input.smv output.mmo

# Get movie info
.\MesenMovieConverter.exe info movie.smv
```

#### Verify CLI Commands
- [ ] `list-formats` shows all supported formats
- [ ] `convert` successfully converts between formats
- [ ] `info` displays movie metadata (frames, rerecords, etc.)
- [ ] Error handling works for invalid files

---

### 8. Lua API for TAS (#17)

#### Test TAS Lua Functions
Create a test script `tas_test.lua`:
```lua
-- TAS State Test Script
emu.addEventCallback(function()
    local state = emu.getTasState()
    
    emu.log("Frame: " .. state.frameCount)
    emu.log("Rerecords: " .. state.rerecordCount)
    emu.log("Lag Frames: " .. state.lagFrameCount)
    emu.log("Recording: " .. tostring(state.isRecording))
    emu.log("Playing: " .. tostring(state.isPlaying))
    emu.log("Read-Only: " .. tostring(state.isReadOnly))
end, emu.eventType.startFrame)
```

#### Verify Lua Functions
- [ ] Load the test script in Debug → Script Window
- [ ] Start recording a movie
- [ ] Verify state.isRecording returns true
- [ ] Verify frameCount increments correctly
- [ ] Load a savestate during recording
- [ ] Verify rerecordCount increments

---

## Integration Tests

### End-to-End TAS Workflow
1. [ ] Start a new recording
2. [ ] Enable all TAS display options (frame counter, lag counter, rerecord counter)
3. [ ] Play for 1 minute, creating savestates every 15 seconds
4. [ ] Rerecord from the 30-second savestate
5. [ ] Verify rerecord counter shows 1
6. [ ] Stop recording
7. [ ] Play back the movie
8. [ ] Verify R/W indicator shows correctly
9. [ ] Export to SMV format
10. [ ] Import the SMV back
11. [ ] Verify playback matches original

### Stress Test
- [ ] Record a 5+ minute movie with multiple rerecords (10+)
- [ ] Verify rerecord counter accuracy
- [ ] Verify movie file integrity after saving
- [ ] Verify playback completes without desyncs

---

## Known Limitations

1. **Format Conversion Accuracy**
   - Some format conversions may lose metadata (author, description)
   - Button mappings may differ between emulators
   - LSMV subframes are approximated

2. **TAS Previous Frame**
   - Currently uses 10-second rewind, not true single-frame rewind
   - Future enhancement: implement proper frame-by-frame rewind

3. **Input Display**
   - Pre-existing feature, not modified in TAS epic
   - Works independently of TAS features

---

## Files Modified in TAS Epic

### Core C++ (Core/)
| File | Changes |
|------|---------|
| `Shared/BaseControlManager.h/cpp` | `_wasLastFrameLag`, `WasLastFrameLag()` |
| `Shared/Emulator.h/cpp` | `WasLastFrameLag()` passthrough |
| `Shared/SettingTypes.h` | `ShowLagFrameIndicator` preference |
| `Shared/Movies/MovieTypes.h` | `TasState` struct |
| `Shared/Movies/MovieManager.h/cpp` | TAS state tracking, read-only mode |
| `Shared/Movies/MovieRecorder.h/cpp` | Rerecord counting, truncation |
| `Shared/Video/SystemHud.h/cpp` | TAS HUD indicators |
| `Debugger/LuaApi.h/cpp` | `getTasState()` Lua function |

### InteropDLL (InteropDLL/)
| File | Changes |
|------|---------|
| `RecordApiWrapper.cpp` | TAS API exports |

### UI Layer (UI/)
| File | Changes |
|------|---------|
| `Config/PreferencesConfig.cs` | TAS preference properties |
| `Config/Shortcuts/EmulatorShortcut.cs` | TAS shortcut enums |
| `Utilities/ShortcutHandler.cs` | TAS shortcut handlers |
| `Interop/RecordApi.cs` | TAS API bindings |
| `ViewModels/MainMenuViewModel.cs` | Import/Export menu items |
| `ViewModels/MovieExportViewModel.cs` | Export dialog logic |
| `Windows/MovieExportWindow.axaml(.cs)` | Export dialog UI |
| `Views/PreferencesConfigView.axaml` | Lag frame indicator checkbox |
| `Localization/resources.en.xml` | TAS-related strings |

### MovieConverter Project
| File | Changes |
|------|---------|
| `MovieConverter/` | Class library for format conversion |
| `MovieConverter.CLI/` | Standalone CLI tool |

---

## Sign-Off

| Reviewer | Date | Status |
|----------|------|--------|
| ________ | ____ | [ ] Passed / [ ] Failed |

### Notes
_Record any issues found during verification:_

---

## Appendix: Quick Reference

### TAS Shortcuts (Default)
| Action | Default Key | Notes |
|--------|-------------|-------|
| Toggle Rerecord Counter | (unassigned) | Set in Preferences |
| Toggle Lag Frame Indicator | (unassigned) | Set in Preferences |
| TAS Frame Advance | (unassigned) | Works when paused |
| TAS Previous Frame | (unassigned) | Uses rewind |
| TAS Toggle Read-Only | (unassigned) | During playback |

### Movie Formats
| Extension | Format | Read | Write |
|-----------|--------|------|-------|
| .mmo | Mesen | ✅ | ✅ |
| .smv | Snes9x | ✅ | ✅ |
| .lsmv | lsnes | ✅ | ✅ |
| .fm2 | FCEUX | ✅ | ✅ |
| .bk2 | BizHawk | ✅ | ✅ |

### TAS HUD Display Options
- Show Frame Counter
- Show Lag Counter  
- Show Rerecord Counter (TAS)
- Flash Screen on Lag Frames (TAS)
