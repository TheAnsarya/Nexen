# TAS/Movie System Architecture

## Overview

The Nexen TAS (Tool-Assisted Speedrun) system provides:

1. **Movie Recording** - Capture inputs during gameplay
2. **Movie Playback** - Replay recorded inputs
3. **Rerecording** - Load savestates during recording to refine sections
4. **Format Conversion** - Import/export from other emulator formats

## Components

```text
┌─────────────────────────────────────────────────────────────────┐
│                          UI Layer                                │
├─────────────────────────────────────────────────────────────────┤
│  TAS HUD          │  Input Display    │  Movie Controls         │
│  - Frame counter  │  - Button display │  - Record/Play/Stop     │
│  - Rerecord count │  - Visual layout  │  - Read-only toggle     │
│  - Lag counter    │                   │  - Frame advance        │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                       MovieManager                               │
├─────────────────────────────────────────────────────────────────┤
│  - Movie state tracking (TasState)                              │
│  - Recording/playback logic                                     │
│  - Rerecord handling                                            │
│  - Input injection/capture                                      │
└─────────────────────────────────────────────────────────────────┘
                              │
                    ┌─────────┴─────────┐
                    ▼                   ▼
┌───────────────────────┐   ┌───────────────────────────────────┐
│     MovieConverter    │   │         Emulator Core             │
├───────────────────────┤   ├───────────────────────────────────┤
│  - Format readers     │   │  - Input polling hooks            │
│  - Format writers     │   │  - Savestate support              │
│  - Data conversion    │   │  - Lag frame detection            │
└───────────────────────┘   └───────────────────────────────────┘
```

## Core Types

### TasState (C++ - Core)

```cpp
struct TasState {
    uint32_t FrameCount;
    uint32_t RerecordCount;
    uint32_t LagFrameCount;
    bool IsRecording;
    bool IsPlaying;
    bool IsPaused;
    bool IsReadOnly;
};
```

### MovieData (C# - MovieConverter)

```csharp
class MovieData {
    // Metadata
    string Author;
    string Description;
    string GameName;
    string RomFileName;
    string Sha1Hash;
    
    // System info
    SystemType SystemType;
    RegionType Region;
    
    // Statistics
    uint RerecordCount;
    uint LagFrameCount;
    
    // Frames
    List<InputFrame> InputFrames;
    
    // Start state
    byte[]? InitialState;
    byte[]? InitialSram;
}
```

### InputFrame (C# - MovieConverter)

```csharp
class InputFrame {
    int FrameNumber;
    ControllerInput[] Controllers;
    FrameCommand Command;
    bool IsLagFrame;
    string? Comment;
}
```

## Recording Flow

```text
┌─────────────────────────────────────────────────────────────────┐
│                        Recording Flow                           │
└─────────────────────────────────────────────────────────────────┘

User: Start Recording
         │
         ▼
    ┌────────────┐
    │ Initialize │ ─── Store initial state (if not from power-on)
    │   Movie    │ ─── Reset frame counter
    └────────────┘ ─── Set IsRecording = true
         │
         ▼
    ┌────────────┐
    │  Per-Frame │ ◄───────────────────────────────┐
    │    Loop    │                                  │
    └────────────┘                                  │
         │                                          │
         ▼                                          │
    ┌────────────┐                                  │
    │ Read Input │ ─── Get controller state         │
    └────────────┘                                  │
         │                                          │
         ▼                                          │
    ┌────────────┐                                  │
    │Store Frame │ ─── Add to InputFrames list      │
    └────────────┘                                  │
         │                                          │
         ▼                                          │
    ┌────────────┐                                  │
    │  Execute   │ ─── Run emulator frame           │
    │   Frame    │                                  │
    └────────────┘                                  │
         │                                          │
         ▼                                          │
    ┌────────────┐   No                             │
    │ Lag Frame? │ ─────────────────────────────────┤
    └────────────┘                                  │
         │ Yes                                      │
         ▼                                          │
    ┌────────────┐                                  │
    │  Mark LAG  │ ─── Set IsLagFrame = true        │
    └────────────┘ ─── Increment LagFrameCount      │
         │                                          │
         └──────────────────────────────────────────┘

User: Stop Recording
         │
         ▼
    ┌────────────┐
    │ Save Movie │ ─── Write to .nexen-movie
    └────────────┘
```

## Playback Flow

```text
┌─────────────────────────────────────────────────────────────────┐
│                        Playback Flow                            │
└─────────────────────────────────────────────────────────────────┘

User: Load Movie
         │
         ▼
    ┌────────────┐
    │ Load Movie │ ─── Parse .nexen-movie
    │    File    │ ─── Verify ROM hash
    └────────────┘
         │
         ▼
    ┌────────────┐
    │ Initialize │ ─── Load initial state (if present)
    │  Playback  │ ─── Set IsPlaying = true
    └────────────┘ ─── Set frame = 0
         │
         ▼
    ┌────────────┐
    │  Per-Frame │ ◄───────────────────────────────┐
    │    Loop    │                                  │
    └────────────┘                                  │
         │                                          │
         ▼                                          │
    ┌────────────┐                                  │
    │  Inject    │ ─── Override controller state    │
    │   Input    │     with movie frame             │
    └────────────┘                                  │
         │                                          │
         ▼                                          │
    ┌────────────┐                                  │
    │  Execute   │ ─── Run emulator frame           │
    │   Frame    │                                  │
    └────────────┘                                  │
         │                                          │
         ▼                                          │
    ┌────────────┐   No                             │
    │ End of     │ ─────────────────────────────────┤
    │  Movie?    │                                  │
    └────────────┘                                  │
         │ Yes
         ▼
    ┌────────────┐
    │   Stop     │ ─── Set IsPlaying = false
    │  Playback  │
    └────────────┘
```

## Rerecording Flow

```text
┌─────────────────────────────────────────────────────────────────┐
│                       Rerecording Flow                          │
└─────────────────────────────────────────────────────────────────┘

During Recording:
         │
         ▼
User: Load Savestate
         │
         ▼
    ┌────────────┐
    │ Truncate   │ ─── Remove frames after savestate frame
    │   Movie    │
    └────────────┘
         │
         ▼
    ┌────────────┐
    │ Increment  │ ─── RerecordCount++
    │  Rerecord  │
    └────────────┘
         │
         ▼
    ┌────────────┐
    │  Resume    │ ─── Continue recording from this point
    │ Recording  │
    └────────────┘
```

## Format Conversion

```text
┌─────────────────────────────────────────────────────────────────┐
│                    Format Conversion Flow                       │
└─────────────────────────────────────────────────────────────────┘

    ┌─────────┐     ┌─────────────┐     ┌─────────┐
    │  .bk2   │ ──► │             │ ──► │.nexen-  │
    │  .lsmv  │     │  MovieData  │     │  movie  │
    │  .fm2   │     │  (Common)   │     │         │
    │  .smv   │     │             │     │         │
    └─────────┘     └─────────────┘     └─────────┘
                          │
                          ▼
                    ┌───────────┐
                    │ Universal │
                    │   Model   │
                    └───────────┘
```

## HUD Layout

```text
┌─────────────────────────────────────────────────────────────────┐
│                         Game Screen                             │
│                                                                 │
│  ┌──────────────────┐                    ┌──────────────────┐  │
│  │ Frame: 12345     │                    │   [▲]            │  │
│  │ Lag: 123         │                    │ [◄][▼][►]        │  │
│  │ Rerecord: 456    │                    │                  │  │
│  │ ● REC  RO        │                    │  B Y S s A X L R │  │
│  └──────────────────┘                    └──────────────────┘  │
│       TAS HUD                                 Input Display    │
│                                                                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Keyboard Shortcuts

| Action | Default Key |
| -------- | ------------- |
| Frame Advance | F |
| Frame Rewind | Shift+F |
| Toggle Pause | P |
| Toggle Recording | R |
| Toggle Read-Only | Q |
| Save Movie | Ctrl+M |
| Load Movie | Ctrl+Shift+M |

## Related Files

- [MovieConverter/](../MovieConverter/) - Format conversion library
- [docs/NEXEN_MOVIE_FORMAT.md](NEXEN_MOVIE_FORMAT.md) - File format specification
- [UI/ViewModels/TasViewModel.cs](../UI/ViewModels/TasViewModel.cs) - UI state (TODO)
- [Core/Shared/Movies/](../Core/Shared/Movies/) - C++ movie handling (TODO)

## GitHub Issues

- [#132 - [Epic] TAS/Movie System](https://github.com/TheAnsarya/Nexen/issues/132)
- [#133 - Core MovieData Models](https://github.com/TheAnsarya/Nexen/issues/133)
- [#134 - .nexen-movie Format](https://github.com/TheAnsarya/Nexen/issues/134)
- [#135 - MovieConverter Library](https://github.com/TheAnsarya/Nexen/issues/135)
- [#136 - Movie Recording & Playback](https://github.com/TheAnsarya/Nexen/issues/136)
- [#137 - Rerecording](https://github.com/TheAnsarya/Nexen/issues/137)
- [#138 - TAS HUD](https://github.com/TheAnsarya/Nexen/issues/138)
- [#139 - Input Display](https://github.com/TheAnsarya/Nexen/issues/139)
- [#140 - Frame Advance](https://github.com/TheAnsarya/Nexen/issues/140)
- [#141 - Lua API](https://github.com/TheAnsarya/Nexen/issues/141)
- [#142 - Import/Export UI](https://github.com/TheAnsarya/Nexen/issues/142)
