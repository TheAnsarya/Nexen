# WonderSwan PPU Cross-Emulator Verification

## Overview

Comprehensive cross-reference of Nexen's WS PPU implementation against hardware documentation and multiple reference emulators to validate correctness.

**Sources analyzed:**

- **WSdev Wiki** (authoritative hardware docs) — via Wayback Machine
- **ares** (gold-standard WS emulator)
- **MAME/MESS** (`src/mame/bandai/wswan_v.cpp`)
- **Mednafen** (`mednafen/wswan/gfx.c` via beetle-wswan-libretro)
- **NitroSwan** — ARM assembly, impractical for detailed comparison

## Comparison Matrix

| Feature | WSdev Wiki | ares | Nexen | MAME | Mednafen |
|---------|-----------|------|-------|------|----------|
| VTOTAL register ($16) | Default 158, 159 lines/frame | `io.vtotal = 158` | `LastScanline = 158` | Hardcoded `% 159` | `LCDVtotal` default 158 |
| Frame boundary | vtotal+1 lines | `max(144, vtotal+1)` | `max(144, LastScanline+1)` ✅ | `% 159` fixed | `MAX(144, LCDVtotal)+1` |
| Scanline modulo (low VTOTAL) | Implied by line counter restart | `y = vcounter % (vtotal+1)` | `renderY = Scanline % (LastScanline+1)` ✅ | ❌ Not implemented | ❌ Not implemented |
| Sprite table copy line | **Line 144** (explicit) | Line 144 | Line 144 | Line 144 | Line **142** ❌ |
| Sprite copy volume | 256 words over 256 clocks | `min(128, count)` entries | 512 bytes (full table) | `memcpy` full table | Partial copy |
| VBlank IRQ | Line 144 | Line 144 | Line 144 | Line 144 | Line 144 |
| Sprite limit/scanline | 32 (Y-only) | 32 (via objects queue) | 32 (`spriteCount >= 32 break`) | Not enforced | Not verified |
| Back Porch ($17) | WSC only, default 155 | `io.vsync = 155` | `BackPorchScanline = 155` (non-SC) | ❌ Not implemented | ❌ Not implemented |
| Register latching | 1-line display delay noted | Per-scanline in `scanline()` | Per-scanline in `ProcessEndOfScanline()` | ❌ No latching | ❌ No latching |
| Display 1-line delay | Palette changes at line 1 affect line 0 | Separate render/DAC phases | `Scanline-1` offset + live palette lookup | ❌ Not implemented | ❌ Not implemented |
| OAM double-buffering | Implicit (copy on 144, use next frame) | `oam[2]` with field toggle | Single buffer, temporal separation | Single buffer | `FrameWhichActive` toggle |
| Palette transparency | Opaque: 0-3, 8-11; Transparent: 4-7, 12-15 | ✅ | ✅ `!(palette & 0x04)` | ✅ | ✅ |
| 4bpp transparency | All palettes transparent (index 0) | ✅ | ✅ `mode <= Color2bpp` check | ✅ | ✅ |
| BG tile bank (color) | Bit 13 selects bank 0/1 (1024 tiles) | ✅ | ✅ `tilemapData & 0x2000` | ✅ | ✅ |
| Sprite tile bank | Bank 0 only (512 tiles) | ✅ | ✅ `bank0Addr` only | ✅ | ✅ |
| FG/Screen2 window | Inside/outside modes | ✅ | ✅ `DrawOutsideBgWindow` | 3 modes | ✅ |
| Sprite window | Per-sprite inside/outside | ✅ | ✅ `showOutsideWindow` flag | ✅ | ✅ |
| High Contrast ($14 bit 1) | LCD drives 2 lines per SoC line | `contrast[2]` stored | `HighContrast` stored | ❌ | ❌ |
| LCD sleep/white screen | Yes | ✅ | ✅ `SendFrame()` check | Not verified | Not verified |

## Key Findings

### Nexen Gets Right (Validated)

1. **VTOTAL / Frame boundary** — `max(144, LastScanline+1)` matches ares and WSdev wiki exactly. Frame always runs to at least 144 scanlines, preventing short frame timing disruption.
2. **Scanline modulo for low VTOTAL** — `renderY = Scanline % (LastScanline + 1)` repeats the displayed image when VTOTAL < 144. Only Nexen and ares implement this; MAME and Mednafen don't.
3. **VBlank inhibit for short VTOTAL** — VBlank IRQ only fires when `LastScanline >= 144`, matching ares behavior where `vcounter == 144` is never processed for `vtotal < 143`.
4. **Sprite table copy at line 144** — Only when `LastScanline >= 144`. Confirmed by WSdev wiki, ares, and MAME; Mednafen's line 142 is wrong.
5. **Full sprite table copy** — WSdev wiki explicitly says "256 words over 256 clocks" = 512 bytes; Nexen matches this better than ares (which optimizes to `min(128, count)`)
6. **Per-scanline register latching** — Matches ares approach for scroll, windows, sprite enable
7. **One-line display delay** — `Scanline-1` output offset + live palette lookups correctly implement the wiki's described behavior
8. **Back Porch register** — WSC-only (excludes SwanCrystal), default 155, matches wiki and ares
9. **Palette transparency rules** — Opaque/transparent palette groups handled correctly for 2bpp and 4bpp modes
10. **Sprite Y wrapping** — `uint8_t tileRow = scanline - y` naturally handles Y=252 wrapping to cover lines 0-3
11. **Framebuffer wrapping** — When VTOTAL < 144, `_prevRenderY` is used for framebuffer output position so that repeated scanlines overwrite the correct rows, matching ares DAC behavior

### Areas Where Nexen Exceeds Other Emulators

- **vs MAME**: Nexen supports variable VTOTAL, scanline modulo, register latching, back porch — MAME has none of these
- **vs Mednafen**: Nexen has correct sprite copy timing (144 vs 142), scanline modulo, register latching
- **vs ares (hardware fidelity)**: Nexen copies the full 512-byte sprite table per-cycle as the wiki describes; ares optimizes to only copy `count` entries

### Minor Items Not Implemented (by any emulator)

- **High Contrast visual effect** — LCD-level effect, not SoC rendering; no emulator implements the visual doubling
- **TFT LCD Configuration ($70-$77)** — SwanCrystal boot ROM only, read-only after lock

### No Actionable Discrepancies Found

The cross-emulator verification confirms that Nexen's WS PPU implementation is correct and in many cases more accurate than MAME and Mednafen. The implementation aligns with the authoritative WSdev wiki documentation and matches or exceeds the ares gold-standard reference.

## Emulator-Specific Notes

### ares (Gold Standard)

- Most complete WS PPU implementation
- Double-buffered OAM with field toggle
- Per-pixel rendering pipeline (screen1 → screen2 → sprite → dac)
- Comment in source: "vtotal<143 inhibits vblank and repeats the screen image until vcounter=144"
- Sprite `scanline(y)` reads from `oam[!field]` (previous frame's copy)

### MAME/MESS

- Hardcoded `m_current_line = (m_current_line + 1) % 159` — no VTOTAL register at all
- No variable frame timing, no scanline modulo, no register latching
- Uses live register values directly in rendering
- Simplest implementation; adequate for most games but not hardware-accurate

### Mednafen (beetle-wswan-libretro)

- HAS VTOTAL register, uses it for frame length (`MAX(144, LCDVtotal)+1`)
- BUT no scanline modulo for rendering — uses `wsLine` directly
- Sprite copy at line 142 (incorrect per WSdev wiki)
- Uses `FrameWhichActive` for OAM double-buffering between frames
- Direct buffer rendering (no double-buffered row data)

## Verification Date

Analysis performed against:

- ares: master branch (2025)
- MAME: `src/mame/bandai/wswan_v.cpp` (current)
- Mednafen: beetle-wswan-libretro `mednafen/wswan/gfx.c`
- WSdev Wiki: `ws.nesdev.org/wiki/Display` (Jan 2025 snapshot)
- Nexen: current master (post-VTOTAL frame boundary fix)
