# Atari Lynx — Accuracy Status

This document tracks the accuracy status of each Lynx hardware subsystem.

**Legend:**
- ✅ Fully implemented and tested
- ⚠️ Partially implemented or minor gaps
- ❌ Missing or not implemented
- 🔬 Needs investigation

## CPU (65SC02)

| Feature | Status | Notes |
|---------|--------|-------|
| All 6502 instructions | ✅ | Base NMOS instruction set |
| CMOS extensions (BRA, PHX/Y, etc.) | ✅ | 65SC02-specific opcodes |
| Zero-page indirect `(zp)` | ✅ | CMOS addressing mode |
| TRB/TSB instructions | ✅ | Test and Reset/Set Bits |
| Decimal mode | ✅ | BCD arithmetic |
| Cycle-accurate timing | ✅ | 4 MHz derived from 16 MHz master |
| IRQ/NMI vectors | ✅ | Mapped through MAPCTL overlay |
| Undocumented opcodes | ✅ | NOPs (65SC02 has no undefined behavior traps) |

**CPU Accuracy: 100%**

## Timers (Mikey)

| Feature | Status | Notes |
|---------|--------|-------|
| 8 independent timers | ✅ | Timers 0-7 |
| Prescaler selection (7 sources) | ✅ | 1μs through 64μs |
| Timer linking/cascade | ✅ | Prescaler 7 = linked mode |
| Timer interrupts | ✅ | Per-timer IRQ enable/status |
| Timer 0 = HBlank | ✅ | Display timing |
| Timer 2 = VBlank | ✅ | Frame timing / display refresh |
| Timer 4-7 = Audio | ✅ | Audio channel clocking |
| Count enable/disable | ✅ | CTLA bit control |
| Reload on underflow | ✅ | Backup register auto-load |
| Clock source change mid-timer | ⚠️ | May cause spurious tick burst (#417) |
| LastTick update on CTLA write | ⚠️ | Not updated, off-by-one risk (#M6) |

**Timer Accuracy: ~90%**

## Display (Mikey)

| Feature | Status | Notes |
|---------|--------|-------|
| 160×102 resolution | ✅ | Correct buffer dimensions |
| 4096-color palette (12-bit RGB) | ✅ | Green/Blue/Red register banks |
| 16-color per-frame palette | ✅ | Pen 0-15 mapping |
| Frame buffer address | ✅ | DISPADDR register |
| Display DMA | ⚠️ | Renders whole frame, not scanline-by-scanline |
| DISPCTL register | ⚠️ | Bits 1-3 (color/mono, flip, resolution) not fully decoded |
| 75 Hz refresh rate | ✅ | Correct frame timing |

**Display Accuracy: ~80%**

## Sprites (Suzy)

| Feature | Status | Notes |
|---------|--------|-------|
| SCB linked list traversal | ✅ | Next pointer chain |
| All 8 sprite types | ✅ | Background through Shadow |
| 1/2/3/4 BPP | ✅ | Variable bits-per-pixel |
| Horizontal/Vertical flip | ✅ | HFLIP/VFLIP in SPRCTL0 |
| Sprite scaling (stretch) | ✅ | H/V stretch factors |
| Sprite tilt | ✅ | Tilt factor for rotation |
| Collision detection buffer | ✅ | Per-pixel collision depository |
| Pen index remapping | ⚠️ | Register write routing issue (#416) |
| LeftHand mode | ⚠️ | Flag read but not applied to X coords (#415) |
| SUZYBUSEN control | ⚠️ | Register write not handled (#M5) |
| Safety limit (256 sprites) | ⚠️ | Artificial cap, hardware has no limit |
| Bug 13.12: NEXT upper byte | ✅ | End-of-chain only checks upper byte |

**Sprite Accuracy: ~85%**

## Math Coprocessor (Suzy)

| Feature | Status | Notes |
|---------|--------|-------|
| 16×16 unsigned multiply | ✅ | Result in MATHD:MATHC |
| 16×16 signed multiply | ✅ | Sign-magnitude handling |
| 32/16 unsigned divide | ✅ | Quotient + remainder |
| Accumulate mode | ✅ | Products added to accumulator |
| Sign flag handling | ✅ | Bit 7 of operand registers |
| Bug 13.8: $8000 positive | ✅ | Sign-magnitude edge case |
| Bug 13.9: Positive remainder | ✅ | Division remainder always positive |
| Bug 13.10: Overflow overwrite | ✅ | MATHC/D status not OR'd |
| Overflow detection | ✅ | MATHC/D register overlap |

**Math Accuracy: 100%**

## Audio (Mikey)

| Feature | Status | Notes |
|---------|--------|-------|
| 4 LFSR channels | ✅ | 12-bit Linear Feedback Shift Register |
| Feedback tap selection | ✅ | Configurable LFSR taps |
| 7-bit volume per channel | ✅ | Register-controlled |
| Stereo attenuation (4-bit) | ✅ | Left/Right panning |
| Integration mode | ✅ | Waveform accumulation |
| Timer-driven clocking | ✅ | Timers 4-7 drive channels |
| Channel 3 DAC/PCM | ⚠️ | Not explicitly differentiated from LFSR mode |
| Attenuation linearity | ⚠️ | Scale may be asymmetric for left/right |

**Audio Accuracy: ~90%**

## Memory System

| Feature | Status | Notes |
|---------|--------|-------|
| 64 KB work RAM | ✅ | Full address space |
| MAPCTL register | ✅ | Enable/disable hardware overlays |
| Suzy address space ($FC00) | ✅ | Register-mapped I/O |
| Mikey address space ($FD00) | ✅ | Register-mapped I/O |
| Boot ROM overlay ($FE00) | ✅ | 512 bytes, overlays RAM |
| Vector space ($FFF8) | ✅ | CPU vectors in overlay |
| MAPCTL bits 4-5 | ⚠️ | Read but not gated (#414) |
| HLE boot (skip ROM) | ⚠️ | Doesn't copy cart data to RAM (#C1) |

**Memory Accuracy: ~90%**

## Input

| Feature | Status | Notes |
|---------|--------|-------|
| D-pad (Up/Down/Left/Right) | ✅ | 4 directional buttons |
| A, B buttons | ✅ | Face buttons |
| Option 1, Option 2 | ✅ | Shoulder/option buttons |
| Pause button | ✅ | Via switch register |
| Joypad register ($FCB0) | ✅ | Button bit positions correct |
| Switch register ($FCB1) | ✅ | Pause + cart pin sense |
| Input polled before frame | ✅ | No 1-frame lag |
| Left-handed rotation | ⚠️ | Hardware flag not applied to controls |

**Input Accuracy: ~95%**

## EEPROM

| Feature | Status | Notes |
|---------|--------|-------|
| 93C46 (64 words) | ✅ | Standard size |
| 93C66 (128 words) | ✅ | Extended size |
| 93C86 (256 words) | ✅ | Large size |
| Serial protocol | ✅ | Clock/data/CS signaling |
| Read command | ✅ | Word-at-a-time reads |
| Write command | ✅ | Word writes with enable |
| Erase command | ✅ | Word erase |
| Write enable/disable | ✅ | EWEN/EWDS commands |

**EEPROM Accuracy: 100%**

## UART (ComLynx)

| Feature | Status | Notes |
|---------|--------|-------|
| Basic UART registers | ✅ | SERCTL, SERDAT |
| Baud rate (62500) | ✅ | Standard ComLynx speed |
| TX/RX interrupts | ⚠️ | Basic implementation |
| Multi-player protocol | ⚠️ | UART exists but no network layer |
| Error detection | ⚠️ | Frame/overrun errors basic |

**UART Accuracy: ~60%**

## Cartridge

| Feature | Status | Notes |
|---------|--------|-------|
| ROM loading | ✅ | LNX/LYX format support |
| Header parsing | ✅ | 64-byte LNX header |
| Bank switching | ✅ | Multi-bank cartridges |
| Decryption | ✅ | Encrypted cart support |
| Cart pin sense | ✅ | AUDIN/cart-present detection |

**Cart Accuracy: 100%**

## Overall Accuracy Summary

| Subsystem | Accuracy | Tests |
|-----------|----------|-------|
| CPU (65SC02) | 100% | ~50 |
| Math Coprocessor | 100% | ~200 |
| EEPROM | 100% | ~30 |
| Cartridge | 100% | ~20 |
| Input | 95% | ~30 |
| Timers | 90% | ~40 |
| Memory | 90% | ~60 |
| Audio | 90% | ~40 |
| Sprites | 85% | ~100 |
| Display | 80% | ~20 |
| UART | 60% | ~10 |
| **Overall** | **~90%** | **828** |

## GitHub Issues for Remaining Work

See [Epic #409](https://github.com/TheAnsarya/Nexen/issues/409) for the full list of tracked accuracy improvements.

| Issue | Title | Severity |
|-------|-------|----------|
| #414 | MAPCTL bits 4-5 not implemented | CRITICAL |
| #415 | LeftHand mode not applied to sprite X coordinates | CRITICAL |
| #416 | Pen index register writes go to TempAddress | HIGH |
| #417 | Timer clock source change causes spurious tick burst | HIGH |
