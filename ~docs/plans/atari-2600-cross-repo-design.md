# Atari 2600 Cross-Repo Design — Flower Toolchain Integration

## Overview

This document describes the Atari 2600 support architecture across all four repositories in the flower toolchain: **Nexen** (emulator), **Pansy** (metadata format), **Poppy** (assembler), and **Peony** (disassembler).

## Current State (Phase 1 — Complete)

### Nexen (Emulator)

- Full Atari 2600 emulation: TIA, RIOT, 6507 CPU (via 6502)
- 5 controller types: Joystick, Paddle, Keypad, Driving, Booster Grip
- Multi-controller config UI with per-port assignment
- Bankswitching: F8, F6, F4, FE, E0, 3F, E7, F0, UA, CV, FA, EF, DPC
- Save state, audio, rendering all functional
- 52 controller unit tests (Epic 24 — #864)
- 87+ total test cases across CPU, mapper, TIA, RIOT, audio, render

### Pansy (Metadata Format)

- Platform ID: `0x08` (`PLATFORM_ATARI_2600`)
- Memory regions: TIA (`$00`-`$7f`), RAM (`$80`-`$ff`), RIOT (`$0280`-`$0297`), ROM (`$f000`-`$ffff`)
- 40+ TIA write register symbols + RIOT register symbols
- Interrupt vector symbols (NMI, RESET, IRQ)
- Epic #81: Phase 2 — TIA read registers, bit field metadata, comprehensive tests

### Poppy (Assembler)

- InstructionSet6507 (delegates to 6502 — functionally identical)
- Atari2600RomBuilder: 8 bankswitching schemes (None, F8, F6, F4, FE, E0, 3F, E7)
- `includes/atari2600/tia.pasm`: 150+ lines with all TIA, RIOT registers and constants
- `.a26` output format, aliases "atari2600"/"2600"/"6507"
- Epic #190: Phase 2 — comprehensive bankswitching tests, RIOT include, audio helpers

### Peony (Disassembler)

- Atari2600Analyzer: 44 TIA write + 14 TIA read + RIOT registers
- 10 bankswitching scheme detection (F8, F6, F4, 3F, E0, FE, E7, F0, UA, CV)
- Cpu6502Decoder: full 6502 instruction decoding
- GetRegisterLabel() auto-labeling, GetMemoryRegion() classification
- Epic #129: Phase 2 — graphics detection, controller heuristics, audio patterns

## Phase 2 Roadmap

### Tier 1: Register Completeness & Tests

| Repo | Issue | Description |
|------|-------|-------------|
| Pansy | #82 | TIA read register symbols (CXM0P..INPT5) |
| Pansy | #83 | Comprehensive symbol tests |
| Poppy | #191 | Bankswitching test suite (all 7 schemes) |
| Poppy | #192 | ROM size validation and vector tests |
| Peony | #131 | Bankswitching validation tests (all 10 schemes) |

### Tier 2: Analysis & Detection

| Repo | Issue | Description |
|------|-------|-------------|
| Pansy | #84 | Register bit field descriptions |
| Poppy | #193 | Separate RIOT include file |
| Peony | #130 | Graphics data detection (GRP/PF patterns) |
| Peony | #132 | Controller type detection heuristics |
| Peony | #133 | Audio code detection and labeling |

### Tier 3: Advanced Features

| Repo | Description |
|------|-------------|
| Pansy | Bankswitching metadata section |
| Poppy | PAL timing variants |
| Peony | WSYNC/HMOVE cycle timing annotation |
| Peony | Collision group recognition |
| Peony | Stack usage analysis |

## Data Flow

```
ROM (.a26)
    │
    ▼
🌺 Peony (disassemble)
    ├── Source (.pasm files)
    ├── 🌼 Pansy metadata (.pansy)
    │   ├── Symbols (register labels, subroutine names)
    │   ├── Code/Data Map (CDL flags)
    │   ├── Cross-References (JSR/JMP/branch targets)
    │   ├── Memory Regions (TIA/RAM/RIOT/ROM)
    │   └── Metadata (platform, author, version)
    └── Analysis hints
         ├── Bankswitching scheme detected
         ├── Graphics data regions identified
         ├── Controller type inferred
         └── Audio routines labeled
    │
    ▼
Edit source + assets
    │
    ▼
🌸 Poppy (assemble)
    ├── ROM (.a26) — byte-identical roundtrip
    └── 🌼 Pansy metadata (.pansy) — symbols from assembly
    │
    ▼
🎮 Nexen (emulate + test)
    ├── Run assembled ROM
    ├── Export 🌼 Pansy metadata (runtime analysis)
    └── Verify emulation accuracy
```

## Bankswitching Scheme Parity

| Scheme | Nexen | Poppy | Peony |
|--------|-------|-------|-------|
| None (2K/4K) | ✅ | ✅ | ✅ |
| F8 (8K) | ✅ | ✅ | ✅ |
| F6 (16K) | ✅ | ✅ | ✅ |
| F4 (32K) | ✅ | ✅ | ✅ |
| FE (Activision) | ✅ | ✅ | ✅ |
| E0 (Parker Bros) | ✅ | ✅ | ✅ |
| 3F (Tigervision) | ✅ | ✅ | ✅ |
| E7 (M-Network) | ✅ | ✅ | ✅ |
| F0 (Megaboy) | ✅ | ❌ | ✅ |
| UA (UA Limited) | ✅ | ❌ | ✅ |
| CV (Commavid) | ✅ | ❌ | ✅ |
| FA (CBS RAM+) | ✅ | ❌ | ❌ |
| EF (64K) | ✅ | ❌ | ❌ |
| DPC (Pitfall II) | ✅ | ❌ | ❌ |

## Controller Type Parity

| Controller | Nexen | Poppy | Peony |
|------------|-------|-------|-------|
| Joystick | ✅ Emulated + Tested | ✅ Include defs | ❌ No detection |
| Paddle | ✅ Emulated + Tested | ✅ Include defs | ❌ No detection |
| Keypad | ✅ Emulated + Tested | ✅ Include defs | ❌ No detection |
| Driving | ✅ Emulated + Tested | ❌ No defs | ❌ No detection |
| Booster Grip | ✅ Emulated + Tested | ❌ No defs | ❌ No detection |

## Related Issues

- Nexen Epic 24: #864 (closed — 52 controller tests)
- Pansy Epic: #81 (open — 3 sub-issues)
- Poppy Epic: #190 (open — 3 sub-issues)
- Peony Epic: #129 (open — 4 sub-issues)
