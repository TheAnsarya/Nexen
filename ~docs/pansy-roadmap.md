# Mesen2 Pansy Integration Roadmap

**Created:** 2026-01-19 18:45 UTC  
**Last Updated:** 2026-01-26 05:15 UTC

## Phase 1: Core Export ✅ COMPLETE

**Completed:** 2026-01-19 18:43 UTC  
**Branch:** `pansy-export`  
**Commit:** `6fd99def`

### Implemented Features
- [x] Binary format writer following Pansy v1.0 spec
- [x] Symbol export (labels with addresses)
- [x] Comment export (per-address comments)
- [x] CDL data export (code/data/jump/sub flags)
- [x] Jump target list export
- [x] Subroutine entry point list export
- [x] Platform ID mapping for all systems
- [x] Menu integration in debugger
- [x] Config option for auto-export
- [x] Localization strings (English)
- [x] File dialog with `.pansy` extension

## Phase 1.5: Auto-Export & Integrity 🔄 IN PROGRESS

**Target:** 2026-01-26  
**Priority:** CRITICAL  
**Plan:** [background-cdl-recording.md](plans/background-cdl-recording.md), [rom-hash-verification.md](plans/rom-hash-verification.md)

### Background CDL Recording
- [ ] Enable CDL recording without opening debugger
- [ ] Auto-start recording on ROM load (if option enabled)
- [ ] Periodic auto-save (configurable interval)
- [ ] Save on ROM unload
- [ ] Background timer for auto-save

### ROM Hash Verification
- [ ] Calculate actual ROM CRC32 (fix placeholder)
- [ ] Store CRC32 in Pansy file header
- [ ] Verify CRC before updating existing Pansy
- [ ] Create separate files for different ROM versions
- [ ] Configuration for CRC mismatch behavior

## Phase 2: Testing & Validation

**Target:** 2026-01-27  
**Priority:** High

### Testing Tasks
- [ ] Create test suite for PansyExporter
- [ ] Unit tests for binary format writing
- [ ] Integration tests with Mesen2 debugger
- [ ] Validate roundtrip (export → Pansy viewer → verify)
- [ ] Test all supported platforms (NES, SNES, GB, GBA, etc.)
- [ ] Performance testing with large ROMs
- [ ] Memory leak testing
- [ ] Cross-platform testing (Windows, Linux, macOS)

### Bug Fixes
- [ ] Add ROM CRC32 calculation (currently placeholder)
- [ ] Handle empty/null CDL data gracefully
- [ ] Validate file write permissions
- [ ] Add error logging for failed exports

## Phase 3: Enhanced Data Export

**Target:** 2026-01-22  
**Priority:** Medium

### New Features
- [ ] **Memory Regions** - Export named memory regions
  - PRG-ROM, CHR-ROM, RAM, SRAM, VRAM mappings
  - Bank information for banked systems
  - Memory type classification
  
- [ ] **Cross-References** - Export call/jump relationships
  - JSR/CALL relationships
  - JMP destinations
  - Branch targets with types
  - Data read/write references
  
- [ ] **Data Types** - Export table/struct definitions
  - Byte arrays, word arrays
  - String tables
  - Pointer tables
  - Custom data structures

### Implementation Details
- Create `BuildMemoryRegionsSection()` method
- Create `BuildCrossReferencesSection()` method
- Create `BuildDataTypesSection()` method
- Update header section count logic

## Phase 4: Performance & Optimization

**Target:** 2026-01-25  
**Priority:** Medium

### Optimizations
- [ ] **Compression** - Add zstd compression
  - Implement per-section compression
  - Add compression flag to header
  - Benchmark compression ratios
  
- [ ] **Incremental Updates** - Only export changed sections
  - Track dirty sections
  - Compare timestamps
  - Partial file updates
  
- [ ] **Async Export** - Don't block UI
  - Background worker thread
  - Progress reporting
  - Cancellation support

### Performance Targets
- Export time: < 100ms for 1MB ROM
- File size: < 50KB for typical NES ROM
- Memory usage: < 10MB during export

## Phase 5: UI Enhancements

**Target:** 2026-01-27  
**Priority:** Low

### UI Improvements
- [ ] **Progress Dialog** - Show export progress
  - Progress bar for large files
  - Section-by-section status
  - Estimated time remaining
  
- [ ] **Export Statistics** - Show export summary
  - Symbols exported count
  - Comments exported count
  - File size saved
  - Compression ratio (if enabled)
  
- [ ] **Auto-Reload** - Hot reload on file changes
  - Watch for external `.pansy` file changes
  - Auto-reload labels if changed
  - Merge conflict resolution

### Shortcuts
- [ ] Add keyboard shortcut for export (Ctrl+Shift+P?)
- [ ] Add toolbar button for quick export
- [ ] Add context menu in code viewer

## Phase 6: Advanced Features

**Target:** 2026-02-01  
**Priority:** Low

### Advanced Capabilities
- [ ] **Diff/Merge** - Compare two Pansy files
  - Show added/removed/modified symbols
  - Merge two analysis sessions
  - Conflict resolution UI
  
- [ ] **Import** - Load Pansy files back into Mesen2
  - Import symbols from `.pansy`
  - Import comments
  - Import memory regions
  - Merge with existing labels
  
- [ ] **Batch Export** - Export multiple ROMs
  - Folder watch mode
  - Batch processing UI
  - Command-line export tool

### Integration
- [ ] Lua API for export control
- [ ] HTTP API for remote tools
- [ ] Plugin system for custom exporters

## Phase 7: Documentation & Community

**Target:** 2026-02-05  
**Priority:** High

### Documentation
- [x] Feature documentation (pansy-integration.md)
- [x] Roadmap (this file)
- [ ] User guide with screenshots
- [ ] Developer guide for contributors
- [ ] Video tutorial
- [ ] Blog post announcement

### Community
- [ ] Submit pull request to upstream Mesen2
- [ ] Announce on Mesen2 Discord
- [ ] Post on ROM hacking forums
- [ ] Create example `.pansy` files
- [ ] Integration with popular tools

## Success Metrics

### Adoption Goals
- **Week 1:** 10 users testing
- **Month 1:** 100 active exports
- **Month 3:** Upstream merge accepted

### Quality Goals
- **Code Coverage:** > 80%
- **User Rating:** > 4.5/5
- **Bug Reports:** < 5/month
- **Export Success Rate:** > 99%

## Dependencies

### External Tools
- **Pansy CLI** - For validation and testing
- **Pansy UI** - For viewing exported files
- **Upstream Mesen2** - For merge compatibility

### Internal Systems
- Mesen2 Debugger API
- Label Manager
- Code Data Logger (CDL)
- Memory Manager

## Risk Assessment

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Upstream API changes | High | Medium | Pin to specific Mesen2 version |
| Pansy format changes | High | Low | Version negotiation |
| Performance issues | Medium | Low | Profiling and optimization |
| User adoption | Medium | Medium | Good documentation |
| Merge conflicts | Low | High | Regular upstream syncs |

## Timeline Summary

```
2026-01-19: ✅ Phase 1 Complete - Core Export
2026-01-20: 🔄 Phase 2 - Testing & Validation
2026-01-22: ⏳ Phase 3 - Enhanced Data Export
2026-01-25: ⏳ Phase 4 - Performance & Optimization
2026-01-27: ⏳ Phase 5 - UI Enhancements
2026-02-01: ⏳ Phase 6 - Advanced Features
2026-02-05: ⏳ Phase 7 - Documentation & Community
```

## Notes

- Keep pansy-export branch in sync with upstream master
- Test each phase thoroughly before moving to next
- Get community feedback after Phase 2
- Consider upstream merge after Phase 4

---

**Maintained by:** TheAnsarya  
**Contact:** Via GitHub issues on pansy or Mesen2 forks
