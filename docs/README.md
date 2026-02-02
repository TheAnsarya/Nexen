# Nexen API Documentation

This directory contains auto-generated API documentation for Nexen and user guides.

## User Documentation

| Document | Description |
|----------|-------------|
| [TAS Editor Manual](TAS-Editor-Manual.md) | Complete user guide for TAS movie editing |
| [TAS Developer Guide](TAS-Developer-Guide.md) | Technical documentation for TAS system |
| [TAS Architecture](TAS_ARCHITECTURE.md) | High-level architecture overview |
| [Nexen Movie Format](NEXEN_MOVIE_FORMAT.md) | NMV file format specification |

## Generating Documentation

### Prerequisites

1. **Install Doxygen** (version 1.9.0 or later):
   - **Windows:** Download from https://www.doxygen.nl/download.html
   - **macOS:** `brew install doxygen`
   - **Linux:** `sudo apt install doxygen` or `sudo dnf install doxygen`

2. **Optional - Graphviz** (for diagrams):
   - **Windows:** Download from https://graphviz.org/download/
   - **macOS:** `brew install graphviz`
   - **Linux:** `sudo apt install graphviz`

### Building Documentation

From the repository root:

```bash
# Generate documentation
doxygen Doxyfile

# Output will be in docs/api/html/
```

### Viewing Documentation

Open `docs/api/html/index.html` in a web browser.

## Documentation Structure

```
docs/
├── api/                    # Generated Doxygen output
│   ├── html/               # HTML documentation
│   ├── xml/                # XML output (for tools)
│   └── nexen.tag           # Tag file for cross-referencing
├── doxygen-warnings.log    # Build warnings log
└── README.md               # This file
```

## Writing Documentation

### Comment Style

Nexen uses XML-style documentation comments compatible with both Doxygen and Visual Studio IntelliSense:

```cpp
/// <summary>
/// Brief description of the function.
/// </summary>
/// <param name="param1">Description of first parameter.</param>
/// <param name="param2">Description of second parameter.</param>
/// <returns>Description of return value.</returns>
/// <remarks>
/// Additional details, implementation notes, or usage examples.
/// </remarks>
void ExampleFunction(int param1, const string& param2);
```

### Class Documentation

```cpp
/// <summary>
/// Brief description of the class.
/// </summary>
/// <remarks>
/// Detailed description with:
/// - Design rationale
/// - Usage examples
/// - Thread safety notes
/// - Performance considerations
/// </remarks>
class ExampleClass {
public:
    /// <summary>Gets the current value.</summary>
    /// <returns>The current value.</returns>
    [[nodiscard]] int GetValue() const;
};
```

### Enum Documentation

```cpp
/// <summary>
/// Describes the type of memory operation.
/// </summary>
enum class MemoryOperationType : uint8_t {
    Read,     ///< Memory read operation
    Write,    ///< Memory write operation
    Execute,  ///< Instruction fetch
    DmaRead,  ///< DMA read transfer
    DmaWrite  ///< DMA write transfer
};
```

### Custom Aliases

The Doxyfile defines these custom aliases for common documentation patterns:

| Alias | Usage |
|-------|-------|
| `@cpu` | CPU type information |
| `@platform` | Platform-specific notes |
| `@threadsafe` | Thread safety information |
| `@performance` | Performance considerations |
| `@constexpr_note` | Note about compile-time evaluation |
| `@nodiscard_note` | Note about [[nodiscard]] attribute |

Example:
```cpp
/// <summary>Converts RGB555 to ARGB8888.</summary>
/// @platform SNES, GBA
/// @constexpr_note
/// @performance Zero runtime overhead when constexpr-evaluated.
constexpr uint32_t Rgb555ToArgb(uint16_t rgb555);
```

## CI/CD Integration

### GitHub Actions

Add to your workflow:

```yaml
- name: Generate Documentation
  run: |
    sudo apt-get install -y doxygen graphviz
    doxygen Doxyfile
    
- name: Deploy to GitHub Pages
  uses: peaceiris/actions-gh-pages@v3
  with:
    github_token: ${{ secrets.GITHUB_TOKEN }}
    publish_dir: ./docs/api/html
```

### Local Validation

Check for documentation warnings:

```bash
doxygen Doxyfile 2>&1 | grep -E "warning:|error:"
# Or check the log file:
cat docs/doxygen-warnings.log
```

## Excluded Files

The following third-party libraries are excluded from documentation:

- `Core/Lua/` - Lua scripting engine
- `Utilities/magic_enum.hpp` - Enum reflection library
- `Utilities/miniz.*` - Compression library
- `Utilities/stb_vorbis.*` - Audio decoding
- `Utilities/emu2413.*` - YM2413 emulation
- `Utilities/ymfm/` - Yamaha FM synthesis
- `Utilities/blip_buf.*` - Band-limited audio synthesis
- `Utilities/NTSC/` - NTSC video filtering
- `Utilities/xBRZ/` - Image scaling

## Related Files

- [Doxyfile](../Doxyfile) - Doxygen configuration
- [FORMATTING.md](../FORMATTING.md) - Code formatting guidelines
- [COMPILING.md](../COMPILING.md) - Build instructions
