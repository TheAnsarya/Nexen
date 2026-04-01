# DiztinGUIsh Project Analysis

**Project:** DiztinGUIsh - SNES Disassembler  
**Repository:** https://github.com/IsoFrieze/DiztinGUIsh  
**Local Path:** C:\Users\me\source\repos\DiztinGUIsh\  
**Analysis Date:** 2025

---

## Overview

DiztinGUIsh is a specialized disassembler for SNES (Super Nintendo Entertainment System) ROMs. It focuses on producing accurate, human-readable disassemblies that can be reassembled with the asar assembler.

### Key Philosophy

Unlike automated disassemblers, DiztinGUIsh takes a semi-manual approach that:
- Assists with tedious disassembly tasks
- Pauses when accuracy cannot be guaranteed
- Requires human input for ambiguous situations
- Produces clean, reassemblable output

---

## Technical Challenges Addressed

### 65C816 Instruction Set Complexity

The SNES uses the 65C816 processor, which has variable-length instructions:

**Problem:** Same byte sequence can disassemble differently based on CPU flags:
```
Bytes: C9 00 F0 48

If M=0 (16-bit accumulator):
  CMP.W #$F000 : PHA

If M=1 (8-bit accumulator):
  CMP.B #$00 : BEQ +72
```

**DiztinGUIsh Solution:**
- Tracks M (accumulator size) and X (index register size) flags
- Maintains flag state across execution flow
- Prevents "desynchronization" during disassembly

### Code vs Data Differentiation

**Problem:** ROMs contain both code and data intermixed
- Code must be disassembled as instructions
- Data should be formatted as data directives
- No automatic way to distinguish them

**DiztinGUIsh Solution:**
- Manual marking of data types (code, data, graphics, pointers, etc.)
- Auto-stepping that follows execution flow
- Stops at uncertain points requiring human decision

### Effective Address Calculation

**Problem:** Many instructions need context to determine full addresses:
```assembly
STA.B $03    ; Needs Direct Page register value
LDA.L $038CDA,X  ; Direct address (easier)
```

**DiztinGUIsh Solution:**
- Tracks Data Bank (DB) register
- Tracks Direct Page (DP) register
- Calculates effective addresses during disassembly

---

## Project Structure

```
DiztinGUIsh/
??? Diz.Core/                    # Core disassembly logic
??? Diz.Core.Interfaces/         # Core interfaces
??? Diz.Cpu.65816/              # 65C816 CPU implementation
??? Diz.Controllers/            # Application controllers
??? Diz.Import/                 # ROM import functionality
??? Diz.LogWriter/              # Assembly output generation
??? Diz.Ui.Views/               # UI view definitions
??? Diz.Ui.Winforms/           # Windows Forms UI (legacy?)
??? Diz.Ui.Eto/                # Eto.Forms UI (cross-platform)
??? Diz.App.Winforms/          # WinForms application
??? Diz.App.Eto/               # Eto application
??? Diz.App.PowerShell/        # PowerShell cmdlets
??? Diz.App.Common/            # Common application code
??? Diz.Test/                  # Unit tests
```

---

## Key Features

### Implemented Features

1. **Manual and Auto Stepping**
   - Step through code instruction by instruction
   - Auto-step following execution flow
   - Stop at uncertain points

2. **Branch/Call Following**
   - Step into branches and subroutine calls
   - Track execution paths

3. **Navigation**
   - Goto effective address
   - Goto first/nearby unreached data
   - Quick navigation shortcuts

4. **Data Type Marking**
   - Plain data
   - Graphics data
   - Pointers
   - Text
   - Custom types

5. **Context Tracking**
   - M & X flag tracking
   - Data Bank register tracking
   - Direct Page register tracking

6. **Output Generation**
   - Customizable assembly output
   - asar assembler compatible
   - Clean, readable formatting

### Planned Features

1. **Additional CPU Support**
   - SPC700 (SNES audio CPU)
   - SuperFX (SNES enhancement chip)

2. **Project Management**
   - Merge multiple project files
   - Collaborative disassembly

3. **Enhanced Features**
   - Better labeling for non-ROM addresses
   - Programmable data viewer for graphics
   - Relocatable code support ("base" per instruction)
   - External binary file generation for large data blocks

4. **Automation**
   - Scripting engine & API
   - Automated pattern detection

---

## Technology Stack

The project appears to use multiple UI frameworks:

### Potential Frameworks
- **.NET** (C#)
- **Windows Forms** (Diz.Ui.Winforms, Diz.App.Winforms)
- **Eto.Forms** (Diz.Ui.Eto, Diz.App.Eto) - Cross-platform UI
- **PowerShell** integration (Diz.App.PowerShell)

### Architecture Pattern
- **MVC/MVVM-style** separation
- Controllers (Diz.Controllers)
- Views (Diz.Ui.Views)
- Core logic (Diz.Core, Diz.Cpu.65816)
- Interfaces for extensibility (Diz.Core.Interfaces)

---

## Integration Points with Mesen2

### Potential Connection Scenarios

#### 1. **Code Data Logger (CDL) Integration**
**Concept:** Use Mesen2's SNES emulation to generate CDL files for DiztinGUIsh

- Mesen2 can track which bytes are executed as code vs read as data
- DiztinGUIsh can use this information for accurate disassembly
- Reduces manual marking of code/data boundaries

**Implementation Path:**
- Export CDL data from Mesen2 debugger
- Import CDL data into DiztinGUIsh
- Map memory addresses between emulator and ROM

#### 2. **Symbol/Label Sharing**
**Concept:** Bidirectional exchange of symbols and labels

- DiztinGUIsh generates labels during disassembly
- Export labels to Mesen2 for enhanced debugging
- Import Mesen2's discovered labels into DiztinGUIsh

**Implementation Path:**
- Define common label format (or use existing .mlb format)
- Export mechanism in DiztinGUIsh
- Import mechanism in Mesen2 debugger

#### 3. **Live Debugging Integration**
**Concept:** Use DiztinGUIsh as external disassembly view for Mesen2

- Launch DiztinGUIsh from Mesen2
- Pass ROM and memory state
- Synchronize execution position
- Real-time disassembly updates

**Implementation Path:**
- IPC mechanism (named pipes, TCP/IP, shared memory)
- Protocol for state synchronization
- Plugin/extension architecture in Mesen2

#### 4. **Trace Log Analysis**
**Concept:** Use Mesen2 trace logs to improve DiztinGUIsh accuracy

- Mesen2 generates execution traces
- DiztinGUIsh analyzes traces for:
  - M/X flag states at each instruction
  - DB/DP register values
  - Actual execution paths

**Implementation Path:**
- Enhanced trace log export from Mesen2
- Trace log parser in DiztinGUIsh
- Automatic context reconstruction

#### 5. **Embedded Disassembly View**
**Concept:** Embed DiztinGUIsh core as library in Mesen2

- Use Diz.Core and Diz.Cpu.65816 as libraries
- Build enhanced SNES disassembly view in Mesen2
- Leverage DiztinGUIsh's accuracy for in-emulator disassembly

**Implementation Path:**
- Package DiztinGUIsh core as NuGet package
- Integrate into Mesen2 UI project
- Create new debugger view using Diz.Core

---

## Next Steps for Integration Research

1. **Examine DiztinGUIsh codebase in detail**
   - Identify public APIs
   - Understand data formats
   - Map core functionality

2. **Examine Mesen2 debugger architecture**
   - Current symbol import/export
   - Existing SNES debugger features
   - Extension points

3. **Define integration scope**
   - Choose most valuable integration scenario
   - Define technical requirements
   - Create implementation plan

4. **Prototype**
   - Build proof-of-concept
   - Test with sample ROMs
   - Validate approach

---

## References

- [DiztinGUIsh GitHub](https://github.com/IsoFrieze/DiztinGUIsh)
- [Mesen2 Documentation](./README.md)
- [Integration Planning](./diztinguish/README.md)

