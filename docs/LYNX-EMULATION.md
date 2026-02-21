# Atari Lynx Emulation in Nexen

Nexen includes a comprehensive Atari Lynx emulator targeting high accuracy and full TAS/Movie support. This document describes the hardware being emulated, the architecture of our implementation, and the current accuracy status.

## Hardware Overview

The **Atari Lynx** (1989) was the first color handheld game console. It featured:

| Component | Specification |
|-----------|---------------|
| **CPU** | WDC 65SC02 @ 4 MHz (16 MHz master / 4) |
| **Display** | 160x102 pixel LCD, 4-bit color (16 from 4096 palette) |
| **Sound** | 4-channel LFSR synthesis, stereo DAC on channel 3 |
| **RAM** | 64 KB work RAM |
| **ROM** | 512 byte boot ROM |
| **Save** | EEPROM (93C46/66/86) via serial protocol |
| **Input** | D-pad, A, B, Option 1, Option 2, Pause |
| **Link** | ComLynx UART for multiplayer (up to 16 players) |
| **Refresh** | 75 Hz (no PAL variant) |

### Custom Chips

- **Mikey** — Timers, audio, display DMA, UART, IRQ controller
- **Suzy** — Sprite engine, math coprocessor (16x16 multiply, 32/16 divide), collision detection, input

## Architecture

### Source Files

| File | Description |
|------|-------------|
| `Core/Lynx/LynxConsole.cpp` | Main console — boot, frame loop, save states |
| `Core/Lynx/LynxCpu.cpp` | 65SC02 CPU emulation with all CMOS extensions |
| `Core/Lynx/LynxMikey.cpp` | Mikey chip — timers, display, audio registers |
| `Core/Lynx/LynxSuzy.cpp` | Suzy chip — sprites, math, collision, input |
| `Core/Lynx/LynxApu.cpp` | Audio processing — 4 LFSR channels, stereo |
| `Core/Lynx/LynxMemoryManager.cpp` | Memory mapping with MAPCTL overlay system |
| `Core/Lynx/LynxCart.cpp` | Cartridge loading, bank switching, decryption |
| `Core/Lynx/LynxEeprom.cpp` | EEPROM save data (93C46/66/86 serial protocol) |
| `Core/Lynx/LynxControlManager.cpp` | Controller input — joystick + switches |
| `Core/Lynx/LynxController.cpp` | Button mapping and TAS input handling |
| `Core/Lynx/LynxTypes.h` | All state structs, constants, and enums |

### Memory Map

```
$0000-$FBFF  Work RAM (64 KB, always accessible)
$FC00-$FCFF  Suzy registers (sprite, math, input)
$FD00-$FDFF  Mikey registers (timer, audio, display, UART)
$FE00-$FFF7  Boot ROM (512 bytes)
$FFF8-$FFFF  CPU vectors (NMI, RESET, IRQ)
```

The **MAPCTL** register (`$FFF9`) controls hardware overlays:
- Bit 0: Disable Suzy space
- Bit 1: Disable Mikey space
- Bit 2: Disable vector space
- Bit 3: Disable ROM space

When disabled, reads/writes go to underlying RAM.

### Timer System

8 programmable timers with prescaler selection:

| Prescaler | Period | CPU Cycles |
|-----------|--------|------------|
| 0 | 1 us | 4 |
| 1 | 2 us | 8 |
| 2 | 4 us | 16 |
| 3 | 8 us | 32 |
| 4 | 16 us | 64 |
| 5 | 32 us | 128 |
| 6 | 64 us | 256 |
| 7 | Linked | Cascaded |

**Special timer assignments:**
- Timer 0: HBlank
- Timer 2: VBlank (display refresh)
- Timers 4-7: Audio channels 0-3

### Sprite Engine

Suzy processes sprites as **linked lists of Sprite Control Blocks (SCBs)** in RAM. Each SCB contains:
- Control registers (type, BPP, flip, collision)
- Position (X, Y) and size (H, V)
- Scaling and tilt parameters
- Pixel data address
- Next SCB pointer

**8 sprite types:**

| Type | ID | Behavior |
|------|----|----------|
| Background Shadow | 0 | Draws all pixels, collision write only |
| Background NonCollide | 1 | Draws all pixels, no collision |
| Boundary Shadow | 2 | Skip pen 0/0x0E/0x0F, collision |
| Boundary | 3 | Skip pen 0/0x0F, collision |
| Normal | 4 | Skip pen 0, collision |
| NonCollidable | 5 | Skip pen 0, no collision |
| XOR Shadow | 6 | Skip pen 0, XOR rendering, collision |
| Shadow | 7 | Skip pen 0, collision (skip 0x0E) |

**Variable bits per pixel:** 1, 2, 3, or 4 BPP (2-16 colors per sprite)

### Math Coprocessor

Hardware multiply/divide unit:
- **Multiply:** 16x16 -> 32 bit (signed or unsigned)
- **Divide:** 32 / 16 -> 16 quotient + 16 remainder
- **Accumulate mode:** Products add to accumulator (dot product)
- **Sign handling:** Sign-magnitude (NOT two's complement!)

**Known hardware bugs emulated:**
- Bug 13.8: `$8000` is treated as positive (sign-magnitude zero)
- Bug 13.9: Division remainder is always positive
- Bug 13.10: Math overflow flag is overwritten (not OR'd) per operation
- Bug 13.12: SCB NEXT pointer only checks upper byte for end-of-chain

### Audio System

4 channels using 12-bit LFSR (Linear Feedback Shift Register) synthesis:
- Feedback taps at shift register bits {0, 1, 2, 3, 4, 5, 7, 10}
- Per-channel volume (7-bit) and stereo attenuation (4-bit L/R)
- Integration mode for waveform accumulation
- Channel 3 supports DAC/PCM mode

## TAS/Movie Support

Nexen provides full TAS support for the Lynx:

### Input Recording
- **9 buttons** recorded per frame: Up, Down, Left, Right, A, B, Option 1, Option 2, Pause
- Movie key string: `"UDLRabOoP"` (9 characters per frame)
- Input polled at start of each frame (before CPU execution)

### Piano Roll
All 9 buttons appear in the piano roll editor with labels:
- Arrow keys: directional buttons
- A, B: face buttons
- O1, O2: option buttons
- Pau: pause button

### BK2 Import/Export
Full BK2 (BizHawk) movie format compatibility:
- Import: Maps BK2 controller buttons to Lynx layout
- Export: Generates valid BK2 files with Lynx button mapping

### Frame Rate
- NTSC: 75 Hz (the Lynx has no PAL variant)
- PAL override: Also 75 Hz (Lynx-specific, not 50 Hz)

## Test Coverage

### Test Files

| File | Tests | Coverage Area |
|------|-------|---------------|
| `LynxHardwareReferenceTests.cpp` | 63 | Hardware spec verification against Epyx docs |
| `LynxSuzyTests.cpp` | ~200 | Math coprocessor, sprite control, fuzz testing |
| `LynxTimerTests.cpp` | ~40 | Timer prescalers, IRQs, display |
| `LynxCpuTests.cpp` | ~50 | CPU state, flags, instructions |
| `LynxMemoryTests.cpp` | ~60 | MAPCTL, EEPROM, cart banking, audit fixes |
| `LynxApuTests.cpp` | ~40 | LFSR, volume, stereo, feedback |
| `LynxControllerTests.cpp` | ~30 | Input mapping, TAS strings, switches |
| `LynxHardwareBugsTests.cpp` | 24 | Documented Lynx hardware bugs |
| + 9 more test files | ~150 | Cart, EEPROM, UART, debugger, etc. |

**Total: 828 tests** (all passing)

### Benchmark Files

| File | Benchmarks | Area |
|------|-----------|------|
| `LynxAllocBench.cpp` | 25 | State sizes, frame buffer, RAM alloc, collision |
| `LynxMemoryBench.cpp` | 15 | Address classification, RAM R/W, MAPCTL |
| `LynxSuzyBench.cpp` | 15 | Math operations, sprite processing |
| `LynxCpuBench.cpp` | 10 | Instruction dispatch, flag computation |
| + 7 more benchmark files | ~54 | APU, cart, UART, timers, etc. |

**Total: 119 Lynx benchmarks**

## Known Limitations

### Documented Issues (GitHub Epic #409)

**Core accuracy items to investigate:**
- MAPCTL bits 4-5 (Mikey/Suzy enable) read but not gated (#414)
- LeftHand mode flag read but not applied to sprite rendering (#415)
- Timer clock source change may cause spurious ticks (#417)
- Pen index register routing (#416)

**Low-priority items:**
- Display DMA not scanline-accurate (renders full frame at once)
- Sprite engine processes SCB chain synchronously
- Some Mikey registers are stubs ($FD82-$FD93)
- Audio DAC mode on channel 3 not explicitly differentiated

### What Works Well
- CPU: Full 65SC02 instruction set with all CMOS extensions
- Timers: Prescaler, linking, IRQ integration
- Math: Unsigned/signed multiply/divide with all hardware bugs
- Sprites: All 8 types, 1-4 BPP, H/V flip, scaling
- Audio: 4 LFSR channels with stereo panning
- EEPROM: Full 93C46/66/86 serial protocol
- Input: All 9 buttons with no lag (polled before frame)
- Save states: Full state serialization/restoration
- TAS: Complete movie recording/playback with piano roll
- ComLynx: Basic UART implementation
