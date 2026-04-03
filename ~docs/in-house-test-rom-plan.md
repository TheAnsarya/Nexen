# In-House Test ROM Development Plan

> Using the 🌷 Flower Toolchain (Poppy, Peony, Pansy) to create custom accuracy test ROMs for under-served systems.

## Motivation

Several systems Nexen supports have very limited or no publicly available test ROMs:

| System | Public Test ROMs | Gap |
|--------|-----------------|-----|
| WonderSwan/WSC | <10 | V30MZ CPU, PPU timing, sound, sprite behavior |
| Atari Lynx | <5 | Suzy graphics, Mikey audio, timer system |
| PC Engine | 10-15 | VDC timing, VCE behavior, CD-ROM subsystem |
| SMS/Game Gear | ~20 | VDP edge cases, FM unit (SMS Mark III) |
| Genesis | ~30 | VDP FIFO, 68K edge cases, Z80 bus arbitration |

Custom test ROMs fill these gaps and also provide a showcase for the Flower Toolchain workflow.

---

## Flower Toolchain Integration

### The Roundtrip Pipeline

```
Custom Test ROM Source (.pasm)
    → 🌸 Poppy (compile) → .nes / .sfc / .gb / .gba / .ws / .lnx
    → 🌺 Peony (disassemble) → verify roundtrip
    → 🌼 Pansy (metadata) → symbols, cross-refs, CDL
```

### Why Poppy for Test ROMs

1. **Multi-architecture:** Poppy already supports 6502, 65816, SM83, 65SC02, ARM7TDMI (and planned: V30MZ, HuC6280, M68000, Z80)
2. **Roundtrip guarantee:** Peony can disassemble the output and Poppy can reassemble it identically
3. **Pansy metadata:** Automatic symbol/CDL generation for debugging test ROMs in Nexen
4. **Dogfooding:** Building test ROMs exercises the entire Flower Toolchain

### Architecture Support Status

| Architecture | Poppy Status | Peony Status | Test ROM Priority |
|--------------|-------------|-------------|-------------------|
| 6502 (NES) | ✅ Supported | ✅ Supported | Low (many public tests exist) |
| 65816 (SNES) | ✅ Supported | ✅ Supported | Medium (some gaps) |
| SM83 (GB) | ✅ Supported | ✅ Supported | Low (mooneye covers most) |
| 65SC02 (Lynx) | ✅ Supported | ✅ Supported | High (very few public tests) |
| ARM7TDMI (GBA) | 🔄 In progress | 🔄 In progress | Medium (jsmolka + mGBA cover basics) |
| V30MZ (WS) | ❌ Planned | ❌ Planned | 🔴 Critical (almost no public tests) |
| HuC6280 (PCE) | ❌ Planned | ❌ Planned | High (limited tests) |
| M68000 (Genesis) | ❌ Planned | ❌ Planned | Medium |
| Z80 (SMS) | ❌ Planned | ❌ Planned | Medium |

---

## Test ROM Design Patterns

### Pattern 1: Memory Result Test

The simplest pattern — used by blargg's NES tests. Write a result byte to a known address.

```
; test-template.pasm (NES example)
.org $c000

test_start:
    ; Run test logic
    lda #$01        ; test passed
    sta $6000       ; write result
    jmp test_done

test_failed:
    lda #$ff        ; test failed
    sta $6000

test_done:
    jmp test_done   ; infinite loop - emulator reads $6000
```

**Automation:** `RunTest("test.nes", 0x6000, NesWorkRam)` → check result

### Pattern 2: Register Protocol (Mooneye-style)

Write Fibonacci sequence to CPU registers for pass, 0x42 for fail.

```
; GB test using Mooneye protocol
pass:
    ld b, 3
    ld c, 5
    ld d, 8
    ld e, 13
    ld h, 21
    ld l, 34
    ld b, b     ; debug breakpoint

fail:
    ld b, $42
    ld c, $42
    ld d, $42
    ld e, $42
    ld h, $42
    ld l, $42
    ld b, b     ; debug breakpoint
```

**Automation:** Detect `LD B, B` breakpoint, read registers

### Pattern 3: Screen-Based Test

Write PASS/FAIL text to screen, optionally use specific tile patterns for hash comparison.

```
; Display "PASS" in green or "FAIL" in red
; Automation: screenshot hash comparison against reference
```

### Pattern 4: Multi-Test Suite

Multiple tests in one ROM with menu navigation (like AccuracyCoin).

```
; Run all tests sequentially
; Write per-test results to a results table in RAM
; Write summary byte at known address
; Automation: movie file for navigation + memory read for results
```

---

## Priority Test ROM Development

### Phase 1: WonderSwan Test ROMs (Critical)

WS/WSC has the least test coverage. These require V30MZ support in Poppy (or hand-assembled binaries).

| Test ROM | Tests | Priority |
|----------|-------|----------|
| ws-cpu-basic | V30MZ basic instruction execution | 🔴 Critical |
| ws-cpu-flags | Flag behavior for all ALU operations | 🔴 Critical |
| ws-ppu-timing | Scanline timing, HBlank/VBlank | 🔴 Critical |
| ws-sprite-basic | Sprite rendering, priority, limits | High |
| ws-sprite-copy | Line 144 sprite table copy verification | High |
| ws-bg-scroll | Background scrolling accuracy | High |
| ws-sound-basic | Sound channel frequency and volume | Medium |
| ws-timer | Timer interrupt accuracy | Medium |
| ws-dma | DMA transfer behavior | Medium |
| ws-serial | Serial communication | Low |

### Phase 2: Atari Lynx Test ROMs (High)

Lynx has the 65SC02 CPU (already supported by Poppy) but custom Suzy/Mikey chips need specific tests.

| Test ROM | Tests | Priority |
|----------|-------|----------|
| lynx-cpu-65sc02 | 65SC02 instruction set (adapted from 6502 tests) | High |
| lynx-suzy-sprite | Sprite rendering engine | High |
| lynx-suzy-collision | Collision detection | High |
| lynx-mikey-timer | Timer system accuracy | High |
| lynx-mikey-audio | Audio channel behavior | Medium |
| lynx-display-timing | Display timing and refresh | Medium |

### Phase 3: PC Engine Test ROMs (High)

| Test ROM | Tests | Priority |
|----------|-------|----------|
| pce-cpu-huc6280 | HuC6280 instructions (65SC02 + extras) | High |
| pce-vdc-timing | VDC (HuC6270) scanline timing | High |
| pce-vdc-sprite | Sprite behavior and limits | High |
| pce-vce-color | VCE color output | Medium |
| pce-psg-basic | PSG sound output | Medium |
| pce-timer | Timer behavior | Medium |

### Phase 4: Gap-Filling for Well-Covered Systems

| System | Test ROM | Tests | Priority |
|--------|----------|-------|----------|
| NES | nes-edge-cases | Edge cases not in AccuracyCoin | Low |
| SNES | snes-coprocessor-edge | SA-1, DSP-1 edge cases | Medium |
| GB | gb-cgb-quirks | CGB-specific timing quirks | Low |
| GBA | gba-dma-timing | DMA timing edge cases | Medium |

---

## Development Workflow

### For Poppy-Supported Architectures

```powershell
# 1. Write test ROM source
# Edit: test-roms/ws/ws-cpu-basic.pasm

# 2. Compile with Poppy
poppy build test-roms/ws/ws-cpu-basic.pasm --output test-roms/ws/ws-cpu-basic.ws --pansy

# 3. Verify with Peony (roundtrip guarantee)
peony disasm test-roms/ws/ws-cpu-basic.ws --output test-roms/ws/verify/

# 4. Test in Nexen
Nexen.exe test-roms/ws/ws-cpu-basic.ws test-roms/ws/ws-cpu-basic.lua --novideo --noaudio --timeout=30

# 5. Add to test manifest
# Edit: tests/accuracy/manifest.json
```

### For Architectures Not Yet in Poppy

Until V30MZ/HuC6280/M68000/Z80 are added to Poppy:

1. **Hand-assemble** test binaries using platform-specific assemblers (WLA-DX, NASM, DASM, etc.)
2. **Document** the assembly process in each test ROM's directory
3. **Plan migration** to Poppy once architecture support is added
4. **Store source** as `.asm` (legacy) alongside the binary, with `TODO: migrate to .pasm` comment

---

## Game Garden Integration

Test ROMs developed in-house can be managed as Game Garden projects:

```
game-garden/games/ws/test-roms/
├── README.md
├── build.ps1
├── build.config.json
├── src/
│   ├── ws-cpu-basic.pasm
│   ├── ws-ppu-timing.pasm
│   └── includes/
│       └── ws-header.pasm
├── metadata/
│   └── ws-cpu-basic.pansy
├── assets/
│   └── editable/
│       └── graphics/
│           └── font.png
└── build/
    ├── ws-cpu-basic.ws
    └── ws-ppu-timing.ws
```

This provides the full Game Garden workflow: build, verify, track status.

---

## Hardware Verification

For test ROMs to be truly useful, results should be verified on real hardware:

| System | Flash Cart | Status |
|--------|-----------|--------|
| NES | EverDrive N8 Pro | Available |
| SNES | FXPak Pro | Available |
| GB/GBC | EverDrive GB X7 | Available |
| GBA | EverDrive GBA X5 | Available |
| Genesis | Mega EverDrive Pro | Available |
| SMS | Master EverDrive | Available |
| PCE | Turbo EverDrive Pro | Check |
| WS | WS Flash Masta | Rare — may need custom solution |
| Lynx | Custom flash cart | Very rare |

**Strategy:** Prioritize emulator cross-reference (ares, mednafen, MAME) for systems where flash carts are unavailable. Use real hardware when accessible.

---

## Related Documents

- [Accuracy Test ROM Catalog](accuracy-test-rom-catalog.md) — Public test ROM catalog
- [Automated Accuracy Testing Plan](automated-accuracy-testing-plan.md) — Automation infrastructure
- [Accuracy Testing Pipeline](accuracy-testing-pipeline.md) — Step-by-step commands
