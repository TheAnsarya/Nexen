# Movie System Guide

## Overview

Nexen movies capture deterministic input history and related metadata for playback, verification, and TAS editing.

## Core Workflows

### Record a Movie

1. Start a game and open Movie Recording Settings.
2. Select Start Recording.
3. Play normally or with frame stepping.
4. Select Stop Recording and save the movie file.

### Playback and Review

1. Open an existing movie.
2. Start playback.
3. Pause, rewind, and inspect key moments.
4. Open TAS editor for frame-level adjustments.

### Import and Export

Nexen supports import and export for common TAS/movie formats. Use this when sharing with external tools or migrating old projects.

## GUI Tips

- Use branches in TAS editor to compare strategies.
- Use save states before major reroutes.
- Use movie playback to verify deterministic outcomes.

## Panel Walkthrough (Screenshot Anchors)

### Walkthrough A: Record and Save a Movie

1. Open Movie Recording Settings from the main UI while a ROM is loaded. Expected result: the Movie Recording Settings window opens.
2. Select Start Recording. Expected result: recording becomes active and frame/input capture begins.
3. Perform gameplay input sequence. Expected result: inputs are appended to the active movie timeline.
4. Select Stop Recording and save the movie file. Expected result: recording stops and the movie file is written to disk.

| Step | Panel | Screenshot Anchor ID | Capture Target |
|---|---|---|---|
| 1 | Movie Recording Settings (Open) | `movie-a-01-controls-open` | `docs/screenshots/movie-system/a-01-controls-open.png` |
| 2 | Movie Recording Settings (Start Recording Active) | `movie-a-02-recording-active` | `docs/screenshots/movie-system/a-02-recording-active.png` |
| 3 | Save Movie Dialog | `movie-a-03-save-dialog` | `docs/screenshots/movie-system/a-03-save-dialog.png` |

### Walkthrough B: Playback and TAS Handoff

1. Open an existing movie. Expected result: movie metadata and timeline are loaded for playback.
2. Start playback and pause at target frame. Expected result: playback halts on the selected verification frame.
3. Open TAS editor and inspect input timeline. Expected result: the timeline displays recorded inputs for frame-level review.
4. Create a branch and continue testing. Expected result: a new TAS branch is created without overwriting baseline movie history.

| Step | Panel | Screenshot Anchor ID | Capture Target |
|---|---|---|---|
| 1 | Open Movie Dialog | `movie-b-01-open-dialog` | `docs/screenshots/movie-system/b-01-open-dialog.png` |
| 2 | Playback View (Paused at Target Frame) | `movie-b-02-paused-target` | `docs/screenshots/movie-system/b-02-paused-target.png` |
| 3 | TAS Editor (Piano Roll) | `movie-b-03-tas-piano-roll` | `docs/screenshots/movie-system/b-03-tas-piano-roll.png` |

## Supported Formats

| Format | Extension | Emulator | Systems | Read | Write |
|---|---|---|---|---|---|
| Nexen | `.nexen-movie` | Nexen | Multi | ✅ | ✅ |
| BK2 | `.bk2` | BizHawk | Multi | ✅ | ✅ |
| LSMV | `.lsmv` | lsnes | SNES, GB | ✅ | ✅ |
| FM2 | `.fm2` | FCEUX | NES | ✅ | ✅ |
| SMV | `.smv` | Snes9x | SNES | ✅ | ✅ |
| VBM | `.vbm` | VisualBoyAdvance | GBA, GB, GBC | ✅ | ❌ |
| GMV | `.gmv` | Gens | Genesis | ✅ | ❌ |

For full technical details on the MovieConverter library, see the [MovieConverter README](../MovieConverter/README.md).

## File Format

- Nexen native movie format: `.nexen-movie`
- Technical specification: [NEXEN_MOVIE_FORMAT.md](NEXEN_MOVIE_FORMAT.md)

## Related Links

- [TAS Editor Manual](TAS-Editor-Manual.md)
- [TAS Developer Guide](TAS-Developer-Guide.md)
- [Save States Guide](Save-States.md)
