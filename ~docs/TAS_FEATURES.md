# Mesen2 TAS (Tool-Assisted Speedrun) Features

This document describes the TAS features implemented in Mesen2, including movie recording/playback, rerecording, and format conversion.

## Overview

Mesen2 now supports comprehensive TAS (Tool-Assisted Speedrun/Superplay) features:

1. **Movie Recording & Playback** - Record and play back input sequences
2. **Rerecording** - Load savestates during recording to branch off and create alternate input sequences
3. **Rerecord Counter** - Track how many times you've rerecorded during a TAS session
4. **Input Display** - Show controller inputs on screen
5. **Movie Format Conversion** - Convert between Mesen, SMV, LSMV, FM2, and BK2 formats

## TAS Settings

### Preferences (Options → Preferences → Display Settings)

- **Show rerecord counter (TAS)** - Displays the current rerecord count on screen during movie recording/playback

### Input Settings (Options → Input)

- **Display ports** - Enable input display for controller ports 1-8
- **Display position** - Choose where inputs are displayed (Top-Left, Top-Right, Bottom-Left, Bottom-Right)
- **Display horizontally** - Stack input displays horizontally or vertically

## Keyboard Shortcuts

New TAS-specific shortcuts (configurable in Options → Preferences → Shortcuts):

| Shortcut | Function |
|----------|----------|
| Toggle Rerecord Counter | Show/hide the rerecord counter on screen |
| TAS Frame Advance | Advance one frame while paused |
| TAS Previous Frame | Step back one frame (uses rewind) |
| TAS Toggle Read-Only | Toggle between read-only and read-write mode |

## Rerecording

Rerecording allows you to create TAS movies by:

1. Start recording a movie (File → Movies → Record)
2. Play through the game, creating savestates at key points
3. When you make a mistake or want to try a different approach:
   - Load a savestate
   - Continue recording from that point (the movie is truncated to that frame)
   - The rerecord counter increments each time you do this

### How It Works

When recording a movie and loading a savestate:
- The movie input log is truncated to the frame of the savestate
- Recording continues from that point
- The rerecord counter is incremented
- The original movie file is preserved until you save

## Movie Format Conversion

The `MovieConverter` utility allows converting between movie formats:

### Supported Formats

| Format | Extension | Emulator |
|--------|-----------|----------|
| Mesen | .mmo | Mesen2 |
| SMV | .smv | Snes9x |
| LSMV | .lsmv | lsnes |
| FM2 | .fm2 | FCEUX |
| BK2 | .bk2 | BizHawk |

### Using MovieConverter

```bash
# Convert a movie
MovieConverter convert input.smv output.mmo

# Get info about a movie file
MovieConverter --info movie.smv

# List supported formats
MovieConverter --list-formats
```

### Import/Export in Mesen2

- **Tools → Import Movie** - Import movie files from other emulators
- **Tools → Export Movie** - Export the current movie to another format

## TAS State Information

The TAS state tracks:

| Field | Description |
|-------|-------------|
| FrameCount | Current frame number in the movie |
| RerecordCount | Number of times the movie has been rerecorded |
| LagFrameCount | Number of lag frames during recording |
| IsRecording | Whether a movie is currently being recorded |
| IsPlaying | Whether a movie is currently being played |

## API for Lua Scripts

### TAS Functions

```lua
-- Get TAS state
local state = emu.getTasState()
print("Frame: " .. state.frameCount)
print("Rerecords: " .. state.rerecordCount)
print("Recording: " .. tostring(state.isRecording))
print("Playing: " .. tostring(state.isPlaying))

-- Check if rerecording is available
local canRerecord = emu.canRerecord()
```

### Input Functions (existing)

```lua
-- Get current input for port 0
local input = emu.getInput(0)

-- Set input for port 0
emu.setInput(0, { a = true, b = false, up = true })
```

## Best Practices for TAS Creation

1. **Save frequently** - Create savestates at checkpoints for easy rerecording
2. **Enable rerecord counter** - Track your progress and complexity
3. **Use frame advance** - For precise inputs, advance frame-by-frame
4. **Use input display** - Verify your inputs are being recorded correctly
5. **Backup your work** - Keep copies of your movie and savestate files

## Technical Notes

### Movie File Format (.mmo)

Mesen movie files (.mmo) are ZIP archives containing:
- `GameSettings.txt` - Game and emulator settings
- `Input.txt` - Frame-by-frame input data
- `SaveState.mss` - Initial savestate (if recording from savestate)

### Rerecord Implementation

Rerecording works by:
1. Storing input lines with frame position markers
2. When loading a savestate, truncating to the appropriate frame
3. Incrementing the rerecord counter
4. Continuing recording from the truncated position

### Input Format

Each frame's inputs are stored as a pipe-separated string:
```
P1:ABLR..UD|P2:....UDLR
```

Where:
- A, B, X, Y, L, R = Buttons
- U, D, L, R = D-Pad
- . = Button not pressed

## Related GitHub Issues

- #18 - MovieConverter Console App
- #19 - TAS Rerecording Support
- #20 - Input Display Overlay
- #21 - Movie Format Import/Export
- #22 - TAS Keyboard Shortcuts
- #23 - TAS State API
- #24 - Rerecord Counter HUD
- #25 - TAS Documentation

## Future Enhancements

Planned features for future releases:
- Loop playback for specific frame ranges
- Slow motion playback
- Input editor with GUI
- Splice/merge movie files
- TAS input prediction display
- Memory watch during TAS
