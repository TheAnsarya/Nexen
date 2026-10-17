# Game Boy & GBA Emulation Core Documentation

This document covers both the Game Boy/Game Boy Color and Game Boy Advance emulation cores.

## Game Boy / Game Boy Color

### Architecture Overview

```text
Game Boy Architecture
═════════════════════

┌──────────────────────────────────────────────────────┐
│                     Gameboy                        │
│  (Main coordinator - DMG/CGB/SGB mode selection)   │
└──────────────────────────────────────────────────────┘
              │
    ┌─────────┼─────────┬──────────────┐
    │        │       │            │
    ▼        ▼       ▼            ▼
┌───────┐ ┌───────┐ ┌─────────┐ ┌───────────┐
│ GbCpu │ │ GbPpu │ │  GbApu  │ │  Memory   │
│      │ │     │ │       │ │  Manager  │
│LR35902│ │ Video │ │  Audio  │ │  + Cart   │
│(SM83) │ │160×144│ │4 channel│ │  (MBC)    │
└───────┘ └───────┘ └─────────┘ └───────────┘
```

### Directory Structure

```text
Core/Gameboy/
├── Main Components
│   ├── Gameboy.h/cpp          - Console coordinator
│   ├── GbCpu.h/cpp          - LR35902 CPU (SM83)
│   ├── GbPpu.h/cpp          - PPU (160×144 LCD)
│   ├── GbMemoryManager.h/cpp   - Memory mapping
│   ├── GbDmaController.h/cpp   - OAM DMA
│   └── GbTimer.h/cpp          - Timer subsystem
│
├── Audio
│   └── APU/                    - 4-channel audio
│      ├── Square 1 (sweep)
│      ├── Square 2
│      ├── Wave
│      └── Noise
│
├── Cartridges
│   └── Carts/                - Memory Bank Controllers
│      ├── GbCart (base)
│      ├── MBC1, MBC2, MBC3
│      ├── MBC5, MBC6, MBC7
│      ├── MMM01, HuC1, HuC3
│      └── Camera, TAMA5
│
├── Input/
│   └── Controller handling
│
├── Debugger/
│   └── GB-specific debug
│
└── Support Files
    ├── GbTypes.h              - State structures
    ├── GbConstants.h          - Hardware constants
    ├── GbBootRom.h          - Boot ROM handling
    └── GameboyHeader.h      - ROM header parsing
```

### Core Components

#### GbCpu (GbCpu.h)

Emulates the Sharp LR35902 (SM83), a modified Z80/8080 hybrid.

**Key Differences from Z80:**

- No IX/IY index registers
- No alternate register set
- Unique STOP and HALT behavior
- Different flag behavior on some operations

**CPU State:**

```cpp
struct GbCpuState {
    uint16_t PC;    // Program Counter
    uint16_t SP;    // Stack Pointer
    uint8_t A;    // Accumulator
    uint8_t Flags;  // F register (ZNHC----)
    uint8_t B, C;   // BC register pair
    uint8_t D, E;   // DE register pair
    uint8_t H, L;   // HL register pair
    bool IME;      // Interrupt Master Enable
    bool Halted;    // HALT state
};
```

**Flags:**

- Z (Zero): Result is zero
- N (Subtract): Last op was subtraction
- H (Half-carry): Carry from bit 3 to 4
- C (Carry): Carry from bit 7

#### GbPpu (GbPpu.h)

Emulates the Game Boy PPU with LCD output.

**Display Specifications:**

- Resolution: 160×144 pixels
- Colors: 4 shades (DMG) or 32,768 (CGB)
- Tile-based: 8×8 pixel tiles
- 40 sprites (OAM), 10 per scanline

**PPU Modes:**

| Mode | Duration | Description |
| ------ | ---------- | ------------- |
| 0 | ~204 dots | H-Blank |
| 1 | 4560 dots | V-Blank (10 lines) |
| 2 | ~80 dots | OAM Search |
| 3 | ~172 dots | Drawing |

**CGB Enhancements:**

- 8 background palettes (4 colors each)
- 8 sprite palettes
- Tile attributes (flip, priority, palette, bank)
- Double-speed mode (8 MHz)
- 32KB VRAM (2 banks)

#### GbApu (APU/)

4-channel audio processor:

1. **Square 1** - Square wave with frequency sweep
2. **Square 2** - Square wave (no sweep)
3. **Wave** - Custom 32-sample waveform
4. **Noise** - LFSR noise generator

**Audio Output:** Stereo, ~65536 Hz internal, resampled

### Memory Map

```text
Game Boy Memory Map
═══════════════════
$0000-$3FFF  ROM Bank 0 (16KB, fixed)
$4000-$7FFF  ROM Bank N (16KB, switchable via MBC)
$8000-$9FFF  VRAM (8KB, CGB: 2 banks)
$A000-$BFFF  External RAM (cartridge, if present)
$C000-$CFFF  WRAM Bank 0 (4KB)
$D000-$DFFF  WRAM Bank N (4KB, CGB: banks 1-7)
$E000-$FDFF  Echo RAM (mirror of $C000-$DDFF)
$FE00-$FE9F  OAM (Sprite Attribute Table)
$FEA0-$FEFF  Unusable
$FF00-$FF7F  I/O Registers
$FF80-$FFFE  High RAM (HRAM)
$FFFF       Interrupt Enable register
```

### Memory Bank Controllers (MBCs)

| MBC | Features | Max ROM | Max RAM |
| ----- | ---------- | --------- | --------- |
| None | No banking | 32KB | - |
| MBC1 | Banking | 2MB | 32KB |
| MBC2 | Built-in RAM | 256KB | 512×4bit |
| MBC3 | RTC | 2MB | 32KB |
| MBC5 | Large ROM | 8MB | 128KB |

---

## Game Boy Advance

### Architecture Overview

```text
GBA Architecture
════════════════

┌──────────────────────────────────────────────────────┐
│                    GbaConsole                    │
│   (Main coordinator - ARM7TDMI + GBA hardware)       │
└──────────────────────────────────────────────────────┘
              │
    ┌─────────┼─────────┬──────────────┐
    │        │       │            │
    ▼        ▼       ▼            ▼
┌───────┐ ┌───────┐ ┌─────────┐ ┌───────────┐
│GbaCpu │ │GbaPpu │ │  GbaApu │ │  Memory   │
│      │ │     │ │       │ │  Manager  │
│ARM7TDM│ │ Video │ │  Audio  │ │ + Cart    │
│ I  │ │240×160│ │6 channel│ │ prefetch  │
└───────┘ └───────┘ └─────────┘ └───────────┘
```

### Directory Structure

```text
Core/GBA/
├── Main Components
│   ├── GbaConsole.h/cpp        - Console coordinator
│   ├── GbaCpu.h/cpp            - ARM7TDMI CPU
│   ├── GbaCpu.Arm.cpp        - ARM instruction set
│   ├── GbaCpu.Thumb.cpp        - Thumb instruction set
│   ├── GbaPpu.h/cpp            - PPU (240×160)
│   ├── GbaMemoryManager.h/cpp  - Memory & I/O
│   ├── GbaDmaController.h/cpp  - 4-channel DMA
│   └── GbaTimer.h/cpp        - 4 timers
│
├── Audio
│   └── APU/                    - 6-channel audio
│      ├── GB-compatible channels (4)
│      └── Direct Sound A/B (2)
│
├── Cartridge
│   └── Cart/                  - ROM handling
│      ├── EEPROM save
│      ├── Flash save
│      └── SRAM save
│
├── Input/
│   └── Controller handling
│
├── Debugger/
│   └── GBA-specific debug
│
└── Support Files
    ├── GbaTypes.h            - State structures
    ├── GbaWaitStates.h      - Memory timing
    └── GbaRomPrefetch.h        - Prefetch buffer
```

### Core Components

#### GbaCpu (GbaCpu.h)

Emulates the ARM7TDMI processor (ARM + Thumb modes).

**Key Features:**

- 32-bit RISC architecture
- 16.78 MHz clock
- ARM mode: 32-bit instructions
- Thumb mode: 16-bit compressed instructions
- 16 general-purpose registers (R0-R15)
- CPSR (Current Program Status Register)

**CPU State:**

```cpp
struct GbaCpuState {
    uint32_t R[16];  // R0-R15 (R15 = PC)
    uint32_t CPSR;    // Current Program Status Register
    uint32_t SPSR[6];   // Saved PSR for each mode
    // Banked registers for FIQ, IRQ, SVC, ABT, UND modes
    CpuMode Mode;      // Current CPU mode
    bool ThumbMode;  // Thumb (16-bit) vs ARM (32-bit)
};
```

**CPU Modes:**

- User/System (normal execution)
- FIQ (fast interrupt)
- IRQ (normal interrupt)
- Supervisor (software interrupt)
- Abort (memory fault)
- Undefined (illegal instruction)

**Instruction Sets:**

- **ARM:** Full 32-bit, conditional execution on all instructions
- **Thumb:** 16-bit compressed, higher code density

#### GbaPpu (GbaPpu.h)

Advanced PPU with multiple rendering modes.

**Display Specifications:**

- Resolution: 240×160 pixels
- Colors: 32,768 (15-bit)
- 4 background layers
- 128 sprites (OAM)

**Background Modes:**

| Mode | BG0 | BG1 | BG2 | BG3 | Features |
| ------ | ----- | ----- | ----- | ----- | ---------- |
| 0 | Tile | Tile | Tile | Tile | 4 text BGs |
| 1 | Tile | Tile | Affine | - | 2 text + 1 affine |
| 2 | - | - | Affine | Affine | 2 affine BGs |
| 3 | - | - | Bitmap | - | 240×160 @ 15bpp |
| 4 | - | - | Bitmap | - | 240×160 @ 8bpp, 2 frames |
| 5 | - | - | Bitmap | - | 160×128 @ 15bpp, 2 frames |

**Affine Backgrounds:**

- Rotation and scaling
- Used for Mode 7-style effects

**Sprite Features:**

- Up to 128 sprites
- Sizes: 8×8 to 64×64
- Affine transformation support
- Semi-transparency

#### GbaApu (APU/)

6-channel audio system:

1-4. **GB-compatible channels** (Square×2, Wave, Noise)
5-6. **Direct Sound A/B** - DMA-fed 8-bit PCM

**Direct Sound Features:**

- 8-bit signed PCM samples
- Timer-driven playback
- FIFO buffers (32 bytes each)
- Hardware mixing with GB channels

### Memory Map

```text
GBA Memory Map
══════════════
$00000000-$00003FFF  BIOS (16KB)
$02000000-$0203FFFF  EWRAM (256KB, 16-bit bus)
$03000000-$03007FFF  IWRAM (32KB, 32-bit bus)
$04000000-$040003FF  I/O Registers
$05000000-$050003FF  Palette RAM (1KB)
$06000000-$06017FFF  VRAM (96KB)
$07000000-$070003FF  OAM (1KB)
$08000000-$09FFFFFF  ROM (Wait State 0, max 32MB)
$0A000000-$0BFFFFFF  ROM (Wait State 1)
$0C000000-$0DFFFFFF  ROM (Wait State 2)
$0E000000-$0E00FFFF  SRAM (64KB max)
```

### DMA Controller

4 DMA channels with different priorities:

- **DMA0:** Highest priority (sound)
- **DMA1/2:** Sound FIFO
- **DMA3:** Lowest priority (general use)

**DMA Features:**

- 16/32-bit transfers
- Increment/decrement/fixed addressing
- Immediate or H-Blank/V-Blank triggered

### Wait States & Prefetch

GBA ROM access has configurable wait states:

- Wait State 0: Fastest (3-8 cycles)
- Wait State 1: Medium
- Wait State 2: Slowest

**Prefetch Buffer:**

- 8-halfword buffer for sequential ROM reads
- Hides ROM latency for linear code

---

## Timing

**Game Boy:**

- DMG: 4.194304 MHz (1 cycle = 238.4 ns)
- CGB: 4.194304 MHz (normal) / 8.388608 MHz (double speed)
- 154 scanlines (144 visible + 10 VBlank)
- ~59.73 Hz frame rate

**GBA:**

- 16.78 MHz (16777216 Hz)
- 228 scanlines (160 visible + 68 VBlank)
- 308 dots per scanline
- ~59.73 Hz frame rate

## Related Documentation

- [NES-CORE.md](NES-CORE.md) - NES emulation
- [SNES-CORE.md](SNES-CORE.md) - SNES emulation
- [ARCHITECTURE-OVERVIEW.md](ARCHITECTURE-OVERVIEW.md) - Overall structure
