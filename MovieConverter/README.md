# Mesen Movie Converter Library

A .NET 8 class library for converting TAS (Tool-Assisted Speedrun) movie files between various emulator formats. Used by Mesen2 for Import/Export functionality and also available as a standalone CLI tool.

## Architecture

- **MovieConverter** (this folder) - Class library containing all conversion logic
- **MovieConverter.CLI** - Command-line tool that uses the library for scripted/external conversions

## Supported Formats

| Format | Extension | Read | Write | Description |
|--------|-----------|------|-------|-------------|
| Mesen | .mmo | ✅ | ✅ | Mesen2 native format (ZIP with text input) |
| Snes9x | .smv | ✅ | ✅ | Snes9x movie (binary, SNES) |
| lsnes | .lsmv | ✅ | ✅ | lsnes movie (ZIP, SNES) |
| FCEUX | .fm2 | ✅ | ✅ | FCEUX movie (text, NES) |
| BizHawk | .bk2 | ✅ | ✅ | BizHawk movie (ZIP, multi-system) |

## Usage

### Convert a movie file

```bash
MesenMovieConverter input.smv output.mmo
```

### View movie information

```bash
MesenMovieConverter --info movie.smv
```

### List supported formats

```bash
MesenMovieConverter --list-formats
```

### Options

- `--author <name>` - Set author name for output
- `--description <text>` - Set description for output
- `--verbose` - Show detailed output
- `--force` - Overwrite existing output file

## Examples

Convert Snes9x movie to Mesen format:
```bash
MesenMovieConverter speedrun.smv speedrun.mmo --author "MyName"
```

Convert lsnes movie to BizHawk format:
```bash
MesenMovieConverter tas.lsmv tas.bk2 --verbose
```

View movie info:
```bash
MesenMovieConverter --info myrun.smv
```

## Notes

### Format Compatibility

- **System compatibility**: Movies can only be converted between emulators that support the same system. Converting a SNES movie to FM2 format (NES) will not produce a valid movie.
- **Input mapping**: Different emulators may have different controller configurations. Some manual adjustment may be needed after conversion.
- **Save states**: Movies with embedded save states may not transfer perfectly between formats.

### Rerecord Count

The rerecord count is preserved during conversion when available in the source format.

### Controller Configuration

Each emulator has slightly different controller support:
- Standard controller inputs (D-pad, face buttons) convert well
- Mouse, Super Scope, and multitap inputs may have limited support
- Some formats (like BK2) support multiple systems with different controller layouts

## Building

Requires .NET 8.0 SDK.

```bash
# Build the library
cd MovieConverter
dotnet build

# Build the CLI tool
cd MovieConverter.CLI
dotnet build
```

The library will be output as `Mesen.MovieConverter.dll`.
The CLI tool will be output as `MesenMovieConverter.exe`.

## Using the Library

```csharp
using Mesen.MovieConverter;

// Detect format from file extension
var format = MovieConverterRegistry.DetectFormat("movie.smv");

// Get converter for the format
var converter = MovieConverterRegistry.GetConverter(format);

// Read a movie
var movieData = converter.Read("input.smv");

// Write to another format
var mesenConverter = MovieConverterRegistry.GetConverter(MovieFormat.Mesen);
mesenConverter.Write(movieData, "output.mmo");
```

## License

Same as Mesen2 (GPLv3).
