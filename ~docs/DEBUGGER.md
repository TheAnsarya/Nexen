# Debugger Subsystem Documentation

This document covers Nexen's comprehensive debugging subsystem, which provides powerful tools for reverse engineering and development across all supported platforms.

---

## Architecture Overview

```text
Debugger Architecture
═════════════════════

┌─────────────────────────────────────────────────────────────────┐
│                        Debugger                               │
│   (Central coordinator for all debugging features)              │
└─────────────────────────────────────────────────────────────────┘
              │
    ┌─────────┴─────────┬────────────────┬────────────────┐
    │                  │                │               │
    ▼                  ▼                ▼               ▼
┌──────────┐   ┌──────────────┐   ┌───────────┐   ┌────────────┐
│ IDebugger│   │ Disassembler │   │ScriptMgr  │   │  CDL/Label │
│ per-CPU  │   │ multi-CPU  │   │ Lua API   │   │  Managers  │
└──────────┘   └──────────────┘   └───────────┘   └────────────┘
    │
    ├── BreakpointManager
    ├── CallstackManager
    ├── ExpressionEvaluator
    ├── TraceLogger
    ├── StepBackManager
    └── EventManager
```

## Directory Structure

```text
Core/Debugger/
├── Core Components
│   ├── Debugger.h/cpp            - Main debugger class
│   ├── IDebugger.h              - Per-CPU debugger interface
│   ├── DebugTypes.h                - Type definitions
│   └── DebugUtilities.h            - Helper utilities
│
├── Breakpoints
│   ├── Breakpoint.h/cpp            - Breakpoint definition
│   ├── BreakpointManager.h/cpp  - Breakpoint management
│   └── FrozenAddressManager.h    - Memory freeze support
│
├── Disassembly
│   ├── Disassembler.h/cpp        - Multi-CPU disassembler
│   ├── DisassemblyInfo.h/cpp      - Disassembly metadata
│   ├── DisassemblySearch.h/cpp  - Search functionality
│   ├── Base6502Assembler.h/cpp  - 6502/65816 assembler
│   └── IAssembler.h                - Assembler interface
│
├── Expression Evaluation
│   ├── ExpressionEvaluator.h/cpp   - Core evaluator
│   ├── ExpressionEvaluator.Nes.cpp - NES registers
│   ├── ExpressionEvaluator.Snes.cpp - SNES registers
│   ├── ExpressionEvaluator.Spc.cpp - SPC700 registers
│   ├── ExpressionEvaluator.Gameboy.cpp - GB registers
│   ├── ExpressionEvaluator.Gba.cpp - GBA/ARM registers
│   ├── ExpressionEvaluator.Sms.cpp - SMS/Z80 registers
│   ├── ExpressionEvaluator.Pce.cpp - PC Engine registers
│   ├── ExpressionEvaluator.Ws.cpp  - WonderSwan registers
│   ├── ExpressionEvaluator.Gsu.cpp - Super FX registers
│   ├── ExpressionEvaluator.Cx4.cpp - Cx4 registers
│   └── ExpressionEvaluator.*.cpp   - Other CPUs
│
├── Tracing & Logging
│   ├── BaseTraceLogger.h          - Trace logger base
│   ├── ITraceLogger.h            - Trace logger interface
│   ├── TraceLogFileSaver.h      - File output
│   └── CallstackManager.h/cpp    - Call stack tracking
│
├── Code/Data Logging
│   ├── CodeDataLogger.h/cpp        - CDL core
│   ├── CdlManager.h/cpp            - CDL file management
│   └── AddressInfo.h              - Address mapping
│
├── Memory Tools
│   ├── MemoryDumper.h/cpp        - Memory access
│   ├── MemoryAccessCounter.h/cpp   - Access statistics
│   └── Profiler.h/cpp            - Performance profiling
│
├── PPU Tools
│   ├── PpuTools.h/cpp            - Graphics debugging
│   └── BaseEventManager.h/cpp    - Event logging
│
├── Scripting
│   ├── ScriptManager.h/cpp      - Script management
│   ├── ScriptHost.h/cpp            - Script execution
│   ├── ScriptingContext.h/cpp    - Script context
│   ├── LuaApi.h/cpp                - Lua bindings
│   └── LuaCallHelper.h/cpp      - Lua utilities
│
└── Step Execution
    ├── StepBackManager.h/cpp      - Rewind debugging
    └── DebugBreakHelper.h        - Break helpers
```

## Genesis Tooling Surfaces

The Genesis debugger runtime now exposes deterministic bridge tooling surfaces through debug memory read/write paths in `GenesisMemoryManager`.

### Bridge Windows

- `$a12000-$a1201f` Sega CD bridge window (debug read/write supported)
- `$a13000-$a1301f` Sega CD bridge lane 2 window (debug read/write supported)
- `$a14000-$a1401f` Sega CD bridge lane 3 window (debug read/write supported)
- `$a15000-$a1501f` 32X bridge window (debug read/write supported)
- `$a16000-$a1601f` Sega CD bridge lane 5 window (debug read/write supported)
- `$a18000-$a1801f` Sega CD bridge lane 6 window (debug read/write supported)

### Deterministic Status Bytes

- `$a12016` Sega CD tooling event-count byte (low byte)
- `$a12017` Sega CD tooling event-count byte (high byte)
- `$a12018` Debug-lane telemetry count byte (low byte)
- `$a12019` Debug-lane telemetry digest byte (low byte)
- `$a15018` 32X tooling event-count byte (low byte)
- `$a15019` 32X tooling event-count byte (high byte)
- `$a1201a` Sega CD tooling capability byte
- `$a1201b` Sega CD tooling digest byte
- `$a1201c` Control port 1 deterministic capability byte
- `$a1201d` Control port 1 deterministic digest byte
- `$a1201e` Control port 2 deterministic capability byte
- `$a1201f` Control port 2 deterministic digest byte

These status bytes are intended for parity and regression checks in debugger-facing validation workflows and are replay-stable across save/load when the same write/read sequence is applied.

### IO and Handshake Windows

The debug memory path also surfaces Genesis IO and Z80 handshake windows with deterministic behavior for parity validation:

- `$a10000-$a1001f` IO window (version/data/control/ext registers)
- `$a11100-$a11101` Z80 bus-request handshake window
- `$a11200-$a11201` Z80 reset handshake window

### Z80 and VDP Debug Windows

Additional debugger parity windows are now exposed through Genesis debug memory access paths:

- `$a00000-$a0ffff` Z80 memory window (ownership-aware debug parity)
- `$c00000-$c0001f` VDP/PSG window (null-safe debug path when VDP backend is unavailable)

Debug writes to controller control registers (`$a10009/$a1000b/$a1000d`) and handshake control bytes are persisted through save/load and can be replay-validated in runtime transcript tests.

Debug-path bridge and handshake access now contributes transcript lane entries through the same deterministic tracking contracts used by runtime traffic, enabling regression-gate checks on debug-driven event sequences.

Genesis runtime metadata also exposes a dedicated debug transcript lane (separate from the runtime lane) for debugger-driven bridge/handshake traffic, so acceptance gates can validate debug parity evidence independently.

The dedicated debug transcript lane also captures debugger-driven IO (`$a100xx`), Z80 window (`$a000xx`), and VDP window (`$c000xx`) access pathways, providing a broader regression-gate evidence surface for debug memory activity.

Debugger tooling can query dedicated lane metadata directly through `GenesisMemoryManager::GetDebugTranscriptLaneCount()` and `GenesisMemoryManager::GetDebugTranscriptLaneDigest()`, and can explicitly reset lane evidence between phases with `GenesisMemoryManager::ClearDebugTranscriptLane()`.

Compatibility matrix checkpoints now include `GEN-COMPAT-DEBUG-LANE`, which enforces deterministic replay parity for debug-lane command/response evidence in scaffold-based regression gates.

Compatibility matrix checkpoints also include `GEN-COMPAT-32X-TELEMETRY`, which validates deterministic replay parity for 32X tooling event telemetry bytes (`$a15018/$a15019`) in scaffold-based regression gates.

Compatibility matrix checkpoints also include `GEN-COMPAT-SCD-TELEMETRY`, which validates deterministic replay parity for Sega CD tooling event telemetry bytes (`$a12016/$a12017`) in scaffold-based regression gates.

Performance-gate result lines (`GEN_PERF_RESULT`) now include deterministic telemetry evidence markers (`SCD_LANE_CT` and `M32X_EVT_CT`) so gate summaries can be correlated with bridge/tooling activity surfaces.

For scaffold-based gate workflows, `GenesisPlatformBusStub::ClearCommandResponseLane()` is the deterministic phase-reset primitive for command/response evidence and related tooling/control parity staging state.

---

## Core Components

### Debugger (Debugger.h)

Central coordinator managing all debugging features.

**Key Responsibilities:**

- Coordinates per-CPU debuggers
- Manages script execution
- Controls execution flow (step, run, pause)
- Provides unified memory access

**CPU Debugger Management:**

```cpp
// CpuInfo holds per-CPU debugging state
struct CpuInfo {
    unique_ptr<IDebugger> Debugger;     // CPU-specific debugger
    unique_ptr<ExpressionEvaluator> Evaluator; // Expression evaluation
};

// Array indexed by CpuType enum
CpuInfo _debuggers[(int)DebugUtilities::GetLastCpuType() + 1];
```

**Shared Components:**

- `ScriptManager` - Lua scripting
- `MemoryDumper` - Memory read/write
- `MemoryAccessCounter` - Access tracking
- `CodeDataLogger` - CDL tracking
- `Disassembler` - Multi-CPU disassembly
- `LabelManager` - Symbol management
- `CdlManager` - CDL file I/O

### IDebugger Interface (IDebugger.h)

Per-CPU debugger interface implemented by each CPU.

**Implementations:**

- `NesDebugger` - 6502 (NES)
- `SnesDebugger` - 65816 (SNES main CPU)
- `SpcDebugger` - SPC700 (SNES audio)
- `GameboyDebugger` - LR35902 (Game Boy)
- `GbaDebugger` - ARM7TDMI (GBA)
- `SmsDebugger` - Z80 (SMS/Game Gear)
- `PceDebugger` - HuC6280 (PC Engine)
- `WsDebugger` - V30MZ (WonderSwan)
- `GsuDebugger` - Super FX
- `Cx4Debugger` - Cx4 coprocessor

---

## Breakpoints

### BreakpointManager (BreakpointManager.h)

High-performance breakpoint evaluation.

**Design Goals:**

- Minimal overhead when no breakpoints
- Fast path for common cases
- Support complex conditions

**Breakpoint Types:**

```cpp
enum class BreakpointType {
    Global,  // Always trigger
    Execute,    // On instruction execution
    Read,      // On memory read
    Write      // On memory write
};
```

**Organization:**

```cpp
// Breakpoints grouped by operation type for fast lookup
vector<Breakpoint> _breakpoints[BreakpointTypeCount];
// Quick check if any breakpoints of type exist
bool _hasBreakpointType[BreakpointTypeCount];
```

**Evaluation Flow:**

1. **Fast path check:** `if(!_hasBreakpoint) return -1;`
2. **Type filter:** Check `_hasBreakpointType[opType]`
3. **Address match:** Linear search breakpoints
4. **Condition eval:** RPN expression if present
5. **Return:** Breakpoint ID or -1

### Breakpoint Conditions

Conditions use the ExpressionEvaluator:

```cpp
// Example conditions
"A == $50"         // Register equals value
"[addr] & $80"     // Memory bit test
"PpuScanline < 100"  // PPU state check
"Value == $FF"     // Access value check
```

---

## Expression Evaluator

### ExpressionEvaluator (ExpressionEvaluator.h)

Evaluates expressions for breakpoint conditions and watch windows.

**Operator Categories:**

- **Arithmetic:** `+ - * / %`
- **Bitwise:** `<< >> & | ^ ~`
- **Logical:** `== != < <= > >= && ||`
- **Memory:** `[addr]` (byte), `{addr}` (word), `{addr}w` (dword)
- **Unary:** `+ - ~ !`

**Platform-Specific Registers:**

| Platform | Registers |
| ---------- | ----------- |
| **6502/NES** | A, X, Y, SP, PC, PS, P.Carry, P.Zero, etc. |
| **65816/SNES** | + DB, PB, DP, K, M, X flags |
| **SPC700** | A, X, Y, SP, PC, YA, PSW |
| **Z80/SMS** | A, B, C, D, E, F, H, L, AF, BC, DE, HL, IX, IY, I, R |
| **LR35902/GB** | A, B, C, D, E, F, H, L, AF, BC, DE, HL, SP, PC |
| **ARM7/GBA** | R0-R15, CPSR, SPSR |
| **HuC6280/PCE** | A, X, Y, SP, PC, PS, MPR0-7 |
| **V30MZ/WS** | AX, BX, CX, DX, SI, DI, BP, SP, CS, DS, ES, SS, IP |

**Special Values:**

```cpp
// PPU state
PpuFrameCount, PpuCycle, PpuScanline, PpuVramAddress

// Interrupts
Nmi, Irq

// Memory operation context
IsRead, IsWrite, IsDma, IsDummy
Value   // Value being read/written
Address  // Address being accessed

// Platform-specific
Sprite0Hit, VerticalBlank  // NES
```

**RPN Compilation:**
Expressions are compiled to Reverse Polish Notation for fast evaluation:

```cpp
// "A == $50" compiles to:
// [RegA] [0x50] [Equal]
vector<int64_t> _rpnList;  // Pre-compiled tokens
```

---

## Code/Data Logging (CDL)

### CodeDataLogger (CodeDataLogger.h)

Tracks which ROM bytes are code vs data.

**CDL Flags:**

```cpp
enum CdlFlags : uint8_t {
    None          = 0x00,
    Code          = 0x01,  // Executed as instruction
    Data          = 0x02,  // Read as data
    JumpTarget  = 0x04,  // Jump/branch destination
    SubEntryPoint = 0x08,  // Subroutine entry
    IndexData8  = 0x10,  // 8-bit indexed access
    IndexData16   = 0x20,  // 16-bit indexed access
    PcmData    = 0x40,  // Audio sample data
    DrawingTile   = 0x80   // Graphics tile data
};
```

**File Format:**

```text
CDL File Format (CDLv2)
════════════════════════
Offset  Size  Content
0x00    5    "CDLv2" magic
0x05    4    ROM CRC32
0x09    N    Flag bytes (1 per ROM byte)
```

**Use Cases:**

- ROM hacking: Find unused space
- Disassembly: Improve code detection
- Coverage: Verify code paths executed
- Size optimization: Strip unused data

### CdlManager (CdlManager.h)

Manages CDL file operations and multi-memory CDL tracking.

---

## Disassembly

### Disassembler (Disassembler.h)

Multi-CPU disassembly engine.

**Supported CPUs:**

- 6502 (NES)
- 65816 (SNES)
- SPC700 (SNES audio)
- LR35902 (Game Boy)
- ARM7TDMI (GBA, ARM + Thumb)
- Z80 (SMS, Game Gear)
- HuC6280 (PC Engine)
- V30MZ (WonderSwan)
- Super FX
- Cx4, NEC DSP, ST018

**Features:**

- Label integration
- Comment support
- CDL-guided code/data detection
- Address mapping (CPU ↔ ROM)

### DisassemblyInfo (DisassemblyInfo.h)

Metadata for disassembled instructions.

```cpp
struct DisassemblyInfo {
    uint8_t  OpSize;       // Instruction size in bytes
    uint8_t  Flags;     // Code/data flags
    uint8_t  ByteCode[8];  // Raw bytes
    uint32_t Address;     // CPU address
    AddressInfo AbsAddress; // Absolute/ROM address
};
```

### Base6502Assembler (Base6502Assembler.h)

Interactive assembler for 6502/65816 code.

**Features:**

- Real-time assembly in debugger
- Label resolution
- Address mode detection
- Immediate error feedback

---

## Tracing & Logging

### TraceLogger (BaseTraceLogger.h)

Instruction-level execution logging.

**Log Format:**

```text
PC   Op  Operand   A  X  Y  SP   Flags   Cycle
$8000  LDA $0300,X  A:00 X:10 Y:00 SP:FD N-BdIZC  1234
```

**Options:**

- CPU registers
- Memory values
- PPU state
- Timestamps
- Custom formatting

### CallstackManager (CallstackManager.h)

Tracks subroutine calls and returns.

**Features:**

- JSR/RTS tracking
- Interrupt context
- Stack depth monitoring
- Call graph building

---

## Memory Tools

### MemoryDumper (MemoryDumper.h)

Unified memory access interface.

**Operations:**

- Read/write any memory type
- Address translation
- Memory type enumeration
- Bulk operations

### MemoryAccessCounter (MemoryAccessCounter.h)

Tracks memory access patterns.

**Statistics:**

- Read count per address
- Write count per address
- Execute count per address
- First/last access timing

### Profiler (Profiler.h)

Performance profiling per function/address.

**Metrics:**

- Cycle counts
- Call frequency
- Inclusive/exclusive time
- Hotspot detection

---

## Scripting

### ScriptManager (ScriptManager.h)

Lua script management and execution.

**Script Types:**

- Startup scripts
- Per-frame scripts
- Event-triggered scripts

### LuaApi (LuaApi.h)

Comprehensive Lua bindings.

**API Categories:**

```lua
-- Memory access
emu.read(addr, memType)
emu.write(addr, value, memType)
emu.readWord(addr, memType)

-- Debugger control
emu.breakExecution()
emu.resume()
emu.step(count)

-- Event callbacks
emu.addEventCallback(callback, eventType)
emu.removeEventCallback(id)

-- State management
emu.saveSavestate()
emu.loadSavestate(slot)

-- Drawing (overlays)
emu.drawPixel(x, y, color)
emu.drawLine(x1, y1, x2, y2, color)
emu.drawString(x, y, text, color)

-- Input
emu.getInput(port)
emu.setInput(port, state)
```

---

## Step Execution

### StepBackManager (StepBackManager.h)

Rewind/step-back debugging.

**Features:**

- Instruction-level rewind
- Savestate ring buffer
- Configurable history depth

**Implementation:**

```cpp
// Ring buffer of savestates for rewind
vector<vector<uint8_t>> _history;
int _historyIndex = 0;
int _historySize = 1000;  // Configurable
```

---

## PPU Tools

### PpuTools (PpuTools.h)

Graphics debugging utilities.

**Features:**

- Tile viewer
- Sprite viewer
- Palette viewer
- Tilemap viewer
- VRAM viewer
- OAM viewer

### BaseEventManager (BaseEventManager.h)

Debug event logging for analysis.

**Event Types:**

- Memory reads/writes
- Register access
- Interrupt triggers
- DMA transfers
- PPU events

---

## Usage Examples

### Setting a Breakpoint

```cpp
// Create execute breakpoint at $8000
Breakpoint bp;
bp.SetType(BreakpointType::Execute);
bp.SetStartAddress(0x8000);
bp.SetEnabled(true);

// With condition
bp.SetCondition("A == $FF && X > $10");

breakpointManager->SetBreakpoints(&bp, 1);
```

### Expression Evaluation

```cpp
// Evaluate expression in current CPU context
EvalResultType resultType;
int64_t result = evaluator->Evaluate(
    "A + X * 2",
    cpuState,
    resultType,
    operationInfo
);
```

### Lua Script Example

```lua
-- Log when specific address is written
function onWrite()
    local value = emu.read(0x0300, emu.memType.nesInternalRam)
    emu.log("Address $0300 = " .. string.format("%02X", value))
end

emu.addEventCallback(onWrite, emu.eventType.write)
```

---

## Related Documentation

- [ARCHITECTURE-OVERVIEW.md](ARCHITECTURE-OVERVIEW.md) - Overall structure
- [NES-CORE.md](NES-CORE.md) - NES debugger specifics
- [SNES-CORE.md](SNES-CORE.md) - SNES debugger specifics
- [GB-GBA-CORE.md](GB-GBA-CORE.md) - GB/GBA debugger specifics
- [SMS-PCE-WS-CORE.md](SMS-PCE-WS-CORE.md) - Other platform debuggers
