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

---

## 8. Converter StringBuilder Pooling

### Purpose

Eliminate per-frame `StringBuilder` and temporary `List<string>` allocations during movie format conversion.

### Problem

Each converter (FM2, LSMV, BK2) created a new `StringBuilder` per frame during write operations. For a 200K-frame movie, that's 200K+ `StringBuilder` allocations. Similarly, `InputFrame.ToNexenLogLine()` allocated a `List<string>` and called `string.Join()` per frame.

### Solution

- **Hoisted StringBuilder:** Allocate once before the loop, call `Clear()` per iteration
- **Shared SB parameter:** Format methods accept a `StringBuilder` parameter instead of returning strings
- **Direct StringBuilder in ToNexenLogLine:** Replace `List<string>` + `string.Join("|", ...)` with direct `StringBuilder.Append()` calls

### Complexity

- **Before:** O(n) allocations for n frames
- **After:** O(1) allocation (single reused StringBuilder)

### Files

- [MovieConverter/Converters/Fm2MovieConverter.cs](../../MovieConverter/Converters/Fm2MovieConverter.cs) — Hoisted SB + shared SB pattern
- [MovieConverter/Converters/LsmvMovieConverter.cs](../../MovieConverter/Converters/LsmvMovieConverter.cs) — Hoisted SB
- [MovieConverter/Converters/Bk2MovieConverter.cs](../../MovieConverter/Converters/Bk2MovieConverter.cs) — Hoisted SB + void format method
- [MovieConverter/InputFrame.cs](../../MovieConverter/InputFrame.cs) — StringBuilder-based ToNexenLogLine

---

## 9. CRC32 Batched Buffer Calculation

### Purpose

Reduce API call overhead when computing CRC32 of movie input data.

### Problem

`CalculateInputCrc32()` called `Crc32.Append()` with a 2-byte buffer for every controller on every frame. For 200K frames with 2 controllers: 400K API calls with 2-byte payloads.

### Solution

Use a 4KB `stackalloc` buffer. Fill it with serialized controller data, flush to `Crc32.Append()` when full. Reduces 400K calls to ~200 calls.

### Complexity

- **Before:** O(n * controllers) `Append()` calls
- **After:** O(n * controllers * 2 / 4096) calls — ~2000x fewer

### Files

- [MovieConverter/MovieData.cs](../../MovieConverter/MovieData.cs) — `CalculateInputCrc32()`

---

## 10. Clone Skip-Init Optimization

### Purpose

Avoid wasted `ControllerInput` allocations during deep copy of `InputFrame`.

### Problem

`InputFrame.Clone()` called `new InputFrame(frameNumber)` → `this()` which created 8 default `ControllerInput` objects. Then the clone loop immediately replaced all 8 with cloned copies. For 200K frames: 1.6M wasted allocations.

### Solution

Private constructor `InputFrame(int frameNumber, bool _)` skips the initialization loop. Clone uses this constructor and fills controllers directly from the source. Handles partial `Controllers` arrays (length < 8) by creating defaults only for missing indices.

### Complexity

- **Before:** 8 + 8 = 16 `ControllerInput` allocations per Clone (8 wasted + 8 cloned)
- **After:** 8 allocations per Clone (only the cloned copies)

### Files

- [MovieConverter/InputFrame.cs](../../MovieConverter/InputFrame.cs) — `Clone()` and private constructor

---

## 11. Incremental ObservableCollection Updates

### Problem

`UpdateFrames()` clears the entire `ObservableCollection<TasFrameViewModel>` and rebuilds it from scratch.
For a 200K-frame movie, this creates 200K new objects and fires 200K+ change notifications per operation.
Operations like Paste, Fork/Truncate, and Undo/Redo all triggered this full rebuild.

### Solution

Batch helper methods that make targeted modifications:

- **InsertFrameViewModels(index, count):** Inserts `count` new VMs at `index`, then renumbers the tail once. O(count + tail).
- **RemoveFrameViewModels(index, count):** Removes `count` VMs by iterating backward from end of range. O(count + tail) for renumbering.
- **TruncateFrameViewModels(startIndex):** Removes all VMs from `startIndex` to end by reverse iteration. O(removed), no renumber needed.

### Call Site Mapping

| Operation | Before | After |
|-----------|--------|-------|
| Paste | `UpdateFrames()` — O(n) | `InsertFrameViewModels` — O(k + tail) |
| Fork/Truncate | `UpdateFrames()` — O(n) | `TruncateFrameViewModels` — O(removed) |
| Undo (Insert) | `UpdateFrames()` — O(n) | `RemoveFrameViewModels` — O(k + tail) |
| Undo (Delete) | `UpdateFrames()` — O(n) | `InsertFrameViewModels` — O(k + tail) |
| Redo (Insert) | `UpdateFrames()` — O(n) | `InsertFrameViewModels` — O(k + tail) |
| Redo (Delete) | `UpdateFrames()` — O(n) | `RemoveFrameViewModels` — O(k + tail) |
| Undo/Redo (Modify) | `UpdateFrames()` — O(n) | `RefreshFromFrame()` — O(1) |

### Files

- [UI/ViewModels/TasEditorViewModel.cs](../../UI/ViewModels/TasEditorViewModel.cs) — Batch helpers and updated call sites

---

## 12. Undo/Redo Incremental Dispatch

### Problem

Every Undo/Redo unconditionally called `UpdateFrames()` regardless of what the action modified.
`ModifyInputAction` (editing a single cell) triggered a full 200K rebuild just to update one frame.

### Solution

`ApplyIncrementalUpdate(UndoableAction, bool isUndo)` pattern-matches on the concrete action type:

- **InsertFramesAction:** Uses `Index` and `Count` properties to dispatch to `InsertFrameViewModels` or `RemoveFrameViewModels`.
- **DeleteFramesAction:** Uses `Index` and `Count` properties — inverse of insert logic.
- **ModifyInputAction:** Uses `FrameRef` to find the affected VM by reference equality, then calls `RefreshFromFrame()` — O(n) scan worst case, but typically the frame is near the scroll position.
- **Unknown action types:** Fall back to `UpdateFrames()` for safety.

### Files

- [UI/ViewModels/TasEditorViewModel.cs](../../UI/ViewModels/TasEditorViewModel.cs) — `ApplyIncrementalUpdate` dispatcher

---

## 13. PianoRoll Zero-Alloc Cache Eviction

### Problem

Frame number cache eviction in the render path used LINQ `.Where().ToList()`, allocating a `List<int>` + LINQ iterator on every eviction. While eviction only triggers every ~200 scroll events, it's still an allocation inside `Render()`.

### Solution

Replaced with a reusable `_evictionKeys` field (`List<int>`) that is `.Clear()`ed and reused on each eviction. Manual `foreach` loop replaces the LINQ query. Zero allocations in the render path.

### Files

- [UI/Controls/PianoRollControl.cs](../../UI/Controls/PianoRollControl.cs) — `_evictionKeys` field and manual eviction loop

---

## 14. Undo System Wiring (All TAS Operations)

### Problem

Several TAS operations bypassed the undo system entirely:

- `ToggleButton()` — directly mutated controller input, not undoable
- `ToggleButtonAtFrame()` — delegated to `SetButtonAtFrame` raw setter
- `DeleteFrames()` — called `Movie.InputFrames.RemoveAt()` directly
- `ClearSelectedInput()` — cleared controller states directly

Users could not undo/redo button toggles, frame deletions, or input clears — a critical TAS workflow deficiency.

### Solution

All operations now route through `ExecuteAction()` with proper undo/redo action objects:

- `ToggleButton()` → creates `ModifyInputAction` + `ExecuteAction()`
- `ToggleButtonAtFrame()` → creates `ModifyInputAction` + `ExecuteAction()`
- `DeleteFrames()` → creates `DeleteFramesAction` + `ExecuteAction()`
- `ClearSelectedInput()` → creates `ClearInputAction` + `ExecuteAction()`

New `ClearInputAction` class captures all controller states before clearing, enabling full undo restoration.

### Files

- [UI/ViewModels/TasEditorViewModel.cs](../../UI/ViewModels/TasEditorViewModel.cs) — `ToggleButton`, `ToggleButtonAtFrame`, `DeleteFrames`, `ClearSelectedInput`, `ClearInputAction`

---

## 15. ModifyInputAction O(1) Indexed Dispatch

### Problem

When `ApplyIncrementalUpdate` dispatched a `ModifyInputAction`, it used a linear O(n) search:

```csharp
for (int i = 0; i < FrameViewModels.Count; i++) {
	if (ReferenceEquals(FrameViewModels[i].Frame, modify.Frame)) {
		RefreshFrameAt(i);
		break;
	}
}
```

For a 10K-frame movie, this scans up to 10,000 entries to refresh a single frame.

### Solution

Added `FrameIndex` property to `ModifyInputAction`. The caller already knows which frame index is being modified, so the index is stored at creation time and used directly:

```csharp
case ModifyInputAction modify:
	RefreshFrameAt(modify.FrameIndex);
	break;
```

### Complexity

- **Before:** O(n) — linear scan of all frame ViewModels
- **After:** O(1) — direct index lookup

### Benchmark Impact

`SingleFrameRefresh`: 2.9 ns with 0 B allocations (92,700x faster than full rebuild)

### Files

- [UI/ViewModels/TasEditorViewModel.cs](../../UI/ViewModels/TasEditorViewModel.cs) — `ModifyInputAction.FrameIndex`, `ApplyIncrementalUpdate` dispatch

---

## 16. C++ Single-Pass Movie Parsing

### Problem

`NexenMovie::Play()` used a double-pass approach to parse input frames:

1. **Pass 1:** Read all lines with `getline` to count frames
2. Seek back to beginning of input section
3. **Pass 2:** Read all lines again with `getline` to parse

This resulted in 2N `getline` calls for N frames, plus a seek operation.

### Solution

Single-pass approach:

1. Get raw string from stringstream (`.str()`)
2. Count newlines with `std::ranges::count(rawInput, '\n')` — fast character scan
3. Reserve vector with known count
4. Single `getline` pass to parse frames
5. Use `string_view(line).substr(1)` instead of `line.substr(1)` to avoid temporary string allocation per frame

### Complexity

- **Before:** O(2N * avg_line_len) — two full passes through the data
- **After:** O(N * avg_line_len) — single pass + fast character count
- **Allocation saving:** N temporary `string` objects eliminated from `substr`

### Files

- [Core/Shared/Movies/NexenMovie.cpp](../../Core/Shared/Movies/NexenMovie.cpp) — `Play()` input parsing

---

## 17. Batch Paint with Single Undo Action

### Problem

Piano roll drag-to-paint (`OnPianoRollCellsPainted`) had two critical issues:

1. Used `SetButtonAtFrame()` which directly mutated controller input without creating an undo action — all paint operations were permanently not undoable
2. After the paint loop, called `RefreshFrames()` which triggers full O(n) `UpdateFrames()` rebuild

For painting 50 frames on a 10K-frame movie: 50 direct mutations + 1 full rebuild of all 10K ViewModels.

### Solution

New `PaintInputAction` class captures all old controller states at construction, applies the button state change on `Execute()`, and restores all old states on `Undo()`. A single action covers the entire paint stroke.

New `PaintButton()` method on TasEditorViewModel:

1. Validate all frame indices
2. Clone old controller inputs for each frame
3. Create single `PaintInputAction`
4. `ExecuteAction(action)` — applies to undo stack
5. `RefreshFrameAt(idx)` for each painted frame only — O(k) not O(n)

`ApplyIncrementalUpdate` handles `PaintInputAction` by refreshing only the painted frames on undo/redo.

### Complexity

- **Before:** O(n) full rebuild after O(k) direct mutations, no undo support
- **After:** O(k) incremental refresh, full undo/redo support
- **For 50 painted frames on 10K-frame movie:** 50 refreshes instead of 10,000 rebuilds

### Files

- [UI/ViewModels/TasEditorViewModel.cs](../../UI/ViewModels/TasEditorViewModel.cs) — `PaintButton`, `PaintInputAction`, `ApplyIncrementalUpdate`
- [UI/Windows/TasEditorWindow.axaml.cs](../../UI/Windows/TasEditorWindow.axaml.cs) — Updated `OnPianoRollCellsPainted` handler

---

## 18. Unified ExecuteAction + Automatic Incremental Dispatch

### Problem

`ExecuteAction()` and `ApplyIncrementalUpdate()` were called separately. Every operation had to manually call both:

```csharp
// Before: every caller had to do this
ExecuteAction(new ModifyInputAction(...));
RefreshFrameAt(frameIndex); // manual, easy to forget
```

This led to bugs where callers (especially Lua API) forgot the view model update or bypassed undo entirely.

### Solution

`ExecuteAction()` now automatically calls `ApplyIncrementalUpdate()`:

```csharp
internal void ExecuteAction(UndoableAction action) {
	action.Execute();
	_undoStack.Push(action);
	_redoStack.Clear();
	ApplyIncrementalUpdate(action, isUndo: false); // automatic
	UpdateUndoRedoState();
	HasUnsavedChanges = true;
}
```

All callers simplified to a single line:

```csharp
// After: just one call
ExecuteAction(new ModifyInputAction(...));
```

### Impact

- **Lua API** — `SetFrameInput`, `ClearFrameInput`, `InsertFrames`, `DeleteFrames` all now call `ExecuteAction()` with proper action types
- **UI InsertFrames** — Uses `InsertFramesAction` instead of direct `Insert()`
- **Fork truncate** — Uses `DeleteFramesAction` instead of direct `RemoveRange()`
- **Undo/redo** — Unchanged (already called `ApplyIncrementalUpdate`)
- **Eliminated** all manual post-execute `RefreshFrameAt()`/`InsertFrameViewModels()`/`RemoveFrameViewModel()` calls

### Files

- [UI/ViewModels/TasEditorViewModel.cs](../../UI/ViewModels/TasEditorViewModel.cs) — `ExecuteAction` (now `internal`, auto-dispatches)
- [UI/TAS/TasLuaApi.cs](../../UI/TAS/TasLuaApi.cs) — All 4 mutation methods now use undo actions
