Nexen v1.4.30 brings UI stability fixes, TAS editor improvements, recording UX enhancements, additional performance optimizations, and code quality improvements.

## Fixed

- **Menu crash: Debug → Event Viewer** — Event Viewer window initialization could fail because a required control field was null during XAML-backed control setup; switched the grid control to `x:Name` and hardened constructor lookup to resolve controls by name before creating the ViewModel (#1245)
- **Menu crash: Tools → TAS Editor → Open/Create New Movie/Record** — TAS Editor window initialization could throw `NullReferenceException` during control wiring when named controls were not initialized as expected; switched key controls (`SearchBox`, `FrameList`, `PianoRoll`) to `x:Name` for reliable code-behind binding (#1245)
- **TAS Editor fire-and-forget async** — `LoadTasMovieAsync` was called with `_ =` (fire-and-forget), masking potential exceptions; replaced with proper `await` (#1263)
- **Recent Files menu disabled when empty** — `File → Recent Files` submenu was disabled when no recent files existed; made it always enabled with an "(empty)" placeholder item (#1264)
- **Event subscription leaks** — AssemblerWindow and PaletteViewerWindow now properly unsubscribe from events in `OnClosing` to prevent memory leaks (#1235)

## Changed

- **Recent Play retention expanded** — Increased rolling Recent Play checkpoint capacity from 12 to 36 slots (5-minute cadence, about 3 hours of history) (#1241)
- **Auto Save behavior upgraded to persistent log** — Auto Saves now use timestamped `_auto_` filenames and are no longer single-file overwrite snapshots; save list origin detection supports both legacy and new naming patterns (#1241)
- **Debugger close path hardening** — Moved final debugger workspace save/release work off the UI thread and resumed execution before debugger release to reduce close-path deadlock risk when closing the last debugger window (#1242)
- **TAS Editor improvements** — ReactiveUI `[Reactive]` property generation, piano roll button label caching, bare catch elimination, async void safety, undo stack optimization (#1250–#1258)
- **Bare catch elimination (phases 6–8)** — Replaced bare `catch` / `catch { }` blocks across UI with typed `catch (Exception ex)` + `Debug.WriteLine` for diagnosability (#1260–#1262)
- **Box-style action notifications** — Tools menu actions (audio/video recording, screenshot, export) now display as centered box-style OSD notifications instead of bottom-left text; simplified message text removes file paths (#1264)
- **Recording filename timestamps** — Sound and video recorder default filenames now include `yyyy-MM-dd_HH-mm-ss` timestamps to prevent silent overwrites of previous recordings (#1265)
- **GitHub Actions metadata** — Added missing `description` properties to `setup-ccache-action` and `build-sha1-action` composite actions

## Performance

- **HexEditor per-frame allocation removed** — `HexEditorDataProvider.Prepare()` called `.ToArray()` on every frame to produce a byte array from an existing byte array; eliminated the redundant copy (#1263)
- **BreakpointManager LINQ → manual loop** — `GetMatchingBreakpoint()` replaced LINQ `.FirstOrDefault()` with a manual `foreach` loop on the hot path (#1263)

## Downloads

### Windows

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-Windows-x64-v1.4.30.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.30/Nexen-Windows-x64-v1.4.30.exe) | Single-file, recommended |
| **Native AOT** | [Nexen-Windows-x64-AoT-v1.4.30.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.30/Nexen-Windows-x64-AoT-v1.4.30.exe) | Faster startup |

### Linux

| Build | Download | Notes |
|-------|----------|-------|
| **AppImage x64** | [Nexen-Linux-x64-v1.4.30.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.30/Nexen-Linux-x64-v1.4.30.AppImage) | Recommended |
| **AppImage ARM64** | [Nexen-Linux-ARM64-v1.4.30.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.30/Nexen-Linux-ARM64-v1.4.30.AppImage) | Raspberry Pi, etc. |
| Binary x64 (clang) | [Nexen-Linux-x64-v1.4.30.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.30/Nexen-Linux-x64-v1.4.30.tar.gz) | Tarball, requires SDL2 |
| Binary x64 (gcc) | [Nexen-Linux-x64-gcc-v1.4.30.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.30/Nexen-Linux-x64-gcc-v1.4.30.tar.gz) | Tarball, requires SDL2 |
| Binary ARM64 (clang) | [Nexen-Linux-ARM64-v1.4.30.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.30/Nexen-Linux-ARM64-v1.4.30.tar.gz) | Tarball, requires SDL2 |
| Binary ARM64 (gcc) | [Nexen-Linux-ARM64-gcc-v1.4.30.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.30/Nexen-Linux-ARM64-gcc-v1.4.30.tar.gz) | Tarball, requires SDL2 |
| Native AOT x64 | [Nexen-Linux-x64-AoT-v1.4.30.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.30/Nexen-Linux-x64-AoT-v1.4.30.tar.gz) | Faster startup |

### macOS (Apple Silicon)

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-macOS-ARM64-v1.4.30.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.30/Nexen-macOS-ARM64-v1.4.30.zip) | App bundle |
| ~~Native AOT~~ | *Temporarily unavailable* | .NET 10 ILC compiler bug |

> **Notes:**
> - Linux requires SDL2 (`sudo apt install libsdl2-2.0-0`)
> - macOS: Right-click → Open on first launch to bypass Gatekeeper
