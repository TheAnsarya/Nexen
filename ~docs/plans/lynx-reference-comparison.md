# Lynx Emulator Reference Comparison

## Handy vs Mednafen/Beetle-Lynx vs Technical Documentation

> **Purpose:** Document differences between the three primary reference sources used for Lynx emulation in Nexen. When sources conflict, this document explains which source was followed and why.
>
> **Sources:**
> - **Handy** — K. Wilkins' original Lynx emulator (1997), [`libretro/libretro-handy`](https://github.com/libretro/libretro-handy), zlib license
> - **Mednafen/Beetle-Lynx** — Fork of Handy with modernizations, [`libretro/beetle-lynx-libretro`](https://github.com/libretro/beetle-lynx-libretro)
> - **Technical Docs** — Original Epyx "Handy" hardware spec (via monlynx.de), synthesized in [`atari-lynx-technical-report.md`](atari-lynx-technical-report.md)
>
> **Date:** 2026-02-19
> **Nexen branch:** `features-atari-lynx`

---

## Table of Contents

- [Lynx Emulator Reference Comparison](#lynx-emulator-reference-comparison)
	- [Handy vs Mednafen/Beetle-Lynx vs Technical Documentation](#handy-vs-mednafenbeetle-lynx-vs-technical-documentation)
	- [Table of Contents](#table-of-contents)
	- [1. Executive Summary](#1-executive-summary)
	- [2. Math Register Addresses](#2-math-register-addresses)
	- [3. Math Operation Behavior](#3-math-operation-behavior)
		- [3.1 Multiply](#31-multiply)
		- [3.2 Divide](#32-divide)
	- [4. Hardware Bug 13.8 — Sign Conversion](#4-hardware-bug-138--sign-conversion)
		- [Algorithm (from both emulators)](#algorithm-from-both-emulators)
		- [Truth table for Bug 13.8](#truth-table-for-bug-138)
		- [Stun Runner Workaround](#stun-runner-workaround)
	- [5. SCB Layout](#5-scb-layout)
		- [SCB Termination](#scb-termination)
	- [6. SPRCTL0 / SPRCTL1 Bit Definitions](#6-sprctl0--sprctl1-bit-definitions)
		- [SPRCTL0 (`$FC80`)](#sprctl0-fc80)
		- [SPRCTL1 (`$FC81`)](#sprctl1-fc81)
		- [ReloadDepth values](#reloaddepth-values)
	- [7. Quadrant Rendering](#7-quadrant-rendering)
		- [Quadrant Map](#quadrant-map)
		- [Starting Quadrant](#starting-quadrant)
		- [Quadrant Sign Convention](#quadrant-sign-convention)
		- [Flip Integration](#flip-integration)
		- [Superclipping](#superclipping)
		- [Known Issue: Tilt-Based Quadrant Flip](#known-issue-tilt-based-quadrant-flip)
		- [Quadrant Offset Fix](#quadrant-offset-fix)
	- [8. Sprite Type Enum](#8-sprite-type-enum)
	- [9. Auto-Clear Cascading Behavior](#9-auto-clear-cascading-behavior)
	- [10. RAM Address Masking](#10-ram-address-masking)
	- [11. Sprite Status Reporting](#11-sprite-status-reporting)
	- [12. Savestate Serialization](#12-savestate-serialization)
	- [13. Type Naming Conventions](#13-type-naming-conventions)
	- [14. Cartridge Port Handling](#14-cartridge-port-handling)
	- [15. Sizing Algorithm](#15-sizing-algorithm)
	- [16. Nexen Implementation Choices](#16-nexen-implementation-choices)
		- [What Nexen follows from Handy/Mednafen (over tech docs):](#what-nexen-follows-from-handymednafen-over-tech-docs)
		- [Where Nexen follows Mednafen's improvements over Handy:](#where-nexen-follows-mednafens-improvements-over-handy)
		- [Nexen-specific design choices:](#nexen-specific-design-choices)
		- [Known gaps vs Mednafen (future work):](#known-gaps-vs-mednafen-future-work)
	- [Appendix A: Source File Cross-Reference](#appendix-a-source-file-cross-reference)

---

## 1. Executive Summary

Handy and Mednafen/Beetle-Lynx are fundamentally the **same implementation** — Mednafen is a modernized fork of Handy. All core algorithms (math, sprites, collision, quadrants) are logic-identical between them. Mednafen makes three meaningful improvements:

1. **RAM address masking** — Masks addresses to `uint16`, preventing OOB access
2. **Savestate quadrant offsets** — Saves `hquadoff`/`vquadoff` in state (prevents desync)
3. **Cycle-based sprite status** — Uses timestamps instead of global sleep flag

The technical documentation (original Epyx spec) agrees with both emulators on register addresses, SCB layout, and math behavior. Where the tech docs are ambiguous or incomplete, the emulator source code serves as the definitive reference since Handy was written by the original hardware designers.

**Nexen policy:** Follow Handy/Mednafen emulator behavior over the tech docs when they conflict, because the emulators encode empirical hardware behavior (including undocumented bugs). Incorporate Mednafen's improvements where applicable.

---

## 2. Math Register Addresses

**All three sources agree.**

| Register | Address | Handy | Mednafen | Tech Docs | Group |
|----------|---------|-------|----------|-----------|-------|
| MATHD | `$FC52` | ✅ | ✅ | ✅ | ABCD (operands / quotient) |
| MATHC | `$FC53` | ✅ | ✅ | ✅ | |
| MATHB | `$FC54` | ✅ | ✅ | ✅ | |
| MATHA | `$FC55` | ✅ | ✅ | ✅ | |
| MATHP | `$FC56` | ✅ | ✅ | ✅ | NP (divisor) |
| MATHN | `$FC57` | ✅ | ✅ | ✅ | |
| MATHH | `$FC60` | ✅ | ✅ | ✅ | EFGH (result / dividend) |
| MATHG | `$FC61` | ✅ | ✅ | ✅ | |
| MATHF | `$FC62` | ✅ | ✅ | ✅ | |
| MATHE | `$FC63` | ✅ | ✅ | ✅ | |
| MATHM | `$FC6C` | ✅ | ✅ | ✅ | JKLM (accumulator / remainder) |
| MATHL | `$FC6D` | ✅ | ✅ | ✅ | |
| MATHK | `$FC6E` | ✅ | ✅ | ✅ | |
| MATHJ | `$FC6F` | ✅ | ✅ | ✅ | |

**Note:** Address space is non-contiguous — gaps at `$FC58-$FC5F` and `$FC64-$FC6B`.

---

## 3. Math Operation Behavior

### 3.1 Multiply

| Aspect | Handy | Mednafen | Tech Docs | Notes |
|--------|-------|----------|-----------|-------|
| Core operation | Unsigned `AB × CD → EFGH` | Identical | "16×16 → 32" | All agree |
| Signed mode | Post-multiply sign apply | Identical | `SPRSYS.signmath` | Sign tracked at write time |
| Accumulate | `JKLM += EFGH` | Identical | `SPRSYS.accumulate` | All agree |
| Trigger | Writing MATHA (`$FC55`) | Identical | "Write A/B to start" | Tech docs say "A/B", emulators trigger on A only |
| Timing | 44 ticks (unsigned) | Identical | 44 ticks (~2.75 µs) | All agree |
| Timing (signed+accum) | 54 ticks | Identical | 54 ticks (~3.375 µs) | All agree |
| Init value | `0xFFFFFFFF` | Identical | Not specified | Stun Runner compatibility |

**Tech doc ambiguity:** The docs say "write A/B to start multiply" which could be read as "write both A and B" or "write A or B". Both emulators only trigger on writing **A** (the high byte, `$FC55`).

### 3.2 Divide

| Aspect | Handy | Mednafen | Tech Docs | Notes |
|--------|-------|----------|-----------|-------|
| Core operation | Unsigned `EFGH ÷ NP → ABCD` | Identical | "32÷16 → 32" | All agree |
| Remainder | `EFGH % NP → JKLM` | Identical | "Not reliable" (Bug 13.9) | All agree |
| Divide by zero | `ABCD=0xFFFFFFFF`, `JKLM=0` | Identical | Not specified | Emulator-defined behavior |
| Trigger | Writing MATHE (`$FC63`) | Identical | "Write N/P to start" | **DISAGREES** — see below |
| Signed mode | Never signed | Identical | "Unsigned only" | All agree |
| Timing | 176 + 14×N ticks | Identical | Same formula | All agree |

**Critical disagreement — Divide trigger:** The tech docs say "write N/P to start divide". Both emulators trigger the divide when writing **MATHE** (`$FC63`), not N/P. The emulators are correct — the tech docs have this backwards. MATHE is the high byte of the dividend; loading it last and triggering the divide makes logical sense (all operands are ready).

---

## 4. Hardware Bug 13.8 — Sign Conversion

**All three sources agree on the existence of this bug. Emulators provide the exact algorithm.**

| Source | Description |
|--------|-------------|
| **Tech Docs** | "$8000 is treated as positive; 0 is treated as negative" |
| **Handy** | `(value - 1) & 0x8000` check; sign conversion at write time (MATHC and MATHA pokes) |
| **Mednafen** | Identical to Handy |

### Algorithm (from both emulators)

```cpp
// When writing MATHC (completing CD pair):
if (signmath) {
    uint16_t cd = MATHC_MATHD;       // 16-bit value just written
    if ((cd - 1) & 0x8000) {          // HW Bug 13.8 check
        cd = cd ^ 0xffff;             // Ones' complement
        cd++;                          // Two's complement
        sign_CD = -1;                  // Track sign
        MATHC_MATHD = cd;             // Store magnitude
    } else {
        sign_CD = +1;
    }
}
```

### Truth table for Bug 13.8

| Input value | `(value-1) & 0x8000` | Treated as | Standard 2's complement |
|------------|----------------------|------------|------------------------|
| `$0000` | `$FFFF & $8000 = TRUE` | **Negative** | Zero (unsigned) |
| `$0001` | `$0000 & $8000 = FALSE` | Positive | Positive |
| `$7FFF` | `$7FFE & $8000 = FALSE` | Positive | Positive |
| `$8000` | `$7FFF & $8000 = FALSE` | **Positive** | Negative (most negative) |
| `$8001` | `$8000 & $8000 = TRUE` | Negative | Negative |
| `$FFFF` | `$FFFE & $8000 = TRUE` | Negative | Negative (-1) |

The bug affects exactly two values: `$0000` (wrongly negative) and `$8000` (wrongly positive). All other values follow standard two's complement sign detection.

### Stun Runner Workaround

Both emulators implement: writing MATHD triggers `Poke(MATHC, 0)` — an automatic cascading clear. This is required because Stun Runner initializes registers in the wrong order (writes D before C), and the sign conversion on C would see stale data.

---

## 5. SCB Layout

**All three sources agree on layout and field ordering.**

| Offset | Field | Size | Notes |
|--------|-------|------|-------|
| 0 | SPRCTL0 | 1 byte | Sprite type, BPP, flips |
| 1 | SPRCTL1 | 1 byte | Direction, reload, sizing |
| 2 | SPRCOLL | 1 byte | Collision number |
| 3–4 | SCBNEXT | 2 bytes (LE) | Pointer to next SCB |
| 5–6 | SPRDLINE | 2 bytes | Sprite data line pointer |
| 7–8 | HPOSSTRT | 2 bytes | Horizontal start (signed) |
| 9–10 | VPOSSTRT | 2 bytes | Vertical start (signed) |
| 11+ | Variable | 2 bytes each | SPRHSIZ, SPRVSIZ (depth≥1) |
| | | | STRETCH (depth≥2), TILT (depth=3) |
| After | Palette | 8 bytes | Only if ReloadPalette bit=0 |

### SCB Termination

| Source | Condition |
|--------|-----------|
| Tech Docs (Bug 13.12) | Upper byte of SCBNEXT = `$00` |
| Handy | `(scbnext >> 8) == 0` |
| Mednafen | Identical to Handy |

**All agree:** Only the **high byte** of SCBNEXT is checked. SCBs cannot reside in zero page.

---

## 6. SPRCTL0 / SPRCTL1 Bit Definitions

**All three sources agree.**

### SPRCTL0 (`$FC80`)

| Bits | Field | All Sources |
|------|-------|-------------|
| [2:0] | Sprite Type (0–7) | `data & 0x07` |
| [3] | Unused | — |
| [4] | Vflip | `data & 0x10` |
| [5] | Hflip | `data & 0x20` |
| [7:6] | PixelBits (0–3 → 1–4 BPP) | `((data >> 6) & 3) + 1` |

### SPRCTL1 (`$FC81`)

| Bit | Field | All Sources |
|-----|-------|-------------|
| 0 | StartLeft | `data & 0x01` |
| 1 | StartUp | `data & 0x02` |
| 2 | SkipSprite | `data & 0x04` |
| 3 | ReloadPalette (inverted: 0=reload) | `data & 0x08` |
| [5:4] | ReloadDepth (0–3) | `(data >> 4) & 0x03` |
| 6 | Sizing | `data & 0x40` |
| 7 | Literal | `data & 0x80` |

### ReloadDepth values

| Value | Fields Loaded from SCB | All Sources |
|-------|----------------------|-------------|
| 0 | HPOS, VPOS only | ✅ |
| 1 | + HSIZE, VSIZE | ✅ |
| 2 | + STRETCH | ✅ |
| 3 | + TILT | ✅ |

---

## 7. Quadrant Rendering

**All three sources describe the same system. Tech docs are briefer.**

### Quadrant Map

```
         NW (2) │ NE (1)
        ────────┼────────
         SW (3) │ SE (0)
```

### Starting Quadrant

| StartLeft | StartUp | Starting Quadrant | All Sources |
|-----------|---------|-------------------|-------------|
| 0 | 0 | 0 (SE) | ✅ |
| 0 | 1 | 1 (NE) | ✅ |
| 1 | 1 | 2 (NW) | ✅ |
| 1 | 0 | 3 (SW) | ✅ |

### Quadrant Sign Convention

| Quadrant | hsign | vsign | Both Emulators |
|----------|-------|-------|----------------|
| 0 (SE) | +1 | +1 | ✅ |
| 1 (NE) | +1 | -1 | ✅ |
| 2 (NW) | -1 | -1 | ✅ |
| 3 (SW) | -1 | +1 | ✅ |

### Flip Integration

Both emulators apply Hflip/Vflip by **inverting** the respective sign:

- `if (Hflip) hsign = -hsign;`
- `if (Vflip) vsign = -vsign;`

### Superclipping

Both emulators use quadrant flip tables for off-screen sprite origins:

```cpp
int vquadflip[] = {1, 0, 3, 2};  // Vertical flip
int hquadflip[] = {3, 2, 1, 0};  // Horizontal flip
```

### Known Issue: Tilt-Based Quadrant Flip

Both emulators have this commented out:

```cpp
// if (enable_tilt && TILT & 0x8000) modquad = hquadflip[modquad];
// Note: "This is causing Eurosoccer to fail!!"
```

This suggests the hardware specification includes a tilt-flip behavior that doesn't work correctly in practice.

### Quadrant Offset Fix

Both emulators apply a 1-pixel offset when transitioning between quadrants to avoid double-drawing the origin line/column. The approach saves quad 0's sign direction and offsets subsequent quads that draw in the opposite direction:

```cpp
// Handy: static locals inside PaintSprites (persist across quads via static)
static int vquadoff = 0;
static int hquadoff = 0;

// For each quadrant:
if (loop == 0) vquadoff = vsign;   // Save quad 0's vertical sign
if (vsign != vquadoff) voff += vsign; // Offset if opposite direction

if (loop == 0) hquadoff = hsign;   // Save quad 0's horizontal sign
if (hsign != hquadoff) hoff += hsign; // Offset if opposite direction
```

**Nexen note:** Fixed in issue #370 — the vertical offset (`vquadoff`) was previously computed but never applied, and both offset variables were scoped inside the render block (resetting each quadrant). Now both are declared before the quad loop and correctly persist across all 4 quadrants.

---

## 8. Sprite Type Enum

**All three sources use the same 8 types with same values.**

| Value | Handy Name | Mednafen Name | Tech Doc Name |
|-------|-----------|---------------|---------------|
| 0 | `sprite_background_shadow` | Same | Background/Shadow |
| 1 | `sprite_background_noncollide` | Same | Background/No-Collide |
| 2 | `sprite_boundary_shadow` | Same | Boundary/Shadow |
| 3 | `sprite_boundary` | Same | Boundary |
| 4 | `sprite_normal` | Same | Normal |
| 5 | `sprite_noncollide` | Same | No-Collide |
| 6 | `sprite_xor_shadow` | Same | XOR/Shadow |
| 7 | `sprite_shadow` | Same | Shadow |

---

## 9. Auto-Clear Cascading Behavior

**Both emulators implement identical cascading clears. Tech docs do not explicitly document this.**

| Write to | Clears | Note |
|----------|--------|------|
| MATHD (`$FC52`) | MATHC → 0 | Stun Runner workaround |
| MATHB (`$FC54`) | MATHA → 0 | |
| MATHP (`$FC56`) | MATHN → 0 | |
| MATHH (`$FC60`) | MATHG → 0 | |
| MATHF (`$FC62`) | MATHE → 0 | |
| MATHM (`$FC6C`) | MATHL → 0, `Mathbit = false` | Overflow cleared |
| MATHK (`$FC6E`) | MATHJ → 0 | |

**Nexen note:** This cascading behavior is critical for correct operation. Without the MATHD→MATHC clear, Stun Runner fails because it writes D then C and expects C's sign conversion to see the fresh pair, not stale data.

---

## 10. RAM Address Masking

**Mednafen is more correct than Handy.**

| | Handy | Mednafen | Nexen |
|---|---|---|---|
| `RAM_PEEK(addr)` | `ram[addr]` | `ram[(uint16)addr]` | Uses 16-bit masking |
| `RAM_PEEKW(addr)` | `ram[addr] + ram[addr+1]<<8` | `ram[(uint16)addr] + ram[(uint16)(addr+1)]<<8` | Uses 16-bit masking |

The Lynx has a 64 KB address space (16-bit). Masking prevents out-of-bounds access when computing addresses that might overflow. Nexen follows Mednafen's approach.

---

## 11. Sprite Status Reporting

**Emulators differ in approach.**

| | Handy | Mednafen | Nexen |
|---|---|---|---|
| Sprite busy check | `gSystemCPUSleep` global | `gSuzieDoneTime` timestamp | State-based flag |
| Precision | Coarse (CPU asleep = sprites running) | Cycle-accurate (done at specific tick) | Frame-level |

Mednafen's timestamp approach is more precise for cycle-accurate emulation. Nexen uses a simple boolean flag, which is sufficient for game compatibility but less accurate for timing-sensitive code.

---

## 12. Savestate Serialization

| Field | Handy | Mednafen | Nexen |
|-------|-------|----------|-------|
| Math registers | Custom `lss_write` | `SFVAR()` macros | `SV()` macros |
| `hquadoff`/`vquadoff` | **Not saved** | **Saved** | **Not saved** (static) |
| Math sign tracking | Saved individually | Saved individually | Saved (grouped) |

**TODO:** Nexen should save `hquadoff`/`vquadoff` in savestates to match Mednafen's improvement. Mid-sprite-chain savestate load could otherwise desync.

---

## 13. Type Naming Conventions

| Concept | Handy | Mednafen | Nexen |
|---------|-------|----------|-------|
| 8-bit unsigned | `UBYTE` | `uint8` | `uint8_t` |
| 16-bit unsigned | `UWORD` | `uint16` | `uint16_t` |
| 32-bit unsigned | `ULONG` | `uint32` | `uint32_t` |
| 16-bit union | `UUWORD` | `Uuint16` | Bit manipulation |
| Word access | `.Word` | `.Val16` | Shift/mask |
| Byte access | `.Byte.Low`/`.High` | `.Union8.Low`/`.High` | Shift/mask |
| Boolean | `TRUE`/`FALSE` (ULONG) | `true`/`false` (bool) | `true`/`false` |

Nexen does not use unions for register access. Instead, grouped `uint32_t` fields (e.g., `MathABCD`) are accessed via shift and mask operations, which is more portable and avoids endianness issues.

---

## 14. Cartridge Port Handling

| Aspect | Handy | Mednafen | Tech Docs | Nexen |
|--------|-------|----------|-----------|-------|
| RCART0 address | `$FCB2` | `$FCB2` | `$FCB2` | `$FCB2` |
| RCART1 address | `$FCB3` | `$FCB3` | `$FCB3` | `$FCB3` |
| AUDIN check on write | Yes (CartGetAudin) | No | Not specified | No |
| EEPROM counter on write | Yes | No | Not specified | Separate EEPROM |

Handy integrates AUDIN and EEPROM counter processing into the cart port write. Mednafen and Nexen handle these at a higher system level.

---

## 15. Sizing Algorithm

| Aspect | Handy | Mednafen | Tech Docs | Nexen |
|--------|-------|----------|-----------|-------|
| Sizing modes | `enable_stretch`/`enable_tilt` vars | Identical | 4 modes (0–3) | Follows Handy |
| `enable_sizing` var | Tracked separately | **Omitted** | Mode flag in SPRCTL1 | Follows Handy |
| Algorithm 3 (shift) | Not used (broken HW) | Not used | Bug 13.11: "completely broken" | Not implemented |

Both emulators effectively skip sizing algorithm 3 because no games use it (the hardware is broken). Nexen follows this same approach.

---

## 16. Nexen Implementation Choices

### What Nexen follows from Handy/Mednafen (over tech docs):

1. **Divide trigger on MATHE** — Tech docs say "write N/P", both emulators trigger on MATHE write
2. **Cascading clears** — Not documented in tech docs, but implemented in both emulators
3. **HW Bug 13.8 exact algorithm** — `(value-1) & 0x8000` check from emulator source
4. **Stun Runner math init to 0xFFFFFFFF** — Not in tech docs, required by game
5. **SCBNEXT termination by high byte only** — Matches Bug 13.12

### Where Nexen follows Mednafen's improvements over Handy:

1. **RAM address masking** — 16-bit mask on all RAM access
2. **C++ standard types** — `uint32_t` over `ULONG`

### Nexen-specific design choices:

1. **Grouped math registers** — `MathABCD` (uint32_t) instead of 4 separate uint16_t union members
2. **Shift/mask access** — No unions, portable across endianness
3. **State-based sprite status** — Simple boolean instead of timestamp or global
4. **Integrated sign tracking** — `MathAB_sign`, `MathCD_sign`, `MathEFGH_sign` as int32_t

### Known gaps vs Mednafen (future work):

1. ~~**Savestate hquadoff/vquadoff** — Not yet serialized (low impact, affects mid-sprite savestate)~~ **FIXED** — Promoted to outer scope and applied correctly (issue #370)
2. **Cycle-accurate sprite timing** — Uses frame-level rather than tick-level timing
3. **AUDIN cart integration** — Not implemented (may affect rare cart types)

---

## Appendix A: Source File Cross-Reference

| Component | Handy | Mednafen | Nexen |
|-----------|-------|----------|-------|
| Suzy main | `susie.cpp` | `susie.cpp` | `LynxSuzy.cpp` |
| Suzy header | `susie.h` | `susie.h` | `LynxSuzy.h` |
| Definitions | `lynxdef.h` | `lynxdef.h` | `LynxTypes.h` |
| State struct | Part of `susie.h` | Part of `susie.h` | `LynxSuzyState` in `LynxTypes.h` |
| Math unit | Inline in `susie.cpp` | Inline in `susie.cpp` | `DoMultiply()`/`DoDivide()` in `LynxSuzy.cpp` |
| Sprite engine | `PaintSprites()` | `PaintSprites()` | `ProcessSprite()` in `LynxSuzy.cpp` |
