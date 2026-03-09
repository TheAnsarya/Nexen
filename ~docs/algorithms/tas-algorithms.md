# TAS Algorithm Documentation

Detailed documentation of the algorithms used in Nexen's TAS (Tool-Assisted Speedrun) and movie system.

---

## 1. XOR Delta Compression (RewindData)

### Purpose

Efficiently store rewind history by only saving frame-to-frame differences.

### Algorithm

Consecutive emulator savestates are very similar — typically only a few bytes change per frame (register updates, VRAM changes). XOR compression exploits this:

```
delta[i] = current_state[i] XOR previous_state[i]
```

Since most bytes are identical, `current XOR previous = 0` for unchanged bytes. The delta is mostly zeros, which compresses extremely well with DEFLATE.

### Implementation

**Compression (SaveState):**

1. Serialize current emulator state to byte array (~2 MB for SNES)
2. Every 30 frames: store a **full state** (keyframe)
3. Between keyframes: XOR current state against previous full state
4. DEFLATE-compress the result

**Decompression (LoadState):**

1. DEFLATE-decompress the stored data
2. If this is a full state: use directly
3. If this is a delta: walk backward through history to find the nearest full state
4. XOR the delta against the full state to reconstruct

**Keyframe interval:** 30 frames (1 full state per ~0.5 seconds at 60 FPS)

### Complexity

- **Space:** O(delta_size * n/30 + full_size * n/30) — roughly 10-100x smaller than storing every full state
- **Compression time:** O(state_size) — single XOR pass + DEFLATE
- **Decompression time:** O(state_size) — DEFLATE + single XOR pass
- **Worst case reconstruction:** O(29 * state_size) — 29 deltas to walk back to keyframe

### Files

- [Core/Shared/RewindData.h](../../Core/Shared/RewindData.h) — Data structure
- [Core/Shared/RewindData.cpp](../../Core/Shared/RewindData.cpp) — Implementation

---

## 2. Ring Buffer Audio (RewindManager)

### Purpose

Manage audio samples during rewind without allocation.

### Algorithm

Audio samples are written to a fixed-size circular buffer. During rewind, the write position moves backward through the buffer, providing reverse audio playback.

```
write_pos = (write_pos + samples) % buffer_size
```

**During normal play:** write position advances forward
**During rewind:** write position moves backward, reading previously-written samples

### Implementation

The `RewindManager` maintains:

- `_audioBuffer`: Fixed-size circular buffer for audio samples
- `_audioPosition`: Current write position in the buffer
- `_audioSampleSize`: Size of each audio sample

Three branches handle buffer wrap-around:

1. Data fits within remaining buffer space — direct copy
2. Data wraps around the end — split into two copies
3. Data exactly fills to end — single copy to end

### Complexity

- **Space:** O(buffer_size) — fixed allocation
- **Time per frame:** O(audio_samples_per_frame) — memcpy only

### Files

- [Core/Shared/RewindManager.h](../../Core/Shared/RewindManager.h) — Buffer management
- [Core/Shared/RewindManager.cpp](../../Core/Shared/RewindManager.cpp) — Implementation

---

## 3. Movie Input Text Protocol

### Purpose

Serialize controller state as human-readable text for storage in movie files.

### Algorithm

Each frame's input is a pipe-delimited string of controller port states:

```
|Port1State|Port2State|...|PortNState
```

Each port's state is a fixed-length string where each character position represents a button:

```
BYsSUDLRAXLR    (SNES — 12 buttons)
RLDUSTBA        (NES — 8 buttons)
```

- Button pressed: letter shown (e.g., `A`, `B`, `U`)
- Button not pressed: `.` (dot)

### Example

```
|.Y..UD...XL.|............    SNES: P1=Y+Up+Down+X+L, P2=nothing
|R...U..A|........            NES: P1=Right+Up+A, P2=nothing
```

### Parsing (per frame during load)

```
line.substr(1)                  // Remove leading '|'
StringUtilities::Split(_, '|')  // Split by '|' into port strings
```

### Serialization (per frame during record)

```
for each device:
    stream << '|' << device.GetTextState()
stream << '\n'
```

### Complexity

- **Serialize:** O(ports * button_count) per frame
- **Parse:** O(line_length) per frame
- **Total load:** O(frames * line_length) for full movie

### Files

- [Core/Shared/Movies/MovieRecorder.cpp](../../Core/Shared/Movies/MovieRecorder.cpp) — Serialization
- [Core/Shared/Movies/NexenMovie.cpp](../../Core/Shared/Movies/NexenMovie.cpp) — Parsing

---

## 4. Greenzone System

### Purpose

Track the range of "verified" frames in the TAS editor — frames that have been replayed through the emulator and confirmed correct.

### Algorithm

The greenzone tracks a contiguous range of frames `[0, greenzone_end]` that have been verified by emulator playback. When a frame is edited:

1. All frames **after** the edited frame become "unverified" (outside greenzone)
2. The greenzone end retracts to the edited frame
3. To re-verify, the user must replay from the edited frame forward

This ensures that modifications are propagated through the emulation correctly, since changing an early frame can affect all subsequent frames.

### Time Complexity

- **Check if frame is green:** O(1) — compare frame number to greenzone boundary
- **Invalidate after edit:** O(1) — update greenzone end to edited frame

### Files

- [UI/Controls/PianoRollControl.cs](../../UI/Controls/PianoRollControl.cs) — Rendering greenzone
- [UI/ViewModels/TasEditorViewModel.cs](../../UI/ViewModels/TasEditorViewModel.cs) — Greenzone management

---

## 5. Input Paint System (PianoRollControl)

### Purpose

Allow drag-to-paint input editing in the piano roll, similar to a paint program.

### Algorithm

1. **Mouse down:** Record starting cell (frame, button), determine paint value (toggle current state)
2. **Mouse move:** For each new cell along the same button lane, apply paint value
3. **Mouse up:** Emit batch paint event with all affected cells

The system uses a `HashSet<(frame, button)>` to track already-painted cells during a single drag, preventing duplicate events.

### Implementation

```csharp
// On mouse down:
_paintValue = !GetCurrentButtonState(frame, button);  // Toggle
_paintedCells.Add((frame, button));

// On mouse move:
if (!_paintedCells.Contains((frame, button))) {
    _paintedCells.Add((frame, button));
    ApplyPaint(frame, button, _paintValue);
}

// On mouse up:
EmitBatchPaintEvent(_paintedCells, _paintButton, _paintValue);
```

### Complexity

- **Per cell:** O(1) — HashSet lookup + insert
- **Total drag:** O(cells_painted) — one event per new cell

### Files

- [UI/Controls/PianoRollControl.cs](../../UI/Controls/PianoRollControl.cs) — Paint input handling

---

## 6. Rendering Cache Strategy (PianoRollControl)

### Purpose

Avoid expensive `FormattedText` allocations during 60 FPS rendering of the piano roll.

### Caches

1. **Button Label Cache** — Maps button name (string) to `FormattedText`. Invalidated on zoom change. Typically 12 entries for SNES.

2. **Frame Number Cache** — Maps frame number (int) to `FormattedText`. Uses sliding window eviction to keep only entries near the visible range. Max 200 entries.

### Eviction Strategy

When the frame number cache exceeds 200 entries:

1. Remove entries outside `[ScrollOffset - visibleFrames, ScrollOffset + 2 * visibleFrames]`
2. If still over limit, clear entirely (fallback)

This ensures frequently-accessed visible frames remain cached while distant frames are evicted.

### Static Resources

All brushes, pens, and typefaces are declared as `static readonly` fields to avoid per-frame allocations:

```csharp
private static readonly SolidColorBrush HeaderBrush = new(Color.FromRgb(240, 240, 240));
private static readonly Pen GridPen = new(Brushes.LightGray, 0.5);
private static readonly Typeface BoldTypeface = new("Segoe UI", FontStyle.Normal, FontWeight.Bold);
```

### Files

- [UI/Controls/PianoRollControl.cs](../../UI/Controls/PianoRollControl.cs) — All rendering code

---

## 7. Movie File Format (ZIP-based)

### Purpose

Store complete TAS movies as self-contained archives.

### Structure

```
movie.nexen-movie (ZIP archive)
├── Input.txt           — Frame-by-frame input data (required)
├── GameSettings.txt    — Configuration key-value pairs (required)
├── MovieInfo.txt       — Author, description metadata (optional)
├── SaveState.nexen-save — Embedded savestate for mid-game start (optional)
├── Battery.*           — SRAM/battery data (optional)
└── PatchData.*         — ROM patches (optional)
```

### Why ZIP?

- Standard format — readable with any ZIP tool
- Human-editable — Input.txt is plain text
- Extensible — new files can be added without breaking old readers
- Compressed — DEFLATE handles the Input.txt efficiently

### Files

- [Core/Shared/Movies/MovieRecorder.cpp](../../Core/Shared/Movies/MovieRecorder.cpp) — Writing ZIP
- [Core/Shared/Movies/NexenMovie.cpp](../../Core/Shared/Movies/NexenMovie.cpp) — Reading ZIP
- [MovieConverter/Converters/NexenMovieConverter.cs](../../MovieConverter/Converters/NexenMovieConverter.cs) — C# read/write
