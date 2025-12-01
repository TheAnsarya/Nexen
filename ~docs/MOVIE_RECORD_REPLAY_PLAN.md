# Mesen2 Movie Record/Replay Enhancement Plan

## Current State

### Existing Movie System
Mesen2 already has a robust movie recording and playback system:

**Files:**
- `Core/Shared/Movies/MovieManager.h/.cpp` - Main manager for recording/playback
- `Core/Shared/Movies/MovieRecorder.h/.cpp` - Records input to movie files
- `Core/Shared/Movies/MesenMovie.h/.cpp` - Plays back Mesen movie files
- `Core/Shared/Movies/MovieTypes.h` - Data structures and keys

**Current Format (Mesen Movie - .mmm):**
The Mesen movie format is a ZIP archive containing:
- `GameSettings.txt` - Emulator settings, ROM info, SHA1 hash, cheats
- `Input.txt` - Per-frame controller input (pipe-delimited text)
- `MovieInfo.txt` - Author and description (optional)
- `SaveState.mss` - Initial save state (optional, for mid-game recordings)
- `Battery*` files - SRAM data (optional)
- `PatchData.dat` - ROM patch file (optional)

**Current Features:**
- Record from: power-on (clean), power-on (with save data), or current state
- Automatic settings backup and restoration
- Cheat support in movies
- Battery/SRAM recording
- ROM patch support

---

## Research: Industry Standard Movie Formats

### FM2 (FCEUX - NES)
- **Type:** Text-based by default
- **Structure:** Header (key-value pairs) + Input Log (pipe-delimited)
- **Features:**
  - Embedded GUID for savestate association
  - Mouse/Zapper support
  - Soft/Hard reset recording
  - Binary format option
- **Input Format:** `|commands|RLDUTSBA|RLDUTSBA|port2|`

### SMV (Snes9x - SNES)
- **Type:** Binary format with 32/64 byte header
- **Structure:** Header → Metadata (UTF-16 title) → ROM Info → Savestate → Controller Data
- **Features:**
  - Multiple versions (1.43, 1.51, 1.52)
  - Peripheral support (Mouse, Super Scope, Justifier)
  - Reset recording (FF FF = soft reset)
  - PAL/NTSC flag
- **Input:** 2 bytes per controller per frame

### LSMV (lsnes - SNES)
- **Type:** ZIP archive with text OR binary format
- **Structure:** Multiple named files in archive
- **Features:**
  - Most comprehensive format
  - System type specification
  - Multiple port/controller configurations
  - SRAM snapshots
  - Subtitle system
  - Rerecord tracking with unique IDs
  - RTC timing support
  - Lua host memory in savestates
- **Input:** `F.|BYsSudlrAXLR` per controller

### BK2 (BizHawk - Multi-system)
- **Type:** ZIP archive with JSON/text
- **Structure:** Multiple files in archive
- **Features:**
  - Multi-system support
  - Modern architecture
  - Extensive metadata

---

## Proposed Enhancements

### Phase 1: Core Improvements

#### 1.1 Rerecord Counter
**Priority:** High  
**Description:** Track how many times the user has loaded a savestate during recording (standard TAS metric)

```cpp
// In MovieRecorder.h
uint32_t _rerecordCount = 0;

// Increment when loading savestate during recording
void OnSaveStateLoaded();
```

#### 1.2 Frame Counter Display
**Priority:** High  
**Description:** Show current frame number during playback/recording in OSD

#### 1.3 Lag Frame Detection
**Priority:** Medium  
**Description:** Track frames where no input was polled (important for TAS)

### Phase 2: Input Enhancements

#### 2.1 Reset Recording
**Priority:** High  
**Description:** Allow recording soft/hard resets mid-movie

```cpp
// Special input markers in Input.txt
// |RESET_SOFT|...
// |RESET_HARD|...
```

#### 2.2 Peripheral Support Verification
**Priority:** Medium  
**Description:** Ensure Mouse, Super Scope, Justifier inputs are properly recorded

#### 2.3 Multitap Support
**Priority:** Medium  
**Description:** Verify 4+ player input recording

### Phase 3: Format Interoperability

#### 3.1 SMV Import
**Priority:** High  
**Description:** Import Snes9x movie files (.smv)

```cpp
class SmvMovie : public IMovie {
    // Parse SMV header
    // Convert input format
    // Handle savestate differences
};
```

#### 3.2 LSMV Import
**Priority:** Medium  
**Description:** Import lsnes movie files (.lsmv)

#### 3.3 Export to Standard Formats
**Priority:** Low  
**Description:** Export Mesen movies to SMV/LSMV for compatibility

### Phase 4: TAS Features

#### 4.1 Frame Advance
**Priority:** High  
**Description:** Step one frame at a time (already exists but verify integration)

#### 4.2 Input Display
**Priority:** High  
**Description:** Show pressed buttons on screen during playback

#### 4.3 Read-Only Mode Toggle
**Priority:** Medium  
**Description:** Switch between read-only playback and read-write (continue recording)

#### 4.4 Savestate Slots During Recording
**Priority:** High  
**Description:** Save/load states while recording (triggers rerecord count)

### Phase 5: UI Improvements

#### 5.1 Movie Recording Dialog
**Priority:** High  
**Description:** Enhanced dialog with:
- Author name field
- Description/comments
- Recording mode selection
- Target filename

#### 5.2 Movie Playback Info
**Priority:** Medium  
**Description:** Show metadata during playback:
- Author
- Rerecord count
- Frame count
- Duration

#### 5.3 Movie Timeline View
**Priority:** Low  
**Description:** Visual timeline of movie with frame markers

### Phase 6: Advanced Features

#### 6.1 Input Editor (TAS Editor)
**Priority:** Low (Complex)  
**Description:** Piano-roll style input editor

#### 6.2 Subtitle Support
**Priority:** Low  
**Description:** Display text at specific frames (like LSMV)

#### 6.3 Branch Support
**Priority:** Low  
**Description:** Create alternate input branches for comparison

---

## File Format Comparison

| Feature | Mesen (.mmm) | FM2 | SMV | LSMV |
|---------|--------------|-----|-----|------|
| Text-based | Yes (in ZIP) | Yes | No | Both |
| Rerecord count | No | Yes | Yes | Yes |
| Reset recording | No | Yes | Yes | Yes |
| Subtitles | No | Yes | No | Yes |
| SRAM support | Yes | No | Yes | Yes |
| Savestate anchor | Yes | Yes | Yes | Yes |
| ROM hash | SHA1 | MD5 | CRC32 | SHA256 |
| Cheats | Yes | No | No | No |
| Settings backup | Yes | Limited | Limited | Yes |

---

## Implementation Priority

1. **Rerecord counter** - Essential TAS feature
2. **Frame counter OSD** - User visibility
3. **SMV import** - Compatibility with existing movies
4. **Input display** - TAS verification
5. **Recording dialog improvements** - UX
6. **Reset recording** - Complete movie fidelity
7. **LSMV import** - Extended compatibility
8. **TAS Editor** - Advanced feature (future)

---

## GitHub Issues to Create

1. **[Enhancement] Add rerecord counter to movie recording**
2. **[Enhancement] Display frame counter during movie playback/recording**
3. **[Enhancement] Add lag frame detection and counter**
4. **[Enhancement] Support recording soft/hard resets in movies**
5. **[Feature] Import Snes9x SMV movie files**
6. **[Feature] Import lsnes LSMV movie files**
7. **[Enhancement] Add input display overlay during playback**
8. **[Enhancement] Improve movie recording dialog with metadata fields**
9. **[Enhancement] Show movie metadata during playback**
10. **[Feature] Read-only mode toggle for movie playback**

---

## Technical Notes

### Input Format (Current)
```
|ButtonState1|ButtonState2|...
```
Where ButtonState is device-specific text representation from `GetTextState()`.

### Proposed Input Format Extension
```
|CMD:command|ButtonState1|ButtonState2|...
```
Where `CMD:` prefix indicates special commands:
- `CMD:RESET_SOFT` - Soft reset
- `CMD:RESET_HARD` - Hard reset
- `CMD:FRAME_LAG` - Lag frame marker

### ROM Verification
Current: SHA1 hash
Consider: Also store CRC32 for compatibility with ROM databases

---

## References

- FM2 Spec: https://fceux.com/web/FM2.html
- SMV Spec: https://tasvideos.org/EmulatorResources/Snes9x/SMV
- LSMV Spec: https://tasvideos.org/EmulatorResources/Lsnes/Lsmv
- TASVideos: https://tasvideos.org/
