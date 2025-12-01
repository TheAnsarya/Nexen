# Mesen Movie Converter

A standalone command-line tool for converting TAS (Tool-Assisted Speedrun) movie files between various emulator formats.

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
cd MovieConverter
dotnet build
```

The output will be placed in `bin/win-x64/Debug/MesenMovieConverter.exe`.

## License

Same as Mesen2 (GPLv3).
