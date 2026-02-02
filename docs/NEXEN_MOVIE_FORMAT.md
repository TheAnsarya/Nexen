# .nexen-movie File Format Specification

**Version:** 1.0  
**Author:** Nexen Development Team  
**Status:** Draft

## Overview

The `.nexen-movie` format is a ZIP-based container for TAS (Tool-Assisted Speedrun) movies. It is designed to be:

1. **Human-readable** - Input logs are plain text, editable with any text editor
2. **Easy to inspect** - Standard ZIP format, can be opened with any archive tool
3. **Complete** - Contains all metadata, inputs, and optional savestates
4. **Portable** - No external dependencies or emulator-specific data

## File Structure

A `.nexen-movie` file is a standard ZIP archive containing:

```text
movie.nexen-movie
├── movie.json       # Required: Metadata
├── input.txt        # Required: Input log
├── savestate.bin    # Optional: Initial savestate (for movies starting from state)
└── sram.bin         # Optional: Initial SRAM (for movies starting from SRAM)
```

## movie.json

JSON file containing movie metadata.

### Schema

```json
{
	"$schema": "https://nexen.dev/schemas/movie-1.0.json",
	
	"formatVersion": "1.0",
	"emulatorVersion": "Nexen 0.1.0",
	"createdDate": "2026-02-01T20:47:00Z",
	
	"author": "string",
	"description": "string",
	"gameName": "string",
	"romFileName": "string",
	
	"sha1Hash": "40 hex chars",
	"sha256Hash": "64 hex chars (optional)",
	"md5Hash": "32 hex chars (optional)",
	"crc32": 12345678,
	
	"systemType": "snes",
	"region": "ntsc",
	
	"controllerCount": 2,
	"portTypes": ["gamepad", "gamepad", "none", "none", "none", "none", "none", "none"],
	
	"rerecordCount": 0,
	"lagFrameCount": 0,
	"totalFrames": 0,
	
	"startsFromSavestate": false,
	"startsFromSram": false
}
```

### Field Descriptions

| Field | Type | Required | Description |
| ------- | ------ | ---------- | ------------- |
| `formatVersion` | string | Yes | Format version (currently "1.0") |
| `emulatorVersion` | string | No | Emulator that created the movie |
| `createdDate` | ISO 8601 | No | Creation timestamp |
| `author` | string | No | TAS author(s) |
| `description` | string | No | Description or comments |
| `gameName` | string | No | Game title |
| `romFileName` | string | No | ROM filename (without path) |
| `sha1Hash` | string | Yes | SHA-1 hash of ROM (for verification) |
| `sha256Hash` | string | No | SHA-256 hash of ROM |
| `md5Hash` | string | No | MD5 hash of ROM |
| `crc32` | integer | No | CRC32 of ROM |
| `systemType` | enum | Yes | Target system (see below) |
| `region` | enum | Yes | Video region (`ntsc` or `pal`) |
| `controllerCount` | integer | Yes | Number of active controller ports |
| `portTypes` | array | Yes | Controller type for each port |
| `rerecordCount` | integer | No | Number of rerecords |
| `lagFrameCount` | integer | No | Number of lag frames |
| `totalFrames` | integer | Yes | Total frame count |
| `startsFromSavestate` | boolean | Yes | Whether movie starts from savestate |
| `startsFromSram` | boolean | Yes | Whether movie starts from SRAM |

### System Types

| Value | System |
| ------- | -------- |
| `nes` | Nintendo Entertainment System |
| `snes` | Super Nintendo |
| `gb` | Game Boy |
| `gbc` | Game Boy Color |
| `gba` | Game Boy Advance |
| `sms` | Sega Master System |
| `genesis` | Sega Genesis/Mega Drive |
| `pce` | PC Engine/TurboGrafx-16 |
| `ws` | WonderSwan |

### Controller Types

| Value | Description |
| ------- | ------------- |
| `none` | No controller |
| `gamepad` | Standard gamepad |
| `mouse` | Mouse (SNES Mouse, etc.) |
| `superscope` | Super Scope light gun |
| `justifier` | Konami Justifier |
| `multitap` | Multitap adapter |
| `zapper` | NES Zapper |
| `powerpad` | Power Pad / Family Trainer |

## input.txt

Plain text file containing input for each frame, one line per frame.

### Format

```text
[CMD:command]|P1_INPUT|P2_INPUT|...|[LAG]|[# comment]
```

- Lines starting with `//` are header comments (ignored)
- Empty lines are ignored
- Each data line represents one frame
- Fields are separated by `|`

### Input Format by System

#### SNES (12 characters)

```text
BYsSUDLRAXLR
```

| Position | Button | Uppercase = Pressed |
| ---------- | -------- | --------------------- |
| 0 | B | B |
| 1 | Y | Y |
| 2 | Select | s |
| 3 | Start | S |
| 4 | Up | U |
| 5 | Down | D |
| 6 | Left | L |
| 7 | Right | R |
| 8 | A | A |
| 9 | X | X |
| 10 | L Shoulder | l |
| 11 | R Shoulder | r |

Example: `BY..U...AX.r` = B + Y + Up + A + X + R pressed

#### NES (8 characters)

```text
RLDUSTBA
```

| Position | Button |
| ---------- | -------- |
| 0 | Right |
| 1 | Left |
| 2 | Down |
| 3 | Up |
| 4 | Start (T) |
| 5 | Select (S) |
| 6 | B |
| 7 | A |

### Special Markers

| Marker | Description |
| -------- | ------------- |
| `LAG` | This frame is a lag frame (input not polled) |
| `# text` | Comment for this frame |
| `CMD:SOFT_RESET` | Soft reset |
| `CMD:HARD_RESET` | Hard reset (power cycle) |
| `CMD:FDS_INSERT` | FDS: Insert disk |
| `CMD:FDS_SELECT` | FDS: Select disk side |
| `CMD:VS_COIN` | VS System: Insert coin |
| `CMD:CTRL_SWAP` | Controller port swap |

### Example

```text
// Nexen Movie Input Log
// Game: Super Mario World
// Author: TASer123
// Frames: 100

............|............
............|............
...S........|............|# Press Start
............|............
.Y..U.......|............
BY..UD.R.X.r|............|LAG
............|............|# Wait for level load
```

## savestate.bin

Optional binary file containing the emulator savestate to start from.

- Format is Nexen's native savestate format
- Compressed with zlib
- Only present if `startsFromSavestate` is true

## sram.bin

Optional binary file containing initial SRAM data.

- Raw SRAM bytes
- Only present if `startsFromSram` is true

## Validation

When loading a movie, emulators should verify:

1. ROM hash matches `sha1Hash` (or fall back to other hashes)
2. `systemType` matches the loaded ROM
3. `totalFrames` matches actual line count in input.txt
4. `controllerCount` matches port configuration

## Compatibility

The format is designed to be forward-compatible:

- Unknown JSON fields should be ignored
- Unknown input markers should be ignored
- Minimum viable movie: `movie.json` with required fields + `input.txt`

## Changelog

### Version 1.0 (2026-02-01)

- Initial specification
- Support for NES, SNES, GB, GBC, GBA, SMS, Genesis, PCE, WonderSwan
