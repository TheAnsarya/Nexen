# TAS Editor Developer Guide

This document describes the technical architecture and implementation details of the Nexen TAS Editor system for developers who want to contribute or extend functionality.

## Architecture Overview

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                           TAS Editor Architecture                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌───────────────────────┐    ┌───────────────────────┐                    │
│  │  TasEditorWindow.axaml │◄──►│ TasEditorViewModel.cs │                    │
│  │       (View)           │    │    (ViewModel)        │                    │
│  └───────────┬───────────┘    └─────────┬─────────────┘                    │
│              │                          │                                   │
│              │                          ▼                                   │
│              │              ┌───────────────────────────┐                  │
│              │              │     Core Services         │                  │
│              │              ├───────────────────────────┤                  │
│              │              │ • GreenzoneManager        │                  │
│              │              │ • InputRecorder           │                  │
│              │              │ • MovieConverter          │                  │
│              │              └───────────────────────────┘                  │
│              │                          │                                   │
│              ▼                          ▼                                   │
│  ┌───────────────────────┐    ┌───────────────────────┐                    │
│  │  PianoRollControl     │    │     Emulator APIs      │                    │
│  │   (Custom Control)    │    │ • EmuApi              │                    │
│  └───────────────────────┘    │ • InputApi            │                    │
│                               │ • RecordApi           │                    │
│                               └───────────────────────┘                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Component Documentation

### GreenzoneManager (`UI/TAS/GreenzoneManager.cs`)

Manages automatic savestates for efficient TAS rerecording and navigation.

#### Key Features

- **Ring Buffer**: Fast access to recent frames
- **Sparse Storage**: Periodic snapshots for older frames
- **GZip Compression**: Reduces memory for old states
- **Memory Limits**: Automatic pruning when limits exceeded

#### Configuration

```csharp
public int CaptureInterval { get; set; } = 60;     // Frames between captures
public int MaxSavestates { get; set; } = 1000;     // Max states in memory
public int RingBufferSize { get; set; } = 120;     // Recent frames (fast access)
public long MaxMemoryBytes { get; set; } = 256 * 1024 * 1024; // 256 MB limit
public bool UseCompression { get; set; } = true;   // Compress old states
public int CompressionThreshold { get; set; } = 300; // Frames before compression
```

#### Key Methods

| Method | Description |
| -------- | ------------- |
| `CaptureCurrentState()` | Manually capture current frame |
| `LoadState(frame)` | Load savestate for specific frame |
| `SeekToFrameAsync(frame)` | Seek using nearest savestate |
| `InvalidateFrom(frame)` | Remove all states after frame (after edit) |
| `Clear()` | Remove all savestates |

#### Events

| Event | Description |
| ------- | ------------- |
| `SavestateCaptured` | Fired when new state captured |
| `SavestateLoaded` | Fired when state loaded |
| `SavestatesPruned` | Fired when states removed for memory |

#### Internal Storage

```csharp
private readonly ConcurrentDictionary<int, SavestateData> _savestates;
private readonly ConcurrentQueue<int> _ringBuffer;  // Most recent frames
```

### InputRecorder (`UI/TAS/InputRecorder.cs`)

Records player input during gameplay into TAS movies.

#### Recording Modes

| Mode | Behavior |
| ------ | ---------- |
| `Append` | Add frames to end of movie |
| `Insert` | Insert frames at current position, shift later frames |
| `Overwrite` | Replace existing frames starting at current position |

#### Key Methods

| Method | Description |
| -------- | ------------- |
| `StartRecording(mode)` | Begin recording in specified mode |
| `StopRecording()` | Stop recording |
| `RecordFrame(inputs)` | Record single frame (called each frame) |
| `RerecordFrom(frame)` | Truncate movie and start recording from frame |
| `CreateBranch(name)` | Save current movie as branch |
| `LoadBranch(branch)` | Restore movie from branch |

#### Events

| Event | Description |
| ------- | ------------- |
| `RecordingStarted` | Recording has begun |
| `RecordingStopped` | Recording ended |
| `FrameRecorded` | New frame recorded |
| `Rerecording` | Starting rerecord from earlier frame |

#### Branch Management

```csharp
public class BranchData
{
	public string Name { get; set; }
	public DateTime CreatedAt { get; set; }
	public int FrameCount { get; set; }
	public MovieData MovieSnapshot { get; set; }
	public byte[]? SavestateData { get; set; }
}
```

### PianoRollControl (`UI/Controls/PianoRollControl.cs`)

Custom Avalonia control for visual timeline editing.

#### Styled Properties

| Property | Type | Description |
| ---------- | ------ | ------------- |
| `Frames` | `List<FrameData>` | Frame data to display |
| `SelectedFrame` | `int` | Currently selected frame |
| `SelectionStart` | `int` | Selection range start |
| `SelectionEnd` | `int` | Selection range end |
| `ScrollOffset` | `int` | Horizontal scroll position |
| `ZoomLevel` | `double` | Zoom (0.25x - 4.0x) |
| `PlaybackFrame` | `int` | Current playback position |
| `ShowGreenzone` | `bool` | Highlight greenzone frames |
| `ShowLagFrames` | `bool` | Highlight lag frames |

#### Events

| Event | Description |
| ------- | ------------- |
| `CellClicked` | User clicked a cell (frame, button) |
| `CellsPainted` | User painted cells (drag) |
| `SelectionChanged` | Selection range changed |
| `MarkerClicked` | User clicked a comment marker |

#### Rendering

The control uses custom rendering via `OnRender()`:

```csharp
public override void Render(DrawingContext context)
{
	DrawBackground(context);
	DrawFrameCells(context);
	DrawButtonLabels(context);
	DrawPlaybackCursor(context);
	DrawSelection(context);
	DrawGreenzone(context);
	DrawMarkers(context);
}
```

#### Button Lanes

Each system has defined button lanes:

```csharp
private static readonly Dictionary<SystemType, string[]> ButtonLayouts = new()
{
	[SystemType.Nes] = ["A", "B", "Select", "Start", "Up", "Down", "Left", "Right"],
	[SystemType.Snes] = ["A", "B", "X", "Y", "L", "R", "Select", "Start", ...],
	// etc.
};
```

### TasEditorViewModel (`UI/ViewModels/TasEditorViewModel.cs`)

Main ViewModel connecting all components.

#### Key Properties

```csharp
// Core components
public GreenzoneManager Greenzone { get; }
public InputRecorder Recorder { get; }

// Movie state
[Reactive] public bool IsModified { get; set; }
[Reactive] public MovieData? CurrentMovie { get; set; }
[Reactive] public int CurrentFrame { get; set; }
[Reactive] public int TotalFrames { get; set; }

// Recording state
[Reactive] public bool IsRecording { get; set; }
[Reactive] public RecordingMode RecordMode { get; set; }
[Reactive] public int RerecordCount { get; set; }

// Greenzone state
[Reactive] public int SavestateCount { get; set; }
[Reactive] public double GreenzoneMemoryMB { get; set; }

// UI state
[Reactive] public bool ShowPianoRoll { get; set; }
```

#### Menu Items

Menus are built dynamically as `List<ContextMenuAction>`:

```csharp
RecordingMenuItems = new List<ContextMenuAction>
{
	new ContextMenuAction { ActionType = ActionType.TasStartRecording, ... },
	new ContextMenuAction { ActionType = ActionType.TasStopRecording, ... },
	new ContextMenuAction { ActionType = ActionType.Separator },
	new ContextMenuAction { ActionType = ActionType.TasRerecordFrom, ... },
	...
};
```

## Integration Points

### Emulator APIs

The TAS Editor uses several core APIs:

#### EmuApi

```csharp
// Savestate management
string statePath = EmuApi.SaveStateFile(filename);
bool success = EmuApi.LoadStateFile(filename);

// Frame execution
EmuApi.ExecuteShortcut(EmulatorShortcut.RunSingleFrame);
EmuApi.ExecuteShortcut(EmulatorShortcut.Pause);
```

#### InputApi

```csharp
// Get current input state
InputState state = InputApi.GetInputState();

// Set input programmatically (for playback)
InputApi.SetInputState(inputs);
```

#### RecordApi

```csharp
// Native movie operations
RecordApi.StartRecording(new RecordMovieOptions {
	Filename = path,
	Format = "nmv"
});
RecordApi.StopRecording();
```

### Movie File Formats

The MovieConverter library handles format conversion:

```csharp
// In MovieConverter.Core
public static MovieData ImportMovie(string path);
public static void ExportMovie(MovieData movie, string path, string format);
```

## Extending the TAS Editor

### Adding New Movie Format

1. Implement `IMovieFormat` in MovieConverter:

```csharp
public class NewFormat : IMovieFormat
{
	public string Extension => ".new";
	public MovieData Import(Stream stream);
	public void Export(MovieData movie, Stream stream);
}
```

1. Register in `MovieFormatRegistry`

### Adding New System Layout

1. Add button layout to `PianoRollControl.ButtonLayouts`
2. Update `TasEditorViewModel.GetControllerLayout()`

### Adding Custom Greenzone Policy

Implement custom capture policy:

```csharp
public interface ICapturPolicy
{
	bool ShouldCapture(int frame, GameState state);
	int GetCaptureInterval(int frame);
}
```

## Performance Considerations

### Memory Management

- **Greenzone**: Uses GZip compression for states older than 300 frames
- **Ring Buffer**: Maintains last 120 states uncompressed for fast seeking
- **Pruning**: Automatically removes oldest states when memory exceeds 256 MB

### Threading

- **Greenzone capture**: Runs on background thread
- **UI updates**: Marshalled to UI thread via `Dispatcher`
- **Seek operations**: Async with progress reporting

### Hot Paths

⚠️ These paths are performance-critical:

1. `RecordFrame()` - Called every frame during recording
2. `PianoRollControl.OnRender()` - Called during scroll/zoom
3. `GreenzoneManager.CaptureState()` - Called at capture interval

Avoid allocations and complex operations in these methods.

## Testing

### Unit Tests

Located in `Nexen.Tests/TAS/`:

```text
GreenzoneManagerTests.cs
InputRecorderTests.cs
PianoRollControlTests.cs
```

### Integration Tests

Test full workflow scenarios:

```csharp
[Test]
public void RecordAndSeek_ShouldMaintainSync()
{
	// Record 1000 frames
	// Seek to frame 500
	// Verify game state matches expected
}
```

## Debugging Tips

### Logging

Enable TAS debug logging:

```csharp
// In GreenzoneManager
private const bool DebugLogging = true;
```

### Common Issues

| Issue | Likely Cause | Solution |
| ------- | -------------- | ---------- |
| Desync after seek | Greenzone state corrupted | Clear greenzone, re-record |
| High memory | Too many states | Increase capture interval |
| Slow seek | No nearby state | Lower capture interval |
| Wrong buttons | Layout mismatch | Check system detection |

## Future Improvements

- [ ] Issue #145: Lua/Scripting integration
- [ ] Undo/Redo for frame edits
- [ ] Multi-track editing (multiple controllers)
- [ ] RNG manipulation tools
- [ ] Memory watch integration
- [ ] Input display overlay
- [ ] TAS competitions support
