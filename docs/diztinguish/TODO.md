# Mesen2 ? DiztinGUIsh Integration TODO List

**Project:** Mesen2-DiztinGUIsh Integration  
**Created:** 2025  
**Status:** Planning ? Implementation  

---

## ?? Current Focus: Phase 1 - CDL Integration

---

## Research & Planning Tasks

### Project Analysis
- [x] Clone and analyze Mesen2 codebase structure
- [x] Document Mesen2 architecture
- [x] Clone and analyze DiztinGUIsh codebase structure
- [x] Document DiztinGUIsh architecture
- [x] Create integration plan document
- [x] Document real-time streaming integration approach
- [ ] Identify Mesen2 SNES debugger entry points
- [ ] Identify DiztinGUIsh import mechanisms
- [ ] Map SNES memory addressing schemes
- [ ] Research existing CDL formats (FCEUX, Mesen NES, etc.)
- [ ] Define Mesen2 CDL format specification

---

## Streaming Integration Plan (New Priority)

### Socket-Based Real-Time Communication

See [streaming-integration.md](./streaming-integration.md) for full details.

#### Research & Planning
- [x] Analyze Mesen2's existing Socket class (Utilities/Socket.h)
- [x] Document netplay architecture as reference
- [x] Design message protocol for DiztinGUIsh bridge
- [ ] Define all message types and payloads
- [ ] Create protocol specification document
- [ ] Design connection lifecycle (handshake, heartbeat, disconnect)
- [ ] Plan security model (authentication, authorization)

#### Mesen2 - DiztinGUIsh Bridge
- [ ] **Task S.1:** Create DiztinGUIsh Bridge infrastructure
  - [ ] Create `Core/Debugger/DiztinguishBridge.h/.cpp`
  - [ ] Create `Core/Debugger/DiztinguishMessage.h`
  - [ ] Create `Core/Debugger/DiztinguishProtocol.h`
  - [ ] Implement server socket using `Utilities/Socket`
  - [ ] Implement message encoding/decoding
  - [ ] Implement thread-safe message queue
  - [ ] Add connection lifecycle management

- [ ] **Task S.2:** Execution trace streaming
  - [ ] Hook SNES CPU execution (per instruction)
  - [ ] Capture PC, opcode, operands, flags
  - [ ] Capture M/X flag state
  - [ ] Capture DB/DP registers
  - [ ] Calculate effective addresses
  - [ ] Batch traces per frame
  - [ ] Implement frame interval throttling
  - [ ] Add trace message encoding

- [ ] **Task S.3:** Memory access streaming
  - [ ] Hook memory read/write events
  - [ ] Categorize access type (code/data/gfx/audio)
  - [ ] Track PPU reads for graphics identification
  - [ ] Track APU reads for audio identification
  - [ ] Batch memory access events
  - [ ] Add memory access message encoding

- [ ] **Task S.4:** State synchronization
  - [ ] Implement CPU state snapshot messages
  - [ ] Implement differential CDL updates
  - [ ] Send state on breakpoint hit
  - [ ] Send periodic state updates

- [ ] **Task S.5:** Debug event broadcasting
  - [ ] Broadcast breakpoint hit events
  - [ ] Broadcast label updates
  - [ ] Broadcast watchpoint events
  - [ ] Add event message types

- [ ] **Task S.6:** Command receiver (from DiztinGUIsh)
  - [ ] Receive breakpoint set/clear commands
  - [ ] Receive memory request commands
  - [ ] Receive label push commands
  - [ ] Implement command validation
  - [ ] Execute commands safely

- [ ] **Task S.7:** UI integration
  - [ ] Add "Start DiztinGUIsh Server" menu item
  - [ ] Add connection settings dialog
  - [ ] Add connection status indicator
  - [ ] Add stream configuration UI
  - [ ] Add connection log viewer

#### DiztinGUIsh - Mesen2 Client
- [ ] **Task S.8:** Create client infrastructure
  - [ ] Create networking module (.NET sockets)
  - [ ] Implement message decoder
  - [ ] Implement connection manager
  - [ ] Add reconnection logic
  - [ ] Add timeout handling
  - [ ] Create event dispatcher

- [ ] **Task S.9:** Trace processing
  - [ ] Receive execution trace messages
  - [ ] Parse trace entries
  - [ ] Extract M/X flag context
  - [ ] Build execution path map
  - [ ] Update disassembly in real-time
  - [ ] Highlight executed code

- [ ] **Task S.10:** CDL integration
  - [ ] Receive CDL update messages
  - [ ] Merge with existing project data
  - [ ] Visualize coverage
  - [ ] Track code/data/gfx/audio regions

- [ ] **Task S.11:** Memory analysis
  - [ ] Receive memory access messages
  - [ ] Identify data patterns
  - [ ] Auto-categorize data types
  - [ ] Visualize memory regions

- [ ] **Task S.12:** Bidirectional control
  - [ ] Send breakpoint commands to Mesen2
  - [ ] Request memory dumps
  - [ ] Push labels to Mesen2
  - [ ] Handle command responses

- [ ] **Task S.13:** UI integration
  - [ ] Add "Connect to Mesen2" menu
  - [ ] Add connection dialog
  - [ ] Add connection status display
  - [ ] Add real-time trace viewer
  - [ ] Add stream statistics

#### Testing & Documentation
- [ ] **Task S.14:** Protocol testing
  - [ ] Unit tests for message encoding/decoding
  - [ ] Test all message types
  - [ ] Test error handling
  - [ ] Test connection lifecycle

- [ ] **Task S.15:** Integration testing
  - [ ] End-to-end connection test
  - [ ] Trace streaming accuracy test
  - [ ] CDL synchronization test
  - [ ] Label bidirectional sync test
  - [ ] Breakpoint sync test
  - [ ] Performance/bandwidth testing

- [ ] **Task S.16:** Documentation
  - [ ] Protocol specification
  - [ ] API documentation
  - [ ] User guide
  - [ ] Troubleshooting guide
  - [ ] Example workflows

### Build Environment Setup
- [ ] Successfully build Mesen2 from source (Windows)
- [ ] Successfully build DiztinGUIsh from source (Windows)
- [ ] Set up dual-IDE debugging environment
- [ ] Create build automation scripts
- [ ] Document build dependencies

### License & Legal
- [ ] Verify GPL v3 compatibility between projects
- [ ] Check DiztinGUIsh license
- [ ] Document attribution requirements
- [ ] Create contributor guidelines

---

## Phase 1: CDL Integration

### Mesen2 - Core Changes

#### CDL Tracking Implementation
- [ ] **Task 1.1:** Create CDL data structure in SNES core
  - [ ] Define CDL flags (Code, Data, PCM, Graphics)
  - [ ] Allocate CDL buffer per ROM
  - [ ] Hook into instruction execution
  - [ ] Hook into memory reads
  - [ ] Hook into PPU reads (graphics)
  - [ ] Hook into DSP reads (audio)

- [ ] **Task 1.2:** Implement memory mapping for CDL
  - [ ] Create ROM address ? CDL index mapper
  - [ ] Handle LoROM mapping
  - [ ] Handle HiROM mapping
  - [ ] Handle ExHiROM mapping
  - [ ] Handle SA-1 mapping
  - [ ] Handle SuperFX mapping

- [ ] **Task 1.3:** CDL state management
  - [ ] Initialize CDL on ROM load
  - [ ] Clear CDL on reset (with option to preserve)
  - [ ] Merge CDL data when loading state
  - [ ] Save CDL with save states (optional)

#### CDL Export Functionality
- [ ] **Task 1.4:** Create CDL file writer
  - [ ] Define file format (binary vs text)
  - [ ] Implement header writing
  - [ ] Implement data writing
  - [ ] Add checksum/validation
  - [ ] Optimize for large ROMs

- [ ] **Task 1.5:** Add export to debugger UI
  - [ ] Add "Export CDL" menu item
  - [ ] Create file save dialog
  - [ ] Add progress indicator
  - [ ] Add completion notification
  - [ ] Handle errors gracefully

#### CDL Import Functionality (Optional)
- [ ] **Task 1.6:** Create CDL file reader
  - [ ] Implement header parsing
  - [ ] Implement data reading
  - [ ] Validate checksum
  - [ ] Handle version compatibility

- [ ] **Task 1.7:** Add import to debugger UI
  - [ ] Add "Import CDL" menu item
  - [ ] Create file open dialog
  - [ ] Merge imported data with existing
  - [ ] Visualize CDL coverage

#### CDL Visualization
- [ ] **Task 1.8:** Add CDL visualization to debugger
  - [ ] Color-code disassembly by CDL flags
  - [ ] Add CDL coverage indicators
  - [ ] Add statistics view (% code coverage, etc.)
  - [ ] Add legend for color coding

### DiztinGUIsh - Integration Changes

#### CDL Import Implementation
- [ ] **Task 1.9:** Create CDL file parser
  - [ ] Implement header reading
  - [ ] Implement data reading
  - [ ] Validate format
  - [ ] Handle errors

- [ ] **Task 1.10:** Map CDL to ROM data
  - [ ] Match ROM checksum
  - [ ] Map addresses to project structure
  - [ ] Handle different ROM formats
  - [ ] Warn on mismatches

- [ ] **Task 1.11:** Apply CDL data to project
  - [ ] Auto-mark code regions
  - [ ] Auto-mark data regions
  - [ ] Preserve existing manual marks
  - [ ] Add confidence indicators
  - [ ] Create undo/redo support

- [ ] **Task 1.12:** Add import UI
  - [ ] Add "Import CDL" menu item
  - [ ] File open dialog
  - [ ] Options dialog (merge vs replace)
  - [ ] Progress indicator
  - [ ] Summary report

#### CDL Visualization
- [ ] **Task 1.13:** Visualize CDL coverage
  - [ ] Show which bytes have CDL data
  - [ ] Highlight uncovered areas
  - [ ] Add coverage statistics
  - [ ] Color-code by data type

### Testing & Validation

#### Unit Tests
- [ ] **Task 1.14:** Mesen2 CDL tests
  - [ ] Test CDL tracking accuracy
  - [ ] Test memory mapping
  - [ ] Test file I/O
  - [ ] Test edge cases

- [ ] **Task 1.15:** DiztinGUIsh CDL tests
  - [ ] Test CDL parsing
  - [ ] Test data application
  - [ ] Test error handling
  - [ ] Test edge cases

#### Integration Tests
- [ ] **Task 1.16:** End-to-end workflow tests
  - [ ] Test with small ROM (<256KB)
  - [ ] Test with large ROM (>2MB)
  - [ ] Test with different ROM types
  - [ ] Test with partially played ROM
  - [ ] Test with fully played ROM

#### Manual Testing
- [ ] **Task 1.17:** Test with real ROMs
  - [ ] Super Mario World
  - [ ] The Legend of Zelda: A Link to the Past
  - [ ] Super Metroid
  - [ ] Homebrew ROM
  - [ ] Custom test ROM

### Documentation

- [ ] **Task 1.18:** Technical documentation
  - [ ] CDL format specification
  - [ ] Memory mapping documentation
  - [ ] API documentation
  - [ ] Architecture diagrams

- [ ] **Task 1.19:** User documentation
  - [ ] Quick start guide
  - [ ] Tutorial with screenshots
  - [ ] Workflow best practices
  - [ ] Troubleshooting guide
  - [ ] FAQ

- [ ] **Task 1.20:** Developer documentation
  - [ ] Contributing guide
  - [ ] Code style guide
  - [ ] Build instructions
  - [ ] Testing guide

---

## Phase 2: Symbol Exchange

### Format Definition
- [ ] **Task 2.1:** Analyze Mesen .mlb format
  - [ ] Document format specification
  - [ ] Create parser
  - [ ] Create writer

- [ ] **Task 2.2:** Analyze DiztinGUIsh label format
  - [ ] Document current export format
  - [ ] Identify gaps vs Mesen format
  - [ ] Design mapping strategy

- [ ] **Task 2.3:** Create converter library
  - [ ] Mesen .mlb ? DiztinGUIsh format
  - [ ] DiztinGUIsh ? Mesen .mlb format
  - [ ] Bidirectional verification tests
  - [ ] Handle format-specific features

### Mesen2 Integration
- [ ] **Task 2.4:** Add label export
  - [ ] Filter labels by system (SNES)
  - [ ] Export UI
  - [ ] File format options
  - [ ] Batch export

- [ ] **Task 2.5:** Add label import
  - [ ] Import UI
  - [ ] Conflict resolution (merge vs replace)
  - [ ] Preview before import
  - [ ] Import validation

### DiztinGUIsh Integration
- [ ] **Task 2.6:** Add .mlb export
  - [ ] Convert internal labels
  - [ ] Handle address types
  - [ ] Export UI
  - [ ] Validation

- [ ] **Task 2.7:** Add .mlb import
  - [ ] Parse .mlb files
  - [ ] Map to internal format
  - [ ] Import UI
  - [ ] Conflict resolution

### Testing
- [ ] **Task 2.8:** Round-trip testing
  - [ ] Export from Mesen ? import to DiztinGUIsh ? export ? compare
  - [ ] Export from DiztinGUIsh ? import to Mesen ? export ? compare
  - [ ] Verify data integrity
  - [ ] Test edge cases

---

## Phase 3: Enhanced Trace Logs

### Mesen2 - Trace Enhancement
- [ ] **Task 3.1:** Extend trace log format
  - [ ] Add M flag to trace entries
  - [ ] Add X flag to trace entries
  - [ ] Add Data Bank to trace entries
  - [ ] Add Direct Page to trace entries
  - [ ] Add effective addresses

- [ ] **Task 3.2:** Optimize trace logging
  - [ ] Reduce overhead
  - [ ] Binary format option
  - [ ] Compression option
  - [ ] Circular buffer for live traces

- [ ] **Task 3.3:** Trace export UI
  - [ ] Format selection
  - [ ] Filter options
  - [ ] Size limits
  - [ ] Export progress

### DiztinGUIsh - Trace Analysis
- [ ] **Task 3.4:** Trace parser
  - [ ] Support text format
  - [ ] Support binary format
  - [ ] Validate trace data
  - [ ] Handle large files

- [ ] **Task 3.5:** Context extraction
  - [ ] Analyze M/X flag changes
  - [ ] Track DB/DP values
  - [ ] Build execution graph
  - [ ] Identify code paths

- [ ] **Task 3.6:** Auto-apply context
  - [ ] Set M/X flags per instruction
  - [ ] Set DB/DP values
  - [ ] Mark high-confidence areas
  - [ ] Flag uncertain areas

- [ ] **Task 3.7:** Import UI
  - [ ] File selection
  - [ ] Analysis options
  - [ ] Progress indicator
  - [ ] Results summary

### Testing
- [ ] **Task 3.8:** Trace analysis validation
  - [ ] Compare auto-detected vs known correct values
  - [ ] Measure accuracy improvement
  - [ ] Test with complex ROMs
  - [ ] Edge case handling

---

## Future Enhancements

### Optional Features
- [ ] Live debugging bridge (IPC between tools)
- [ ] Plugin architecture for extensibility
- [ ] NES support (extend beyond SNES)
- [ ] GB/GBC support
- [ ] Web-based viewer for CDL/labels
- [ ] Collaborative disassembly features
- [ ] Cloud storage for projects

### Community Features
- [ ] Database of common ROM labels
- [ ] Share CDL data for popular ROMs
- [ ] Community-contributed enhancements
- [ ] Integration with GitHub for version control

---

## Infrastructure Tasks

### CI/CD
- [ ] Set up automated builds
- [ ] Set up automated tests
- [ ] Code coverage reporting
- [ ] Nightly builds
- [ ] Release automation

### Project Management
- [ ] Create GitHub project board
- [ ] Create milestones
- [ ] Create issue templates
- [ ] Set up discussions
- [ ] Create wiki pages

### Community
- [ ] Create Discord server or channel
- [ ] Set up forums/discussions
- [ ] Create contributor guidelines
- [ ] Create code of conduct
- [ ] Set up Q&A

---

## Notes

### Dependencies to Track
- Mesen2: .NET 8, Avalonia 11.3, SDL2
- DiztinGUIsh: (TBD - need to verify)
- Common: C#, Windows/Linux/macOS

### Known Challenges
1. SNES memory mapping is complex (many formats)
2. CDL coverage depends on gameplay (may not cover all code)
3. Large ROMs may create large CDL files
4. Need to handle ROM hacks and modifications
5. Version compatibility between tools

### Success Criteria
- Phase 1: 95%+ CDL accuracy, <5s export/import for 4MB ROM
- Phase 2: 100% label preservation in round-trip
- Phase 3: 30%+ improvement in auto-context accuracy

---

**Last Updated:** 2025  
**Next Review:** After Phase 1 completion

