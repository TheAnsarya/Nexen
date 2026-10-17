# Channel F Remaining Features Status

## Overview

Status of remaining open Channel F issues. Created 2026-03-31 to track what's actionable now vs. what needs prerequisites.

## Open Issues Summary

### Core Emulation (Workstream 1)

| # | Title | Status | Prerequisites | Actionable? |
|---|-------|--------|---------------|-------------|
| #974 | PSU/SMI/DMI port stubs | Open | CPU core (#972/#973 done) | ✅ Yes |
| #976 | Cart board types | Open | Memory manager done | ✅ Yes |
| #977 | Video pipeline | Open | CPU + memory done | ✅ Yes |
| #979 | Audio generation | Open | CPU + timing done | ✅ Yes |

### Platform Integration (Workstream 2)

| # | Title | Status | Prerequisites | Actionable? |
|---|-------|--------|---------------|-------------|
| #1001 | Debug panes | Open | Debugger infrastructure | ⚠️ Needs scaffold |
| #1006 | SRAM persistence | Open | Cart types (#976) | ⚠️ After #976 |
| #1007 | Console variants | Open | Core emulation done | ⚠️ After core |

### TAS & QA (Workstream 3)

| # | Title | Status | Prerequisites | Actionable? |
|---|-------|--------|---------------|-------------|
| #1011 | TAS Editor | Open | Movie recording | ⚠️ Needs movie first |
| #1017 | Diff testing harness | Open | Save states working | ⚠️ After states |

## Priority Order

1. **#977 Video pipeline** (HIGH) — Most visible, needed for any visual validation
2. **#979 Audio generation** (HIGH) — Completes A/V for basic playability
3. **#976 Cart board types** (MEDIUM) — Needed for games beyond BIOS demos
4. **#974 PSU/SMI/DMI stubs** (MEDIUM) — Port stubs for hardware completeness
5. **#1001 Debug panes** (LOW) — Quality of life, not blocking
6. **#1006 SRAM persistence** (LOW) — Few games use SRAM
7. **#1007 Console variants** (LOW) — System II differences
8. **#1017 Diff testing** (LOW) — QA tooling
9. **#1011 TAS Editor** (LOW) — After movie support

## Architecture Notes

### Video (#977)

The Channel F uses a unique 128×64 display with 2bpp color (4 colors from 8-color palette). VRAM is 8192 bytes. No sprites, no scrolling — the CPU directly writes to VRAM.

Key implementation points:

- Frame buffer: 128×64 × 2bpp = 8192 bytes
- Colors: black, red, green, blue (+ background variants)
- No hardware sprites — all rendering is CPU-driven
- Scanline-accurate timing: CyclesPerFrame = 29829 at 1.7898 MHz
- Output to DefaultVideoFilter for upscaling

### Audio (#979)

The Channel F has a simple tone generator:

- Single-channel square wave
- Frequency controlled by port writes
- Volume is on/off (no volume levels)
- 500Hz and 1000Hz tones for most games

### Cart Types (#976)

Standard Channel F carts are simple ROM:

- $0800-$FFFF: Cart ROM space (up to ~62KB)
- Most carts are 2KB-16KB
- No mapper/banking for standard carts
- Some late-era carts may have banking

### PSU/SMI/DMI (#974)

F8 CPU has 6 I/O ports (0-5):

- Port 0: Controller 1 (active low directional + push/pull/twist)
- Port 1: Controller 2
- Port 4: Controller 1 (active high, active during read)
- Port 5: Audio output
- Ports 2-3: Unused on standard hardware
