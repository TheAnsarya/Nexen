# Nexen — Long-Term Emulation Roadmap: Missing Platforms

## Overview

Nexen currently emulates 8 systems: NES, SNES, Game Boy, GBA, SMS, PCE, WonderSwan, Atari Lynx.
Two platforms are supported by Poppy/Peony/Pansy but not by Nexen:

- **Atari 2600** (MOS 6507)
- **Sega Genesis / Mega Drive** (Motorola M68000 + Zilog Z80 sound)

Emulation is a much larger engineering effort than disassembly or assembly tooling.
These are long-term goals only — no implementation in the near term.

## Atari 2600 (Feasibility: Medium)

### CPU: MOS 6507

- Subset of 6502 — only 13 address lines (8KB address space)
- Nexen already has a full 6502 core (`NesCpu`) that could be adapted
- No illegal opcode differences — 6507 is a pin-reduced 6502

### Hardware

- **TIA** (Television Interface Adapter): custom video/audio/input chip
  - Scanline-based rendering (no framebuffer)
  - Player/missile/ball sprite system
  - Collision detection registers
- **RIOT** (RAM-I/O-Timer): 128 bytes RAM, I/O ports, timer
- No PPU in the traditional sense — CPU must race the beam

### Challenges

- **Cycle-exact CPU required**: programs count cycles to position sprites
- **No vertical blank interrupt in early games**: CPU must manually sync
- **Bankswitching**: Many mapper variants (F8, F6, F4, FE, E0, 3F, etc.)
- **Analog paddle input**: Timing-based measurement

### Estimated Effort: Medium

- CPU core: Low (reuse 6502 with address masking)
- TIA: High (cycle-exact scanline rendering is complex)
- RIOT: Low
- Mappers: Medium (many simple variants)

## Sega Genesis / Mega Drive (Feasibility: Hard)

### CPU: Motorola 68000 (primary) + Zilog Z80 (sound)

- Completely new 16/32-bit CPU architecture
- 68000: 56 instruction types, 14 addressing modes, 8 data + 8 address registers
- Z80: secondary CPU for backward compatibility and sound processing
- Both CPUs share bus with arbitration

### Hardware

- **VDP** (Video Display Processor): Based on TMS9918, 2 scroll planes + sprite plane
  - 64 colors from 512 palette
  - DMA for VRAM fills, copies, 68K-to-VRAM transfers
  - H/V interrupts, HBlank/VBlank timing
- **YM2612** (FM Synthesis): 6-channel FM sound, DAC channel
- **SN76489** (PSG): 4-channel PSG (3 square + 1 noise), shared with SMS
- **Z80 subsystem**: 8KB RAM, controls YM2612 and SN76489

### Challenges

- **Two CPUs**: 68000 + Z80 running simultaneously with bus sharing
- **New CPU architecture**: M68000 has no existing core in Nexen
- **VDP complexity**: DMA timing, scroll effects, sprite limits
- **FM synthesis**: YM2612 is notoriously difficult to emulate accurately
- **Sega CD / 32X**: Extensions add even more hardware

### Estimated Effort: Very High

- 68000 CPU core: Very High (new architecture from scratch)
- Z80 CPU core: Medium-High (new architecture, used for SMS already)
- VDP: High
- YM2612 + PSG: High
- Bus arbitration: Medium
- Overall: This is essentially a new emulator

## Recommendations

1. **Atari 2600 first** — Simpler system, reuses existing 6502 knowledge
2. **Genesis later** — Requires M68000 core which is a major undertaking
3. Both should be gated by community demand and contributor interest
