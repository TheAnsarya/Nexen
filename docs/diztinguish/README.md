# Mesen2 ? DiztinGUIsh Integration Plan

**Status:** Planning Phase  
**Target Systems:** SNES (primary), potentially NES (future)  
**Integration Approaches:** Multiple options evaluated  

---

## Executive Summary

This document outlines the technical plan for integrating Mesen2 (multi-system emulator with debugger) and DiztinGUIsh (SNES disassembler). The goal is to leverage the strengths of both projects to create superior debugging and disassembly workflows for retro game development.

---

## Integration Options Analysis

### Option 1: CDL (Code/Data Logger) Exchange ? **RECOMMENDED**

**Description:** Mesen2 generates Code/Data Log files during emulation; DiztinGUIsh imports them to mark code vs data.

**Benefits:**
- ? Minimal coupling between projects
- ? Clear separation of concerns
- ? Both projects remain independent
- ? CDL is a proven concept (used by other tools)
- ? Relatively simple to implement

**Implementation Steps:**
1. Add CDL export to Mesen2 SNES debugger
2. Add CDL import to DiztinGUIsh
3. Define standard CDL format (or use existing format)
4. Map emulator memory to ROM addresses

**Estimated Effort:** Medium (2-4 weeks)

**Priority:** HIGH - Best ROI

---

### Option 2: Symbol/Label Bidirectional Sync

**Description:** Share symbol and label databases between both tools.

**Benefits:**
- ? Enhanced debugging with meaningful names
- ? Better disassembly output
- ? Supports iterative workflow (disassemble ? debug ? refine)

**Challenges:**
- ?? Need to agree on format (or implement converters)
- ?? Address space mapping complexity
- ?? Keeping symbols in sync

**Implementation Steps:**
1. Analyze existing Mesen2 label formats (.mlb)
2. Analyze DiztinGUIsh label export
3. Create bidirectional converter
4. Add import/export UI in both tools

**Estimated Effort:** Medium (3-5 weeks)

**Priority:** MEDIUM - High value, moderate complexity

---

### Option 3: Live Debugging Bridge

**Description:** Run both tools simultaneously with real-time synchronization.

**Benefits:**
- ? Real-time disassembly updates
- ? Best-in-class debugging experience
- ? Leverages strengths of both tools

**Challenges:**
- ?? Complex IPC mechanism required
- ?? State synchronization complexity
- ?? Performance considerations
- ?? UI/UX coordination challenges

**Implementation Steps:**
1. Design IPC protocol (named pipes, TCP, or shared memory)
2. Define state synchronization messages
3. Implement listener in DiztinGUIsh
4. Implement broadcaster in Mesen2
5. Create UI for launching/connecting tools

**Estimated Effort:** High (6-10 weeks)

**Priority:** LOW - High complexity, uncertain ROI

**Note:** A more detailed streaming integration approach is documented in [streaming-integration.md](./streaming-integration.md), which leverages Mesen2's existing socket infrastructure for real-time bidirectional communication.

---

### Option 4: Library Integration

**Description:** Integrate DiztinGUIsh core (Diz.Core, Diz.Cpu.65816) as library into Mesen2.

**Benefits:**
- ? Best disassembly accuracy in Mesen2
- ? No external tool dependency
- ? Single unified UI

**Challenges:**
- ?? Tight coupling
- ?? Need to package DiztinGUIsh as library
- ?? Potential licensing concerns
- ?? Maintenance burden
- ?? May duplicate existing Mesen2 functionality

**Implementation Steps:**
1. Package Diz.Core and Diz.Cpu.65816 as NuGet
2. Add NuGet reference to Mesen2 UI project
3. Create new SNES disassembly view using Diz.Core
4. Integrate with existing debugger

**Estimated Effort:** High (8-12 weeks)

**Priority:** LOW - High effort, questionable value

---

### Option 5: Trace Log Enhancement

**Description:** Enhanced trace logs from Mesen2 for DiztinGUIsh analysis.

**Benefits:**
- ? Helps DiztinGUIsh determine M/X flags automatically
- ? Discovers execution paths
- ? Reduces manual work

**Challenges:**
- ?? Large trace files
- ?? Processing time
- ?? May not cover all code paths

**Implementation Steps:**
1. Enhance Mesen2 trace logger with context info (M/X flags, DB/DP registers)
2. Add trace import to DiztinGUIsh
3. Create analysis algorithm to extract context

**Estimated Effort:** Medium-High (4-6 weeks)

**Priority:** MEDIUM - Good value for automated analysis

---

## Recommended Implementation Phases

### Phase 1: CDL Integration (Foundation) ??

**Goal:** Enable basic code/data marking flow

**Mesen2 Changes:**
- [ ] Implement CDL tracking in SNES core
- [ ] Add CDL export to debugger UI
- [ ] Support standard CDL format
- [ ] Document CDL format

**DiztinGUIsh Changes:**
- [ ] Add CDL import functionality
- [ ] Map CDL data to ROM addresses
- [ ] Auto-mark code/data based on CDL
- [ ] Update UI to show CDL coverage

**Deliverables:**
1. CDL export from Mesen2 debugger
2. CDL import into DiztinGUIsh
3. Documentation and tutorial
4. Sample workflow

**Success Criteria:**
- Run ROM in Mesen2
- Export CDL after gameplay
- Import into DiztinGUIsh
- See code/data automatically marked
- Verify accuracy with manual inspection

---

### Phase 2: Symbol Exchange (Enhanced Workflow)

**Goal:** Share debugging symbols between tools

**Mesen2 Changes:**
- [ ] Analyze current .mlb format
- [ ] Add export for SNES labels
- [ ] Add import from DiztinGUIsh format
- [ ] UI for import/export

**DiztinGUIsh Changes:**
- [ ] Add export to Mesen .mlb format
- [ ] Add import from Mesen .mlb format
- [ ] UI for import/export
- [ ] Documentation

**Deliverables:**
1. Bidirectional label converter
2. Import/export UI in both tools
3. Workflow documentation
4. Sample project demonstrating round-trip

**Success Criteria:**
- Create labels in DiztinGUIsh
- Export to Mesen2 format
- Import into Mesen2
- Verify labels show in debugger
- Test reverse flow

---

### Phase 3: Enhanced Trace Logs (Advanced Analysis)

**Goal:** Use execution traces to improve disassembly accuracy

**Mesen2 Changes:**
- [ ] Add M/X flag state to trace output
- [ ] Add DB/DP register values to trace
- [ ] Add memory read/write addresses
- [ ] Enhanced trace export format
- [ ] Performance optimization

**DiztinGUIsh Changes:**
- [ ] Trace log parser
- [ ] Context extraction algorithm
- [ ] Auto-apply discovered context
- [ ] Confidence indicators
- [ ] UI for trace import/review

**Deliverables:**
1. Enhanced trace logger in Mesen2
2. Trace analysis in DiztinGUIsh
3. Workflow guide
4. Comparison showing improved accuracy

**Success Criteria:**
- Record trace in Mesen2
- Import into DiztinGUIsh
- See M/X flags auto-detected
- See DB/DP values discovered
- Improved disassembly accuracy

---

## Technical Specifications

### CDL Format Specification (Phase 1)

**File Format:** Binary or Text (TBD)

**Per-Byte Flags:**
```
Bit 0: Code (executed as instruction)
Bit 1: Data (read as data)
Bit 2: PCM Audio (read by audio hardware)
Bit 3: Graphics (read for graphics)
Bit 4: (Reserved)
Bit 5: (Reserved)
Bit 6: (Reserved)
Bit 7: (Reserved)
```

**Address Mapping:**
```
ROM Address = [Emulator Address] mapped through memory map
Handle banks, mirrors, and special regions
```

**File Structure:**
```
Header:
  - Magic number: "MESEN2CDL" or similar
  - Version: 1
  - System: SNES
  - ROM size: [size in bytes]
  - Checksum: [ROM checksum]

Body:
  - One byte per ROM address
  - Byte value = flags (as defined above)
```

---

### Label Exchange Format (Phase 2)

**Option A: Use Existing .mlb Format**
- Leverage Mesen2's existing format
- DiztinGUIsh adds .mlb export/import

**Option B: Create Common JSON Format**
```json
{
  "version": "1.0",
  "system": "SNES",
  "labels": [
    {
      "address": "$808000",
      "name": "InitRoutine",
      "type": "code",
      "comment": "System initialization"
    },
    {
      "address": "$80FFFE",
      "name": "NMI_Vector",
      "type": "data",
      "dataType": "pointer"
    }
  ]
}
```

**Recommendation:** Start with .mlb support, add JSON as enhancement

---

### Enhanced Trace Format (Phase 3)

**Text Format (Human Readable + Parseable):**
```
[Address] [Opcode] [Mnemonic] [M] [X] [DB] [DP] [Flags]
$008000   LDA      #$00       1   1   $80  $0000  nzIHCc
$008002   STA      $4200      1   1   $80  $0000  NzIHCc
```

**Binary Format (Compact):**
```
struct TraceEntry {
  uint32_t address;      // Program counter
  uint8_t  opcode;       // Instruction opcode
  uint8_t  flags;        // CPU flags (NVMXDIZc)
  uint8_t  m_flag;       // M flag state
  uint8_t  x_flag;       // X flag state
  uint8_t  data_bank;    // Data bank register
  uint16_t direct_page;  // Direct page register
  uint32_t effective_addr; // Calculated effective address (if applicable)
};
```

---

## Implementation Roadmap

### Milestone 1: Research & Prototyping (Weeks 1-2)
- [ ] Clone both projects
- [ ] Build both projects successfully
- [ ] Study Mesen2 SNES debugger code
- [ ] Study DiztinGUIsh core code
- [ ] Create proof-of-concept CDL export
- [ ] Create proof-of-concept CDL import
- [ ] Validate approach

### Milestone 2: Phase 1 Implementation (Weeks 3-6)
- [ ] Implement CDL tracking in Mesen2 SNES core
- [ ] Implement CDL export UI
- [ ] Implement CDL import in DiztinGUIsh
- [ ] Testing and bug fixes
- [ ] Documentation
- [ ] Release Phase 1

### Milestone 3: Phase 2 Implementation (Weeks 7-11)
- [ ] Implement label export from Mesen2
- [ ] Implement label import into Mesen2
- [ ] Implement label export from DiztinGUIsh
- [ ] Implement label import into DiztinGUIsh
- [ ] Testing and bug fixes
- [ ] Documentation
- [ ] Release Phase 2

### Milestone 4: Phase 3 Implementation (Weeks 12-17)
- [ ] Enhance trace logger in Mesen2
- [ ] Implement trace parser in DiztinGUIsh
- [ ] Implement context analysis
- [ ] UI integration
- [ ] Testing and bug fixes
- [ ] Documentation
- [ ] Release Phase 3

### Milestone 5: Polish & Release (Weeks 18-20)
- [ ] User testing
- [ ] Tutorial videos
- [ ] Blog posts
- [ ] Community feedback
- [ ] Final refinements
- [ ] Public release

---

## Risk Analysis

### Technical Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Address mapping complexity | Medium | High | Create comprehensive mapping tables; extensive testing |
| CDL format incompatibility | Low | Medium | Use simple, well-documented format; version field |
| Performance issues with large ROMs | Medium | Medium | Optimize algorithms; use binary formats where needed |
| DiztinGUIsh code is complex | Medium | Medium | Start with simple integration; iterate |
| Mesen2 changes frequently | Low | High | Work on fork; submit PRs to upstream carefully |

### Project Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Scope creep | High | High | Stick to phased approach; resist feature bloat |
| Time underestimation | Medium | Medium | Add 25% buffer to estimates |
| Upstream merge conflicts | Medium | Low | Keep changes isolated; modular design |
| License incompatibility | Low | High | Verify licenses early (both GPL-compatible) |
| Community disinterest | Low | Medium | Engage community early; show prototypes |

---

## Success Metrics

### Phase 1 Success Metrics
- [ ] Can export CDL from Mesen2 for any SNES ROM
- [ ] Can import CDL into DiztinGUIsh
- [ ] 95%+ accuracy in code/data marking (manual verification)
- [ ] Process completes in < 5 seconds for 4MB ROM

### Phase 2 Success Metrics
- [ ] Can export/import labels in both directions
- [ ] 100% label preservation (no data loss)
- [ ] Labels appear correctly in both tools
- [ ] Round-trip test passes (export ? import ? export ? compare)

### Phase 3 Success Metrics
- [ ] Trace logs capture M/X flags reliably
- [ ] Context extraction improves disassembly accuracy by 30%+
- [ ] Trace processing completes in reasonable time (< 1 min for 1M instructions)
- [ ] User can disable manual M/X flag entry for traced sections

---

## Community Engagement Plan

### Pre-Release
1. Post on Mesen2 Discord/Forums
2. Post on SNES homebrew forums
3. Create GitHub issue in both repos to track integration
4. Solicit feedback on design

### During Development
1. Regular progress updates
2. Screenshots and demos
3. Beta testing with volunteers
4. Incorporate feedback

### Post-Release
1. Tutorial videos
2. Blog post writeups
3. Submit PRs to upstream projects
4. Ongoing support and refinement

---

## Alternative Use Cases

Beyond SNES, this integration approach could extend to:

1. **NES Integration**
   - DiztinGUIsh-style disassembly for NES
   - 6502 CPU (simpler than 65816)
   - Mesen2 already has NES support

2. **GB/GBC Integration**
   - Game Boy disassembly tools
   - LR35902 CPU
   - Apply similar CDL/trace concepts

3. **Generic Disassembly Framework**
   - Modular architecture for any system
   - Plugin-based CPU cores
   - Shared CDL/trace formats

---

## References

- [Mesen2 Documentation](../README.md)
- [DiztinGUIsh Analysis](./analysis.md)
- [Real-Time Streaming Integration](./streaming-integration.md)
- [CDL Format Research](./cdl-format.md) (TBD)
- [SNES Memory Mapping](./snes-memory.md) (TBD)
- [65816 CPU Reference](./65816-reference.md) (TBD)

---

## Appendices

### Appendix A: Existing CDL Tools
- FCEUX (NES emulator) - Has CDL support
- Mesen (NES) - Supports CDL import/export
- Other tools to research

### Appendix B: SNES ROM Formats
- LoROM
- HiROM
- ExHiROM
- SA-1
- SuperFX
- (Document memory mapping for each)

### Appendix C: Build Environment Setup
- Mesen2 build instructions
- DiztinGUIsh build instructions
- Development environment recommendations

