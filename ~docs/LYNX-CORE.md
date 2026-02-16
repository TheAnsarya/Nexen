# Atari Lynx Emulation Core Documentation

The Atari Lynx is a handheld game console (1989) built around a WDC 65C02 CPU and two custom ASICs: **Mikey** (timers, video, audio, I/O) and **Suzy** (sprite engine, math coprocessor, input). Uniquely, the Lynx has no traditional PPU — Suzy renders sprites directly into work RAM and Mikey outputs scanlines from a framebuffer.

**Key specifications:**
- **CPU:** WDC 65C02 @ 4 MHz (no NMI line)
- **RAM:** 64 KB flat address space
- **Display:** 160 × 102, 4 bpp (16 of 4096 colors), ~75 Hz
- **Audio:** 4 LFSR channels, stereo
- **Cartridge:** Dual-bank sequential ROM with optional EEPROM saves
- **Input:** D-pad + A/B + Option 1/2 + Pause, hardware rotation support

## Architecture Overview

```text
╔══════════════════════════════════════════════════════════════════════╗
║                        Atari Lynx Architecture                      ║
╠══════════════════════════════════════════════════════════════════════╣
║                                                                      ║
║  ┌────────────┐     ┌──────────────────────────────────────────┐    ║
║  │  65C02 CPU │◄───►│         Memory Manager (64 KB)           │    ║
║  │   4 MHz    │     │  MAPCTL overlay: RAM/Suzy/Mikey/ROM/Vec  │    ║
║  └────────────┘     └──────────┬───────────────┬───────────────┘    ║
║        │                       │               │                     ║
║        ▼                       ▼               ▼                     ║
║  ┌────────────────────┐  ┌──────────────────────────────┐           ║
║  │    Mikey ($FD00)    │  │       Suzy ($FC00)           │           ║
║  │                     │  │                              │           ║
║  │  • 8 Timers         │  │  • Sprite Engine (SCB chain) │           ║
║  │  • Video output     │  │  • Hardware Multiply/Divide  │           ║
║  │  • 4 Audio channels │  │  • Collision Detection       │           ║
║  │  • IRQ controller   │  │  • Joystick/Switches         │           ║
║  │  • Palette regs     │  │  • EEPROM interface          │           ║
║  │  • UART (ComLynx)   │  │                              │           ║
║  └────────────────────┘  └──────────────────────────────┘           ║
║        │                       │                                     ║
║        ▼                       ▼                                     ║
║  ┌────────────┐        ┌────────────┐                               ║
║  │ Framebuffer │        │ Cartridge  │                               ║
║  │  (in RAM)   │        │ Dual-bank  │                               ║
║  └────────────┘        │ + EEPROM   │                               ║
║                         └────────────┘                               ║
╚══════════════════════════════════════════════════════════════════════╝
```

## Directory Structure

```text
Core/Lynx/
├── LynxConsole.h/cpp             - Main coordinator, frame loop, ROM loading
├── LynxCpu.h/cpp                 - WDC 65C02 CPU core (256 opcodes)
├── LynxMikey.h/cpp               - Mikey ASIC: timers, video, audio, IRQ
├── LynxApu.h/cpp                 - 4 LFSR audio channels (inside Mikey)
├── LynxSuzy.h/cpp                - Suzy ASIC: sprite engine, math, input
├── LynxMemoryManager.h/cpp       - 64 KB address space with MAPCTL overlays
├── LynxCart.h/cpp                 - Dual-bank cartridge with LNX header
├── LynxEeprom.h/cpp              - 93C46/56/66/76/86 serial EEPROM
├── LynxControlManager.h/cpp      - Input management
├── LynxController.h              - Controller definition (9 buttons)
├── LynxDefaultVideoFilter.h/cpp  - Screen rotation (None/Left/Right)
├── LynxGameDatabase.h/cpp        - CRC32 game database (~80 titles)
├── LynxTypes.h                   - All type definitions and constants
└── Debugger/
    ├── LynxDebugger.h/cpp        - Debugger integration
    ├── LynxDisUtils.h/cpp        - 65C02 disassembler
    ├── LynxAssembler.h/cpp       - 65C02 inline assembler
    ├── LynxTraceLogger.h/cpp     - CPU trace logging
    ├── LynxEventManager.h/cpp    - Event viewer (IRQ, DMA, sprites)
    ├── LynxPpuTools.h/cpp        - Palette/sprite debug viewers
    └── DummyLynxCpu.h/cpp        - Side-effect-free CPU for disassembly
```

## Core Components

### LynxConsole (LynxConsole.h)

**Responsibilities:**
- Owns all Lynx hardware components and orchestrates the frame loop
- Loads ROMs (LNX format with 64-byte header, or headerless)
- CRC32-based game database lookup for auto-detection of rotation, EEPROM type, and title
- State serialization/deserialization for save states
- Power-on initialization (boot ROM vector fetch from $FFFC)

```cpp
struct LynxCartInfo {
    LynxRotation Rotation;
    uint32_t Bank0PageSize;
    uint32_t Bank1PageSize;
    LynxEepromType EepromType;
    // ...
};
```

**Key Methods:**
```cpp
void LoadRom(VirtualFile& romFile);       // Parse LNX header, load ROM
void RunFrame();                          // Execute one full frame
void Serialize(Serializer& s);            // Save state support
LynxSuzy* GetSuzy();                     // Component access
LynxMikey* GetMikey();
uint8_t* GetWorkRam();
```

### LynxCpu (LynxCpu.h)

The Lynx uses a **WDC 65C02** (not the MOS 6502). Key differences from NMOS 6502:
- `(zp)` indirect addressing mode (no index)
- New instructions: `STZ`, `TRB`, `TSB`, `BRA`, `PHX/PLX`, `PHY/PLY`, `INC A`, `DEC A`
- `WAI` (Wait for Interrupt) and `STP` (Stop)
- BRK and IRQ clear the Decimal flag
- ADC/SBC in decimal mode set Z/N flags from the BCD result (not binary)
- Fixed JMP (ind) page-crossing bug from NMOS
- Undefined opcodes are NOPs (not illegal instructions)

**CPU State:**
```cpp
struct LynxCpuState {
    uint8_t A, X, Y;          // Registers
    uint8_t SP;                // Stack pointer ($0100-$01FF)
    uint16_t PC;               // Program counter
    uint8_t PS;                // Processor status (NV-BDIZC)
    uint64_t CycleCount;
    LynxCpuStopState StopState; // Running / WaitingForIrq / Stopped
};
```

**Opcode Table:**
- 256-entry function pointer table `_opTable[256]`
- Addressing mode table `_addrMode[256]`
- All undefined opcodes map to NOP

**Hardware Bugs Emulated:**
- **HW Bug 13.1 (WAI/STP recovery):** On real hardware, WAI/STP can only recover if Suzy holds the bus. Emulated as unconditional halt.
- **IRQ Break flag:** IRQ pushes PS with Break=0, Reserved=1. The original code had an operator precedence bug that was fixed.

### LynxMikey (LynxMikey.h)

**Mikey** is the larger of the two custom ASICs, handling timing, video output, audio, and I/O.

**Key Features:**
- 8 hardware timers with cascading (linked chains)
- Scanline-based video output from framebuffer in RAM
- 4 LFSR audio channels with stereo attenuation
- IRQ controller (one source per timer)
- UART for ComLynx serial communication
- Palette registers (16 entries × 12 bits = 4096 color space)

#### Timer System

8 programmable timers with cascading support:

| Timer | Primary Use | Link Chain |
|-------|-----------|------------|
| T0 | Audio ch0 / General | T0 → T2 → T4 |
| T1 | Audio ch1 / General | T1 → T3 → T5 → T7 |
| T2 | Audio ch2 / General | (see T0) |
| T3 | Audio ch3 / General | (see T1) |
| T4 | General | (see T0) |
| T5 | General | (see T1) |
| T6 | General (standalone) | None |
| T7 | Video HSync | (see T1) |

Clock sources per timer: System clock prescalers (1 MHz, 500 kHz, 250 kHz, ... 15.625 kHz) or linked to previous timer's borrow output.

#### Video Rendering

- **Resolution:** 160 × 102 pixels
- **Color depth:** 4 bits per pixel (16 colors per scanline)
- **Palette:** 16 entries, each 12-bit RGB (4 bits per channel, 4096 total colors)
- **Scanlines:** 105 total (102 visible + 3 VBlank)
- **Frame rate:** ~75 Hz (master clock / cycles-per-frame)
- **Framebuffer:** Stored in work RAM, address set via `DISPADR` register ($FD94)

The video system reads 80 bytes per scanline (160 pixels × 4 bpp / 8) from RAM and maps through the palette to produce output colors.

#### Audio Channels (LynxApu)

4 LFSR-based audio channels with configurable feedback polynomial:

```cpp
struct LynxApuChannelState {
    uint8_t Volume;           // 7-bit signed volume
    uint8_t FeedbackEnable;   // LFSR tap mask
    uint8_t ShiftRegister;    // LFSR state
    uint8_t AudControl;       // Integration mode, clock source
    uint8_t Counter;          // Timer countdown
    uint8_t BackupValue;      // Timer reload value
    int16_t Output;           // Current output sample
};
```

**Features:**
- Per-channel stereo attenuation registers (`ATTEN_A` through `ATTEN_D`)
- Channel 3 supports 8-bit DAC mode for PCM playback
- LFSR feedback polynomial is user-configurable via `FeedbackEnable` register
- Integration mode accumulates output values

### LynxSuzy (LynxSuzy.h)

**Suzy** handles sprite rendering, hardware math, joystick input, and EEPROM access.

#### Sprite Engine

Sprites are defined by **Sprite Control Blocks (SCBs)** stored in RAM, linked in a chain:

```text
SCB Layout (fixed 27-byte format in emulator):
Offset  Field        Size    Description
0x00    NEXT         2       Pointer to next SCB (chain terminates when high byte = 0)
0x02    SPRCTL0      1       Sprite type, BPP, flip
0x03    SPRCTL1      1       Reload flags, start/draw quadrant
0x04    DATA         2       Packed pixel data address
0x06    HPOS         2       Horizontal position (signed)
0x08    VPOS         2       Vertical position (signed)
0x0A    HSIZE        2       Horizontal size (8.8 fixed point)
0x0C    VSIZE        2       Vertical size (8.8 fixed point)
0x0E    STRETCH      2       Horizontal stretch per scanline
0x10    TILT         2       Horizontal tilt per scanline
0x12    PALETTE[8]   8       Pen index remap table
0x1A    COLLNUM      1       Collision number
```

**SPRCTL0 fields:**
- Bits 7:6 — BPP (1, 2, 3, or 4 bits per pixel)
- Bit 5 — Horizontal flip
- Bit 4 — Vertical flip
- Bits 2:0 — Sprite type (background, normal, boundary, shadow, XOR shadow, non-collidable)

**SPRCTL1 reload flags (bits 4-7):**
When a reload flag is cleared, the corresponding SCB fields are NOT loaded from memory and instead persist from the previous sprite. This is an optimization for sprite chains sharing common properties.

**Sprite types:**
1. Background draw — non-zero pixels replace
2. Background no-collide — same but no collision
3. Boundary — writes to collision buffer boundary
4. Normal — standard sprite with collision
5. Non-collidable — draws but doesn't participate in collision
6. XOR shadow — XOR blend mode
7. Shadow — darkening effect

#### Hardware Math

```cpp
// 16×16→32 Multiply with Accumulate
// Write operands to MATHD/MATHC (multiplier) and MATHB/MATHA (multiplicand)
// Results appear in MATHH:MATHG:MATHF:MATHE (32-bit product)
// Accumulate adds to existing result if SPRSYS bit set

// 32÷16 Divide
// Writes dividend to MATHH:MATHG:MATHF:MATHE, divisor to MATHD:MATHC
// Quotient in MATHD:MATHC, remainder in MATHB:MATHA
```

### LynxMemoryManager (LynxMemoryManager.h)

The Lynx uses a flat 64 KB address space with configurable overlays controlled by `MAPCTL` ($FFF9):

```text
╔═════════════════════════════════════════════╗
║          Lynx Memory Map                     ║
╠═════════════════════════════════════════════╣
║  $0000-$FBFF  Work RAM (always)              ║
║  $FC00-$FCFF  Suzy registers / RAM (MAPCTL) ║
║  $FD00-$FDFF  Mikey registers / RAM (MAPCTL)║
║  $FE00-$FFF7  Boot ROM / RAM (MAPCTL)        ║
║  $FFF8        MAPCTL / RAM (MAPCTL)          ║
║  $FFF9        Boot ROM enable                ║
║  $FFFA-$FFFF  Vectors / RAM (MAPCTL)         ║
╚═════════════════════════════════════════════╝
```

**MAPCTL register ($FFF9) bits:**
| Bit | Function |
|-----|----------|
| 0 | Suzy space enable ($FC00-$FCFF) |
| 1 | Mikey space enable ($FD00-$FDFF) |
| 2 | Boot ROM enable ($FE00-$FFF7) |
| 3 | Vector space enable ($FFFA-$FFFF) |

When a space is disabled, reads/writes go to underlying RAM.

### LynxCart (LynxCart.h)

**Dual-bank cartridge system:**
- Bank 0 and Bank 1 have independent page counters and page sizes
- Sequential access: CPU reads the RCART register to get the next byte and advance the counter
- Page sizes: 256, 512, 1024, 2048 bytes (set in LNX header)
- Games switch banks by writing the IODAT register

**LNX Header Format (64 bytes):**
```text
Offset  Size  Field
0x00    4     Magic ("LYNX")
0x02    2     Bank 0 page count
0x04    2     Bank 1 page count
0x06    2     Version
0x08    32    Game title
0x28    2     Manufacturer code
0x2A    1     Rotation (0=None, 1=Left, 2=Right)
0x2B    5     Reserved
0x30    2     Bank 0 page size (power of 2)
0x32    2     Bank 1 page size
0x34    12    Reserved
```

### LynxEeprom (LynxEeprom.h)

Microwire serial EEPROM for persistent game saves:

| Type | Size | Address Bits | Notes |
|------|------|-------------|-------|
| 93C46 | 128 bytes | 6 | Most common |
| 93C56 | 256 bytes | 8 | |
| 93C66 | 512 bytes | 8 | |
| 93C76 | 1024 bytes | 10 | |
| 93C86 | 2048 bytes | 10 | |

**Protocol:** Serial interface via Suzy I/O pins (Clock, Data In, Data Out, Chip Select). Commands: READ, WRITE, ERASE, EWEN (erase/write enable), EWDS (erase/write disable), ERAL (erase all), WRAL (write all).

### LynxGameDatabase (LynxGameDatabase.h)

Static CRC32-keyed database of ~80 commercial Lynx titles plus homebrew entries:

```cpp
struct Entry {
    uint32_t PrgCrc32;
    const char* Name;
    LynxRotation Rotation;
    LynxEepromType EepromType;
    uint8_t PlayerCount;
};
```

Used for:
- Auto-detecting screen rotation for headerless ROMs
- Identifying EEPROM type for save game support
- Displaying game title in the UI

## Timing

| Parameter | Value |
|-----------|-------|
| Master clock | 16 MHz |
| CPU clock | 4 MHz (master / 4) |
| Display resolution | 160 × 102 |
| Visible scanlines | 102 |
| Total scanlines | 105 (102 visible + 3 VBlank) |
| Frame rate | ~75 Hz |
| Cycles per scanline | ~508 |
| Cycles per frame | ~53,333 |

## Integration Points

### With Shared Infrastructure

- **Emulator base class:** `LynxConsole` extends `IConsole`, integrates with Nexen's unified emulator framework
- **Memory access:** `LynxMemoryManager` implements the standard `MemoryRead`/`MemoryWrite` interface for debugger integration
- **Input system:** `LynxControlManager` / `LynxController` map to the shared input configuration
- **Video pipeline:** `LynxDefaultVideoFilter` transforms the framebuffer for display, handling rotation
- **Save states:** All components implement `Serialize()` for the shared save state system
- **Pansy export:** Lynx is supported in the Pansy metadata export pipeline

### With Emulator

- **ROM loading:** Supports `.lnx` (with header) and headerless ROM files
- **Configuration:** `LynxConfig` exposes Model and Rotation settings in the UI
- **TAS support:** Full movie recording/playback with BK2 format (9-char mnemonic: `UDLRABOoP`)
- **Debugger:** Complete 65C02 debug tooling (breakpoints, trace, disassembly, assembly, memory viewer)

## Debugging Features

| Feature | Implementation |
|---------|---------------|
| CPU debugger | `LynxDebugger` — breakpoints, step, register inspection |
| Disassembler | `LynxDisUtils` — all 65C02 addressing modes |
| Assembler | `LynxAssembler` — inline 65C02 assembly |
| Trace logger | `LynxTraceLogger` — per-instruction CPU state logging |
| Event viewer | `LynxEventManager` — IRQ, DMA, sprite events on timeline |
| Palette viewer | `LynxPpuTools` — 16-entry × 12-bit palette display |
| Sprite viewer | `LynxPpuTools` — SCB chain walker with preview rendering |
| Register viewer | Mikey and Suzy register panels in debugger |
| Dummy CPU | `DummyLynxCpu` — predictive disassembly without side effects |

## Related Documentation

- [ARCHITECTURE-OVERVIEW.md](ARCHITECTURE-OVERVIEW.md) - Overall Nexen emulator architecture
- [NES-CORE.md](NES-CORE.md) - NES emulation (6502 CPU variant reference)
- [SMS-PCE-WS-CORE.md](SMS-PCE-WS-CORE.md) - Other handheld/console cores
- [MOVIE-TAS.md](MOVIE-TAS.md) - TAS recording and playback system
- [INPUT-SUBSYSTEM.md](INPUT-SUBSYSTEM.md) - Controller and input handling
- [VIDEO-RENDERING.md](VIDEO-RENDERING.md) - Video output pipeline
- [AUDIO-SUBSYSTEM.md](AUDIO-SUBSYSTEM.md) - Audio mixing and output
- [DEBUGGER.md](DEBUGGER.md) - Debugging infrastructure
