# 🎬 Nexen MovieConverter

A .NET library for converting TAS (Tool-Assisted Speedrun) movie files between different emulator formats.

## Features

- **Read/Write** Nexen native format (`.nexen-movie`)
- **Import** from popular TAS formats:
	- BizHawk (`.bk2`) - Multi-system
	- lsnes (`.lsmv`) - SNES/GB
	- FCEUX (`.fm2`) - NES
	- Snes9x (`.smv`) - SNES
	- VisualBoyAdvance (`.vbm`) - GBA/GB/GBC
	- Gens (`.gmv`) - Genesis/Mega Drive
- **Common data model** for format-independent movie manipulation
- **Async API** for high-performance I/O
- **FrozenDictionary** for O(1) format lookups

## Supported Formats

| Format | Extension | Emulator | Systems | Read | Write |
| -------- | ----------- | ---------- | --------- | ------ | ------- |
| Nexen | `.nexen-movie` | Nexen | Multi | ✅ | ✅ |
| BK2 | `.bk2` | BizHawk | Multi | ✅ | ✅ |
| LSMV | `.lsmv` | lsnes | SNES, GB | ✅ | ✅ |
| FM2 | `.fm2` | FCEUX | NES | ✅ | ✅ |
| SMV | `.smv` | Snes9x | SNES | ✅ | 🔲 |
| VBM | `.vbm` | VisualBoyAdvance | GBA, GB, GBC | ✅ | 🔲 |
| GMV | `.gmv` | Gens | Genesis | ✅ | 🔲 |

Legend: ✅ Implemented | 🔲 Planned | ❌ Not supported

## Nexen Movie Format (`.nexen-movie`)

The Nexen native format is a ZIP archive designed to be human-readable and easy to edit:

```text
movie.nexen-movie (ZIP archive)
├── movie.json     # Metadata (JSON)
├── input.txt      # Input log (text, one line per frame)
├── savestate.bin  # Optional: Initial savestate
└── sram.bin       # Optional: Initial SRAM
```

### movie.json Example

```json
{
  "formatVersion": "1.0",
  "emulatorVersion": "Nexen 0.1.0",
  "createdDate": "2026-02-01T20:47:00Z",
  
  "author": "TASer123",
  "description": "100% completion in 1:23:45",
  "gameName": "Super Mario World",
  "romFileName": "Super Mario World (U).sfc",
  
  "sha1Hash": "abc123...",
  "systemType": "snes",
  "region": "ntsc",
  
  "controllerCount": 2,
  "portTypes": ["gamepad", "gamepad", "none", "none", "none", "none", "none", "none"],
  
  "rerecordCount": 12345,
  "lagFrameCount": 1234,
  "totalFrames": 123456,
  
  "startsFromSavestate": false,
  "startsFromSram": false
}
```

### input.txt Format

```text
// Nexen Movie Input Log
// Game: Super Mario World
// Author: TASer123
// Frames: 123456
// Format: P1|P2|...|[LAG]|[# comment]
// Buttons: BYsSUDLRAXLR (SNES)

............|............
............|............
...S........|............|# Title screen
.Y..U.......|............
BY..UD.R.X.r|............|LAG
```

Button legend (SNES): `B Y s S U D L R A X l r`

- Uppercase = pressed, `.` = not pressed
- `s` = Select, `S` = Start, `l` = L shoulder, `r` = R shoulder

## Usage

### Library

```csharp
using Nexen.MovieConverter;

// Read any supported format (auto-detected)
var movie = MovieConverterRegistry.Read("input.bk2");

// Access movie data
Console.WriteLine($"Author: {movie.Author}");
Console.WriteLine($"Frames: {movie.TotalFrames}");
Console.WriteLine($"Duration: {movie.Duration}");
Console.WriteLine($"Rerecords: {movie.RerecordCount}");

// Iterate frames
foreach (var frame in movie.InputFrames) {
    var p1 = frame.Controllers[0];
    if (p1.A) Console.WriteLine($"Frame {frame.FrameNumber}: A pressed");
}

// Write to Nexen format
MovieConverterRegistry.Write(movie, "output.nexen-movie");

// Or get a specific converter
var converter = MovieConverterRegistry.GetConverter(MovieFormat.Nexen);
converter?.Write(movie, "output.nexen-movie");
```

### Async API (.NET 10+)

```csharp
using Nexen.MovieConverter;

// Async reading and writing for I/O-bound scenarios
var movie = await MovieConverterRegistry.ReadAsync("input.bk2");

// Async conversion with cancellation support
using var cts = new CancellationTokenSource(TimeSpan.FromSeconds(30));
await MovieConverterRegistry.ConvertAsync("input.bk2", "output.nexen-movie", cancellationToken: cts.Token);

// Individual converter async methods
var converter = MovieConverterRegistry.GetConverter(MovieFormat.Nexen);
if (converter is not null) {
    var movie = await converter.ReadAsync(fileStream, "movie.nexen-movie");
    await converter.WriteAsync(movie, outputStream);
}
```

### Format Detection

```csharp
// Auto-detect format from extension
var format = MovieConverterRegistry.DetectFormat("movie.bk2"); // Returns MovieFormat.Bk2

// Get converter by format or extension
var converter = MovieConverterRegistry.GetConverter(MovieFormat.Lsmv);
var converter2 = MovieConverterRegistry.GetConverterByExtension(".fm2");

// Get file dialog filters
string openFilter = MovieConverterRegistry.GetOpenFileFilter();
// "All Movie Files (*.nexen-movie;*.bk2;*.lsmv;...)|*.nexen-movie;*.bk2;*.lsmv;...|..."
```

### CLI

```bash
# Convert BK2 to Nexen format
nexen-movie convert input.bk2 output.nexen-movie

# Show movie info
nexen-movie info movie.nexen-movie

# Batch convert
nexen-movie convert *.bk2 --output-dir converted/ --format nexen
```

## Building

```bash
cd MovieConverter
dotnet build
dotnet test
```

## Related Issues

- [#132 - [Epic] TAS/Movie System](https://github.com/TheAnsarya/Nexen/issues/132)
- [#133 - [TAS.1] Core MovieData Models](https://github.com/TheAnsarya/Nexen/issues/133)
- [#135 - [TAS.3] MovieConverter Library](https://github.com/TheAnsarya/Nexen/issues/135)

## License

The MovieConverter library code is released under **The Unlicense** (public domain).

Note: The base Nexen/Mesen2 emulator is GPL-3.0 licensed, but all code in MovieConverter/
written by TheAnsarya is dedicated to the public domain.
