# 🌼 Pansy Export - User Guide

Nexen now supports exporting and importing debug metadata in the **Pansy** format, a universal disassembly metadata format designed for sharing and preserving your reverse engineering work.

## Overview

The Pansy export feature allows you to:

- **Export** all your labels, comments, and code/data markings to a portable file
- **Import** Pansy files to restore or merge debug information
- **Sync** automatically with folder-based storage for seamless workflow
- **Convert** from other debug formats (ca65 DBG, WLA-DX, SDCC, etc.)

## Quick Start

### Exporting a Pansy File

1. Open a ROM in Nexen
2. Add labels and comments as you analyze
3. Go to **Debug → Integration → Export Pansy File...**
4. Choose a save location
5. Your metadata is now saved!

### Importing a Pansy File

1. Open the same ROM (or a compatible one)
2. Go to **Debug → Integration → Import Pansy File...**
3. Select the `.pansy` file
4. Labels and comments are merged with your workspace

## Configuration Options

Access via **Options → Debugger Options → Integration** tab.

### Auto-Save Settings

| Option | Description |
| -------- | ------------- |
| **Auto-save Pansy files** | Automatically export when ROM is loaded/unloaded |
| **Auto-save interval** | How often to save in background (1-60 minutes) |
| **Save on ROM unload** | Export when closing a ROM |

### Export Options

| Option | Description |
| -------- | ------------- |
| **Include memory regions** | Export named memory regions (RAM banks, etc.) |
| **Include cross-references** | Export who-calls-who relationships |
| **Include data blocks** | Export data type annotations |
| **Use compression** | Compress file with DEFLATE (smaller files) |

### Folder-Based Storage

Enable **Use folder-based storage** for advanced workflows:

| Option | Description |
| -------- | ------------- |
| **Sync MLB files** | Auto-sync Nexen Label Files (human-readable) |
| **Sync CDL files** | Auto-sync Code/Data Log files |
| **Keep version history** | Maintain backups of previous exports |
| **Watch for external changes** | Detect when files are modified externally |
| **Auto-reload on change** | Automatically import external changes |

## File Format

### Pansy File Structure

```text
┌─────────────────────────────────────────┐
│ Header (32 bytes)                    │
│  - Magic: "PANSY\0\0\0"                │
│  - Version: 1.0 or 1.1                  │
│  - Platform: NES/SNES/GB/GBA/etc      │
│  - Flags: compression, features        │
│  - ROM Size + CRC32                    │
│  - Timestamp                          │
├─────────────────────────────────────────┤
│ Sections (variable)                    │
│  - Symbols (labels)                    │
│  - Comments                            │
│  - Code offsets                        │
│  - Data offsets                        │
│  - Jump targets                        │
│  - Subroutine entry points              │
│  - Memory regions                    │
│  - Cross-references                    │
├─────────────────────────────────────────┤
│ Footer                                  │
│  - Section CRC32                      │
└─────────────────────────────────────────┘
```

### Supported Platforms

| Platform | ID | Notes |
| ---------- | ----- | ------- |
| NES | 0x01 | 6502 CPU |
| SNES | 0x02 | 65816 CPU, SPC700, DSP |
| Game Boy | 0x03 | LR35902 CPU |
| GBA | 0x04 | ARM7TDMI |
| SMS/Game Gear | 0x06 | Z80 CPU |
| PC Engine | 0x07 | HuC6280 |
| WonderSwan | 0x0B | V30MZ |

## Folder Storage Layout

When using folder-based storage, Nexen creates a `GameName_debug/` folder next to your ROM:

```text
MyGame_debug/
├── manifest.json          # ROM info, settings
├── deadbeef.mlb            # Labels (Nexen format)
├── deadbeef.cdl            # Code/Data Log
├── deadbeef.pansy        # Full Pansy export
└── .history/              # Version history
    ├── 20240115_120000.mlb
    ├── 20240115_130000.mlb
    └── ...
```

## Converting from Other Formats

### Supported Source Formats

| Format | Extension | Tool |
| -------- | ----------- | ------ |
| ca65 Debug | `.dbg` | cc65/ca65 |
| WLA-DX | `.sym` | WLA-DX assembler |
| RGBDS | `.sym` | Game Boy assembler |
| SDCC | `.sym` | Small Device C Compiler |
| ELF | `.elf` | GCC/Clang output |
| Nexen MLB | `.mlb` | Nexen native |

### Converting

1. Go to **Debug → Integration → Convert to Pansy...**
2. Select your debug file(s)
3. Choose output location
4. Pansy file is generated!

Batch conversion is supported for multiple files.

## Best Practices

### For Game Analysis

1. **Enable auto-save** to never lose work
2. Use **folder storage** for easy external tool integration
3. **Export before major changes** for backup

### For Sharing

1. Export with **compression** enabled for smaller files
2. Include **cross-references** for navigation
3. Document your analysis in label comments

### For Collaboration

1. Use **folder storage** with version control (Git)
2. The `.mlb` files are human-readable text
3. The `.pansy` file can be diffed with specialized tools

## Troubleshooting

### Import Shows 0 Labels

- Check that the ROM CRC matches the Pansy file
- Verify import filters aren't blocking labels
- Try importing with "Import other labels" enabled

### File is Very Large

- Enable **Use compression** in export options
- Disable **Include cross-references** (largest section)

### External Changes Not Detected

- Ensure **Watch files for external changes** is enabled
- Check that the debug folder exists
- Some editors don't trigger file system events

## Keyboard Shortcuts

| Action | Default Shortcut |
| -------- | ------------------ |
| Quick Export Pansy | Ctrl+Shift+P |
| Quick Import Pansy | Ctrl+Shift+I |
| Force Sync | Ctrl+Shift+S |

(Customize in **Options → Debugger Options → Shortcuts**)

## API Reference

For developers integrating with Pansy:

- See `PansyExporter.cs` for export implementation
- See `PansyImporter.cs` for import implementation
- See `DebugFolderManager.cs` for folder storage
- See `SyncManager.cs` for file watching

The Pansy format specification is documented in the [Pansy repository](https://github.com/user/pansy).
