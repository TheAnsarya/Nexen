# Movie System Guide

## Overview

Nexen movies capture deterministic input history and related metadata for playback, verification, and TAS editing.

## Core Workflows

### Record a Movie

1. Start a game and open movie controls.
2. Begin recording.
3. Play normally or with frame stepping.
4. Stop and save the movie file.

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

## File Format

- Nexen native movie format: `.nexen-movie`
- Technical specification: [NEXEN_MOVIE_FORMAT.md](NEXEN_MOVIE_FORMAT.md)

## Related Links

- [TAS Editor Manual](TAS-Editor-Manual.md)
- [TAS Developer Guide](TAS-Developer-Guide.md)
- [Save States Guide](Save-States.md)
