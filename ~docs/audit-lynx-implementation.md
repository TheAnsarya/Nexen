# Atari Lynx Implementation — Comprehensive Audit Report

> **Scope:** All Lynx-related code in the Nexen project (Mesen2 fork)
> **Date:** 2026-02-16
> **Files audited:** ~70 files across `Core/Lynx/`, `Core.Tests/Lynx/`, `Core.Benchmarks/Lynx/`, `UI/Interop/`, `MovieConverter.Tests/`

---

## Table of Contents

1. [File Inventory](#1-file-inventory)
2. [CPU (LynxCpu)](#2-cpu-lynxcpu)
3. [Display / PPU (Mikey Scanline Rendering)](#3-display--ppu-mikey-scanline-rendering)
4. [Audio (LynxApu)](#4-audio-lynxapu)
5. [Memory Manager](#5-memory-manager)
6. [Sprite Engine & Math (Suzy)](#6-sprite-engine--math-suzy)
7. [Timers & IRQ (Mikey)](#7-timers--irq-mikey)
8. [UART / ComLynx (Mikey)](#8-uart--comlynx-mikey)
9. [Input (Controller)](#9-input-controller)
10. [Cartridge & EEPROM](#10-cartridge--eeprom)
11. [Debugger Suite](#11-debugger-suite)
12. [Save States (Serialization)](#12-save-states-serialization)
13. [Tests & Benchmarks](#13-tests--benchmarks)
14. [UI Integration](#14-ui-integration)
15. [Configuration](#15-configuration)
16. [TODOs, Stubs & Missing Features](#16-todos-stubs--missing-features)
17. [Hardware Bugs Emulated](#17-hardware-bugs-emulated)
18. [Comparison with Other Console Implementations](#18-comparison-with-other-console-implementations)
19. [Overall Assessment](#19-overall-assessment)

---

## 1. File Inventory

### Core Emulation (`Core/Lynx/` — 20 files)

| File | Lines | Purpose |
|------|-------|---------|
| `LynxTypes.h` | 772 | Central types, constants, enums, all state structs |
| `LynxConsole.h/.cpp` | ~600 | Console orchestration, ROM loading, HLE boot, frame loop |
| `LynxCpu.h/.cpp` | ~1111 | WDC 65C02 CPU, full opcode table, addressing modes |
| `LynxMikey.h/.cpp` | ~1200 | Mikey chip: 8 timers, display DMA, IRQ, UART, palette |
| `LynxSuzy.h/.cpp` | ~1500 | Suzy chip: sprites, math coprocessor, collision, input |
| `LynxApu.h/.cpp` | ~400 | 4-channel LFSR audio, stereo attenuation |
| `LynxMemoryManager.h/.cpp` | ~350 | MAPCTL address decoding, overlay logic |
| `LynxCart.h/.cpp` | ~200 | 2-bank page switching, LNX header metadata |
| `LynxController.h` | ~150 | 9-button input device, turbo, TAS button names |
| `LynxControlManager.h` | ~50 | Control manager integration |
| `LynxDecrypt.h/.cpp` | ~400 | RSA cart decryption, Montgomery multiplication |
| `LynxEeprom.h/.cpp` | ~300 | 93C46/56/66/76/86 Microwire serial EEPROM |
| `LynxGameDatabase.h/.cpp` | ~300 | ~80 CRC32 entries, auto-detect rotation + EEPROM type |
| `LynxDefaultVideoFilter.h/.cpp` | ~200 | Screen rotation (0°/90°L/90°R), palette adjustment |

### Debugger (`Core/Lynx/Debugger/` — 10 files)

| File | Purpose |
|------|---------|
| `LynxDebugger.h/.cpp` | Full IDebugger: breakpoints, callstack, CDL, step back, predictive BPs |
| `LynxTraceLogger.h` | Per-instruction state trace (PC, A, X, Y, SP, PS, CYC, SL) |
| `LynxEventManager.h/.cpp` | Event viewer with categories (IRQ, Mikey, Suzy, Audio, Palette, Timer R/W) |
| `LynxPpuTools.h/.cpp` | Palette viewer, tilemap viewer (framebuffer), sprite viewer (SCB walk) |
| `LynxDisUtils.h/.cpp` | Disassembly with 65C02 extensions |
| `LynxAssembler.h/.cpp` | 65C02 assembler (extends Base6502Assembler) |
| `DummyLynxCpu.h/.cpp` | Side-effect-free CPU for breakpoint evaluation |

### Tests (`Core.Tests/Lynx/` — 17 files)

| File | Area |
|------|------|
| `LynxCpuTests.cpp` | CPU types, state, flags |
| `LynxCpuInstructionTests.cpp` | All 65C02 instructions |
| `LynxTimerTests.cpp` | Timer registers, cascading, prescalers |
| `LynxSuzyTests.cpp` | Sprite engine, math, registers |
| `LynxMemoryTests.cpp` | MAPCTL, overlay logic, address mapping |
| `LynxUartTests.cpp` | UART TX/RX, loopback, break, errors |
| `LynxApuTests.cpp` | Audio channels, LFSR, stereo |
| `LynxAssemblerTests.cpp` | 65C02 assembler |
| `LynxCartTests.cpp` | Cart access, bank switching |
| `LynxControllerTests.cpp` | Input encoding, button combos |
| `LynxDebuggerTests.cpp` | Breakpoints, callstack, CDL |
| `LynxEventManagerTests.cpp` | Event tracking |
| `LynxEepromTests.cpp` | Microwire protocol, R/W/erase/eral |
| `LynxDecryptTests.cpp` | RSA encryption/decryption |
| `LynxGameDatabaseTests.cpp` | CRC lookup, rotation, EEPROM type |
| `LynxHardwareReferenceTests.cpp` | Hardware spec compliance |
| `LynxHardwareBugsTests.cpp` | Documented hardware bug reproduction |

### Benchmarks (`Core.Benchmarks/Lynx/` — 11 files)

`LynxCpuBench`, `LynxMemoryBench`, `LynxSuzyBench`, `LynxMikeyBench`, `LynxApuBench`, `LynxCartBench`, `LynxEepromBench`, `LynxDecryptBench`, `LynxUartBench`, `LynxGameDatabaseBench`, `LynxAllocBench`

### MovieConverter Tests

| File | Purpose |
|------|---------|
| `MovieConverter.Tests/LynxTasTests.cs` | BK2 round-trip, button encoding, all 32 combos, frame rate |

---

## 2. CPU (LynxCpu)

**Status: Complete and thorough**

- **Architecture:** WDC 65C02 at 4 MHz (16 MHz master / 4)
- **Full 6502 base instruction set** plus all 65C02 extensions:
	- `BRA` (branch always)
	- `PHX`/`PLX`, `PHY`/`PLY` (push/pull X/Y)
	- `STZ` (store zero)
	- `TRB`/`TSB` (test and reset/set bits)
	- `WAI`/`STP` (wait for interrupt / stop processor)
	- `BIT` immediate mode
	- `INC A` / `DEC A` (accumulator increment/decrement)
	- `(zp)` zero-page indirect addressing mode (no index)
- **Decimal mode:** Correct 65C02 behavior — extra cycle in BCD mode, correct Z/N flags (unlike NMOS 6502)
- **BRK/IRQ:** Clears Decimal flag (65C02 fix over 6502)
- **Undefined opcodes:** Multi-byte NOP handling including WDC-specific `$5C` (8 cycles), `$DC`/`$FC` (4 cycles)
- **Stop states:** `WaitingForIrq` (WAI), `Stopped` (STP) — properly handled in frame loop
- **Serialization:** All registers, cycle count, stop state, IRQ pending

**No issues found.**

---

## 3. Display / PPU (Mikey Scanline Rendering)

**Status: Complete**

- **Resolution:** 160×102 at 4bpp (80 bytes/scanline)
- **Frame rate:** ~75.0 FPS, 105 scanlines (102 visible + 3 VBlank)
- **Palette:** 16 colors from 4096 (RGB444), green separate from blue/red packed
- **Display DMA:** Reads from configurable DISPADR base address
- **Scanline rendering:** Triggered by Timer 0 underflow, processes 80 bytes → 160 pixels
- **Video filter:** `LynxDefaultVideoFilter` handles screen rotation (0°/90°L/90°R), brightness/contrast/hue/saturation, 4096-entry adjusted palette cache
- **Auto-rotation:** Game database provides per-game rotation hints

**Display address wrapping** uses `& 0xFFFF` mask, safe because workRam is exactly 64KB.

**No issues found.**

---

## 4. Audio (LynxApu)

**Status: Complete**

- **4 LFSR-based channels** at 22050 Hz sample rate
- Per-channel stereo attenuation via ATTEN registers
- Prescaler-based timing with channel cascading
- Integration mode (channel 3)
- DAC mode on channel 3
- Full serialization of all channel state
- Config exposes per-channel volume controls (Ch1–Ch4)

**No issues found.**

---

## 5. Memory Manager

**Status: Complete and well-structured**

- **64KB flat address space** with MAPCTL (`$FFF9`) overlay control
- **4 overlay regions** (active-low bits):
	- Bit 0: Suzy space (`$FC00–$FCFF`)
	- Bit 1: Mikey space (`$FD00–$FDFF`)
	- Bit 2: Boot ROM (`$FE00–$FFF7`)
	- Bit 3: Vector space (`$FFFA–$FFFF`)
- **5 memory types** for debugger: `LynxMemory`, `LynxWorkRam`, `LynxPrgRom`, `LynxBootRom`, `LynxSaveRam`
- **DebugRead/DebugWrite:** Side-effect-free access, including proper Peek variants for Suzy/Mikey
- **GetAbsoluteAddress/GetRelativeAddress:** Correct mapping for debugger address translation
- **MAPCTL write** does NOT store to RAM (hardware register only) — correct behavior
- **Vector/ROM space writes** correctly ignored when overlays active
- **DebugWrite** allows writing to RAM under overlay regions (for debugger convenience)

**No issues found.**

---

## 6. Sprite Engine & Math (Suzy)

**Status: Very thorough, closely matches Handy reference**

### Sprite Engine

- **SCB chain walking** with HW Bug 13.12 (upper byte zero terminates chain)
- **4-quadrant rendering** with configurable start quadrant
- **Variable-length SCB fields** (reload depth 0–3 for HPOS, VPOS, HSIZE, VSIZE, STRETCH, TILT)
- **Pixel formats:** 1/2/3/4 bpp, packed (RLE) and literal modes
- **Pen index remapping** table (16 entries)
- **Superclipping:** Off-screen sprite origins only render visible quadrants
- **Quad offset fix:** Prevents squashed look on multi-quad sprites (matches Handy)
- **8 sprite types:** BackgroundShadow, BackgroundNonCollide, BoundaryShadow, Boundary, Normal, NonCollidable, XorShadow, Shadow — all with distinct pixel/collision rules
- **Collision detection:** RAM-based collision buffer (same layout as video), per-sprite max tracking, collision depositary write, EVERON tracking
- **Bus cycle stalling:** `_spriteBusCycles` tracks sprite RAM accesses

### Math Coprocessor

- **16×16→32 unsigned multiply** with sign tracking
- **32÷16 divide** with accumulate mode
- **HW Bug 13.8:** `$8000` treated as positive, `$0000` as negative in signed math
- **HW Bug 13.10:** MathOverflow flag overwrite behavior
- **Cascading register clears:** Writing MATHD clears C, writing MATHB clears A, etc. (Stun Runner fix)
- **Write triggers:** MATHA triggers multiply, MATHE triggers divide
- **MATHP write** clears N (matching Handy)

### Input

- Joystick register and Switches register readable via Suzy
- Cart access (RCART0/RCART1) with auto-increment, PeekData for debugger

**No issues found.**

---

## 7. Timers & IRQ (Mikey)

**Status: Complete**

- **8 hardware timers** with individual prescaler periods and enable bits
- **Cascading:** Timer link targets with clock source 7 = linked mode
- **HW Bug 13.6:** TimerDone flag stops counting (except Timer 4)
- **Timer 4:** UART baud generator — special treatment (no TimerDone, calls TickUart, always counts)
- **Timer 0:** Horizontal timer — triggers scanline rendering
- **Timer 2:** Vertical timer — scanline counter
- **IRQ system:** Per-timer interrupt enable in CTLA bit 7, INTSET/INTRST registers, no separate enable mask
- **CTLA bit 6:** Self-clearing reset strobe (do not store)
- **CTLB write:** Only clears timer-done flag, preserves read-only status bits
- **CPUSLEEP (`$FD91`):** Sets CPU stop state to WaitingForIrq

**No issues found.**

---

## 8. UART / ComLynx (Mikey)

**Status: Complete with well-documented reference to hardware manual**

- **11-bit framing** (1 start, 8 data, parity/mark, 1 stop)
- **Circular RX queue** (32 entries) with input/output pointers
- **Self-loopback** per §7.2: TX output front-inserts to RX queue
- **Inter-byte delay:** 55 Timer 4 ticks between queued bytes
- **HW Bug §9.1:** Level-sensitive IRQ — re-asserts continuously while condition holds
- **SERCTL write (§2):** TX/RX interrupt enable, parity config, RESETERR, TXBRK
- **SERCTL read (§3):** Status register with TXRDY, RXRDY, TXEMPTY, OVERRUN, FRAMERR, RXBRK, PARBIT
- **SERDAT write (§4):** Starts transmission with 9th bit support
- **SERDAT read (§4):** Returns data byte, clears RXRDY
- **Break signal (§6.2):** Auto-repeats every 11 ticks via UartBreakCode
- **Overrun detection (§7.3):** Sets flag when new byte delivered before previous read

**Parity calculation is unimplemented** (per §9 — "Leave at zero!!") — this is intentionally not emulated, matching real-world usage.

**No issues found.**

---

## 9. Input (Controller)

**Status: Complete**

- **9 buttons:** Up, Down, Left, Right, A, B, Option1, Option2, Pause
- **Active-low encoding** (bits cleared when pressed)
- **TAS button names:** `"UDLRabOoP"` — compact representation for movie files
- **Turbo support** via Mesen's standard turbo framework
- **InputHud drawing:** Custom drawing for on-screen display
- **Left-handed mode:** Suzy SPRSYS bit 3 (LeftHand) — I/O pin swap happens at hardware level

**Missing:**

- `ProcessInputOverrides()` is empty (TODO) — see §16

---

## 10. Cartridge & EEPROM

**Status: Complete**

### Cartridge (LynxCart)

- **LNX format:** 64-byte header parsing with validation (magic "LYNX", version)
- **Headerless ROM support:** Falls back to game database CRC32 lookup
- **2-bank page switching** with address counter auto-increment
- **PeekData:** Side-effect-free read for debugger

### EEPROM (LynxEeprom)

- **5 chip types:** 93C46, 93C56, 93C66, 93C76, 93C86
- **Microwire serial protocol:** Full state machine (Idle → ReceivingOpcode → ReceivingAddress → ReceivingData/SendingData)
- **Operations:** Read, Write, Erase, ERAL (erase all), WRAL (write all), EWEN/EWDS
- **Wired through Mikey I/O:** IODIR/IODAT registers (`$FD88`/`$FD89`)
- **Auto-detection:** From LNX header byte 60 or game database
- **Battery save/load:** Integration with Mesen's save system

### RSA Decryption (LynxDecrypt)

- **Montgomery multiplication** for modular exponentiation
- **Block-based processing:** 51-byte blocks
- **Public modulus + private exponent** embedded
- **Encrypt/Decrypt/Validate** functions

**No issues found.**

---

## 11. Debugger Suite

**Status: Comprehensive — matches other console debuggers in the project**

### Core Debugger (`LynxDebugger`)

- Full `IDebugger` interface implementation
- **Breakpoints:** Read/write/execute with address matching
- **Callstack:** JSR/RTS/BRK/RTI tracking, interrupt frame support
- **Code/Data Logger (CDL):** Tracks executed code vs accessed data
- **Step back:** Uses cycle-based history
- **Predictive breakpoints:** Via `DummyLynxCpu` (side-effect-free execution)
- **BreakOnBrk:** Configurable via `DebugConfig.LynxBreakOnBrk`

### Trace Logger (`LynxTraceLogger`)

- Per-instruction: PC, A, X, Y, SP, PS (flags string), CYC, SL

### Event Manager (`LynxEventManager`)

- Categories: IRQ, Mikey reads/writes, Suzy reads/writes, Audio reads/writes, Palette reads/writes, Timer reads/writes

### PPU Tools (`LynxPpuTools`)

- **Palette viewer:** 16 colors, RGB444, returns correct color + RGB channel info
- **Sprite viewer:** Walks SCB chain from Suzy state, decodes pixel data (packed + literal), renders preview thumbnails
- **Tilemap viewer:** Shows raw framebuffer (160×102)

### Disassembler (`LynxDisUtils`)

- Full 65C02 instruction decoding with all addressing modes
- Opcode classification for correct display

### Assembler (`LynxAssembler`)

- Extends `Base6502Assembler` with 65C02 extensions

### DummyCpu (`DummyLynxCpu`)

- Side-effect-free CPU reads for breakpoint evaluation during predictive analysis

**Issues:**

- Tilemap viewer shows raw framebuffer, not DMA-accurate rendering (TODO)
- No tile viewer for cart ROM sprite data (TODO)
- `GetEffectiveAddress` not fully implemented (TODO for DummyCpu approach)

---

## 12. Save States (Serialization)

**Status: Complete across all subsystems**

Every component properly implements `Serialize()`:

| Component | Serialized State |
|-----------|-----------------|
| **LynxCpu** | All registers (A, X, Y, SP, PC, PS), cycle count, stop state, IRQ pending |
| **LynxMikey** | All 8 timer states, IRQ pending/enabled, display control/address, all 16 palette entries (green + BR), UART full queue (32 entries) + all TX/RX state, I/O dir/data, hardware revision, current scanline |
| **LynxSuzy** | SCB address, sprite control registers, all math registers (ABCD/EFGH/JKLM/NP + signs), sprite rendering registers (offsets, base addresses, sizes), collision state, pen index table (16 entries), persistent SCB fields, input state |
| **LynxMemoryManager** | MAPCTL value, 4 overlay visibility flags, BootRomActive |
| **LynxApu** | All 4 channel states (volume, shift register, feedback, integration, barrel shift) |
| **LynxCart** | Bank address counters, selected bank |
| **LynxEeprom** | Serial state machine, data registers, write enable |
| **LynxConsole** | Master cycle, frame count, RAM (64KB), all subsystem Serialize calls |

**Additional details:**

- UART RX queue is fully serialized (all 32 entries + input/output pointers + waiting counter)
- Framebuffer is serialized as part of console state
- Palette is serialized at both the register level (PaletteGreen/PaletteBR arrays) and the computed ARGB level (Palette[16])

**No issues found.**

---

## 13. Tests & Benchmarks

### Unit Tests (17 test files)

| Test File | Coverage Area |
|-----------|--------------|
| `LynxCpuTests` | Initial state, flags (C/Z/I/D/B/V/N), stack pointer range, stop state values |
| `LynxCpuInstructionTests` | All 65C02 opcodes including extensions |
| `LynxHardwareBugsTests` | Bug 13.1 (WAI/STP), 13.2 (BRK/IRQ B flag), 13.3 (timer link), 13.6 (timer done), 13.8 (signed math), 13.10 (overflow), 13.12 (SCB chain), §9.1 (UART IRQ) |
| `LynxHardwareReferenceTests` | Hardware specification compliance |
| `LynxTimerTests` | Register R/W, cascading, prescaler periods |
| `LynxSuzyTests` | Sprite types, math operations, register encoding |
| `LynxMemoryTests` | MAPCTL overlay logic, address mapping |
| `LynxUartTests` | TX/RX loopback, break signals, overrun, framing |
| `LynxApuTests` | Channel state, LFSR, stereo attenuation |
| `LynxAssemblerTests` | 65C02 assembly encoding |
| `LynxCartTests` | Bank switching, data access |
| `LynxControllerTests` | Button encoding, combos |
| `LynxDebuggerTests` | Breakpoints, callstack, CDL |
| `LynxEventManagerTests` | Event tracking categories |
| `LynxEepromTests` | Microwire protocol, R/W/erase |
| `LynxDecryptTests` | RSA en/decryption validation |
| `LynxGameDatabaseTests` | CRC lookup, rotation, EEPROM detection |

### C# Tests

| Test File | Coverage Area |
|-----------|--------------|
| `LynxTasTests.cs` (410 lines) | BK2 format round-trip, all 32 button combinations, frame rate encoding |

### Benchmarks (11 files)

`LynxCpuBench`, `LynxMemoryBench`, `LynxSuzyBench`, `LynxMikeyBench`, `LynxApuBench`, `LynxCartBench`, `LynxEepromBench`, `LynxDecryptBench`, `LynxUartBench`, `LynxGameDatabaseBench`, `LynxAllocBench`

**Assessment:** Test coverage is excellent. Every major subsystem has dedicated tests, hardware bugs have explicit test cases, and TAS integration has thorough round-trip validation.

---

## 14. UI Integration

**Status: Complete — fully integrated with Mesen2 UI framework**

### Enums & Types (registered in shared headers)

- `ConsoleType::Lynx` (value 7)
- `CpuType::Lynx`
- `RomFormat::Lynx`
- `FirmwareType::LynxBootRom`
- `ControllerType::LynxController`
- `DebuggerFlags::LynxDebuggerEnabled`
- Memory types: `LynxMemory`, `LynxWorkRam`, `LynxPrgRom`, `LynxBootRom`, `LynxSaveRam`

### C# UI Interop Files

- `EmuApi.cs`: `ConsoleType.Lynx = 7`, `CpuType.Lynx`, `FirmwareType.LynxBootRom`
- `DebugApi.cs`: PPU state fetching (`LynxPpuState`), PPU tools state (`EmptyPpuToolsState`), event viewer config
- `ConfigApi.cs`: Lynx-specific config binding
- `CpuTypeExtensions.cs`: `LynxDebuggerEnabled` flag mapping
- `MemoryTypeExtensions.cs`: Full Lynx memory type mapping (CPU, ROM, WRAM, BOOT, SRAM labels)
- `FirmwareTypeExtensions.cs`: `lynxboot.img` with SHA-256 hash (`FCD403DB...`)

### Settings Integration

- `EmuSettings.cpp`: Lynx-specific configuration handling
- `Debugger.cpp`: Lynx debugger creation
- `DisassemblyInfo.cpp`: 65C02 disassembly support
- `DebugUtilities.h`: Lynx memory type utilities
- `ExpressionEvaluator.cpp`: Lynx expression evaluation support

**No issues found.**

---

## 15. Configuration

**`LynxConfig` struct (in `SettingTypes.h`):**

| Setting | Type | Purpose |
|---------|------|---------|
| `Controller` | `ControllerConfig` | Player 1 controller config |
| `UseBootRom` | `bool` | Use real boot ROM vs HLE boot |
| `AutoRotate` | `bool` | Auto-rotate screen based on game database |
| `BlendFrames` | `bool` | Frame blending |
| `DisableSprites` | `bool` | Debug: disable sprite layer |
| `DisableBackground` | `bool` | Debug: disable background layer |
| `Ch1Vol` – `Ch4Vol` | `uint32_t` | Per-channel audio volume |

**Also in `DebugConfig`:**

- `LynxBreakOnBrk` — break on BRK instruction

**Assessment:** Configuration is appropriate for the Lynx. The platform has fewer options than multi-region consoles (no overscan, no region selection, no palette variants) — this is correct, not a gap.

---

## 16. TODOs, Stubs & Missing Features

### Active TODOs

| # | Location | Description | Severity |
|---|----------|-------------|----------|
| 1 | `LynxMikey.cpp:690` | `// TODO: system control (power off, cart power, etc.)` — SYSCTL1 (`$FD87`) write handler is empty | **Low** — No known games rely on SYSCTL1 writes for core gameplay |
| 2 | `LynxMikey.cpp:426` | `return 0; // stub` — SYSCTL1 read returns 0 | **Low** — Matches expected default |
| 3 | `LynxMikey.cpp:466` | PeekRegister also stubs SYSCTL1 | **Low** — Consistent with read stub |
| 4 | `LynxPpuTools.h:21-22` | `// TODO: Tile viewer: Decode sprite pixel data from cart ROM as tiles` | **Low** — Debug tool only |
| 5 | `LynxPpuTools.cpp:97` | `// TODO: Parse actual scanline DMA control blocks` — Tilemap shows raw framebuffer | **Low** — Debug tool only |
| 6 | `LynxDisUtils.cpp:286` | `// TODO: Implement using DummyCpu approach like PceDisUtils` — `GetEffectiveAddress` incomplete | **Medium** — Affects debugger address resolution accuracy |
| 7 | `LynxDebugger.cpp:405` | `// TODO: Implement input override for Lynx controller` — `ProcessInputOverrides` empty | **Medium** — TAS/debug feature gap |

### Stubs

| Location | Register | Current Behavior |
|----------|----------|-----------------|
| `LynxMikey.cpp` | SYSCTL1 (`$FD87`) read | Returns `0` |
| `LynxMikey.cpp` | SYSCTL1 (`$FD87`) write | No-op |

### Intentionally Not Emulated

| Feature | Reason |
|---------|--------|
| Parity calculation (UART) | Per §9: "Leave at zero!!" — no real games use it |
| TXOPEN (open collector) | Per §9: Not meaningfully emulable without multi-Lynx networking |
| AUDIN (audio comparator input) | No external audio source in emulation; reads 0 |
| Multi-Lynx networking | Self-loopback is implemented; external ComLynx would need network code |

---

## 17. Hardware Bugs Emulated

All documented Atari Lynx hardware bugs from the hardware reference manual are implemented and tested:

| Bug | Description | Implementation |
|-----|-------------|----------------|
| **§13.1** | WAI/STP behavior — WAI wakes on IRQ, STP only on reset | CPU stop state machine |
| **§13.2** | BRK sets Break flag, IRQ clears it (65C02 push semantics) | `BRK()`/`IRQ()` in LynxCpu |
| **§13.3** | Timer link bits — clock source 7 means linked | Timer cascade logic |
| **§13.6** | Timer Done flag stops counting (except Timer 4) | `TickTimer()` and `CascadeTimer()` |
| **§13.8** | Signed multiply: `$8000` treated as positive, `$0000` as negative | Sign conversion in math register writes |
| **§13.10** | MathOverflow flag overwrite | Math overflow handling in multiply/divide |
| **§13.12** | SCB chain termination — upper byte 0 terminates regardless of lower | `ProcessSpriteChain()` |
| **§9.1** | UART IRQ is level-sensitive — fires continuously while condition holds | `UpdateUartIrq()` |

All bugs have corresponding test cases in `LynxHardwareBugsTests.cpp`.

---

## 18. Comparison with Other Console Implementations

### Feature Parity Matrix

| Feature | NES | SNES | GB | GBA | SMS | PCE | **Lynx** |
|---------|-----|------|----|-----|-----|-----|----------|
| CPU emulation | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| PPU/display | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Audio | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Save states | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Debugger | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Trace logger | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Event viewer | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| PPU tools | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ (partial) |
| Assembler | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Disassembler | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Callstack | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| CDL | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Input overrides | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ (TODO) |
| GetEffectiveAddr | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ⚠️ (TODO) |
| Tile viewer | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ (N/A*) |
| TAS support | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Unit tests | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅✅ (17 files) |
| Benchmarks | varies | varies | varies | varies | varies | varies | ✅✅ (11 files) |

\* The Lynx has no traditional tile/tilemap hardware — it uses a pure sprite engine. A tile viewer would need to decode cart ROM sprite data, which is a different concept.

### Where Lynx Exceeds Other Consoles

- **Test coverage:** 17 dedicated test files + 11 benchmark files is among the highest in the project
- **Hardware bug documentation:** Every emulated bug is cross-referenced to the hardware manual section
- **UART/serial emulation:** Full ComLynx with queue, loopback, break signals — many emulators skip this entirely
- **RSA decryption:** Complete Montgomery multiplication implementation for encrypted carts

### Where Lynx Has Gaps vs Other Consoles

- **Input overrides** (`ProcessInputOverrides`) is the only functional gap affecting TAS workflows
- **GetEffectiveAddress** is the only debugger accuracy gap
- **SYSCTL1** stub is the only hardware register not fully emulated (but no known games need it)

---

## 19. Overall Assessment

**The Atari Lynx implementation is remarkably thorough and production-ready.** Key strengths:

1. **Hardware accuracy:** All documented hardware bugs emulated and tested. The emulation closely follows the Handy reference implementation while adding modern Mesen2 integration.

2. **Code quality:** Extensive inline documentation referencing hardware manual sections (§ numbers). Every UART status bit, every sprite type, every math register quirk is commented with its hardware reference.

3. **Complete debugger suite:** All standard Mesen2 debugger features are present — breakpoints, callstack, CDL, trace logger, event viewer, PPU tools, assembler.

4. **Thorough serialization:** Every register of every subsystem is saved/restored, including the 32-entry UART RX queue and all 16 pen index remap entries.

5. **Excellent test coverage:** 17 C++ test files cover every subsystem, hardware bugs have dedicated test cases, and TAS integration has 410 lines of C# tests.

### Priority Fixes

| Priority | Item | Effort |
|----------|------|--------|
| **P2** | Implement `ProcessInputOverrides` for Lynx controller | Small |
| **P2** | Implement `GetEffectiveAddress` using DummyCpu approach | Medium |
| **P3** | Add SYSCTL1 register logic (power off, cart power) | Small |
| **P3** | Add tile viewer for cart ROM sprite data in PPU tools | Medium |
| **P3** | Parse DMA control blocks in tilemap viewer | Medium |

None of these are blocking issues — they are quality-of-life improvements for debugging and TAS workflows.
