# Mesen2 Open Pull Requests — Evaluation & Tracking

> **Last Updated:** 2026-03-02
> **Source:** [SourMesen/Mesen2 Pull Requests](https://github.com/SourMesen/Mesen2/pulls)
> **Total Open PRs Evaluated:** 24
> **Scope:** All open PRs as of 2026-03-01. Closed/merged PRs are not tracked (assumed already incorporated or irrelevant).

---

## Summary

| Priority | Count | Description |
|----------|-------|-------------|
| 🔴 HIGH | 6 | Emulation accuracy fixes — all applied |
| 🟡 MEDIUM | 5 | Features & stability — 2 applied, 3 deferred |
| 🟢 LOW | 13 | Build system, minor UI, stale — all skipped or deferred |

---

## Status Legend

| Status | Meaning |
|--------|---------|
| ✅ Applied | Fix re-implemented in Nexen |
| ⏳ In Progress | Currently being worked on |
| 🔍 Researching | Evaluating applicability |
| ⏸️ Deferred | Valid but not prioritized now |
| ❌ Skipped | Not applicable to Nexen |

---

## 🔴 HIGH Priority — Emulation Accuracy

### PR #87 — SNES: Fix integer overflow for calculating DMA overhead
- **Mesen2 PR:** [#87](https://github.com/SourMesen/Mesen2/pull/87)
- **Author:** denniskempin
- **Nexen Issue:** #509
- **Status:** ✅ Applied
- **Category:** SNES DMA
- **Risk:** Zero — one-line type widening

**Problem:** `RunDma` transfers up to 2^16 bytes, but the loop counter `i` is `uint8_t`, causing overflow at 256 iterations. This makes `SyncEndDma` miscalculate cycle overhead.

**Mesen2 Fix:** Change `uint8_t i = 0;` → `uint32_t i = 0;` in `SnesDmaController.cpp`.

**Nexen Implementation:** Re-implemented the same fix in our codebase. The DMA controller code is structurally identical.

---

### PR #82 — NES: Fixed GetInternalOpenBus returning external bus value by mistake
- **Mesen2 PR:** [#82](https://github.com/SourMesen/Mesen2/pull/82)
- **Author:** zdg-kinlon
- **Nexen Issue:** #510
- **Status:** ✅ Applied
- **Category:** NES Bus Accuracy
- **Risk:** Zero — one-line return value fix

**Problem:** `OpenBusHandler::GetInternalOpenBus()` returns `_externalOpenBus` instead of `_internalOpenBus`. After a `$4015` read, internal and external bus values differ, breaking correctness for code relying on the internal bus state.

**Mesen2 Fix:** `return _externalOpenBus;` → `return _internalOpenBus;`

**Nexen Implementation:** Re-implemented the same fix. Our `OpenBusHandler` has the same structure.

---

### PR #86 — SNES: CX4 cache and timing improvements
- **Mesen2 PR:** [#86](https://github.com/SourMesen/Mesen2/pull/86)
- **Author:** AkiteruSDA
- **Nexen Issue:** #511
- **Status:** ✅ Applied
- **Category:** SNES CX4 Coprocessor
- **Risk:** Low — affects only CX4 games (Mega Man X2/X3)

**Problem:** Multiple CX4 timing inaccuracies:
1. Bus access adds an extra cycle (should be 1+WS, not 1+1+WS)
2. Cache preloading via `$7F48` write doesn't work correctly
3. Program bank not set correctly for MMX2/3's CX4 setup subroutine
4. IRQ flag initialized incorrectly

**Impact:** Fixes attract mode desync in Mega Man X2. Already validated in bsnes and MiSTer.

**Mesen2 Fix:** Removes +1 from bus delay (4 locations), adds `Preload` flag to `Cx4Cache`, sets PB from ProgramBank on `$7F48` write, fixes IRQ flag `true`→`false`, changes cycle comparison `>` to `>=`, serializes new field.

**Nexen Implementation:** Re-implemented all changes against our CX4 codebase.

---

### PR #80 — SNES: Fixed high resolution blending
- **Mesen2 PR:** [#80](https://github.com/SourMesen/Mesen2/pull/80)
- **Author:** Stovehead
- **Nexen Issue:** #512
- **Status:** ✅ Applied
- **Category:** SNES Video Filter
- **Risk:** Zero — only affects "Blend Hi-Res" option

**Problem:** The "Blend high resolution modes" option blurs the entire screen because every pixel is blended with its right neighbor. Correct behavior is to pair pixels (0-1, 2-3, etc.) and blend only within pairs.

**Mesen2 Fix:** In `SnesDefaultVideoFilter.cpp`, change loop from `j++` to `j += 2`, create references to pixel pairs, blend and set both to same result.

**Nexen Implementation:** Re-implemented the blending fix in our video filter code.

---

### PR #31 — Fix various bugs in NES NTSC filter and PPU palettes
- **Mesen2 PR:** [#31](https://github.com/SourMesen/Mesen2/pull/31)
- **Author:** Gumball2415 (Persune)
- **Nexen Issue:** #513
- **Status:** ✅ Applied
- **Category:** NES Video Accuracy
- **Risk:** Medium — touches multiple video subsystems

**Problem:** Three distinct issues:
1. **Dot crawl phase offset** — Incorrect calculation causes wrong NTSC color rendering per-frame
2. **Bisqwit NTSC matrix** — Non-standard RGB-YIQ coefficients
3. **RGB PPU emphasis** — Default palette emphasis behavior incorrect

**Mesen2 Fix:** 10 files changed. Adds `_masterClockFrameStart` tracking, replaces hardcoded phase offsets with `GetVideoPhaseOffset()`, updates Bisqwit NTSC matrix to standard RGB-YIQ values, fixes RGB PPU emphasis.

**Nexen Implementation:** Re-implemented all three fixes across 10 files:
1. Added `_masterClockFrameStart` to `BaseNesPpu.h`, captured at scanline 0 cycle 1 in `NesPpu.cpp`, used for phase calculation (`(_masterClockFrameStart % 6) * 2`)
2. Renamed `VideoPhase` → `VideoPhaseOffset` through entire chain (`RenderedFrame.h`, `BaseVideoFilter.h/.cpp`, `VideoDecoder.cpp`)
3. Updated `NesNtscFilter.cpp` and `BisqwitNtscFilter.cpp` to use new `GetVideoPhaseOffset()` with correct phase math
4. Replaced Bisqwit IQ coefficients with standard RGB-YIQ matrix values
5. Added `PpuModel` parameter to `GenerateFullColorPalette()` — 2C02 uses 0.84 dimming, RGB PPUs set emphasized channel to 0xFF

---

### PR #74 — SNES: Add support for ExLoRom mapping
- **Mesen2 PR:** [#74](https://github.com/SourMesen/Mesen2/pull/74)
- **Author:** yuriks (Yuri Kunde Schlesner)
- **Nexen Issue:** #514
- **Status:** ✅ Applied
- **Category:** SNES ROM Mapping
- **Risk:** Low — adds new mapping, doesn't change existing behavior

**Problem:** ExLoRom (Extended LoRom) memory mapping is used by ROM hacks but unsupported. Also has an incorrect `0x22` map mode check that triggers for S-DD1 games.

**Mesen2 Fix:** In `BaseCartridge.cpp`: adds `0x400000`/`0x400200` header scan addresses, masks header score address, sets `CartFlags::ExLoRom`, registers handlers with `0x400` page offset, removes incorrect `0x22` check.

**Nexen Implementation:** Re-implemented ExLoRom support and fixed S-DD1 map mode check in our cart loading code.

---

## 🟡 MEDIUM Priority — Features & Stability

### PR #85 — UI: Improve Memory View performance on Linux
- **Mesen2 PR:** [#85](https://github.com/SourMesen/Mesen2/pull/85)
- **Author:** Vrabbers
- **Nexen Issue:** #515
- **Status:** ✅ Applied
- **Category:** UI Performance
- **Risk:** Zero — caching optimization only

**Problem:** On Linux, `SKTypeface.FromFamilyName()` calls through FontConfig are extremely slow, causing UI freezes when Memory View is open during emulation.

**Mesen2 Fix:** Caches `SKTypeface` objects with a static tuple, adds `GetCachedTypeface()` helper.

**Nexen Implementation:** Re-implemented typeface caching in our `HexEditor.HexViewDrawOperation.cs`. We already had some caching from our performance work, but this adds the FontConfig-specific optimization.

---

### PR #76 — Debugger: Lua - Fix CTD if callback raises non-string error
- **Mesen2 PR:** [#76](https://github.com/SourMesen/Mesen2/pull/76)
- **Author:** HertzDevil (Quinton Miller)
- **Nexen Issue:** #516
- **Status:** ✅ Applied
- **Category:** Debugger Crash Fix
- **Risk:** Zero — replaces null-unsafe call with safe alternative

**Problem:** Lua's `error()` can raise any object. When a non-string is raised, `lua_tostring` returns null, and `std::string(nullptr)` crashes.

**Mesen2 Fix:** Replace `lua_tostring` with `luaL_tolstring`, adjust `lua_pop` count from 1 to 2.

**Nexen Implementation:** Applied the same fix to our `ScriptingContext.cpp`.

---

### PR #75 — Debugger: Lua - Add `emu.readRegister` and `emu.writeRegister`
- **Mesen2 PR:** [#75](https://github.com/SourMesen/Mesen2/pull/75)
- **Author:** HertzDevil (Quinton Miller)
- **Nexen Issue:** #517
- **Status:** ⏸️ Deferred
- **Category:** Debugger API
- **Risk:** Medium — 29 files, touches all platform debuggers

**Problem:** `emu.getState()` returns a massive table (602 entries for SNES) just to read one register value. Dedicated register access functions would be more efficient.

**Mesen2 Fix:** Adds `RegisterType` enum, implements `GetRegisterValue`/`SetRegisterValue` on all platform debuggers, adds Lua bindings and C# interop. 29 files changed.

**Nexen Assessment:** Excellent feature for TAS scripting but very large scope. Deferred for dedicated implementation sprint. Should be adapted to our codebase structure rather than directly ported, especially since we have Lynx and WonderSwan additions.

---

### PR #58 — MSU-1: Add support for .ogg files, and the .msu1 zip-file format
- **Mesen2 PR:** [#58](https://github.com/SourMesen/Mesen2/pull/58)
- **Author:** Gutawer
- **Nexen Issue:** #518
- **Status:** ⏸️ Deferred
- **Category:** SNES MSU-1 Enhancement
- **Risk:** Medium — new audio codec + archive format

**Problem:** MSU-1 PCM files are huge. OGG support (10x smaller) and `.msu1` zip format (used by Snes9x) improves usability for ROM hacks using MSU-1 audio.

**Mesen2 Fix:** Moves OggReader to shared location, adds `.ogg` fallback in PcmReader, adds `.msu1` archive format detection. 12 files changed.

**Nexen Assessment:** Good feature but lower priority than emulation accuracy. Deferred for later sprint.

---

### PR #81 — 128KB VRAM SNES core
- **Mesen2 PR:** [#81](https://github.com/SourMesen/Mesen2/pull/81)
- **Author:** slidelljohn
- **Nexen Issue:** #519
- **Status:** ⏸️ Deferred
- **Category:** SNES PPU (Experimental)
- **Risk:** HIGH — changes fundamental VRAM size, missing 64K/128K toggle

**Problem:** Some hardware-modified consoles have 128KB VRAM. This PR enables support but lacks a toggle — it always uses 128KB, which could break standard games.

**Mesen2 Fix:** Changes `VideoRamSize` from 64K to 128K, widens address masks, updates all VRAM references.

**Nexen Assessment:** Interesting experimental feature but **unsafe without a configuration toggle**. Would need additional work to add a setting before adoption. Deferred.

---

## 🟢 LOW Priority — Build System, Minor UI, Stale

### PR #79 — Fix Linux + Mac Builds
- **Mesen2 PR:** [#79](https://github.com/SourMesen/Mesen2/pull/79)
- **Author:** culix-7
- **Nexen Issue:** #520
- **Status:** ❌ Skipped
- **Category:** Build System
- **Reason:** Nexen has its own CI/build system. Not applicable.

### PR #78 — Add C++ Test Project
- **Mesen2 PR:** [#78](https://github.com/SourMesen/Mesen2/pull/78)
- **Author:** culix-7
- **Nexen Issue:** #520
- **Status:** ❌ Skipped
- **Category:** Testing
- **Reason:** Nexen already has `Core.Tests` with 1495+ Google Test tests. MS Test approach not needed.

### PR #77 — Windows Build Fix
- **Mesen2 PR:** [#77](https://github.com/SourMesen/Mesen2/pull/77)
- **Author:** culix-7
- **Nexen Issue:** #520
- **Status:** ❌ Skipped
- **Category:** Build System
- **Reason:** Nexen has its own Windows build setup.

### PR #83 — Linux Compilation and AppImage Fixes
- **Mesen2 PR:** [#83](https://github.com/SourMesen/Mesen2/pull/83)
- **Author:** DocJr90
- **Nexen Issue:** #520
- **Status:** ❌ Skipped
- **Category:** Build System
- **Reason:** Nexen has its own Linux build and AppImage setup.

### PR #72 — Combine macOS Releases into Universal Binary
- **Mesen2 PR:** [#72](https://github.com/SourMesen/Mesen2/pull/72)
- **Author:** jroweboy
- **Nexen Issue:** #520
- **Status:** ❌ Skipped
- **Category:** Build System (macOS)
- **Reason:** macOS CI approach may differ. Can reference later if needed.

### PR #84 — Add Galaxian to Cheats
- **Mesen2 PR:** [#84](https://github.com/SourMesen/Mesen2/pull/84)
- **Author:** BdR76
- **Nexen Issue:** #520
- **Status:** ⏸️ Deferred
- **Category:** Cheat Database
- **Reason:** Minor cheat DB addition. Note: PR has a JSON syntax error that needs fixing.

### PR #56 — Debugger: Tilemap Viewer - Display Attribute Bits (NES)
- **Mesen2 PR:** [#56](https://github.com/SourMesen/Mesen2/pull/56)
- **Author:** gzip
- **Nexen Issue:** #520
- **Status:** ⏸️ Deferred
- **Category:** Debugger UI
- **Reason:** Minor 3-line debugger display enhancement. Can add later.

### PR #57 — Debugger: Tile Viewer - Navigate by Tile Button
- **Mesen2 PR:** [#57](https://github.com/SourMesen/Mesen2/pull/57)
- **Author:** gzip
- **Nexen Issue:** #520
- **Status:** ⏸️ Deferred
- **Category:** Debugger UI
- **Reason:** Minor UX improvement. Can add later.

### PR #49 — Debugger: Tile Viewer - Copy/Paste Tile Memory
- **Mesen2 PR:** [#49](https://github.com/SourMesen/Mesen2/pull/49)
- **Author:** gzip
- **Nexen Issue:** #520
- **Status:** ⏸️ Deferred
- **Category:** Debugger UI
- **Reason:** Useful for ROM hacking workflow. Can add in a debugger enhancement sprint.

### PR #48 — Debugger: Tilemap Viewer - 8x8 Edit + Attribute Memory View
- **Mesen2 PR:** [#48](https://github.com/SourMesen/Mesen2/pull/48)
- **Author:** gzip
- **Nexen Issue:** #520
- **Status:** ⏸️ Deferred
- **Category:** Debugger UI
- **Reason:** Debugger enhancement. Can add later.

### PR #61 — UI: Trace Logger Localization Strings
- **Mesen2 PR:** [#61](https://github.com/SourMesen/Mesen2/pull/61)
- **Author:** icefairy64
- **Nexen Issue:** #520
- **Status:** ⏸️ Deferred
- **Category:** UI / Localization
- **Reason:** Minor localization improvement. Low priority.

### PR #32 — UI: Respect Data Storage Location Setting
- **Mesen2 PR:** [#32](https://github.com/SourMesen/Mesen2/pull/32)
- **Author:** Qxe5
- **Nexen Issue:** #520
- **Status:** ⏸️ Deferred
- **Category:** UI Config
- **Reason:** Minor config cleanup. Low priority.

### PR #18 — Add Lua LSP Integration Support
- **Mesen2 PR:** [#18](https://github.com/SourMesen/Mesen2/pull/18)
- **Author:** SalHe
- **Nexen Issue:** #520
- **Status:** ❌ Skipped
- **Category:** Debugger (Stale)
- **Reason:** 3+ years old, targets Avalonia preview6. Would need complete rewrite. Concept is interesting but implementation is dead.

---

## Implementation Log

| Date | PR | Action | Commit |
|------|----|--------|--------|
| 2026-03-01 | #87 | Applied DMA overflow fix | b193f0c5 |
| 2026-03-01 | #82 | Applied internal open bus fix | b193f0c5 |
| 2026-03-01 | #86 | Applied CX4 timing fixes | b193f0c5 |
| 2026-03-01 | #80 | Applied hi-res blending fix | b193f0c5 |
| 2026-03-01 | #74 | Applied ExLoRom mapping support | b193f0c5 |
| 2026-03-01 | #85 | Applied typeface caching | b193f0c5 |
| 2026-03-01 | #76 | Applied Lua crash fix | b193f0c5 |
| 2026-03-02 | #31 | Applied NES NTSC filter + PPU palette fixes | 4de511a5 |
