# 🌼 Pansy Export - Documentation Index

Welcome to the Pansy export feature documentation for Nexen. This feature enables exporting and importing debug metadata in the universal [Pansy format](https://github.com/TheAnsarya/pansy).

## Documentation

| Document | Description | Audience |
| ---------- | ------------- | ---------- |
| [User Guide](pansy-export-user-guide.md) | How to use Pansy export/import | End users |
| [Tutorials](pansy-export-tutorials.md) | Step-by-step workflows | End users |
| [API Documentation](pansy-export-api.md) | Class and method reference | Developers |
| [Developer Guide](pansy-export-developer-guide.md) | Architecture and extension guide | Contributors |

## Quick Links

### For Users

- **[Quick Start](pansy-export-user-guide.md#quick-start)** - Get started in 5 minutes
- **[Configuration Options](pansy-export-user-guide.md#configuration-options)** - All settings explained
- **[Folder Storage](pansy-export-user-guide.md#folder-storage-layout)** - Advanced storage options
- **[Troubleshooting](pansy-export-user-guide.md#troubleshooting)** - Common issues and fixes

### For Developers

- **[Architecture Overview](pansy-export-developer-guide.md#architecture-overview)** - System design
- **[Binary Format](pansy-export-api.md#binary-format-reference)** - File format specification
- **[Adding Features](pansy-export-developer-guide.md#adding-new-features)** - Extending the system
- **[Testing Guidelines](pansy-export-developer-guide.md#testing-guidelines)** - How to write tests

## Feature Overview

### Export Capabilities

| Feature | Status | Notes |
| --------- | -------- | ------- |
| Labels/Symbols | ✅ | All memory types supported |
| Comments | ✅ | Per-address comments |
| Code Offsets | ✅ | From CDL data |
| Data Offsets | ✅ | From CDL data |
| Jump Targets | ✅ | Branch destinations |
| Subroutine Entry Points | ✅ | Function starts |
| Memory Regions | ✅ | Named regions |
| Cross-References | ✅ | Who-calls-who |
| Compression | ✅ | DEFLATE |
| Progress UI | ✅ | Cancel support |

### Import Capabilities

| Feature | Status | Notes |
| --------- | -------- | ------- |
| Labels/Symbols | ✅ | Merged with existing |
| Comments | ✅ | Merged with existing |
| Code/Data Offsets | ✅ | Applied to CDL |
| Memory Regions | ✅ | Created in Nexen |
| Cross-References | ✅ | Read-only display |
| Validation | ✅ | CRC checking |

### Storage & Sync

| Feature | Status | Notes |
| --------- | -------- | ------- |
| File Export | ✅ | Single .pansy file |
| Folder Storage | ✅ | MLB/CDL/Pansy sync |
| Auto-Save | ✅ | Configurable interval |
| File Watching | ✅ | External change detection |
| Version History | ✅ | Backup previous versions |
| MLB Sync | ✅ | Human-readable labels |
| CDL Sync | ✅ | Code/data log |

### Format Conversion

| Source Format | Status | Notes |
| --------------- | -------- | ------- |
| Nexen MLB | ✅ | Native format |
| ca65 DBG | ✅ | cc65 toolchain |
| WLA-DX SYM | ✅ | Game Boy assembler |
| RGBDS SYM | 🔄 | Planned |
| SDCC SYM | 🔄 | Planned |
| ELF/DWARF | 🔄 | Planned |

## Implementation Status

### Completed Phases

| Phase | Description | Commit |
| ------- | ------------- | -------- |
| Phase 1 | Core Pansy export | ✅ |
| Phase 1.5 | Background CDL & ROM verification | ✅ |
| Phase 3 | Memory regions & cross-references | ✅ |
| Phase 4 | Performance & compression | ✅ |
| Phase 7.5a | Folder-based storage | ✅ |
| Phase 7.5b | MLB sync | ✅ |
| Phase 7.5c | CDL sync | ✅ |
| Phase 7.5d | DBG integration | ✅ |
| Phase 7.5e | Sync manager | ✅ |
| Phase 2 | Test suite (152 tests) | ✅ |
| Phase 5 | UI enhancements | ✅ |
| Phase 6 | Import functionality | ✅ |
| Phase 7 | Documentation | ✅ |

### Test Coverage

| Test Suite | Tests | Status |
| ------------ | ------- | -------- |
| PansyExporterTests | 30 | ✅ |
| PansyImporterTests | 50 | ✅ |
| DebugFolderManagerTests | 21 | ✅ |
| SyncManagerTests | 26 | ✅ |
| DbgToPansyConverterTests | 25 | ✅ |
| **Total** | **152** | ✅ |

## Related Documentation

### Implementation Notes

- [Pansy Integration](pansy-integration.md) - Original design document
- [Pansy Roadmap](pansy-roadmap.md) - Feature roadmap
- [Phase 7.5 Sync](phase-7.5-pansy-sync.md) - Sync feature design

### External Resources

- [Pansy Format Specification](https://github.com/TheAnsarya/pansy/blob/main/docs/format.md)
- [Pansy Repository](https://github.com/TheAnsarya/pansy)
- [Nexen Repository](https://github.com/TheAnsarya/Nexen)

## Getting Help

### Issues

Found a bug? Have a feature request?

1. Check existing [GitHub Issues](https://github.com/TheAnsarya/Nexen/issues)
2. Search this documentation
3. Create a new issue with:
   - Nexen version
   - Steps to reproduce
   - Expected vs actual behavior
   - Pansy file (if relevant)

### Contributing

Want to contribute?

1. Read the [Developer Guide](pansy-export-developer-guide.md)
2. Check the [roadmap](pansy-roadmap.md) for planned features
3. Fork, branch, implement, test, PR!

## Version History

| Version | Date | Changes |
| --------- | ------ | --------- |
| 1.0.0 | 2026-01 | Initial implementation |
|  |  | - Core export/import |
|  |  | - Folder storage |
|  |  | - Sync manager |
|  |  | - Format conversion |
|  |  | - Comprehensive tests |
|  |  | - Full documentation |
