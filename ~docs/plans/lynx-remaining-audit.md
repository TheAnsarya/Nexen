# Lynx Emulator — Remaining Audit Work

## Status Summary

**Epic:** [#409 — Lynx Emulator Accuracy & UI Completeness Audit](https://github.com/TheAnsarya/Nexen/issues/409)

**Completed sub-issues (28 closed):**

- #410–#417 (Phase 1): Input, movie, shortcuts, MAPCTL, LeftHand, pen index, timer burst
- #480–#482: PAL framerate, input overrides, GetEffectiveAddress
- #489: NZ table, LFSR popcount, branch hints
- #490–#495: Audit fixes, StopOnCurrent, TimerDone HW Bug
- #496–#503 audit/perf rounds
- #504–#506: Sprite viewer packed mode, tilemap flip, DummyCpu allocation

**Remaining open issues (2 sub + 1 epic):**

- **#496** — Audio timers duplicated independently from Mikey system timers (medium priority, architectural)
- **#499** — EEPROM Save Support — Cart write path (low priority)
- **#409** — Epic tracker (stay open until all sub-issues complete)

---

## Issue #496: Audio Timer Dedup (Architectural)

### Problem

On real Lynx hardware, audio channels 0–3 ARE Mikey timers 0–3. They share the same
timer registers at `$FD00–$FD0F`. The current implementation has a separate `LynxApu`
class with its own independent timer system (`TickChannelTimer` with separate prescaler
tracking, `LastTick` per channel), while Mikey has its own `TickTimer` for timers 0–7.

### Impact

- Audio channels won't respond to IRQ enables on corresponding Mikey timers
- Timer linking/cascade won't interact properly between audio and system timers
- Games reading timer COUNT values via Mikey registers won't see audio timer state
- Most games work fine because they configure audio channels via the audio channel
  register block and don't mix timer/audio access

### Architecture Assessment

Current separation:

```text
LynxMikey::Tick() → TickTimer(0..7) for system timers
LynxApu::Tick()   → TickChannelTimer(0..3) for audio   ← DUPLICATE
```

Target architecture:

```text
LynxMikey::Tick() → TickTimer(0..3) for timers 0-3
                     ↓ on underflow
                     ClockAudioChannel(ch)  → LynxApu::ClockChannel(ch)
```

### Phase Plan

1. **Research** — Document exactly which timer registers overlap and how
   Mikey timer 0–3 and audio channel 0–3 share state on real hardware
2. **Design** — Create data flow diagram showing unified timer architecture
3. **Prototype** — Branch experiment merging timer & audio state
4. **Implement** — Merge `LynxApu` channel timers into `LynxMikey` timer 0–3
5. **Test** — Verify audio still sounds correct in test ROMs

**Estimated effort:** 3–5 sessions (high risk, requires careful testing)

---

## Issue #499: EEPROM Save — Cart Write Path

### Problem

The Suzy RCART0/RCART1 write path (`$FCB2`/`$FCB3`) is stubbed — writes are ignored.
On real hardware, writes go through the cart interface to the EEPROM chip using the
Microwire serial protocol (93C46/93C66/93C86).

### Current State

- `LynxEeprom` class fully implements the Microwire protocol (read, write, erase, EWEN/EWDS)
- EEPROM is accessed via IODAT/IODIR ($FD88/$FD89) — CS, CLK, DI/DO signals
- Battery save/load already works via `LynxEeprom::SaveBattery()`/`LoadBattery()`
- The only missing piece: Suzy `$FCB2`/`$FCB3` write path doesn't forward to cart

### Assessment

The EEPROM actually works through the Mikey I/O port (IODAT/$FD89), NOT through the
cart write path. The Suzy RCART0/RCART1 write path is for games that write to cartridge
RAM (some homebrew carts have writable SRAM). This is a LOW priority issue because:

1. Commercial games with EEPROM use the Mikey I/O port (already implemented)
2. Only homebrew multicarts with SRAM need the cart write path
3. The issue title is slightly misleading — EEPROM itself already works

### Fix Plan

1. Add `WriteData(uint8_t value)` to `LynxCart` that writes to save RAM
2. Wire Suzy `$FCB2`/`$FCB3` write cases to `LynxCart::WriteData()`
3. Test with a homebrew ROM that uses cart SRAM

**Estimated effort:** 1 session (low risk)

---

## Other Minor Improvements Found

### SYSCTL1 ($FD87) — System Control Register

**Currently:** `// TODO: system control (power off, cart power, etc.)` — writes ignored
**On hardware:**

- Bit 1: Cart address strobe (directly controls CART0/CART1 accent lines)
- Bit 2: Cart power control
- Bit 0: Power off (sends CPU to sleep)
- Bit 3: Signal to external hardware

Most bits are hardware-only (power off = suspend console, cart power = physical rail).
The cart address strobe is the only emulation-relevant bit.

**Fix:** Document what each bit does and log writes for debugging. No emulation change
needed for power off/cart power (those are physical actions).

### LynxPpuTools TODO — Tile Viewer Sprite Data

The tile viewer TODO says "Decode sprite pixel data from cart ROM as tiles". This would
allow viewing sprite graphics stored in ROM without having them rendered. Low priority
as the sprite viewer already works.

---

## Files Overview (40 files, ~6,700 lines)

| Component | Files | Lines | Notes |
|-----------|-------|-------|-------|
| LynxSuzy | .h+.cpp | 1,175 | Sprite engine, math coprocessor |
| LynxMikey | .h+.cpp | 1,031 | Timers, palette, display, UART |
| LynxCpu | .h+.cpp | 961 | 65SC02 CPU core |
| LynxConsole | .h+.cpp | 521 | Top-level integration |
| LynxPpuTools | .h+.cpp | 468 | Debug viewers |
| LynxDisUtils | .h+.cpp | 400 | Disassembly |
| LynxDecrypt | .h+.cpp | 621 | ROM decrypt/decrypt verification |
| LynxDebugger | .h+.cpp | 441 | Debugger hooks |
| LynxApu | .h+.cpp | 340 | Audio (4-channel LFSR) |
| LynxEeprom | .h+.cpp | 309 | 93C46/66/86 Microwire |
| LynxCart | .h+.cpp | 162 | Cart/bank switching |
| LynxMemoryManager | .h+.cpp | 284 | Memory map |
| LynxTypes | .h | 500+ | Enums, constants, state structs |
| Other | 7 files | ~500 | Controller, filter, trace, event |
