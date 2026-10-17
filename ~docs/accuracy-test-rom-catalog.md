# Accuracy Test ROM Catalog

> Comprehensive catalog of hardware accuracy test ROMs for all systems supported by Nexen.

## Overview

This document catalogs all known publicly available test ROMs for validating emulator accuracy across every system Nexen supports. Each entry includes the test name, author, what it tests, source URL, and automation notes.

**Target directory:** `c:\reference-roms\` organized by system.

---

## NES (6502 / RP2A03)

The NES has the most comprehensive test ROM ecosystem of any retro system, with hundreds of tests covering CPU, PPU, APU, mappers, and input.

### Primary Sources

- **NESdev Wiki — Emulator tests:** [nesdev.org/wiki/Emulator_tests](https://www.nesdev.org/wiki/Emulator_tests)
- **nes-test-roms archive:** [github.com/christopherpow/nes-test-roms](https://github.com/christopherpow/nes-test-roms) — mirrors many dead download links
- **AccuracyCoin:** [github.com/100thCoin/AccuracyCoin](https://github.com/100thCoin/AccuracyCoin) — 137 tests on a single NROM cart

### CPU Tests

| Test | Author | Tests | Automation |
|------|--------|-------|------------|
| nestest | kevtris | Comprehensive CPU operation (start at $c000, compare log) | Log comparison |
| instr_test_v5 | blargg | All official+unofficial opcodes; writes results to $6000 | Memory read at $6000 |
| instr_timing | blargg | Timing of all instructions including unofficial | Screen hash or $6000 |
| cpu_timing_test6 | blargg | Instruction timing (all except branch/halt) | Screen hash |
| cpu_interrupts_v2 | blargg | IRQ and NMI behavior and timing | Screen hash or $6000 |
| cpu_reset | blargg | Register state after power/reset, RAM preservation | Screen hash |
| cpu_dummy_reads | blargg | CPU dummy read behavior | Screen hash |
| cpu_dummy_writes | bisqwit | CPU dummy write behavior | Screen hash |
| cpu_exec_space | bisqwit | Execute code from any memory location including I/O | Screen hash |
| branch_timing_tests | blargg | Branch instruction timing edge cases | Screen hash |
| instr_misc | blargg | Misc instruction behavior (16-bit wrap, dummy reads) | Screen hash |
| AccuracyCoin (Pages 1-2) | 100thCoin | ROM writability, RAM mirroring, PC wraparound, decimal flag, B flag, dummy reads/writes, open bus, all NOPs, addressing wraparound | PASS/FAIL on screen + error codes in RAM |
| AccuracyCoin (Pages 3-11) | 100thCoin | All unofficial instructions: SLO, RLA, SRE, RRA, SAX, LAX, DCP, ISC, ANC, ASR, ARR, ANE, LXA, AXS, SBC, SHA, SHX, SHY, SHS, LAE | PASS/FAIL on screen |
| AccuracyCoin (Page 12) | 100thCoin | CPU interrupts: interrupt flag latency, NMI overlap BRK, NMI overlap IRQ | PASS/FAIL on screen |
| AccuracyCoin (Page 20) | 100thCoin | Instruction timing (cycle-accurate), implied dummy reads, branch dummy reads, JSR edge cases | PASS/FAIL on screen |

### PPU Tests

| Test | Author | Tests | Automation |
|------|--------|-------|------------|
| ppu_vbl_nmi | blargg | VBL flag, NMI enable, NMI interrupt (PPU-cycle accuracy) | Screen hash or $6000 |
| ppu_sprite_hit | blargg | Sprite 0 hit behavior and timing | Screen hash or $6000 |
| sprite_overflow_tests | blargg | Sprite overflow behavior and timing | Screen hash or $6000 |
| oam_read | blargg | OAM reading via $2004 | Screen hash |
| oam_stress | blargg | OAM address and read/write stress test | Screen hash |
| ppu_open_bus | blargg | PPU open bus behavior on reads | Screen hash |
| ppu_read_buffer | bisqwit | $2007 read buffer behavior | Screen hash |
| nmi_sync | blargg | NMI timing verification via screen pattern | Screen hash |
| scanline | Quietust | Rendering accuracy — glitches indicate emulation errors | Visual comparison |
| sprdma_and_dmc_dma | blargg | Cycle stealing of DMC DMA during sprite DMA | Screen hash |
| AccuracyCoin (Page 16) | 100thCoin | CHR ROM writability, PPU register mirroring, open bus, read buffer, palette RAM quirks, rendering flag behavior, $2007 read w/ rendering | PASS/FAIL |
| AccuracyCoin (Page 17) | 100thCoin | VBlank beginning/end timing, NMI control, NMI timing, NMI suppression | PASS/FAIL |
| AccuracyCoin (Page 18) | 100thCoin | Sprite overflow, sprite 0 hit, $2002 flag timing, suddenly resize sprite, arbitrary sprite zero, misaligned OAM, $2004 behavior, OAM corruption, INC $4014 | PASS/FAIL |
| AccuracyCoin (Page 19) | 100thCoin | Attributes as tiles, t register quirks, stale BG shift registers, BG serial in, sprites on scanline 0, $2004 stress test, $2007 stress test | PASS/FAIL |

### APU Tests

| Test | Author | Tests | Automation |
|------|--------|-------|------------|
| apu_test | blargg | Many APU aspects visible to CPU | Screen hash |
| apu_mixer | blargg | Sound channel mixer, relative volumes, non-linear mixing | Audio comparison |
| apu_reset | blargg | Initial APU state at power, effect of reset | Screen hash |
| blargg_apu_2005.07.30 | blargg | Length counters, frame counter, IRQ | Screen hash or $6000 |
| dmc_dma_during_read4 | blargg | Register read/write during DMA | Screen hash |
| test_apu_env | blargg | APU envelope | Screen hash |
| test_apu_sweep | blargg | Sweep unit behavior | Screen hash |
| test_apu_timers | blargg | Frequency timer of all 5 channels | Screen hash |
| test_tri_lin_ctr | blargg | Triangle linear counter | Screen hash |
| pal_apu_tests | blargg | PAL APU tests | Screen hash |
| AccuracyCoin (Page 13) | 100thCoin | DMA + open bus, DMA + $2002/$2007/$4015/$4016 reads, DMC DMA bus conflicts, DMC+OAM DMA, explicit/implicit DMA abort | PASS/FAIL |
| AccuracyCoin (Page 14) | 100thCoin | APU length counter, length table, frame counter IRQ, frame counter 4/5-step, DMC, APU register activation, controller strobing/clocking | PASS/FAIL |

### Mapper Tests

| Test | Author | Tests | Automation |
|------|--------|-------|------------|
| mmc3_test_2 | blargg | MMC3 scanline counter and IRQ | Screen hash |
| Holy Mapperel | tepples | Detects 12+ mappers, verifies PRG/CHR banks, mirroring, IRQ, WRAM | Screen hash |
| BNTest | tepples | PRG bank reachability in BxROM/AxROM | Screen hash |
| FdsIrqTests | Sour | FDS IRQ behavior | Screen hash |
| mmc5test_v2 | AWJ | MMC5 tests | Screen hash |
| vrc24test | AWJ | VRC2/4 variant detection and testing | Screen hash |
| serom | lidnariq | MMC1 SEROM/SHROM/SH1ROM constraints | Screen hash |
| test28 | tepples | Mapper 28 tests | Screen hash |

### Input Tests

| Test | Author | Tests | Automation |
|------|--------|-------|------------|
| allpads | tepples | Multiple controller types, raw 32-bit report | Input injection |
| read_joy3 | blargg | Controller tests including DMC DMA corruption | Screen hash |

### Misc Tests

| Test | Author | Tests | Automation |
|------|--------|-------|------------|
| 240pee | tepples | 240p Test Suite — comprehensive display/color test | Visual |
| NEStress | Flubba | Multi-test suite (some tests expected to fail on HW) | Screen hash |
| AccuracyCoin (Page 15) | 100thCoin | Power-on state: PPU reset flag, CPU RAM, CPU registers, PPU RAM, palette RAM | DRAW (informational) |

---

## SNES (65816 / S-CPU)

### Primary Sources

- **SNESdev Wiki — Emulator tests:** [snes.nesdev.org/wiki/Emulator_tests](https://snes.nesdev.org/wiki/Emulator_tests)
- **TASVideos SNES Accuracy Tests:** [tasvideos.org/EmulatorResources/SNESAccuracyTests](https://tasvideos.org/EmulatorResources/SNESAccuracyTests)
- **undisbeliever SNES test ROMs:** [github.com/undisbeliever/snes-test-roms](https://github.com/undisbeliever/snes-test-roms)
- **PeterLemon/SNES:** [github.com/PeterLemon/SNES](https://github.com/PeterLemon/SNES) — CPU, PPU, SPC700 test demos
- **gilyon/snes-tests:** [github.com/gilyon/snes-tests](https://github.com/gilyon/snes-tests)
- **SNES-HiRomGsuTest:** [github.com/astrobleem/SNES-HiRomGsuTest](https://github.com/astrobleem/SNES-HiRomGsuTest)

### Blargg's SNES Tests (9 tests)

| Test | Tests | Automation |
|------|-------|------------|
| ADC/SBC | CPU arithmetic (BCD and binary mode, all flag combinations) | Screen hash |
| Flash Screen | PPU rendering timing test | Screen hash |
| OAM Test | Sprite/OAM behavior | Screen hash |
| TSC | Transfer Stack to C register timing | Screen hash |
| 5 additional tests | Various CPU/PPU | Screen hash |

### Official SNES Testing Suite (30 tests)

From the aging test cartridge:

| Category | Tests | Count |
|----------|-------|-------|
| Work RAM | RAM read/write patterns | Multiple |
| DRAM | Dynamic RAM refresh and access | Multiple |
| VRAM | Video RAM access patterns | Multiple |
| DMA | Direct Memory Access timing and behavior | Multiple |
| OAM | Object Attribute Memory | Multiple |
| CG RAM | Color Generation RAM | Multiple |
| MPY | Hardware multiplier | Multiple |
| DIV | Hardware divider | Multiple |
| Timers | H/V timer IRQ systems | Multiple |

**Total: 30 tests — higan v094 and lsnes score 100% (30/30)**

### Coprocessor Tests

| Test | Tests | Count |
|------|-------|-------|
| Mega Man X2/X3 Cx4 Test | Cx4 math coprocessor | 8 tests |
| SPC7110 Check Program | SPC7110 data decompression chip | 12 tests |
| SNES-HiRomGsuTest | HiROM + GSU-2 + MSU-1 (9 hardware tests) | 9 tests |

### undisbeliever Test ROMs

Comprehensive test ROMs covering various SNES subsystems. Available at GitHub with source.

### PeterLemon/SNES

| Directory | Tests |
|-----------|-------|
| CPUTest/ | 65816 CPU instruction tests |
| PPU/ | PPU mode and rendering tests |
| SPC700/ | SPC700 audio processor tests |
| CHIP/GSU/ | SuperFX/GSU coprocessor tests |

### PPU Tests (SNESdev Wiki)

| Test | Author | Tests |
|------|--------|-------|
| gradient-test | NovaSquirrel | CGWSEL demonstration |
| Two Ship | rainwarrior | Mode 5 + interlacing vs Mode 1 |
| PPU bus activity | lidnariq | All modes 0-6 on single screen |
| Elasticity | rainwarrior | Mode 3 color depth beyond 8bpp |

---

## Game Boy / Game Boy Color (SM83 / Sharp LR35902)

### Primary Sources

- **Mooneye Test Suite:** [github.com/Gekkio/mooneye-test-suite](https://github.com/Gekkio/mooneye-test-suite) — 146 stars, hardware-verified, MIT license
- **Blargg's GB tests:** Available in nes-test-roms archive
- **mattcurrie dmg-acid2:** [github.com/mattcurrie/dmg-acid2](https://github.com/mattcurrie/dmg-acid2) — PPU rendering acid test
- **mattcurrie cgb-acid2:** [github.com/mattcurrie/cgb-acid2](https://github.com/mattcurrie/cgb-acid2) — CGB PPU acid test
- **LIJI32 SameSuite:** [github.com/LIJI32/SameSuite](https://github.com/LIJI32/SameSuite) — SameBoy's test suite
- **gb-ctr (Complete Technical Reference):** [github.com/Gekkio/gb-ctr](https://github.com/Gekkio/gb-ctr)

### Mooneye Test Suite

The gold standard for Game Boy accuracy testing. Hardware-verified on 20+ physical devices.

| Category | Description | Pass/Fail Protocol |
|----------|-------------|-------------------|
| acceptance/ | Main acceptance tests (PPU, timer, CPU, MBC, interrupts) | Fibonacci sequence in B/C/D/E/H/L registers, or 0x42 for fail |
| emulator-only/ | Tests requiring special hardware to verify on real HW | Same protocol |
| misc/ | CGB/AGB-specific extra tests | Same protocol |

**Automation:** Check registers B/C/D/E/H/L for Fibonacci 3/5/8/13/21/34 (pass) or 0x42 (fail). Alternative: detect `LD B, B` opcode as debug breakpoint.

**Model support:** Tests tagged with DMG, MGB, SGB, SGB2, CGB, AGB, AGS to indicate which hardware variant.

### Blargg's GB Tests

| Test | Tests | Automation |
|------|-------|------------|
| cpu_instrs | All CPU instructions | Screen text + serial output |
| instr_timing | Instruction cycle timing | Screen text |
| mem_timing | Memory access timing | Screen text |
| mem_timing-2 | Extended memory timing | Screen text |
| halt_bug | HALT instruction bug behavior | Screen text |
| oam_bug | OAM corruption bug | Screen text |
| dmg_sound | DMG APU tests (12 sub-tests) | Screen text + serial output |
| cgb_sound | CGB APU tests | Screen text |

### Acid Tests

| Test | Author | Tests | Automation |
|------|--------|-------|------------|
| dmg-acid2 | mattcurrie | DMG PPU rendering correctness | Screenshot comparison to reference |
| cgb-acid2 | mattcurrie | CGB PPU rendering correctness | Screenshot comparison to reference |

---

## Game Boy Advance (ARM7TDMI)

### Primary Sources

- **jsmolka/gba-tests:** [github.com/jsmolka/gba-tests](https://github.com/jsmolka/gba-tests) — 127 stars, MIT license
- **mgba-emu/suite:** [github.com/mgba-emu/suite](https://github.com/mgba-emu/suite) — mGBA's official test suite, 92 stars
- **DenSinH/FuzzARM:** [github.com/DenSinH/FuzzARM](https://github.com/DenSinH/FuzzARM) — ARM CPU fuzzer
- **destoer/armwrestler-gba-fixed:** [github.com/destoer/armwrestler-gba-fixed](https://github.com/destoer/armwrestler-gba-fixed) — ARM/Thumb CPU test
- **jsmolka/gba-suite (AGS):** Older but comprehensive test set

### jsmolka/gba-tests

| Directory | Tests | Automation |
|-----------|-------|------------|
| arm/ | ARM instruction set tests | Failed test number displayed (BG mode 4) |
| thumb/ | Thumb instruction set tests | Same |
| memory/ | Memory access and timing tests | Same |
| ppu/ | PPU rendering tests | Same |
| bios/ | BIOS function tests | Same |
| save/ | Save type tests (SRAM, Flash, EEPROM) | Same |
| nes/ | NES-style test ROMs for GBA | Same |
| unsafe/ | Dangerous/undefined behavior tests | Same |

### mGBA Test Suite

Comprehensive GBA test suite by the mGBA developer (Vicki Pfau). Covers:

- Memory timing
- DMA behavior
- Timer accuracy
- PPU rendering (BG modes, sprite behavior, windowing)
- SIO register read/write
- BG toggle behavior

---

## Sega Genesis / Mega Drive (M68000 + Z80)

### Primary Sources

- **BlastEm test suite:** Bundled with BlastEm emulator
- **PeterLemon/MD:** [github.com/PeterLemon/MD](https://github.com/PeterLemon/MD) — Genesis assembly tests
- **TmEE/GenTestSuite:** VDP and CPU tests
- **Nemesis Genesis tests:** Various VDP timing and behavior tests
- **flame/gentest:** [github.com/flame/gentest](https://github.com/flame/gentest)

### Key Test Categories

| Category | Tests | Notes |
|----------|-------|-------|
| 68000 CPU | Instruction execution, flags, timing | Multiple suites available |
| Z80 CPU | Sound CPU instruction tests | ZEXALL-compatible tests |
| VDP | Rendering modes, DMA, FIFO, HV counter | Nemesis tests are gold standard |
| PSG | Sound chip tests | Limited availability |
| YM2612 | FM synthesis tests | Limited, mostly audio comparison |
| Controller | Input read tests | Various |

### ZEXALL (Z80 Instruction Exerciser)

A comprehensive Z80 CPU test used across multiple systems (Genesis Z80, SMS, GB as adapted).

---

## Sega Master System / Game Gear (Z80)

### Primary Sources

- **Flubba's SMS tests:** Various PPU and CPU tests
- **ZEXALL:** Z80 CPU exerciser
- **VDPTest:** VDP behavior tests
- **Wikipedia test ROMs:** Limited availability

### Key Test Categories

| Category | Tests | Notes |
|----------|-------|-------|
| Z80 CPU | ZEXALL, instruction tests | Well-covered |
| VDP | Mode tests, sprite behavior, timing | Moderate coverage |
| PSG | Sound output tests | Limited |
| Input | Controller read tests | Limited |

---

## PC Engine / TurboGrafx-16 (HuC6280)

### Primary Sources

- **PCEdev community tests:** Limited but growing
- **mednafen internal tests:** Used for mednafen development
- **Near's higan/ares test framework:** Some PCE-specific tests

### Key Test Categories

| Category | Tests | Notes |
|----------|-------|-------|
| HuC6280 CPU | 65SC02-derived CPU tests | Some available |
| VDC (HuC6270) | Video display controller | Limited |
| VCE | Video color encoder | Limited |
| PSG | Programmable sound generator | Limited |
| CD-ROM | CD system tests | Very limited |

**Note:** PCE test ROMs are significantly less available than NES/SNES/GB. Custom test ROM development is a priority here.

---

## WonderSwan / WonderSwan Color (V30MZ / NEC)

### Primary Sources

- **WSdev community:** [wsdev.org](https://wsdev.org/) (via Wayback Machine)
- **ares test suite:** Some WS-specific test ROMs
- **robert_collins WS tests:** Community-contributed tests

### Key Test Categories

| Category | Tests | Notes |
|----------|-------|-------|
| V30MZ CPU | x86-subset CPU tests | Very limited |
| PPU | Display controller tests | Very limited |
| Sound | Audio subsystem tests | Very limited |
| Input | Button/touch tests | Very limited |

**Note:** WonderSwan has the least test ROM coverage of any Nexen-supported system. Custom development is essential.

---

## Atari Lynx (65SC02)

### Primary Sources

- **Nexen's boot-smoke.lua:** Existing Lynx boot smoke test (screen buffer sampling)
- **Near's ares tests:** Some Lynx-specific tests
- **Community contributions:** Very limited

### Key Test Categories

| Category | Tests | Notes |
|----------|-------|-------|
| 65SC02 CPU | CPU instruction tests | Can adapt 6502 tests |
| Suzy (graphics) | Sprite rendering engine | Very limited |
| Mikey (audio) | Audio subsystem | Very limited |
| Timers | Timer system | Very limited |

**Note:** Similar to WonderSwan, Lynx test ROMs are scarce. The 65SC02 CPU can leverage adapted 6502 tests, but the custom Suzy/Mikey hardware requires original test ROM development.

---

## Test ROM Priority Matrix

Systems ranked by test ROM availability and Nexen's need for accuracy testing:

| Priority | System | Test ROM Availability | Nexen Coverage Gap | Action |
|----------|--------|----------------------|-------------------|--------|
| 🔴 Critical | NES | Excellent (200+) | Many not automated | Automate existing |
| 🔴 Critical | SNES | Good (60+) | Many not automated | Automate existing |
| 🔴 Critical | GB/GBC | Good (100+) | Many not automated | Automate existing |
| 🟡 High | GBA | Good (50+) | Many not automated | Automate existing |
| 🟡 High | Genesis | Moderate (30+) | Need to collect | Collect + automate |
| 🟡 High | SMS/GG | Moderate (20+) | Need to collect | Collect + automate |
| 🟠 Medium | PCE | Limited (10-15) | Almost none | Collect + develop custom |
| 🟠 Medium | WS/WSC | Very limited (<10) | Almost none | Develop custom (priority) |
| 🟠 Medium | Lynx | Very limited (<5) | Almost none | Develop custom |

---

## Folder Structure

```
c:\reference-roms\
├── nes\
│   ├── blargg\                 # Blargg's CPU/PPU/APU tests
│   │   ├── cpu_instrs\
│   │   ├── ppu_vbl_nmi\
│   │   ├── ppu_sprite_hit\
│   │   ├── sprite_overflow\
│   │   ├── apu_test\
│   │   └── ...
│   ├── accuracycoin\           # AccuracyCoin single-cart test
│   ├── kevtris\                # nestest
│   ├── bisqwit\                # cpu_dummy_writes, ppu_read_buffer
│   ├── tepples\                # allpads, 240pee, Holy Mapperel
│   ├── mapper-tests\           # MMC1/MMC3/MMC5/FDS/VRC tests
│   └── misc\                   # NEStress, other tests
├── snes\
│   ├── blargg\                 # Blargg's SNES tests
│   ├── official-aging\         # Official SNES aging test suite
│   ├── undisbeliever\          # undisbeliever test ROMs
│   ├── peterlemon\             # PeterLemon CPU/PPU/SPC tests
│   ├── coprocessor\            # Cx4, SPC7110, GSU tests
│   ├── hirom-gsu-test\         # SNES-HiRomGsuTest
│   └── misc\                   # gilyon, PPU tests
├── gb\
│   ├── mooneye\                # Mooneye Test Suite
│   │   ├── acceptance\
│   │   ├── emulator-only\
│   │   └── misc\
│   ├── blargg\                 # Blargg's GB tests
│   ├── acid2\                  # dmg-acid2, cgb-acid2
│   └── samesuite\              # SameSuite (SameBoy)
├── gba\
│   ├── jsmolka\                # jsmolka/gba-tests
│   ├── mgba-suite\             # mGBA test suite
│   ├── fuzzarm\                # FuzzARM CPU fuzzer
│   └── armwrestler\            # ARM/Thumb CPU test
├── genesis\
│   ├── blastem\                # BlastEm test suite
│   ├── peterlemon\             # PeterLemon/MD tests
│   ├── zexall\                 # Z80 CPU exerciser
│   └── vdp-tests\              # VDP timing/behavior tests
├── sms\
│   ├── flubba\                 # Flubba's SMS tests
│   ├── zexall\                 # Z80 CPU exerciser
│   └── vdp-tests\              # VDP tests
├── pce\
│   ├── community\              # PCEdev community tests
│   └── custom\                 # In-house custom tests
├── ws\
│   ├── community\              # WSdev community tests
│   └── custom\                 # In-house custom tests (priority!)
└── lynx\
    ├── community\              # Community tests
    └── custom\                 # In-house custom tests
```

---

## Download and Acquisition Notes

### Freely Available (Open Source / Public Domain)

- AccuracyCoin — MIT license, `.nes` ROM included in repo
- Mooneye Test Suite — MIT license, prebuilt ROMs at gekkio.fi
- jsmolka/gba-tests — MIT license, prebuilt ROMs
- mgba-emu/suite — MIT license
- PeterLemon/SNES — public domain, prebuilt `.sfc` ROMs included
- SNES-HiRomGsuTest — public domain, prebuilt `.sfc` ROM
- dmg-acid2 / cgb-acid2 — MIT license
- undisbeliever/snes-test-roms — open source

### Requires Building from Source

- Mooneye Test Suite (optional — prebuilts available)
- mGBA suite (needs devkitPro/devkitARM)
- jsmolka/gba-tests (needs FASMARM)

### Archive Downloads

- Blargg's tests — originally at blargg.8bitalley.com (archived at nes-test-roms)
- Official SNES aging test — from snescentral.com
- Various NESdev tests — forum attachments, archived at christopherpow/nes-test-roms

---

## Related Documents

- [Automated Accuracy Testing Plan](automated-accuracy-testing-plan.md) — Infrastructure for running tests
- [In-House Test ROM Development Plan](in-house-test-rom-plan.md) — Using Flower Toolchain to build custom tests
- [Accuracy Testing Pipeline](accuracy-testing-pipeline.md) — Step-by-step reproducible commands
