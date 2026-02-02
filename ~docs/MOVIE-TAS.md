# Movie and TAS Subsystem Documentation

This document covers Nexen's movie recording, playback, and TAS (Tool-Assisted Speedrun) features.

---

## Architecture Overview

```text
Movie/TAS Architecture
══════════════════════

┌─────────────────────────────────────────────────────────────────┐
│					  MovieManager							   │
│   (Central coordinator for recording and playback)			  │
└─────────────────────────────────────────────────────────────────┘
			  │
	┌─────────┴─────────┐
	│				   │
	▼				   ▼
┌───────────┐	┌───────────────┐
│MovieRecord│	│   IMovie	  │
│	er	 │	│   Players	 │
└───────────┘	└───────────────┘
						│
		 ┌──────────────┼──────────────┐
		 │			  │			  │
		 ▼			  ▼			  ▼
   ┌──────────┐  ┌──────────┐  ┌──────────┐
   │NexenMovie│  │ FM2/FM3  │  │  BK2/	│
   │  (.msm)  │  │ (FCEUX)  │  │  LSMV	│
   └──────────┘  └──────────┘  └──────────┘
```

## Directory Structure

```text
Core/Shared/Movies/
├── MovieManager.h/cpp	 - Central movie coordinator
├── MovieRecorder.h/cpp	- Movie recording
├── NexenMovie.h/cpp	   - Nexen native format
├── MovieTypes.h		   - Type definitions
└── (format parsers)	   - Format-specific readers
```

---

## Core Components

### MovieManager (MovieManager.h)

Central coordinator for movie operations.

**Supported Formats:**

| Format | Extension | System | Notes |
| -------- | ----------- | -------- | ------- |
| Nexen | .msm | Multi | Native format |
| FCEUX | .fm2, .fm3 | NES | FCEUX TAS format |
| BizHawk | .bk2 | Multi | Multi-system TAS |
| Lsnes | .lsmv | SNES | SNES TAS format |

**Recording:**

```cpp
struct RecordMovieOptions {
	string Filename;		// Output file path
	MovieFormat Format;	 // Output format
	string Author;		  // TAS author name
	string Description;	 // Movie description
	bool RecordFromPowerOn; // Start from power-on
	bool RecordFromSaveState; // Start from save state
};

void Record(RecordMovieOptions options);
```

**Playback:**

```cpp
// Play movie file (auto-detects format)
bool Play(VirtualFile& file);

// Check playback status
bool IsPlaying();
bool IsRecording();

// Stop recording/playback
void Stop();
```

### MovieRecorder (MovieRecorder.h)

Handles movie recording.

**Recording Process:**

1. Initialize with ROM info and start state
2. Each frame: Capture controller input
3. Store frame data incrementally
4. On stop: Finalize and write file

**Data Captured:**

- Controller input (per-frame)
- ROM hash (for verification)
- Initial save state (if starting from state)
- Battery/save data
- Frame count and timing

### IMovie Interface

Base interface for movie players.

```cpp
class IMovie : public IInputProvider {
public:
	virtual bool Play(VirtualFile& file) = 0;
	virtual void Stop() = 0;
	virtual bool IsPlaying() = 0;
	
	// IInputProvider interface
	virtual bool SetInput(BaseControlDevice* device) = 0;
};
```

---

## Nexen Movie Format (.msm)

Native movie format with full feature support.

### File Structure

```text
MSM File Format
═══════════════

Header:
├── Magic: "MSM\x1A" (4 bytes)
├── Version: uint32
├── Console Type: uint8
├── ROM Info
│   ├── ROM Name
│   ├── ROM CRC32
│   └── ROM SHA1
├── Start State (compressed)
│   └── Optional save state data
└── Movie Info
	├── Author
	├── Description
	├── Rerecord Count
	└── Frame Count

Frame Data:
├── Controller 1 state (per-frame)
├── Controller 2 state (per-frame)
├── Controller 3 state (per-frame)
├── Controller 4 state (per-frame)
└── ... (variable controller count)
```

### Input Encoding

```cpp
// Per-frame input (NES example)
struct NesInputFrame {
	uint8_t Port1;  // Button bits: A B Sel St U D L R
	uint8_t Port2;  // Same format
};

// Button bit positions
enum NesButton {
	A	  = 0x01,
	B	  = 0x02,
	Select = 0x04,
	Start  = 0x08,
	Up	 = 0x10,
	Down   = 0x20,
	Left   = 0x40,
	Right  = 0x80
};
```

---

## TAS Features

### Rerecording

Record over previous input from any point.

**Workflow:**

1. Play movie to target frame
2. Create save state
3. Continue recording (overwriting remaining)
4. Increment rerecord counter

```cpp
// Rerecord counter tracking
uint32_t _rerecordCount = 0;

void IncrementRerecordCount() {
	_rerecordCount++;
}
```

### Lag Frame Detection

Frames where input wasn't polled.

**Detection:**

```cpp
// In BaseControlManager
bool _wasInputRead = false;

void SetInputReadFlag() {
	_wasInputRead = true;
	_pollCounter++;
}

void ProcessEndOfFrame() {
	if (!_wasInputRead) {
		_lagCounter++;  // Lag frame!
	}
	_wasInputRead = false;
}
```

**Display:**

- Frame counter shows current frame
- Lag counter shows total lag frames
- Lag frames highlighted in HUD

### Frame Advance

Step through execution frame-by-frame.

**Controls:**

- Frame Advance: Execute single frame
- Hold Advance: Repeat advance while held
- Run: Resume normal speed

### Save State Integration

Movies can start from or include save states.

**Start Options:**

- **Power-On**: Clean start, most verifiable
- **Save State**: Start from specific point
- **Battery Save**: Include SRAM/save data

---

## Format Compatibility

### FCEUX (.fm2/.fm3)

NES movie format from FCEUX emulator.

**Features:**

- Text-based header
- Binary input data
- Subtitle support

**Import Process:**

1. Parse header (ROM hash, controller config)
2. Load input frames
3. Apply to Nexen NES core

### BizHawk (.bk2)

Multi-system TAS format.

**Features:**

- ZIP archive structure
- Multiple system support
- Subtitles and annotations

**Contents:**

```text
movie.bk2/
├── Header.txt	 - Movie metadata
├── Input Log.txt  - Frame-by-frame input
├── SyncSettings.json
└── (save state)   - Optional start state
```

### Lsnes (.lsmv)

SNES TAS format from lsnes emulator.

**Features:**

- Comprehensive SNES support
- Subframe input
- Full verification

---

## Movie Verification

### ROM Matching

Verify ROM matches recorded movie.

```cpp
struct RomInfo {
	string Name;
	uint32_t Crc32;
	string Sha1;
};

bool VerifyRom(const RomInfo& movieRom, const RomInfo& currentRom) {
	return movieRom.Sha1 == currentRom.Sha1;
}
```

### Desync Detection

Detect when movie desyncs from recording.

**Causes:**

- Wrong ROM version
- Different emulator version
- Random number inconsistency
- Save data mismatch

**Handling:**

- Display warning on potential desync
- Log frame of desync
- Option to continue or stop

---

## Input Display

### Movie HUD

On-screen display during playback/recording.

**Elements:**

- Current frame number
- Total frame count
- Lag frame count
- Recording/playback indicator
- Rerecord count

### Input Overlay

Visual representation of controller input.

```cpp
// Configure input display
struct InputDisplayConfig {
	bool ShowP1 = true;
	bool ShowP2 = false;
	Position Position = TopLeft;
	uint8_t Opacity = 200;
};
```

---

## Synchronization

### Determinism Requirements

For perfect movie sync:

- Same ROM (exact match)
- Same emulator version
- Same settings
- Same initial state

### Frame Timing

```cpp
// Ensure consistent frame timing
void RunFrame() {
	// 1. Read input (from movie if playing)
	// 2. Execute frame
	// 3. Record input (if recording)
	// 4. Update frame counter
}
```

---

## Configuration

### Recording Settings

```cpp
struct MovieSettings {
	// Format
	MovieFormat DefaultFormat = MovieFormat::Nexen;
	
	// Recording
	bool AutoRecord = false;
	string OutputPath = "./movies/";
	
	// Playback
	bool PauseOnEnd = true;
	bool LoopPlayback = false;
};
```

### Controller Configuration

Movie stores controller types:

- Standard controller
- Multitap configuration
- Special controllers (Zapper, etc.)

---

## API Usage

### Recording Example

```cpp
// Start recording
RecordMovieOptions options;
options.Filename = "speedrun.msm";
options.Author = "TASer";
options.RecordFromPowerOn = true;
movieManager->Record(options);

// ... play game ...

// Stop recording
movieManager->Stop();
```

### Playback Example

```cpp
// Play movie file
VirtualFile movieFile("speedrun.msm");
if (movieManager->Play(movieFile)) {
	// Movie playing
} else {
	// Failed to load
}

// Check status
if (movieManager->IsPlaying()) {
	// Still playing
}
```

---

## Related Documentation

- [INPUT-SUBSYSTEM.md](INPUT-SUBSYSTEM.md) - Input handling
- [DEBUGGER.md](DEBUGGER.md) - Debugging features
- [ARCHITECTURE-OVERVIEW.md](ARCHITECTURE-OVERVIEW.md) - Overall structure
