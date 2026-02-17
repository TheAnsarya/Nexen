# Atari Lynx — Comprehensive Technical Report for Emulation

> **Purpose:** This document provides a complete hardware reference for the Atari Lynx handheld console, synthesized from original Epyx "Handy" hardware specifications (via monlynx.de), Wikipedia, and community documentation. It is intended to drive emulation implementation decisions for the Nexen project.
>
> **Date:** 2025-07-10
> **Sources:** monlynx.de (original 1987 Epyx spec), Wikipedia, atarilynxdev.net, oxyron.de (6502 opcode reference)

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [CPU — 65SC02 / 65C02](#2-cpu--65sc02--65c02)
3. [Mikey Custom Chip](#3-mikey-custom-chip)
4. [Suzy (Susie) Custom Chip](#4-suzy-susie-custom-chip)
5. [Memory Architecture](#5-memory-architecture)
6. [Display System](#6-display-system)
7. [Audio System](#7-audio-system)
8. [Input System](#8-input-system)
9. [Cartridge / ROM Cart](#9-cartridge--rom-cart)
10. [Hardware Revisions (Lynx I vs Lynx II)](#10-hardware-revisions-lynx-i-vs-lynx-ii)
11. [Boot ROM & Startup Sequence](#11-boot-rom--startup-sequence)
12. [Timing & Bus Architecture](#12-timing--bus-architecture)
13. [Known Hardware Bugs](#13-known-hardware-bugs)
14. [LNX File Format](#14-lnx-file-format)
15. [Emulation Implementation Notes](#15-emulation-implementation-notes)

---

## 1. System Overview

| Property | Value |
|---|---|
| **Manufacturer** | Atari Corporation (designed by Epyx as "Handy") |
| **Release** | September 1989 (Lynx I), July 1991 (Lynx II) |
| **CPU** | 65SC02 (Lynx I) / 65C02 (Lynx II), embedded in Mikey |
| **Custom chips** | Mikey (8-bit, audio/video/timers/UART) + Suzy (16-bit, sprites/math/input) |
| **RAM** | 64 KB |
| **Display** | 160×102, 4bpp, 16 from 4096 colors, LCD |
| **Audio** | 4 channels, 8-bit DAC each, mono (Lynx I) / stereo (Lynx II) |
| **Input** | D-pad, A, B, Option 1, Option 2, Pause |
| **Cart sizes** | 128 KB – 1 MB |
| **Link** | ComLynx serial (UART) up to 62,500 baud |
| **System clock** | 16 MHz crystal |
| **CPU speed** | Up to 4 MHz (~3.6 MHz average due to DMA) |
| **Power** | 6× AA batteries, 4–6 hour life |

---

## 2. CPU — 65SC02 / 65C02

### 2.1 Variants

| Revision | CPU Core | Key Difference |
|---|---|---|
| **Lynx I** | VLSI VL65NC02-02LG (65**SC**02 variant) | Missing `WAI`, `STP`, `SMBx`, `RMBx`, `BBSx`, `BBRx` |
| **Lynx II** | Full 65**C**02 | All WDC 65C02 instructions available |

The CPU core is **embedded inside the Mikey chip**, not a separate IC.

### 2.2 Registers

| Register | Size | Description |
|---|---|---|
| **A** | 8-bit | Accumulator |
| **X** | 8-bit | Index register X |
| **Y** | 8-bit | Index register Y |
| **SP** | 8-bit | Stack pointer (stack at $0100–$01FF) |
| **PC** | 16-bit | Program counter |
| **P** | 8-bit | Status register: N V - B D I Z C |

### 2.3 Status Register Flags

| Bit | Flag | Meaning |
|---|---|---|
| 7 | N | Negative (bit 7 of result) |
| 6 | V | Overflow (signed arithmetic) |
| 5 | - | Always 1 |
| 4 | B | Break (BRK vs IRQ distinction) |
| 3 | D | Decimal (BCD mode) |
| 2 | I | IRQ disable |
| 1 | Z | Zero |
| 0 | C | Carry |

### 2.4 65C02 New Instructions (vs NMOS 6502)

These are the instructions added by the 65C02 that are **not** in the original 6502:

| Mnemonic | Opcode(s) | Description |
|---|---|---|
| **BRA** | $80 | Branch Always (relative) |
| **PHX** | $DA | Push X to stack |
| **PLX** | $FA | Pull X from stack |
| **PHY** | $5A | Push Y to stack |
| **PLY** | $7A | Pull Y from stack |
| **STZ** | $64, $74, $9C, $9E | Store Zero (zp, zpx, abs, abx) |
| **TRB** | $14, $1C | Test and Reset Bits (zp, abs) |
| **TSB** | $04, $0C | Test and Set Bits (zp, abs) |
| **INC A** | $1A | Increment accumulator |
| **DEC A** | $3A | Decrement accumulator |
| **BIT imm** | $89 | BIT with immediate mode |
| **BIT zpx** | $34 | BIT with zero page,X |
| **BIT abx** | $3C | BIT with absolute,X |
| **JMP (abs,X)** | $7C | Jump indexed absolute indirect |
| **LDA (zp)** | $B2 | LDA zero-page indirect (no index) |
| **STA (zp)** | $92 | STA zero-page indirect |
| **ORA (zp)** | $12 | ORA zero-page indirect |
| **AND (zp)** | $32 | AND zero-page indirect |
| **EOR (zp)** | $52 | EOR zero-page indirect |
| **ADC (zp)** | $72 | ADC zero-page indirect |
| **SBC (zp)** | $F2 | SBC zero-page indirect |
| **CMP (zp)** | $D2 | CMP zero-page indirect |

**65C02-only (NOT in 65SC02)** — only available on Lynx II:

| Mnemonic | Opcodes | Description |
|---|---|---|
| **WAI** | $CB | Wait for Interrupt (low-power halt) |
| **STP** | $DB | Stop the Processor (halt until reset) |
| **SMB0–SMB7** | $87–$F7 (odd, x7) | Set Memory Bit in zero page |
| **RMB0–RMB7** | $07–$77 (odd, x7) | Reset Memory Bit in zero page |
| **BBS0–BBS7** | $8F–$FF (odd, xF) | Branch on Bit Set |
| **BBR0–BBR7** | $0F–$7F (odd, xF) | Branch on Bit Reset |

### 2.5 65C02 Bug Fixes (vs NMOS 6502)

| Bug | NMOS 6502 | 65C02 Fix |
|---|---|---|
| `JMP ($xxFF)` page wrap | Fetches MSB from $xx00 instead of $(xx+1)00 | Fixed, correct page crossing |
| Decimal flag after reset/IRQ | Undefined | Automatically cleared |
| BCD N/V/Z flags | Reflect binary result | Reflect decimal result (extra cycle cost) |
| R-M-W double write | Writes old value then new value | Does double read then single write |
| Indexed cross-page dummy read | Reads invalid address | Reads opcode address instead |
| BRK + hardware IRQ collision | BRK lost | Both handled correctly |
| Undefined opcodes | Various side effects, some crash | All unused opcodes are NOPs |

### 2.6 Addressing Modes

The 65C02 supports 16 addressing modes:

| Mode | Syntax | Bytes | Example |
|---|---|---|---|
| Implied | - | 1 | `CLC` |
| Accumulator | A | 1 | `INC A` |
| Immediate | #$nn | 2 | `LDA #$42` |
| Zero Page | $nn | 2 | `LDA $80` |
| Zero Page,X | $nn,X | 2 | `LDA $80,X` |
| Zero Page,Y | $nn,Y | 2 | `LDX $80,Y` |
| Absolute | $nnnn | 3 | `LDA $1234` |
| Absolute,X | $nnnn,X | 3 | `LDA $1234,X` |
| Absolute,Y | $nnnn,Y | 3 | `LDA $1234,Y` |
| Indirect | ($nnnn) | 3 | `JMP ($1234)` |
| Indexed Indirect | ($nn,X) | 2 | `LDA ($80,X)` |
| Indirect Indexed | ($nn),Y | 2 | `LDA ($80),Y` |
| Zero Page Indirect | ($nn) | 2 | `LDA ($80)` ← **NEW in 65C02** |
| Indexed Abs. Indirect | ($nnnn,X) | 3 | `JMP ($1234,X)` ← **NEW in 65C02** |
| Relative | $nn | 2 | `BNE label` |
| Zero Page + Relative | $nn,$rr | 3 | `BBR0 $80,label` ← **65C02 only** |

### 2.7 Cycle Timing on Lynx

The Lynx runs at a 16 MHz master clock. CPU ticks are counted in these 62.5 ns ticks:

| Access Type | Ticks | Time |
|---|---|---|
| Page-mode RAM read | 4 | 250 ns |
| Normal RAM read/write | 5 | 312.5 ns |
| Normal ROM read | 5 | 312.5 ns |
| Suzy hardware write | 5 | 312.5 ns (blind write) |
| Suzy hardware read | 9–15 | 562.5–937.5 ns |
| Mikey hardware access | 5 | 312.5 ns |
| Max hardware access | 128 | 8 µs (Suzy DPRAM) |

Average CPU speed is approximately **3.6 MHz** due to video and refresh DMA stealing cycles.

### 2.8 Interrupt Vectors

| Vector | Address | Purpose |
|---|---|---|
| NMI | $FFFA–$FFFB | Non-Maskable Interrupt |
| RESET | $FFFC–$FFFD | Power-on / Reset |
| IRQ/BRK | $FFFE–$FFFF | Maskable Interrupt / Software Break |

---

## 3. Mikey Custom Chip

Mikey is an 8-bit custom VLSI CMOS chip running at 16 MHz. It contains:
- CPU core (65SC02/65C02)
- Sound engine (4 channels)
- Video DMA driver (LCD shift-out)
- 8 hardware timers
- UART (ComLynx serial)
- Interrupt controller
- Color palette registers
- System control

I/O space: **$FD00–$FDFF**

### 3.1 Timer System

8 independent timers at $FD00–$FD1F, each occupying 4 bytes:

| Timer | Base Addr | Primary Use |
|---|---|---|
| Timer 0 | $FD00 | **Horizontal line timing (Hcount)** |
| Timer 1 | $FD04 | General purpose |
| Timer 2 | $FD08 | **Vertical line counter (Vcount)** — linked from Timer 0 |
| Timer 3 | $FD0C | General purpose |
| Timer 4 | $FD10 | **UART baud rate generator** |
| Timer 5 | $FD14 | General purpose |
| Timer 6 | $FD18 | General purpose |
| Timer 7 | $FD1C | General purpose |

#### Timer Register Layout (per timer, 4 bytes at base+N*4):

| Offset | Register | Description |
|---|---|---|
| +0 | Backup | Reload value (8-bit) |
| +1 | Control A | Static control bits |
| +2 | Count | Current count value (8-bit, write sets timer) |
| +3 | Control B | Dynamic control bits (read-only) |

#### Control A Bits:

| Bit | Name | Function |
|---|---|---|
| 7 | ENABLE_INTERRUPT | Generate IRQ when timer reaches zero |
| 6 | RESET_DONE | Write 1 to reset the Timer Done status |
| 5 | — | Reserved |
| 4 | ENABLE_RELOAD | Reload from backup on underflow |
| 3 | ENABLE_COUNT | Enable counting |
| 2–0 | CLOCK_SELECT | Clock source selector |

#### Clock Source Select:

| Value | Source | Period |
|---|---|---|
| 0 | 1 µs | 16 ticks |
| 1 | 2 µs | 32 ticks |
| 2 | 4 µs | 64 ticks |
| 3 | 8 µs | 128 ticks |
| 4 | 16 µs | 256 ticks |
| 5 | 32 µs | 512 ticks |
| 6 | 64 µs | 1024 ticks |
| 7 | Linking | Borrow from linked timer |

#### Control B (Dynamic / Read-only):

| Bit | Name | Function |
|---|---|---|
| 3 | TIMER_DONE | Timer has counted down to zero |
| 2 | LAST_CLOCK | Last clock edge state |
| 1 | BORROW_IN | Borrow signal received |
| 0 | BORROW_OUT | Borrow signal generated |

#### Timer Linking Chain:

```
Group A: Timer0 → Timer2 → Timer4
Group B: Timer1 → Timer3 → Timer5 → Timer7 → Audio0 → Audio1 → Audio2 → Audio3 → Timer1
```

Timer range: 1 µs to 16,384 µs (cascading for longer periods).

### 3.2 Interrupt Controller

| Register | Address | R/W | Description |
|---|---|---|---|
| INTRST | $FD80 | R/W | Read: poll interrupts; Write: reset (clear) matched bits |
| INTSET | $FD81 | R/W | Read: poll interrupts; Write: set (force) matched bits |

#### Interrupt Bits:

| Bit | Source | Description |
|---|---|---|
| 7 | Timer 7 | General purpose |
| 6 | Timer 6 | General purpose |
| 5 | Timer 5 | General purpose |
| 4 | Serial | UART (ComLynx) |
| 3 | Timer 3 | General purpose |
| 2 | Timer 2 | **Vertical blank (Vcount)** |
| 1 | Timer 1 | General purpose |
| 0 | Timer 0 | **Horizontal line (Hcount)** |

All interrupts are active-high and directly drive the CPU's IRQ line (active-low, active if any bit set and not masked).

### 3.3 Video DMA

The video system uses DMA to shift pixel data from RAM to the LCD controller. It does **not** generate pixels — it reads a pre-rendered frame buffer.

| Register | Address | Description |
|---|---|---|
| DISPCTL | $FD92 | Display control |
| PBKUP | $FD93 | LCD timing parameter (magic P count) |
| DISPADR | $FD94–$FD95 | Display buffer start address (little-endian, bottom 2 bits ignored) |

#### DISPCTL ($FD92) Bits:

| Bit | Name | Function |
|---|---|---|
| 3 | COLOR | 1 = color mode, 0 = monochrome |
| 2 | FOURBIT | 1 = 4bpp, 0 = 2bpp |
| 1 | FLIP | 1 = display flipped 180° |
| 0 | DMA_ENABLE | 1 = enable video DMA |

### 3.4 Color Palette

16 palette entries, each 12-bit RGB (4 bits per component):

| Address Range | Component |
|---|---|
| $FDA0–$FDAF | Green (4 bits per entry, 16 entries) |
| $FDB0–$FDBF | Blue (high nibble) + Red (low nibble) per entry, 16 entries |

Palette entry N:
- Green: `$FDA0 + N` (low nibble = green value)
- Blue: `$FDB0 + N` (high nibble = blue value)
- Red: `$FDB0 + N` (low nibble = red value)

Total color space: 4096 colors (12-bit), 16 simultaneous.

### 3.5 UART (ComLynx)

| Register | Address | Function |
|---|---|---|
| SERCTL | $FD8C | Serial control (write) / status (read) |
| SERDAT | $FD8D | Serial data (read/write) |

#### SERCTL Write Bits:

| Bit | Name | Function |
|---|---|---|
| 7 | TXINTEN | TX interrupt enable |
| 6 | RXINTEN | RX interrupt enable |
| 5 | — | Reserved |
| 4 | PAREN | Parity enable |
| 3 | RESETERR | Reset error flags |
| 2 | TXOPEN | TX pin open-collector mode |
| 1 | TXBRK | Force TX break |
| 0 | PAREVEN | 0 = odd parity, 1 = even parity |

#### SERCTL Read Bits:

| Bit | Name | Function |
|---|---|---|
| 7 | TXRDY | Transmitter ready for data |
| 6 | RXRDY | Received data available |
| 5 | TXEMPTY | Transmitter completely empty |
| 4 | PARERR | Parity error |
| 3 | OVERRUN | Receiver overrun |
| 2 | FRAMERR | Framing error |
| 1 | RXBRK | Break received |
| 0 | PARBIT | Received parity bit value |

Frame format: 1 start + 8 data (LSB first) + 1 parity + 1 stop = **11 bits**.

Baud rate = `CLOCK4 / (TIMER4_backup + 1) / 8`

| Baud Rate | Clock Source | Timer 4 Backup |
|---|---|---|
| 62,500 | 1 µs | 1 |
| 31,250 | 1 µs | 3 |
| 9,600 | 1 µs | 12 |
| 2,400 | 1 µs | 51 |
| 1,200 | 1 µs | 103 |
| 300 | 2 µs | 207 |

### 3.6 System & I/O Registers

| Register | Address | Description |
|---|---|---|
| SYSCTL1 | $FD87 | B1=power, B0=cart address strobe |
| MIKEYHREV | $FD88 | Hardware revision = $01 |
| IODIR | $FD8A | I/O data direction (0=input, 1=output) |
| IODAT | $FD8B | I/O data |
| CPUSLEEP | $FD91 | Write any value → CPU sleeps |
| DISPCTL | $FD92 | Display control |
| PBKUP | $FD93 | Timer backup for display |
| DISPADR | $FD94–$FD95 | Display buffer address |
| MSTEREO | $FD50 | Stereo panning control |
| MAPCTL | $FFF9 | Memory map control |

#### IODAT ($FD8B) Bits:

| Bit | Name | Direction | Function |
|---|---|---|---|
| 4 | AUDIN | Input | Audio in (active low) |
| 3 | REST | Output | External REST |
| 2 | NOEXP | Input | No expansion (active low) |
| 1 | CART_ADDR_DATA | Output | Cart address data line |
| 0 | EXTPOWER | Input | External power sense (active low) |

#### MAPCTL ($FFF9) — Memory Map Control:

| Bit | Name | Function |
|---|---|---|
| 7 | SEQUENTIAL_DISABLE | Disable sequential access optimization |
| 3 | VECTOR_SPACE_RAM | Map $FFFA–$FFFF to RAM (overlay vectors) |
| 2 | ROM_SPACE_RAM | Map $FE00–$FFF7 to RAM (overlay boot ROM) |
| 1 | MIKEY_SPACE_RAM | Map $FD00–$FDFF to RAM (overlay Mikey) |
| 0 | SUZY_SPACE_RAM | Map $FC00–$FCFF to RAM (overlay Suzy) |

When a bit is **set**, the corresponding hardware space is hidden and RAM is visible at that address.

---

## 4. Suzy (Susie) Custom Chip

Suzy is a 16-bit custom VLSI CMOS chip running at 16 MHz. It contains:
- Hardware sprite engine (blitter)
- Math coprocessor (16×16 multiply, 32÷16 divide)
- Joystick & switch readers
- ROM cart interface

I/O space: **$FC00–$FCFF**

### 4.1 Sprite Engine

The Lynx sprite engine is hardware-driven, processing linked lists of Sprite Control Blocks (SCBs) from RAM. It operates as a bus master, reading SCBs and sprite data, then writing processed pixels to the display and collision buffers.

#### Sprite Control Block (SCB) Structure:

An SCB is a variable-length structure in RAM. The core fields, loaded sequentially:

| Offset | Field | Size | Description |
|---|---|---|---|
| 0 | SPRCTL0 | 1 byte | Sprite type, bits/pixel, flips |
| 1 | SPRCTL1 | 1 byte | Sizing, reload, skip, direction |
| 2 | SPRCOLL | 1 byte | Collision number and control |
| 3–4 | SCBNEXT | 2 bytes | Pointer to next SCB ($0000 in high byte = end of list) |
| 5 | SPRDLINE | 1 byte | Sprite data line offset |
| 6–7 | HPOSSTRT | 2 bytes | Horizontal start position (signed) |
| 8–9 | VPOSSTRT | 2 bytes | Vertical start position (signed) |
| — | (optional) | varies | SPRHSIZ, SPRVSIZ, STRETCH, TILT (if reload bits set) |

#### Suzy Hardware Registers (Sprite-Related):

| Register | Address | Description |
|---|---|---|
| TMPADR | $FC00–$FC01 | Temporary address |
| TILTACUM | $FC02–$FC03 | Tilt accumulator |
| HOFF | $FC04–$FC05 | Horizontal offset |
| VOFF | $FC06–$FC07 | Vertical offset |
| VIDBAS | $FC08–$FC09 | Video buffer base address |
| COLLBAS | $FC0A–$FC0B | Collision buffer base address |
| VIDADR | $FC0C–$FC0D | Current video address |
| COLLADR | $FC0E–$FC0F | Current collision address |
| SCBNEXT | $FC10–$FC11 | Next SCB pointer |
| SPRDLINE | $FC12–$FC13 | Sprite data line pointer |
| HPOSSTRT | $FC14–$FC15 | Horizontal start position |
| VPOSSTRT | $FC16–$FC17 | Vertical start position |
| SPRHSIZ | $FC18–$FC19 | Sprite horizontal size |
| SPRVSIZ | $FC1A–$FC1B | Sprite vertical size |
| STRETCH | $FC1C–$FC1D | Stretch value |
| TILT | $FC1E–$FC1F | Tilt value |
| SPRDOFF | $FC20–$FC21 | Sprite data offset |
| SPRVPOS | $FC22–$FC23 | Current sprite vertical position |
| COLLOFF | $FC24–$FC25 | Collision offset |
| VSIZACUM | $FC26–$FC27 | Vertical size accumulator |
| HSIZOFF | $FC28–$FC29 | Horizontal size offset |
| VSIZOFF | $FC2A–$FC2B | Vertical size offset |
| SCBADR | $FC2C–$FC2D | Current SCB address |
| PROCADR | $FC2E–$FC2F | Process address |

#### SPRCTL0 ($FC80) — Sprite Control 0:

| Bits | Name | Function |
|---|---|---|
| 7–6 | BITS_PER_PIXEL | 00=1bpp, 01=2bpp, 10=3bpp, 11=4bpp |
| 5 | HFLIP | Horizontal flip |
| 4 | VFLIP | Vertical flip |
| 3–0 | SPRITE_TYPE | Sprite type (see below) |

#### Sprite Types (SPRCTL0 bits 3–0):

| Value | Type | Description |
|---|---|---|
| 0 | Background (shadow) | Writes if pen > collide |
| 1 | Background (no collision) | Background, no collision check |
| 2 | Boundary-shadow | No write if pen index = 0 or 14/15 |
| 3 | Boundary | No write if pen index = 0 |
| 4 | **Normal** | Standard sprite with collision |
| 5 | Non-collidable | Writes pixels, no collision |
| 6 | XOR shadow | XOR pixel with existing value |
| 7 | Shadow | Only updates collision buffer |

#### SPRCTL1 ($FC81) — Sprite Control 1:

| Bit | Name | Function |
|---|---|---|
| 7 | LITERAL | 1 = raw pixel data (no packed/RLE runs) |
| 6 | SIZING | Enable stretch/tilt sizing |
| 5:4 | RELOAD_DEPTH | Reload depth: 0=HVST+palette, 1=HVS, 2=HV, 3=just SPRDLINE |
| 3 | RELOAD_PALETTE | 0 = reload palette from SCB, 1 = skip palette reload |
| 2 | SKIP_SPRITE | Skip this sprite in chain |
| 1 | START_UP | Start drawing upward (1=up, 0=down) |
| 0 | START_LEFT | Start drawing left (1=left, 0=right) |

**RELOAD_DEPTH values (bits 5:4):**
| Value | Fields loaded from SCB |
|---|---|
| 0 | SPRDLINE, HPOS, VPOS, HSIZE, VSIZE, STRETCH, TILT, palette |
| 1 | SPRDLINE, HPOS, VPOS, HSIZE, VSIZE |
| 2 | SPRDLINE, HPOS, VPOS |
| 3 | SPRDLINE only |

#### SPRCOLL ($FC82) — Sprite Collision:

| Bit | Name | Function |
|---|---|---|
| 5 | DONT_COLLIDE | Disable collision for this sprite |
| 3–0 | COLL_NUMBER | Collision number (0–15) |

#### Key Sprite Control Registers:

| Register | Address | Description |
|---|---|---|
| SPRGO | $FC91 | B0=start sprite processing, B2=everon enable |
| SPRSYS (write) | $FC92 | signmath, accumulate, no_collide, vstretch, lefthand, clr_unsafe, spritestop |
| SPRSYS (read) | $FC92 | mathworking, mathwarning, mathcarry, vstretching, lefthanded, unsafe_access, spritetostop, spriteworking |
| SUZYBUSEN | $FC90 | B0=enable Suzy bus access |
| SUZYHREV | $FC88 | Hardware revision = $01 |
| SPRINIT | $FC83 | Must be set to $F3 after 100ms power-up delay |

#### Sprite Data Compression:

Sprite data uses a custom packetized compression format:
- Each horizontal line begins with a line offset byte
- Pixel data is packed in configurable bits-per-pixel (1/2/3/4 bpp)
- Data can be **literal** (1:1 pixel data) or **packed** with run-length encoding:
  - First 5 bits = packet header (count of pixels in packet)
  - Each packet is either literal data or a repeat of a single pixel value
  - `00000` signals end of line data
- Sprite rendering uses a 512×512 virtual drawing space with hardware clipping to the 160×102 visible area

### 4.2 Math Coprocessor

All math operations occur in Suzy's 16-bit register space.

#### 16×16 → 32 Multiply:

| Register | Address | Function |
|---|---|---|
| MATHD | $FC52 | Multiplicand low byte |
| MATHC | $FC53 | Multiplicand high byte |
| MATHB | $FC54 | Multiplier low byte |
| MATHA | $FC55 | Multiplier high byte |
| MATHP | $FC56 | — |
| MATHN | $FC57 | — |
| MATHH | $FC60 | Result byte 0 (lowest) |
| MATHG | $FC61 | Result byte 1 |
| MATHF | $FC62 | Result byte 2 |
| MATHE | $FC63 | Result byte 3 (highest) |

**Write A/B to start multiply**, result appears in E-H after completion.

- **Unsigned multiply**: 44 ticks (~2.75 µs)
- **Signed multiply + accumulate**: 54 ticks (~3.375 µs)
- `SPRSYS.signmath` controls signed mode
- `SPRSYS.accumulate` adds to existing result in E-H instead of replacing

#### 32÷16 → 32 Divide:

| Input | Registers | Description |
|---|---|---|
| Dividend | MATHE–MATHH ($FC60–$FC63) | 32-bit value to divide |
| Divisor | MATHN–MATHP ($FC56–$FC57) | 16-bit divisor |

| Output | Registers | Description |
|---|---|---|
| Quotient | MATHA–MATHD ($FC52–$FC55) | 32-bit result |
| Remainder | MATHM–MATHL ($FC6C–$FC6F) | Not reliable (see bugs) |

**Write N/P to start divide**, result in A-D.

- **Unsigned only** — no signed divide
- 176 + 14×N ticks (N = number of leading zeros in divisor), max ~400 ticks

### 4.3 Joystick & Switch Readers

| Register | Address | Description |
|---|---|---|
| JOYPAD | $FCB0 | Joystick state (active low) |
| SWITCHES | $FCB1 | Switch state |

(See [Section 8: Input System](#8-input-system) for details.)

### 4.4 Cart Interface Registers

| Register | Address | Description |
|---|---|---|
| RCART0 | $FCB2 | Read from cart bank 0 |
| RCART1 | $FCB3 | Read from cart bank 1 |

---

## 5. Memory Architecture

### 5.1 Memory Map

```
$0000–$FBFF  RAM (63.75 KB)
$FC00–$FCFF  Suzy hardware registers (or RAM if MAPCTL.0 set)
$FD00–$FDFF  Mikey hardware registers (or RAM if MAPCTL.1 set)
$FE00–$FFF7  Boot ROM - 512 bytes (or RAM if MAPCTL.2 set)
$FFF8         Reserved
$FFF9         MAPCTL (Memory Map Control register)
$FFFA–$FFFB  NMI vector (or RAM if MAPCTL.3 set)
$FFFC–$FFFD  RESET vector (or RAM if MAPCTL.3 set)
$FFFE–$FFFF  IRQ/BRK vector (or RAM if MAPCTL.3 set)
```

### 5.2 Physical Memory

- **64 KB RAM**: Two 64K×4 DRAM chips (120 ns RAS access, 60 ns page-mode CAS)
- **512 bytes boot ROM**: Embedded in Mikey at $FE00–$FFF7
- All addresses always have underlying RAM; hardware spaces **overlay** RAM
- Video DMA and DRAM refresh bypass the memory map — they always see raw RAM

### 5.3 MAPCTL Overlay Mechanics

MAPCTL at $FFF9 controls which hardware overlays are visible. When a bit is **set** (1), the corresponding hardware is hidden and RAM appears instead:

| Bit | Effect when SET |
|---|---|
| 0 | Suzy ($FC00–$FCFF) → RAM visible |
| 1 | Mikey ($FD00–$FDFF) → RAM visible |
| 2 | ROM ($FE00–$FFF7) → RAM visible |
| 3 | Vectors ($FFFA–$FFFF) → RAM visible |
| 7 | Disable sequential access optimization |

Default (all bits 0): All hardware overlays active.

### 5.4 Display Buffer Layout

At 4bpp, 160×102:
- Bytes per line: 160 pixels × 4 bits / 8 = **80 bytes**
- Total display buffer: 80 × 102 = **8,160 bytes**
- Collision buffer: same size = **8,160 bytes**
- Double-buffered: 2 × 8,160 + 8,160 = **24,480 bytes** (~24 KB)

Display buffer start address must be **4-byte aligned** (bottom 2 bits of DISPADR ignored).

### 5.5 Zero Page & Stack

| Range | Use |
|---|---|
| $0000–$00FF | Zero page (fast addressing modes) |
| $0100–$01FF | Hardware stack (grows downward from $01FF) |

**Important**: SCB NEXT field bug — only checks upper byte for zero. SCBs cannot reside in page 0 ($0000–$00FF).

---

## 6. Display System

### 6.1 LCD Specifications

| Property | Value |
|---|---|
| Resolution | **160 × 102 pixels** |
| LCD physical pixels | 480 horizontal (3 per color triad) |
| Color depth | 4 bits per pixel (16 simultaneous colors) |
| Color palette | 12-bit RGB (4096 possible colors) |
| Alternate mode | 2 bits per pixel (4 colors, half buffer size) |

### 6.2 Frame Timing

The display is driven by Timer 0 (Hcount) and Timer 2 (Vcount).

| Parameter | Typical 60 Hz | Formula |
|---|---|---|
| Line period | 159 µs | Timer 0 backup + 1 (in µs ticks) |
| Lines per frame | 105 | Timer 2 backup + 1 (linked from Timer 0) |
| Frame period | 16.695 ms | Line period × Lines per frame |
| Frame rate | ~59.9 Hz | 1 / Frame period |
| VBlank lines | 3 minimum | 105 total − 102 visible = 3 |

Achievable frame rates by adjusting timer values:

| Frame Rate | Approximate Setup |
|---|---|
| 50 Hz | May cause visible LCD flicker |
| **60 Hz** | Standard / recommended |
| 75 Hz | Maximum useful rate |
| 78.7 Hz | Absolute maximum |

### 6.3 Display Rendering Pipeline

The Lynx uses a **software-rendered, hardware-shifted** display model:

1. **CPU sets up SCB chain** in RAM (sprite list)
2. **Suzy processes sprites** by reading SCBs, decompressing sprite data, scaling/flipping/tilting, and compositing to the display and collision buffers in RAM
3. **Mikey's video DMA** reads the completed display buffer and shifts it to the LCD

This means the frame buffer is a simple linear bitmap in RAM. There are no tiles, no scroll registers, no background planes — everything is sprites.

### 6.4 Sprite Rendering Coordinate System

- Sprites are positioned in a virtual **512×512 space**
- The visible **160×102** window is positioned within this space
- Hardware clipping occurs at window boundaries
- Coordinates are **signed 16-bit** — sprites can be partially off-screen

### 6.5 Double Buffering

Games typically use double buffering:
- Frame N renders sprites to buffer A
- Video DMA displays buffer B
- On VBlank: swap buffers

Memory usage: 2 display buffers + 1 collision buffer ≈ 24 KB of 64 KB RAM.

### 6.6 Flip Mode (DISPCTL bit 1)

When flip is enabled:
- Display data is read from the bottom of the buffer upward
- Effectively rotates the display 180°
- Joystick directions are also swapped (hardware via SPRSYS.lefthand)
- Used for left-handed play mode

---

## 7. Audio System

### 7.1 Overview

| Property | Value |
|---|---|
| Channels | 4 identical channels |
| DAC resolution | 8-bit per channel |
| Output | Mono (Lynx I) or Stereo (Lynx II) |
| Mixing | Digital (additive) |
| Anti-alias filter | 1-pole low-pass at ~4 kHz |

### 7.2 Channel Registers

Each channel occupies 8 bytes. Channels are at:

| Channel | Base Address |
|---|---|
| Audio 0 | $FD20 |
| Audio 1 | $FD28 |
| Audio 2 | $FD30 |
| Audio 3 | $FD38 |

#### Per-Channel Register Layout:

| Offset | Register | Description |
|---|---|---|
| +0 | VOLUME | 8-bit volume (signed, 2's complement) |
| +1 | FEEDBACK | Feedback tap selector (selects taps from shift register) |
| +2 | OUTPUT | Current 8-bit output value (reflects DAC state) |
| +3 | SHIFT_LOW | Lower 8 bits of the 12-bit shift register |
| +4 | BACKUP | Timer reload value |
| +5 | CONTROL | Timer control (same format as Mikey timers) |
| +6 | COUNTER | Current timer count value |
| +7 | SHIFT_HIGH | Upper 4 bits of the 12-bit shift register |

### 7.3 Sound Generation

Each channel contains:
- An **8-bit timer/counter** (clocked same as Mikey timers)
- A **12-bit linear-feedback shift register (LFSR)** for waveform generation
- A **feedback tap selector** that picks which bits of the LFSR are XORed to produce feedback
- An **8-bit signed volume** register

**Waveform generation process:**
1. Timer counts down at the selected clock rate
2. On each timer underflow, the LFSR shifts one bit
3. The new bit is computed by XORing selected taps
4. The output bit (LFSR bit 0) is multiplied by the volume
5. Result is sent to the DAC

**Available taps** for LFSR feedback (from the 12-bit shift register):

| Tap | LFSR Bit |
|---|---|
| 0 | Bit 0 |
| 1 | Bit 1 |
| 2 | Bit 2 |
| 3 | Bit 3 |
| 4 | Bit 4 |
| 5 | Bit 5 |
| 6 | Bit 7 |
| 7 | Bit 10 |
| 8 | Bit 11 (XOR of all taps, combined into bit 11) |

Multiple taps can be selected simultaneously.

### 7.4 Waveform Types

| Type | Configuration | Result |
|---|---|---|
| **Square wave** | Specific tap selections | Periodic square wave at timer frequency |
| **Noise** | Multiple taps | Pseudo-random noise |
| **Integration (triangle)** | Integration mode enabled | Volume added/subtracted from running total |

#### Integration Mode:

When enabled, instead of directly outputting volume × LFSR_bit:
- The volume is **added to** (if LFSR bit = 1) or **subtracted from** (if LFSR bit = 0) a running accumulator
- The accumulator clips at min/max bounds
- This produces approximately **triangular waves** or more complex integrated waveforms

### 7.5 Stereo Control

| Register | Address | Description |
|---|---|---|
| MSTEREO | $FD50 | Per-channel left/right enable |

Each channel can be independently routed to left, right, both, or neither speaker.

**MSTEREO bits:**

| Bit | Function |
|---|---|
| 7 | Audio 3 left disable |
| 6 | Audio 2 left disable |
| 5 | Audio 1 left disable |
| 4 | Audio 0 left disable |
| 3 | Audio 3 right disable |
| 2 | Audio 2 right disable |
| 1 | Audio 1 right disable |
| 0 | Audio 0 right disable |

Default $00 = all channels to both speakers (mono compatible).

### 7.6 Audio Linking

Audio channels participate in the timer linking chain:
```
Timer7 → Audio0 → Audio1 → Audio2 → Audio3 → Timer1
```
This allows using timer linking for precise audio frequency control.

### 7.7 Direct DAC Mode

Each channel can be used as a direct **8-bit DAC** by writing values directly to the OUTPUT register (offset +2). This allows PCM playback.

---

## 8. Input System

### 8.1 Joystick (JOYPAD at $FCB0)

Active-low bits (directly readable, active when bit = 0):

| Bit | Normal (Right-hand) | Left-hand Mode |
|---|---|---|
| 7 | Down | Up |
| 6 | Up | Down |
| 5 | Right | Left |
| 4 | Left | Right |
| 3 | Option 1 | Option 1 |
| 2 | Option 2 | Option 2 |
| 1 | B button (inside) | B button |
| 0 | A button (outside) | A button |

Left-hand mode is controlled by `SPRSYS.lefthand` and physically swaps D-pad directions in hardware.

### 8.2 Switches (SWITCHES at $FCB1)

| Bit | Name | Function |
|---|---|---|
| 2 | CART1_INACTIVE | 1 = no cart in slot 1 |
| 1 | CART0_INACTIVE | 1 = no cart in slot 0 |
| 0 | PAUSE | Pause button state (active low) |

### 8.3 Button Conventions

| Combination | Convention |
|---|---|
| Pause + Option 1 | Game reset |
| Pause + Option 2 | Screen flip (left/right hand toggle) |

### 8.4 Total Button Count

9 buttons total: D-pad (4 directions), A, B, Option 1, Option 2, Pause.

---

## 9. Cartridge / ROM Cart

### 9.1 Cart Address Generator

The cartridge uses an address generator built from:
- An **8-bit shift register** (serial address input via CartAddressData line)
- An **11-bit counter** (auto-incrementing for sequential reads)
- Combined: 8 + 11 = **19 address bits** = 512 KB per strobe

Two chip-select strobes (active-low):
- **CART0/** → first 512 KB bank (RCART0 at $FCB2)
- **CART1/** → second 512 KB bank (RCART1 at $FCB3)

Total addressable: **1 MB**.

### 9.2 Cart Access Timing

| Operation | Timing |
|---|---|
| Cart read | **15 CPU ticks** (392 ns access time at pins) |
| Cart write | Blind write, 562.5 ns strobe, 12-tick lockout after |

### 9.3 Common Cart Sizes

| Size | Configuration |
|---|---|
| 128 KB | Single bank, partial address space |
| 256 KB | Single bank |
| 512 KB | Single bank (full CART0 space) |
| 1 MB | Both banks (CART0 + CART1) |

### 9.4 Cart Power Control

Cart power is managed via the CartAddressData signal:
- **Low** = power on
- Controlled via IODAT bit 1

### 9.5 EEPROM Save

Some cartridges include EEPROM for save game data. Access is through the cart interface using specific protocols defined per-game.

### 9.6 Cart Boot Process

1. Boot ROM shifts the initial load address into the cart address generator
2. Sequential reads pull initial program code from cart into RAM
3. Jump to loaded code

---

## 10. Hardware Revisions (Lynx I vs Lynx II)

| Feature | Lynx I (1989) | Lynx II (July 1991) |
|---|---|---|
| **CPU core** | VL65NC02 (65**SC**02) | Full 65**C**02 |
| **Missing instructions** | WAI, STP, SMBx, RMBx, BBSx, BBRx | None — all 65C02 opcodes |
| **Audio output** | Mono (single headphone channel) | **Stereo** (MSTEREO register functional) |
| **Headphone jack** | Mono | Stereo |
| **Form factor** | Larger, heavier | Smaller, sleeker, rubber grips |
| **Screen** | Backlit LCD | Clearer backlit LCD |
| **Power save** | No | **Backlight off mode** |
| **Battery life** | 4–5 hours (6× AA) | 5–6 hours (6× AA) |
| **Retail price** | $179.95 launch → $99 | $99 |
| **Weight** | Heavier (~740g w/ batteries) | Lighter (~370g w/ batteries) |

### Emulation Implications:

- **Lynx I games** may rely on undefined 65SC02 behavior for opcodes $x7/$xF (treat as NOP)
- **Lynx II games** may use the Rockwell/WDC bit manipulation instructions
- Stereo vs mono is a Mikey register difference, not a fundamental architectural change
- An emulator should support **both** by configuring which CPU variant is active

---

## 11. Boot ROM & Startup Sequence

### 11.1 Boot ROM Location

512 bytes at $FE00–$FFF7, embedded in Mikey silicon. Contains the initial game loading code.

### 11.2 Boot Sequence

1. **Hardware reset** pulls CPU RESET line
2. CPU reads reset vector from $FFFC/$FFFD (pointing into boot ROM)
3. Boot ROM initializes Mikey and Suzy
4. Boot ROM reads **initial program loader** from cartridge:
   - Sets up the cart address generator via serial bit-banging
   - Sequentially reads bytes from cart into RAM
5. After loading, jumps to loaded code
6. Loaded code typically sets up display, loads more data from cart, begins game

### 11.3 SPRINIT Requirement

After power-on, there is a **100 ms hardware settling period** during which Suzy must not be used. After this delay, SPRINIT ($FC83) must be written with **$F3** before any sprite operations.

### 11.4 Boot ROM Bug

The boot ROM sets the IODAT REST pin to **output** mode. This is a bug — it should be set to input for external power detection. Games that need correct external power sensing must fix this themselves.

### 11.5 Overlay After Boot

Once the game's loader has copied necessary data into RAM, it typically sets MAPCTL bits to overlay the boot ROM and possibly Suzy/Mikey spaces with RAM, making the full 64 KB available.

---

## 12. Timing & Bus Architecture

### 12.1 Master Clock

- **16 MHz** crystal oscillator
- Basic timing tick: **62.5 ns** (= 1/16 MHz)

### 12.2 Bus Masters

The system bus has four possible masters, prioritized:

| Priority | Master | Description |
|---|---|---|
| 1 (highest) | **Video DMA** | LCD display data shift-out |
| 2 | **DRAM Refresh** | Automatic DRAM refresh cycles |
| 3 | **CPU** | 65C02 code execution |
| 4 (lowest) | **Suzy** | Sprite engine / math operations |

**Only one master at a time.** Higher-priority masters preempt lower ones.

### 12.3 Suzy Bus Access

- Suzy must be explicitly enabled via SUZYBUSEN ($FC90) bit 0
- Maximum latency from Suzy bus request to grant: **2.5 µs** (40 ticks)
- Suzy is the only bus master controlled by software on/off

### 12.4 CPU / Suzy Interaction

- When Suzy is processing sprites, the CPU is blocked from the bus
- CPU can put itself to sleep (write to CPUSLEEP at $FD91) and wait for Suzy to finish
- **BUG**: CPU sleep is broken in Mikey — CPU won't stay asleep unless Suzy currently has the bus
- Typical pattern: Start sprite processing → CPU sleep → Suzy IRQ wakes CPU

### 12.5 DMA Cycle Stealing

Video DMA and DRAM refresh continually steal cycles from the CPU:
- This reduces effective CPU speed from 4 MHz to approximately **3.6 MHz**
- The cycle stealing is deterministic and can be emulated precisely using timer-based scheduling

### 12.6 Timing Precision Requirements

For accurate emulation:
- Timer resolution: **1 µs** minimum (16 ticks)
- Horizontal line timing: **~159 µs** per line
- Vertical frame: **~16.7 ms** (60 Hz)
- Sprite processing time: variable, depends on sprite complexity
- Math unit timing: 44–400 ticks

---

## 13. Known Hardware Bugs

These bugs must be accurately emulated for compatibility:

### 13.1 CPU Sleep Bug
**Location**: Mikey (CPUSLEEP at $FD91)
**Bug**: The CPU will not remain asleep unless Suzy currently holds the bus. If Suzy isn't active, the CPU immediately wakes.
**Impact**: Games must start Suzy sprite processing BEFORE sleeping the CPU.

### 13.2 UART Interrupt Level Sensitivity
**Location**: Mikey serial controller
**Bug**: TX and RX interrupts are **level-sensitive** instead of edge-sensitive. The interrupt fires continuously as long as the condition is true, not just on the transition.
**Impact**: ComLynx-linked games may hang if interrupt handler doesn't properly manage this.

### 13.3 UART TXD Power-Up State
**Location**: Mikey serial controller
**Bug**: TXD pin powers up as TTL HIGH instead of open-collector state.
**Impact**: Other Lynx units on the ComLynx network will see garbage data at power-on.

### 13.4 Audio Lower Nibble
**Location**: Mikey audio engine
**Bug**: The lower nibble of audio output is processed through a missing inverter in the DAC. The signed-to-unsigned conversion has a discontinuity at the value 8→9 transition.
**Impact**: Audio output has a slight glitch; faithfully reproducing this creates more accurate sound.

### 13.5 Timer Done Bit Not a Pulse
**Location**: Mikey timers
**Bug**: The Timer Done bit in Control B is not automatically cleared (it's not a pulse). It must be manually reset via RESET_DONE in Control A.
**Impact**: Software must explicitly clear the done flag; polling loops must account for this.

### 13.6 Timer Done Requires Clearing to Count
**Location**: Mikey timers
**Bug**: A timer will not continue counting if the Timer Done flag is set. The flag must be cleared before the timer can count again.
**Impact**: Interrupt handlers must always clear Timer Done.

### 13.7 Sprite Shadow Polarity
**Location**: Suzy sprite engine
**Bug**: Shadow sprite polarity is inverted from the documentation. Shadow writes where it shouldn't, and doesn't where it should.
**Impact**: Shadow sprite type behavior must be inverted in the emulator.

### 13.8 Signed Multiply Bugs
**Location**: Suzy math unit
**Bug 1**: $8000 is treated as **positive** (should be the most negative 16-bit signed value).
**Bug 2**: 0 is treated as **negative**.
**Impact**: Signed multiply results are wrong for edge cases. Emulate these exact erroneous behaviors.

### 13.9 Divide Remainder Errors
**Location**: Suzy math unit
**Bug**: The remainder value after division contains errors.
**Impact**: Games shouldn't rely on remainder, but emulate the buggy behavior anyway.

### 13.10 Math Overflow Bit
**Location**: Suzy math unit
**Bug**: The overflow/carry bit from math operations is not permanently saved — it can be lost.
**Impact**: Reading SPRSYS.mathcarry may give incorrect results if not read promptly.

### 13.11 Sizing Algorithm 3 (Broken)
**Location**: Suzy sprite engine
**Bug**: SPRCTL1 sizing algorithm 3 (shift-based sizing) is completely broken in hardware.
**Impact**: All games use algorithm 4 (adder-based). Emulator should emulate the broken behavior or simply treat algo 3 as algo 4.

### 13.12 SCB NEXT Zero-Check Bug
**Location**: Suzy sprite engine
**Bug**: The end-of-chain check only examines the **upper byte** of SCBNEXT. If the upper byte is $00, Suzy stops processing regardless of the lower byte.
**Impact**: SCBs cannot be placed in zero page ($0000–$00FF). SCBNEXT = $00xx always terminates the chain.

### 13.13 Sprite End-of-Data Packet Bug
**Location**: Suzy sprite engine
**Bug**: The 5-bit `00000` end-of-data marker in sprite data has edge-case bugs.
**Impact**: Sprite data must be carefully formatted. Emulate the exact parsing behavior.

---

## 14. LNX File Format

The `.lnx` file format is the standard ROM image format for Atari Lynx cartridges.

### 14.1 Header Structure (64 bytes)

| Offset | Size | Field | Description |
|---|---|---|---|
| 0 | 4 | Magic | ASCII `LYNX` ($4C $59 $4E $58) |
| 4 | 2 | Bank 0 size | Size of bank 0 in 256-byte pages (little-endian) |
| 6 | 2 | Bank 1 size | Size of bank 1 in 256-byte pages (little-endian) |
| 8 | 2 | Version | Header version (typically 1) |
| 10 | 32 | Cart name | Null-terminated ASCII string |
| 42 | 16 | Manufacturer | Null-terminated ASCII string |
| 58 | 1 | Rotation | 0=none, 1=left, 2=right |
| 59 | 5 | Spare | Reserved (zero-filled) |

### 14.2 Data Layout

After the 64-byte header:
- **Bank 0 data**: `bank0_pages × 256` bytes
- **Bank 1 data**: `bank1_pages × 256` bytes (if present)

### 14.3 Rotation Field

| Value | Meaning |
|---|---|
| 0 | No rotation needed |
| 1 | Rotate left 90° (Lynx held vertically, card slot left) |
| 2 | Rotate right 90° (Lynx held vertically, card slot right) |

Games like *Gauntlet III* and *Klax* use rotated display modes.

### 14.4 Loading

1. Read and validate 64-byte header (magic = "LYNX")
2. Calculate bank sizes: `bank0_bytes = bank0_pages * 256`, same for bank1
3. Load ROM data after header
4. Map into cart address space

---

## 15. Emulation Implementation Notes

### 15.1 Architecture Recommendations

Given Nexen's existing multi-system emulator architecture:

| Component | Recommendation |
|---|---|
| **CPU** | Extend existing 6502 core (used for NES) with 65C02 additions. Implement a mode flag for 65SC02-vs-65C02 to handle Lynx I/II differences. |
| **Mikey** | Single class handling timers, audio, video DMA, UART, interrupts, palette. Registers at $FD00–$FDFF. |
| **Suzy** | Single class handling sprite engine, math unit, joystick, cart interface. Registers at $FC00–$FCFF. |
| **Memory** | 64 KB flat RAM + overlay system controlled by MAPCTL. Video DMA bypasses overlays. |
| **Display** | Software-rendered framebuffer (160×102×4bpp). Suzy writes to RAM; video output reads from RAM. |
| **Audio** | 4-channel LFSR + timer synthesis. Mix digitally, apply filter. |

### 15.2 Timing Model

The Lynx requires **cycle-accurate** emulation for:
- Timer cascade chains (especially Timer 0 → Timer 2 for display timing)
- Suzy bus arbitration vs CPU execution
- DMA cycle stealing

Recommended approach: **tick-based scheduler** with 16 MHz resolution (62.5 ns ticks), dispatching events to timers, video DMA, CPU, and Suzy.

### 15.3 Sprite Engine Emulation

The Suzy sprite engine is the most complex component:
1. Parse SCB linked list from RAM
2. For each SCB:
   - Decode sprite control bytes
   - Decompress sprite data (variable bpp, packed/literal)
   - Apply scaling, flipping, tilting
   - Composite to display buffer with type-specific behavior
   - Update collision buffer
3. Track cycle count for bus arbitration

### 15.4 Critical Compatibility Items

These behaviors **must** be emulated for game compatibility:

1. **CPU sleep bug** — many games depend on the exact sleep/wake behavior
2. **Timer done flag** — must be manually cleared
3. **SCB NEXT zero-check** — upper byte only
4. **Math bugs** — signed multiply edge cases
5. **Sprite shadow polarity** — inverted from documentation
6. **MAPCTL overlay** — RAM beneath hardware spaces

### 15.5 Save State Considerations

Lynx save states must capture:
- 64 KB RAM (full, including areas under overlays)
- All Mikey registers (timers, audio channels, palette, UART, interrupts, MAPCTL)
- All Suzy registers (sprite, math, input)
- CPU state (A, X, Y, SP, PC, P)
- Boot ROM state (enabled/disabled via MAPCTL)
- Timer cascading state (each timer's current count, done flags, linking state)
- Audio LFSR state (4 × 12-bit shift registers + accumulators)
- Active sprite processing state (if mid-frame)

### 15.6 Existing Nexen Code to Leverage

Nexen already emulates the PC Engine which uses a **HuC6280** (65C02 derivative at 7.16 MHz). The PCE CPU core likely already implements most 65C02 instructions and could serve as a starting point for the Lynx CPU, needing primarily:
- Clock speed adjustment (4 MHz vs 7.16 MHz)
- Lynx-specific timing (variable tick counts for different access types)
- 65SC02 mode (disable Rockwell/WDC extensions for Lynx I)

---

## Appendix A: Complete Register Map

### Suzy Registers ($FC00–$FCFF)

| Address | Name | R/W | Description |
|---|---|---|---|
| $FC00–$FC01 | TMPADR | R/W | Temporary address |
| $FC02–$FC03 | TILTACUM | R/W | Tilt accumulator |
| $FC04–$FC05 | HOFF | R/W | Horizontal offset |
| $FC06–$FC07 | VOFF | R/W | Vertical offset |
| $FC08–$FC09 | VIDBAS | R/W | Video buffer base |
| $FC0A–$FC0B | COLLBAS | R/W | Collision buffer base |
| $FC0C–$FC0D | VIDADR | R/W | Current video address |
| $FC0E–$FC0F | COLLADR | R/W | Current collision address |
| $FC10–$FC11 | SCBNEXT | R/W | Next SCB pointer |
| $FC12–$FC13 | SPRDLINE | R/W | Sprite data line pointer |
| $FC14–$FC15 | HPOSSTRT | R/W | Horizontal start position |
| $FC16–$FC17 | VPOSSTRT | R/W | Vertical start position |
| $FC18–$FC19 | SPRHSIZ | R/W | Sprite horizontal size |
| $FC1A–$FC1B | SPRVSIZ | R/W | Sprite vertical size |
| $FC1C–$FC1D | STRETCH | R/W | Stretch value |
| $FC1E–$FC1F | TILT | R/W | Tilt value |
| $FC20–$FC21 | SPRDOFF | R/W | Sprite data offset |
| $FC22–$FC23 | SPRVPOS | R/W | Current vertical position |
| $FC24–$FC25 | COLLOFF | R/W | Collision buffer offset |
| $FC26–$FC27 | VSIZACUM | R/W | Vertical size accumulator |
| $FC28–$FC29 | HSIZOFF | R/W | Horizontal size offset |
| $FC2A–$FC2B | VSIZOFF | R/W | Vertical size offset |
| $FC2C–$FC2D | SCBADR | R/W | Current SCB address |
| $FC2E–$FC2F | PROCADR | R/W | Process address |
| $FC52 | MATHD | W | Math D (multiplicand low) |
| $FC53 | MATHC | W | Math C (multiplicand high) |
| $FC54 | MATHB | W | Math B (multiplier low) |
| $FC55 | MATHA | W | Math A (multiplier high) — write triggers multiply |
| $FC56 | MATHP | W | Math P (divisor low) |
| $FC57 | MATHN | W | Math N (divisor high) — write triggers divide |
| $FC60 | MATHH | R/W | Math result byte 0 / dividend byte 0 |
| $FC61 | MATHG | R/W | Math result byte 1 / dividend byte 1 |
| $FC62 | MATHF | R/W | Math result byte 2 / dividend byte 2 |
| $FC63 | MATHE | R/W | Math result byte 3 / dividend byte 3 |
| $FC6C | MATHM | R | Remainder high (unreliable) |
| $FC6D | MATHL | R | Remainder low (unreliable) |
| $FC80 | SPRCTL0 | W | Sprite control 0 |
| $FC81 | SPRCTL1 | W | Sprite control 1 |
| $FC82 | SPRCOLL | W | Sprite collision control |
| $FC83 | SPRINIT | W | Sprite init ($F3 required after boot) |
| $FC88 | SUZYHREV | R | Suzy hardware revision ($01) |
| $FC90 | SUZYBUSEN | R/W | Suzy bus enable |
| $FC91 | SPRGO | W | Start sprite processing |
| $FC92 | SPRSYS | R/W | Sprite system control / status |
| $FCB0 | JOYPAD | R | Joystick state |
| $FCB1 | SWITCHES | R | Switch state |
| $FCB2 | RCART0 | R | Read cart bank 0 |
| $FCB3 | RCART1 | R | Read cart bank 1 |

### Mikey Registers ($FD00–$FDFF)

| Address | Name | R/W | Description |
|---|---|---|---|
| $FD00–$FD03 | TIMER0 | R/W | Timer 0 (Hcount) |
| $FD04–$FD07 | TIMER1 | R/W | Timer 1 |
| $FD08–$FD0B | TIMER2 | R/W | Timer 2 (Vcount) |
| $FD0C–$FD0F | TIMER3 | R/W | Timer 3 |
| $FD10–$FD13 | TIMER4 | R/W | Timer 4 (UART baud) |
| $FD14–$FD17 | TIMER5 | R/W | Timer 5 |
| $FD18–$FD1B | TIMER6 | R/W | Timer 6 |
| $FD1C–$FD1F | TIMER7 | R/W | Timer 7 |
| $FD20–$FD27 | AUDIO0 | R/W | Audio channel 0 |
| $FD28–$FD2F | AUDIO1 | R/W | Audio channel 1 |
| $FD30–$FD37 | AUDIO2 | R/W | Audio channel 2 |
| $FD38–$FD3F | AUDIO3 | R/W | Audio channel 3 |
| $FD50 | MSTEREO | W | Stereo control |
| $FD80 | INTRST | R/W | Interrupt reset |
| $FD81 | INTSET | R/W | Interrupt set |
| $FD87 | SYSCTL1 | W | System control 1 |
| $FD88 | MIKEYHREV | R | Mikey hardware revision ($01) |
| $FD8A | IODIR | R/W | I/O direction |
| $FD8B | IODAT | R/W | I/O data |
| $FD8C | SERCTL | R/W | Serial control / status |
| $FD8D | SERDAT | R/W | Serial data |
| $FD91 | CPUSLEEP | W | CPU sleep trigger |
| $FD92 | DISPCTL | R/W | Display control |
| $FD93 | PBKUP | R/W | LCD timing backup |
| $FD94–$FD95 | DISPADR | R/W | Display buffer address |
| $FDA0–$FDAF | GREEN | W | Palette green values (16 entries) |
| $FDB0–$FDBF | BLUERED | W | Palette blue+red values (16 entries) |
| $FFF9 | MAPCTL | R/W | Memory map control |

---

## Appendix B: References

1. **Original Epyx "Handy" Hardware Specification** — HTML version at monlynx.de/lynx/
   - Main document: http://monlynx.de/lynx/lynx.html
   - Hardware overview: http://monlynx.de/lynx/lynx0.html
   - CPU/ROM: http://monlynx.de/lynx/lynx1.html
   - Hardware addresses: http://monlynx.de/lynx/lynx2.html
   - Display: http://monlynx.de/lynx/lynx3.html
   - Audio: http://monlynx.de/lynx/lynx4.html
   - Sprites & collision: http://monlynx.de/lynx/lynx5.html
   - Timers & interrupts: http://monlynx.de/lynx/lynx8.html
   - UART: http://monlynx.de/lynx/lynx8a.html
   - System reset: http://monlynx.de/lynx/lynx10.html
   - Bus interplay: http://monlynx.de/lynx/lynx11.html
   - Interrupts/CPU sleep: http://monlynx.de/lynx/irq.html
   - Sprite engine appendix: http://monlynx.de/lynx/sprite.html

2. **Wikipedia: Atari Lynx** — https://en.wikipedia.org/wiki/Atari_Lynx

3. **Wikipedia: WDC 65C02** — https://en.wikipedia.org/wiki/WDC_65C02
   - 65SC02 section: variant without WAI, STP, and bit instructions

4. **Atari Lynx Developer Wiki** — https://atarilynxdev.net/wiki/

5. **6502/65C02 Opcode Reference** — http://www.oxyron.de/html/opcodes02.html

---

*End of Technical Report*
