# Atari Lynx Emulation Research

## Executive Summary

**Every existing Atari Lynx emulator traces back to a single codebase**: K. Wilkins' **Handy** (1996–1997). There is no independent Lynx emulator implementation. The three active derivatives are:

| Emulator | Base | License | Status |
|----------|------|---------|--------|
| **Beetle-Lynx** (Mednafen) | Handy fork via Mednafen | GPL-2.0 | Active (libretro) |
| **Libretro-Handy** | Handy direct port | zlib/libpng | Active (libretro) |
| **Handy-Fork** (bspruck) | Handy 0.90–0.98 collection | zlib/libpng | Archive/patches |

**Recommendation for Nexen**: The Handy core (zlib/libpng license) is permissive and compatible with Nexen's licensing. The architecture is straightforward C++ with ~15 source files. The Lynx is a relatively simple system (65C02 CPU, two custom chips, 64KB RAM, 160×102 display) making it a good candidate for integration.

---

## 1. Atari Lynx Hardware Overview

### Specifications

| Component | Detail |
|-----------|--------|
| **CPU** | 65C02 (WDC 65SC02) @ 4 MHz effective (16 MHz crystal, 4:1 divider) |
| **System Clock** | 16 MHz crystal, basic tick = 62.5 ns |
| **RAM** | 64 KB (two 64K×4 DRAMs, 120ns RAS / 60ns CAS page mode) |
| **Display** | 160×102 pixels, 4096 color palette (12-bit: 4 bits per R/G/B), 16 levels per pixel |
| **LCD** | 480 physical pixels × 102 lines (3 pixels per triad = 160 triads) |
| **Audio** | 4 channels, 8-bit DAC each, mono output through 2" 8Ω speaker |
| **Custom Chips** | Mikey (timers, audio, display, UART) + Suzy (sprites, math, I/O) |
| **Cartridge** | 128KB–1MB ROM, shift-register based addressing |
| **Serial** | ComLynx (UART), max 62,500 baud, bi-directional |
| **Input** | 4-direction D-pad, 2 fire buttons (A/B), Pause, Option 1, Option 2 |
| **Boot ROM** | 512 bytes at $FE00–$FFFF (inside Mikey) |
| **Screen Rotation** | Hardware supports left/right rotation for games like Gauntlet |

### Memory Map

| Address Range | Component | Notes |
|---------------|-----------|-------|
| `$0000–$FBFF` | RAM | Always accessible |
| `$FC00–$FCFF` | Suzy (or RAM) | Controlled by MAPCTL at $FFF9 |
| `$FD00–$FDFF` | Mikey (or RAM) | Controlled by MAPCTL at $FFF9 |
| `$FE00–$FFF7` | Boot ROM (or RAM) | Controlled by MAPCTL at $FFF9 |
| `$FFF8` | (unused) | |
| `$FFF9` | MAPCTL | Memory map control register |
| `$FFFA–$FFFB` | NMI vector | |
| `$FFFC–$FFFD` | Reset vector | |
| `$FFFE–$FFFF` | IRQ vector | |

### MAPCTL Register ($FFF9)

Controls which hardware is mapped into the upper address space. Games typically start with ROM/hardware mapped, then switch to RAM-only after boot for full 64KB access.

### Interrupt System

- **IRQ only** (no NMI in normal operation; NMI used by dev kit hardware)
- **8 timer sources** (Timer 0–7), each with independent enable
- Timer 0 = HBL (Horizontal Blank)
- Timer 2 = VBL (Vertical Blank)
- Timer 4 = UART (level-sensitive, unlike others which are edge-sensitive)
- Timers 1, 3, 5, 6, 7 = General purpose
- Interrupt registers: INTSET ($FD81) and INTRST ($FD80)

### Display Timing

- 105 HBL interrupts per frame (102 visible + 3 blank lines)
- Display DMA controlled by Mikey
- Double-buffered video with buffer swap on VBL

---

## 2. Emulator Analysis

### 2.1 Handy (Original) — K. Wilkins

**Repository**: [bspruck/handy-fork](https://github.com/bspruck/handy-fork) (archive of multiple versions)
**Author**: K. Wilkins (1996–1997)
**License**: **zlib/libpng** (permissive — allows commercial use, modification, redistribution)

#### Architecture

The original Handy uses a clean object-oriented C++ design with one class per hardware component, all owned by a central `CSystem` class:

```text
CSystem (system.h/cpp)
├── C65C02  (c65c02.h, c6502mak.h) — CPU
├── CMikie  (mikie.h/cpp)          — Mikey chip
├── CSusie  (susie.h/cpp)          — Suzy chip
├── CCart   (cart.h/cpp)            — Cartridge
├── CRam    (ram.h/cpp)             — 64KB RAM
├── CRom    (rom.h/cpp)             — 512B Boot ROM
└── CMemMap (memmap.h/cpp)          — Memory mapper ($FFF9)
```

All components inherit from `CLynxBase` (lynxbase.h), which provides a virtual interface:

- `Peek(addr)` — Read byte
- `Poke(addr, data)` — Write byte
- `Reset()` — Hardware reset
- `BankSelect(mode)` — Bank switching
- `ReadCycle()` / `WriteCycle()` — Cycle timing

`CSystem` inherits from `CSystemBase` (sysbase.h) providing `Poke_CPU()` / `Peek_CPU()` pure virtuals.

#### CPU Implementation

- 65C02 instruction set via macro-based decoding in `c6502mak.h`
- BCD lookup tables pre-computed
- Fast path for addresses < $FC00 (direct RAM array access)
- Slow path for $FC00+ (dispatches through `CLynxBase*` memory handlers array)
- Macros: `CPU_PEEK(addr)` / `CPU_POKE(addr, data)`

#### Key Constants (lynxdef.h)

```cpp
#define HANDY_SYSTEM_FREQ       16000000    // 16 MHz crystal
#define HANDY_TIMER_FREQ        20          // Timer tick
#define HANDY_AUDIO_SAMPLE_FREQ 22050       // Audio sample rate
#define HANDY_AUDIO_SAMPLE_PERIOD (HANDY_SYSTEM_FREQ / HANDY_AUDIO_SAMPLE_FREQ)
#define HANDY_FILETYPE_LNX      0           // .lnx with 64-byte header
#define HANDY_FILETYPE_HOMEBREW 1           // BS93 format
#define HANDY_FILETYPE_RAW      2           // Headerless ROM
```

#### Versions in handy-fork

| Version | Notes |
|---------|-------|
| 0.90 (original) | Earliest available source |
| 0.95 (original) | Windows source release |
| 0.95 (patched) | EEPROM fixes, HLE BIOS, dual cart banks |
| 0.98 (handy-sdl) | SDL port, continued development |

#### Accuracy

- Considered "good enough" for the commercial Lynx library
- Known sprite engine bug faithfully emulated (off-by-one in `LineGetBits`)
- Timer-based scheduling with next-event prediction (not cycle-stepping)
- No known accuracy-focused forks exist

#### Features

- LNX file format with 64-byte header (magic "LYNX")
- BS93 homebrew format support
- Raw/headerless ROM support (auto-detection by size: 128K/256K/512K)
- Save states (custom binary LSS format)
- Screen rotation support
- ComLynx serial emulation

---

### 2.2 Beetle-Lynx (Mednafen Fork) — libretro

**Repository**: [libretro/beetle-lynx-libretro](https://github.com/libretro/beetle-lynx-libretro)
**License**: **GPL-2.0** (viral — affects derived works)
**Languages**: C 58.7%, C++ 33.2%
**Contributors**: 40

#### Description

"Standalone port of Mednafen Lynx to libretro, itself a fork of Handy." This is Mednafen's Lynx module extracted into a standalone libretro core.

#### Architecture Differences from Handy

Same core component classes (CSystem, C65C02, CMikie, CSusie, CCart, CRam, CRom, CMemMap) but wrapped in Mednafen infrastructure:

- **State System**: Uses `MDFNSS_StateAction()` with `SFORMAT` arrays instead of custom LSS format
- **Video Output**: `EmulateSpecStruct` + `MDFN_Surface` instead of direct buffer
- **File I/O**: `MDFNFILE` abstraction layer
- **ROM Database**: Built-in `LYNX_DB` struct with CRC32→game name/size/rotation mapping
- **Boot ROM**: Loaded from `lynxboot.img` in system directory (512 bytes)

#### ROM Database

Contains ~85 entries mapping CRC32 to game metadata:

```cpp
struct LYNX_DB {
    uint32 crc32;
    const char *name;
    uint32 filesize;
    uint32 rotation;    // CART_NO_ROTATE, CART_ROTATE_LEFT, CART_ROTATE_RIGHT
    uint32 reserved;
};
```

Notable entries with rotation:

- `Centipede` — CART_ROTATE_LEFT
- `Gauntlet - The Third Encounter` — CART_ROTATE_LEFT
- `Klax` — CART_ROTATE_LEFT
- `Lexis` — CART_ROTATE_LEFT
- `NFL Football` — CART_ROTATE_LEFT
- `Raiden` — CART_ROTATE_LEFT

Also includes `[BIOS] Atari Lynx (USA, Europe)` (CRC32: 0x0d973c9d, 512 bytes).

#### Key Source Files

All in `mednafen/lynx/`:

| File | Description |
|------|-------------|
| `system.cpp/h` | CSystem — main emulation, owns all components |
| `c65c02.h` | C65C02 — CPU class definition |
| `c6502mak.h` | CPU instruction macros (included by c65c02.h) |
| `mikie.cpp/h` | CMikie — timers, audio, display DMA, UART, IRQ |
| `susie.cpp/h` | CSusie — sprites, math, collision, input |
| `cart.cpp/h` | CCart — cartridge + ROM database |
| `ram.cpp/h` | CRam — 64KB system RAM |
| `rom.cpp/h` | CRom — 512B boot ROM |
| `memmap.cpp/h` | CMemMap — memory mapper at $FFF9 |
| `lynxbase.h` | CLynxBase — abstract base class |
| `sysbase.h` | CSystemBase — system interface |
| `lynxdef.h` | System constants and enums |
| `machine.h` | Build macros |

Top-level:

| File | Description |
|------|-------------|
| `libretro.cpp` | Libretro API implementation |
| `scrc32.c/h` | CRC32 calculation |

#### Cartridge Handling

```cpp
// File type detection in CSystem constructor
if (!strcmp(&clip[6], "BS93"))
    mFileType = HANDY_FILETYPE_HOMEBREW;   // BS93 homebrew
else if (!strcmp(&clip[0], "LYNX"))
    mFileType = HANDY_FILETYPE_LNX;        // Standard .lnx
else if (fp->size == 128*1024 || fp->size == 256*1024 || fp->size == 512*1024)
    mFileType = HANDY_FILETYPE_RAW;        // Headerless by size

// Cart bank sizes
enum CTYPE { C64K, C128K, C256K, C512K, C1024K };
// page_size_bank0 in header maps to shift count and mask
```

#### LNX Header Format (64 bytes)

```cpp
struct LYNX_HEADER {
    uint8  magic[4];          // "LYNX"
    uint16 page_size_bank0;   // Cart size / 256
    uint16 page_size_bank1;   // Second bank (usually 0)
    uint16 version;           // Must be 1
    uint8  cartname[32];      // Game name
    uint8  manufname[16];     // Manufacturer
    uint8  rotation;          // 0=none, 1=left, 2=right
    uint8  spare[5];          // Reserved
};
```

---

### 2.3 Libretro-Handy — Direct Handy Port

**Repository**: [libretro/libretro-handy](https://github.com/libretro/libretro-handy)
**License**: **zlib/libpng** (permissive — same as original Handy)
**Languages**: C 54.8%, C++ 36.8%
**Contributors**: 33

#### Key Additions Over Original Handy

1. **EEPROM Support** (`CEEPROM` class)
   - 93C46 / 93C66 / 93C86 serial EEPROMs
   - Required for a few commercial games

2. **HLE BIOS** (High-Level Emulation)
   - `HLE_BIOS_FE00()` — Reset vector emulation
   - `HLE_BIOS_FE19()` — Decryption routine bypass
   - `HLE_BIOS_FE4A()` — Cart loading
   - `HLE_BIOS_FF80()` — Additional boot logic
   - Allows running without the 512-byte boot ROM (`lynxboot.img`)

3. **CPU Overclock** — `Overclock()` method for speed hacking

4. **Audio** — Blip_Buffer-based audio synthesis with `FetchAudioSamples()`

5. **Context Save/Load** — Enhanced LSS format with tagged sections:
   - `"CSusie::ContextSave"`, `"CMikie::ContextSave"`, etc.

#### Source Structure

All Lynx source in `lynx/` directory — same file layout as Handy plus:

- `eeprom.h/cpp` — CEEPROM class
- Additional HLE BIOS routines in system code

---

## 3. Detailed Component Analysis

### 3.1 Mikey Chip (CMikie)

**Address Range**: $FD00–$FDFF

| Register Range | Function |
|----------------|----------|
| $FD00–$FD1F | Timer 0–7 (4 bytes each: backup, control A, count, control B) |
| $FD20–$FD3F | Audio channels 0–3 (8 bytes each) |
| $FD40–$FD43 | Attenuation registers |
| $FD44–$FD49 | Stereo/panning |
| $FD50–$FD5F | Audio control |
| $FD80 | INTRST — Interrupt reset (write clears flags) |
| $FD81 | INTSET — Interrupt set (read: flags; write: force interrupt) |
| $FD84–$FD8B | Display address, serial control |
| $FD90–$FD9F | Display control, video DMA |
| $FDA0–$FDAF | Palette green values |
| $FDB0–$FDBF | Palette blue/red values |

**Key Behaviors**:

- 8 timers with configurable prescalers and chain linking
- Timer expiry triggers interrupt (if enabled) and optional linked timer tick
- Display DMA: Mikey reads pixel data from RAM and shifts it to LCD
- Audio: 4-channel waveshaper with volume envelopes
- UART: ComLynx serial with TX/RX interrupts (level-sensitive, unlike timer IRQs)

### 3.2 Suzy Chip (CSusie)

**Address Range**: $FC00–$FCFF

| Register Range | Function |
|----------------|----------|
| $FC00–$FC01 | TMPADRL/H — Temp address |
| $FC02–$FC03 | TILTACUML/H |
| $FC04–$FC0B | Other sprite registers |
| $FC10–$FC1F | Sprite control block pointers |
| $FC20–$FC3F | Sprite engine state |
| $FC52–$FC53 | MATHD, MATHC (hardware math inputs) |
| $FC54–$FC5F | MATHAB, MATHEFGH, MATHJKLM, MATHNP |
| $FC60–$FC61 | MATHP_SIGN, MATHABC_SIGN |
| $FC80–$FC8F | Sprite process control |
| $FC90–$FC93 | JOYSTICK / SWITCHES |
| $FCB0–$FCB3 | Suzy control / SPRGO |

**Sprite Engine**:

- Hardware sprite rendering with collision detection
- 8 sprite types: background_shadow, background_noncollide, boundary_shadow, boundary, normal, noncollide, xor_shadow, shadow
- Line-packed sprite data (RLE-like) with literal and packed modes
- `PaintSprites()` — Main sprite rendering entry point
- `ProcessPixel()`, `WritePixel()`, `ReadPixel()` — Per-pixel operations
- `WriteCollision()`, `ReadCollision()` — Collision buffer operations

**Hardware Math Unit**:

- 16×16 → 32 multiply (signed/unsigned)
- 32/16 → 16 divide with remainder
- Results in MATHABCD, MATHEFGH, MATHJKLM, MATHNP registers
- Known hardware bug emulated: off-by-one in `LineGetBits` (`<=` instead of `<`)

**Input**:

- Joystick at $FCB0: Up, Down, Left, Right
- Buttons: A, B, Option 1, Option 2, Pause

### 3.3 Cartridge (CCart)

**Features**:

- Shift-register based address counter (mimics actual hardware)
- Bank 0 and Bank 1 support
- Sizes: 64K, 128K, 256K, 512K, 1024K
- LNX header parsing with "LYNX" magic
- CRC32 checksums for ROM identification
- Support for Cart RAM (some games use cart as writable storage)
- EEPROM simulation (in libretro-handy variant)

### 3.4 Memory Architecture

The `CSystem` class maintains a 65536-entry array of `CLynxBase*` pointers, one per byte address. Most point to `CRam`, but the upper region ($FC00–$FFFF) can be switched between RAM and hardware registers via the MAPCTL register.

**CPU Access Pattern**:

```cpp
// Fast path (hot)
#define CPU_PEEK(addr) (addr < 0xFC00 ? mRamPointer[addr] : mSystem.Peek_CPU(addr))
#define CPU_POKE(addr, data) { if (addr < 0xFC00) mRamPointer[addr] = data; else mSystem.Poke_CPU(addr, data); }
```

This is a critical hot path — the $FC00 boundary check occurs on every memory access.

---

## 4. ROM Formats

| Format | Extension | Header | Notes |
|--------|-----------|--------|-------|
| LNX | .lnx | 64-byte "LYNX" header | Standard Handy format, most ROMs |
| BS93 | .o, .obj | 10-byte "BS93" header | BLL homebrew format |
| Raw | .lyx, .bin | None | Headerless, detected by size (128K/256K/512K) |
| Epyx ROM | .rom | Encrypted header | Original dev kit format |

### LNX Header (64 bytes)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 4 | Magic | "LYNX" (ASCII) |
| 4 | 2 | Bank0 page size | Cart ROM size / 256 |
| 6 | 2 | Bank1 page size | Second bank size / 256 (usually 0) |
| 8 | 2 | Version | Must be 1 |
| 10 | 32 | Cart name | Game title (null-padded) |
| 42 | 16 | Manufacturer | Publisher name |
| 58 | 1 | Rotation | 0=none, 1=left, 2=right |
| 59 | 5 | Spare | Reserved (zeros) |

### BS93 Header (10 bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0 | 2 | (unknown) |
| 2 | 2 | Load address (big-endian) |
| 4 | 2 | Size (big-endian) |
| 6 | 4 | Magic "BS93" |

---

## 5. ROM Database

The beetle-lynx `cart.cpp` contains a built-in database of ~85 commercial Lynx titles with CRC32 checksums. Key data per entry:

- CRC32 hash (computed on ROM data minus header)
- Game name
- File size (for headerless ROM identification)
- Rotation flag
- Reserved field

This is the most comprehensive Lynx ROM database in any open-source emulator. The CRC32 is used for:

1. Auto-detecting screen rotation
2. Setting correct ROM size for headerless dumps
3. Game identification

Including the BIOS: `{ 0x0d973c9d, "[BIOS] Atari Lynx (USA, Europe)", 512, 0, 0 }`

Notable: The `Lynx II Production Test Program (v0.02) (Proto)` is in the database — this is the closest thing to a "test ROM" in the commercial database.

---

## 6. Test ROMs and Accuracy Testing

### Available Test Resources

| Resource | Type | URL |
|----------|------|-----|
| **42Bastian's lynx_hacking** | Homebrew demos & benchmarks | [github.com/42Bastian/lynx_hacking](https://github.com/42Bastian/lynx_hacking) |
| **cycle_check** | Opcode cycle timing benchmark | `lynx_hacking/cycle_check/` |
| **cycle_check_hbl** | HBL-synchronized cycle test | `lynx_hacking/cycle_check_hbl/` |
| **chained_scbs** / **chained_scbs2** | Sprite chain speed tests | `lynx_hacking/chained_scbs/` |
| **248b demos** | Boot-sector size constraint demos | `lynx_hacking/248b/` |
| **microloader** | Minimal boot-sector loader | `lynx_hacking/microloader/` |

The `cycle_check` programs from 42Bastian's lynx_hacking repo are specifically designed to **benchmark real opcode cycle counts on actual Lynx hardware**. These are the most useful accuracy validation tools available.

### No Formal Test Suite Exists

Unlike NES (nestest), GB (Blargg's tests), or SNES (various test ROMs), there is **no comprehensive Lynx test suite** for:

- CPU instruction correctness
- Timer edge cases
- Sprite rendering accuracy
- Math unit precision
- UART timing

This is partly because all emulators share the same Handy codebase, so bugs are consistently replicated rather than caught.

### Known Accuracy Gaps

1. **Sprite engine off-by-one**: Intentionally preserved from Handy as it matches a known hardware bug
2. **Timer chaining edge cases**: Limited testing on unusual timer configurations
3. **UART level-sensitive vs edge-sensitive**: ComLynx edge cases poorly tested
4. **EEPROM timing**: Implementation based on datasheet rather than hardware verification
5. **HLE BIOS accuracy**: The HLE routines bypass the actual boot ROM decryption sequence

---

## 7. Hardware Documentation Sources

| Resource | Content | URL |
|----------|---------|-----|
| **Epyx Developer Docs** (monlynx.de) | Official hardware reference (digitized by 42Bastian) | [monlynx.de/lynx/lynxdoc.html](https://www.monlynx.de/lynx/lynxdoc.html) |
| **Register Map** | All Mikey/Suzy register addresses | [monlynx.de/lynx/hardware.html](https://www.monlynx.de/lynx/hardware.html) |
| **IRQ Documentation** | Interrupt handling appendix | [monlynx.de/lynx/irq.html](https://www.monlynx.de/lynx/irq.html) |
| **Timer Documentation** | Timer section | [monlynx.de/lynx/lynx8.html](https://www.monlynx.de/lynx/lynx8.html) |
| **Atari Lynx Dev Blog** | 18-part programming tutorial | [atarilynxdeveloper.wordpress.com](https://atarilynxdeveloper.wordpress.com/) |
| **AtariAge Lynx Forum** | Active community, edge case discussions | [atariage.com/forums/forum/53](http://atariage.com/forums/forum/53-atari-lynx-programming/) |
| **new_bll SDK** | Modern Lynx development tools | [github.com/42Bastian/new_bll](https://github.com/42Bastian/new_bll) |
| **CC65 Lynx target** | C compiler with Lynx support | [github.com/cc65/cc65](https://github.com/cc65/cc65) |

The most authoritative source is the **Epyx Developer Kit reference manual** — a several-hundred-page document originally provided with the Pinky/Mandy and Howard/Howdy development hardware. The digitized version at monlynx.de (maintained by 42Bastian Schick) is the primary reference used by all emulator authors.

---

## 8. Development Tools Ecosystem

| Tool | Description | License |
|------|-------------|---------|
| **new_bll** | Modern Lynx SDK (assembler, tools) | Apache-2.0 |
| **lyxass** | Lynx assembler (from new_bll) | Apache-2.0 |
| **sprpck** | Sprite packer | Apache-2.0 |
| **make_lnx** | ROM builder (creates .lnx from .lyx) | Apache-2.0 |
| **CC65** | C/asm cross-compiler suite with Lynx target | zlib |

---

## 9. Integration Assessment for Nexen

### Complexity Estimate

| Component | Files | Complexity | Notes |
|-----------|-------|------------|-------|
| CPU (65C02) | 2 | Medium | Macro-based, similar to NES 6502 but with 65C02 extensions |
| Mikey | 2 | High | Timers, audio, display DMA, UART — many registers |
| Suzy | 2 | High | Sprite engine is the most complex part |
| Cart | 2 | Low | Shift-register addressing, header parsing |
| RAM | 2 | Trivial | 64KB array |
| ROM | 2 | Trivial | 512-byte array |
| MemMap | 2 | Low | Single register controlling address space |
| **Total** | **~14** | **Medium-High** | |

### CPU Reuse Potential

Nexen already has a 6502 core for NES (`NesCpu`). The Lynx uses a **65C02** (WDC variant) which adds:

- New instructions: PHX, PHY, PLX, PLY, STZ, TRB, TSB, BRA, BBR, BBS, RMB, SMB
- New addressing modes: (zp) — zero-page indirect without index
- Decimal mode fixes (BCD works correctly unlike NMOS 6502)
- No "undocumented" opcodes

Options:

1. **Extend existing 6502**: Add 65C02 instruction support (recommended)
2. **Separate CPU class**: Clean separation but code duplication

### Licensing Strategy

| Component | License | Nexen-Compatible? |
|-----------|---------|-------------------|
| Handy core code | zlib/libpng | **Yes** — very permissive |
| Beetle-Lynx wrapper | GPL-2.0 | **No** — viral license contamination |
| Libretro-Handy additions | zlib/libpng | **Yes** |
| Mednafen infrastructure | GPL-2.0 | **No** |
| ROM database (beetle-lynx) | GPL-2.0 | **No** — but CRC32/name data is factual |

**Recommendation**: Use Handy/libretro-handy as reference (zlib license). Build ROM database independently or from public sources (No-Intro DAT files).

### Key Design Decisions

1. **Boot ROM requirement**: Should support both:
   - Real boot ROM (`lynxboot.img`, 512 bytes)
   - HLE BIOS (no boot ROM needed, like libretro-handy)

2. **Save state format**: Nexen already has its own save state system — integrate directly

3. **Audio**: 4-channel synthesis with waveshaper — moderate complexity

4. **Display**: 160×102 at up to 75 fps — simple rendering pipeline

5. **Screen rotation**: Must support per-game rotation (left/right/none)

6. **EEPROM**: A few games need EEPROM save support (93C46/66/86)

---

## 10. Accuracy-Focused Projects

**None exist.** There is no "cycle-accurate" or "accuracy-focused" Lynx emulator project analogous to higan/bsnes (SNES), Gambatte (GB), or Mesen (NES). All Lynx emulators are forks of the same Handy codebase with the same accuracy characteristics.

This presents an opportunity: if Nexen implements Lynx emulation from scratch (using Handy as reference but rewriting), it could potentially become the first independently-verified Lynx emulator, discovering bugs that have been inherited across all Handy derivatives.

Key areas for accuracy improvement:

- Cycle-accurate CPU stepping (current implementations use next-event scheduling)
- Hardware math unit timing (currently instantaneous in emulation)
- Sprite DMA cycle stealing (affects CPU timing during sprite rendering)
- Timer cascade edge cases
- Display DMA timing and CPU contention

---

## 11. Community and Resources

### Active Community

- **AtariAge Lynx Programming Forum**: Most active Lynx development community
- **42Bastian Schick**: Maintains monlynx.de docs, new_bll SDK, lynx_hacking demos
- **Karri Kaksonen**: Lynx homebrew developer, flashcard firmware
- **Alex Thissen**: Lynx development blog author (18-part tutorial series)

### Notable Homebrew

The homebrew scene is small but active, producing games that can serve as additional compatibility tests:

- Shaken (not stirred)
- Tiny Lynx Adventure
- Various 248-byte demos (fit in boot sector)
- Raycasting engine port

---

## Appendix A: Complete File Listing (Beetle-Lynx Core)

```text
mednafen/lynx/
├── c6502mak.h      — CPU instruction macros
├── c65c02.cpp      — CPU implementation
├── c65c02.h        — CPU class definition
├── cart.cpp         — Cartridge + ROM database
├── cart.h           — Cartridge class
├── lynxbase.h       — Abstract base class
├── lynxdef.h        — Constants and enums
├── machine.h        — Build configuration
├── memmap.cpp       — Memory mapper
├── memmap.h         — Memory mapper class
├── mikie.cpp        — Mikey chip (timers, audio, display, UART)
├── mikie.h          — Mikey class
├── ram.cpp          — System RAM
├── ram.h            — RAM class
├── rom.cpp          — Boot ROM
├── rom.h            — ROM class
├── susie.cpp        — Suzy chip (sprites, math, I/O)
├── susie.h          — Suzy class
├── sysbase.h        — System base class
├── system.cpp       — Main system (CSystem)
├── system.h         — System class + top-level API
└── license.txt      — License file
```

## Appendix B: Sprite Types

| Type | Value | Description |
|------|-------|-------------|
| `sprite_background_shadow` | 0 | Background with shadow |
| `sprite_background_noncollide` | 1 | Background, no collision |
| `sprite_boundary_shadow` | 2 | Boundary with shadow |
| `sprite_boundary` | 3 | Boundary |
| `sprite_normal` | 4 | Normal sprite |
| `sprite_noncollide` | 5 | No collision detection |
| `sprite_xor_shadow` | 6 | XOR with shadow |
| `sprite_shadow` | 7 | Shadow only |

## Appendix C: Cart Size Mapping

| page_size_bank0 | Enum | Size | Mask |
|-----------------|------|------|------|
| 0x000 | — | 0 | — |
| 0x100 | C64K | 64KB | 0x00FFFF |
| 0x200 | C128K | 128KB | 0x01FFFF |
| 0x400 | C256K | 256KB | 0x03FFFF |
| 0x800 | C512K | 512KB | 0x07FFFF |
| 0x1000 | C1024K | 1MB | 0x0FFFFF |
