# Pansy Metadata Export - Nexen Integration

**Created:** 2026-01-19 18:44 UTC  
**Branch:** `pansy-export`  
**Status:** Implemented, ready for testing

## Overview

Nexen now supports automatic export of debugging metadata to the [Pansy format](https://github.com/TheAnsarya/pansy), a universal disassembly metadata format designed for retro game development.

## Features

### Automatic Export

- **When enabled:** Nexen automatically exports a `.pansy` file whenever debugging data changes
- **Location:** Same folder as ROM, with `.pansy` extension
- **Default:** Enabled by default for seamless workflow

### Manual Export

- **Menu Location:** Debug → Export Pansy metadata
- **Keyboard Shortcut:** None (can be configured)
- **File Dialog:** Choose custom location and filename

### Exported Data

The following metadata is exported from Nexen:

| Category | Description |
| ---------- | ------------- |
| **Symbols** | All labels and their addresses |
| **Comments** | Per-address comments |
| **Code/Data Map** | CDL flags (code, data, jump targets, subroutines) |
| **Jump Targets** | All branch/jump destinations |
| **Subroutines** | Function entry points |
| **Memory Regions** | Planned (not yet implemented) |
| **Cross-References** | Planned (not yet implemented) |

## Configuration

### Enable/Disable Auto-Export

1. Open **Debug → Debugger Settings**
2. Navigate to **Integration** tab
3. Check/uncheck **Automatically export .PANSY files**

### Supported Platforms

Pansy export supports all Nexen platforms with correct Platform IDs:

- **NES** (iNes, FDS, NSF) → Platform ID 0x01
- **SNES** (SFC, SPC) → Platform ID 0x02
- **Game Boy** (GB, GBS) → Platform ID 0x03
- **Game Boy Advance** → Platform ID 0x04
- **Sega Genesis/SMS/Game Gear** → Platform ID 0x05/0x06
- **PC Engine** → Platform ID 0x07
- **WonderSwan** → Platform ID 0x0B
- **Others** → Platform ID 0xFF (custom)

## File Format

Pansy files are binary metadata files with the following structure:

```text
┌──────────────────────────────────────┐
│          Header (32 bytes)          │
├──────────────────────────────────────┤
│        Section Count (4 bytes)      │
├──────────────────────────────────────┤
│         Section Table            │
├──────────────────────────────────────┤
│        Section Data                │
│  - CODE_DATA_MAP (CDL flags)       │
│  - SYMBOLS (labels)                 │
│  - COMMENTS                         │
│  - JUMP_TARGETS                     │
│  - SUB_ENTRY_POINTS                 │
├──────────────────────────────────────┤
│         Footer (12 bytes)        │
│  - ROM CRC32   (placeholder)     │
│  - Metadata CRC32 (placeholder)     │
│  - File CRC32 (placeholder)      │
└──────────────────────────────────────┘
```

### Header Details

| Offset | Size | Type | Description |
| -------- | ------ | ------ | ------------- |
| 0x00 | 8 | char[8] | Magic: "PANSY\0\0\0" |
| 0x08 | 2 | uint16 | Version: 0x0100 |
| 0x0A | 1 | uint8 | Platform ID |
| 0x0B | 1 | uint8 | Flags (0x00 = no compression) |
| 0x0C | 4 | uint32 | ROM size (from CDL stats) |
| 0x10 | 4 | uint32 | ROM CRC32 (placeholder: 0) |
| 0x14 | 8 | uint64 | Timestamp (Unix epoch) |
| 0x1C | 4 | uint32 | Reserved (0) |

## Usage Examples

### Example 1: Export During Debugging Session

1. Load a ROM in Nexen
2. Add labels and comments in the debugger
3. Code/Data Logger tracks execution
4. `.pansy` file is automatically updated in ROM folder

### Example 2: Import into Pansy Viewer

```bash
# View the exported metadata
pansy info game.pansy

# List all symbols
pansy symbols game.pansy

# Show cross-references
pansy xrefs game.pansy
```

### Example 3: Use with Assembler

Many assemblers can import Pansy files for symbol resolution:

```bash
# Compile with Pansy symbols
poppy assemble game.asm --symbols game.pansy

# Disassemble with Pansy metadata
peony disassemble game.bin --metadata game.pansy
```

## Implementation Details

### Code Structure

- **File:** `UI/Debugger/Labels/PansyExporter.cs`
- **Class:** `PansyExporter` (static utility class)
- **Methods:**
	- `Export(path, romInfo, memoryType)` - Manual export
	- `AutoExport(romInfo, memoryType)` - Called on debugger shutdown
	- `GetPansyFilePath(romName)` - Get default file path

### Integration Points

1. **Menu Action:** `DebuggerWindowViewModel.cs` line ~860
2. **Config Setting:** `IntegrationConfig.cs` line ~20
3. **UI Checkbox:** `DebuggerConfigWindow.axaml` line ~238
4. **Localization:** `resources.en.xml` lines 3255-3257, 1562-1563

### Future Enhancements

Planned improvements for future releases:

- [ ] **Compression** - zstd compression for large files
- [ ] **Memory Regions** - Export memory map definitions
- [ ] **Cross-References** - Export call/jump relationships
- [ ] **Data Types** - Export table/struct definitions
- [ ] **Auto-reload** - Hot reload on file changes
- [ ] **CRC Validation** - Proper ROM CRC32 calculation
- [ ] **Progress Dialog** - Show progress for large exports
- [ ] **Incremental Updates** - Only export changed sections

## Testing

### Manual Testing Checklist

- [x] Build succeeds without errors
- [ ] Export menu item appears in debugger
- [ ] File dialog opens with correct extension
- [ ] Pansy file is created successfully
- [ ] Symbols are exported correctly
- [ ] Comments are exported correctly
- [ ] CDL data matches expected flags
- [ ] Auto-export works on shutdown
- [ ] Config option toggles auto-export
- [ ] File can be opened by Pansy viewer
- [ ] All supported platforms work

### Test Files

Create test `.pansy` files for validation:

- Small NES ROM with ~10 labels
- SNES ROM with comments and CDL
- Game Boy ROM with subroutines marked

## Troubleshooting

### Export Fails Silently

**Problem:** No error shown, no file created  
**Solution:** Check debugger folder write permissions

### File Is Empty or Corrupted

**Problem:** Pansy file doesn't open  
**Solution:** Ensure CDL data exists (run some code in debugger)

### Symbols Not Appearing

**Problem:** Labels missing from export  
**Solution:** Verify labels are saved in label manager

### Platform ID Wrong

**Problem:** Pansy shows wrong platform  
**Solution:** Check RomFormat mapping in PlatformIds dictionary

## Related Links

- [Pansy GitHub Repository](https://github.com/TheAnsarya/pansy)
- [Pansy File Format Specification](https://github.com/TheAnsarya/pansy/blob/main/docs/FILE-FORMAT.md)
- [Nexen Debugger Documentation](https://nexen.theansarya.com/docs/debugging/)

## Change Log

### 2026-01-19 - Initial Implementation

- Created PansyExporter class
- Added menu integration
- Added config option
- Added localization strings
- Implemented binary format writer
- Committed to pansy-export branch
