# Hot Path Optimization Plan — Phase 2

**Date:** 2026-02-11
**Branch:** `cpp-modernization`
**Epic:** [Epic 216] Hot Path Optimization Phase 2
**Prerequisite:** Phase 1 complete (commit ae778ff4 — branchless NES/SNES `SetZeroNegativeFlags`, HexUtilities buffer, ColorHelper float)

## Overview

Comprehensive performance optimization targeting CPU emulation hot paths across all three major systems (Game Boy, NES, SNES), plus PPU rendering and memory dispatch annotations. All changes are **zero-cost or low-risk** — no behavioral changes, no accuracy regressions.

**Key Lesson from Phase 1:** GB `HalfCarry` branchless was **36% slower** than branched — branch predictor handles well-predicted boolean patterns efficiently. Always benchmark before committing.

---

## Optimization Categories

### Category A: `[[likely]]`/`[[unlikely]]` Annotations (ZERO RISK)

These are compiler hints only. No behavioral change. No accuracy risk. Can be batched into a single commit.

| ID | Location | Function | Pattern | Calls/Frame |
|----|----------|----------|---------|-------------|
| A1 | `SnesMemoryManager.cpp:232` | `Exec()` | `_hClock == _nextEventClock` → `[[unlikely]]` | ~335,000 |
| A2 | `SnesMemoryManager.cpp:244` | `Exec()` | `_cart->SyncCoprocessors()` inner check → `[[unlikely]]` | ~335,000 |
| A3 | `SnesCpu.cpp:120` | `ProcessCpuCycle()` | `HasPendingTransfer()` → `[[unlikely]]` | ~180,000 |
| A4 | `SnesCpu.Shared.h:1062` | `DetectNmiSignalEdge()` | `_state.NmiFlagCounter` → `[[unlikely]]` | ~180,000 |
| A5 | `NesCpu.cpp:326` | `EndCpuCycle()` | `!_prevNmiFlag && _state.NmiFlag` → `[[unlikely]]` | ~90,000 |
| A6 | `NesPpu.cpp:1387` | `Exec()` | `_needStateUpdate` → `[[unlikely]]` | ~89,000 |
| A7 | `NesPpu.cpp:853` | `GetPixelColor()` | Sprite0Hit condition → `[[unlikely]]` | ~61,440 |
| A8 | `GbMemoryManager.cpp:183` | `Read()` | `HasCheats<>()` → `[[unlikely]]` | ~17,556 |
| A9 | `GbMemoryManager.cpp:175` | `Read()` | `IsOamDmaRunning()` → `[[unlikely]]` | ~17,556 |
| A10 | `SnesCpu.Instructions.h:6` | `Add8()` | `CheckFlag(ProcFlags::Decimal)` → `[[unlikely]]` | ~3,000–15,000 |
| A11 | `SnesCpu.Instructions.h:38` | `Add16()` | `CheckFlag(ProcFlags::Decimal)` → `[[unlikely]]` | ~3,000–15,000 |
| A12 | `SnesCpu.Instructions.h:89` | `Sub8()` | `CheckFlag(ProcFlags::Decimal)` → `[[unlikely]]` | ~3,000–15,000 |
| A13 | `SnesCpu.Instructions.h:121` | `Sub16()` | `CheckFlag(ProcFlags::Decimal)` → `[[unlikely]]` | ~3,000–15,000 |

**Issue:** [216.1] `[[likely]]`/`[[unlikely]]` annotations across hot paths

---

### Category B: Branchless GB `SetFlagState` (LOW RISK)

The current `GbCpu::SetFlagState` at `GbCpu.cpp:1053` uses if/else branching:

```cpp
void GbCpu::SetFlagState(uint8_t flag, bool state) {
    if (state) {
        SetFlag(flag);       // _state.Flags |= flag
    } else {
        ClearFlag(flag);     // _state.Flags &= ~flag
    }
}
```

**Branchless replacement:**

```cpp
void GbCpu::SetFlagState(uint8_t flag, bool state) {
    _state.Flags = (_state.Flags & ~flag) | (state ? flag : 0);
    // Or: _state.Flags = (_state.Flags & ~flag) | (-static_cast<uint8_t>(state) & flag);
}
```

**Call frequency:** Called 3–4× per ALU instruction. GB CPU runs ~17,556 M-cycles/frame with ~4,000–8,000 ALU instructions → **~12,000–32,000 calls/frame**.

**Callers (20+ call sites):** `INC`, `DEC`, `ADD`, `ADC`, `SUB`, `SBC`, `AND`, `OR`, `XOR`, `CP`, `LD_HL`, `ADD_SP`, `RL`, `RLC`, `RR`, `RRC`, `SRL`, `SRA`, `SLA`, `SWAP`, `BIT`.

**Flag constants:** Zero=0x80, AddSub=0x40, HalfCarry=0x20, Carry=0x10.

**⚠️ MUST BENCHMARK:** The Phase 1 HalfCarry branchless was 36% slower. Existing benchmarks in `Core.Benchmarks/Gameboy/GbCpuBench.cpp` already test zero flag branchless — extend with `SetFlagState` comparison.

**Issue:** [216.2] Branchless GB `SetFlagState`

---

### Category C: Branchless NES `CMP` Flags (LOW RISK)

The current `NesCpu::CMP` at `NesCpu.h:332` uses three separate branches:

```cpp
void CMP(uint8_t reg, uint8_t value) {
    ClearFlags(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
    auto result = reg - value;
    if (reg >= value) SetFlags(PSFlags::Carry);      // Branch 1
    if (reg == value) SetFlags(PSFlags::Zero);        // Branch 2
    if ((result & 0x80) == 0x80) SetFlags(PSFlags::Negative);  // Branch 3
}
```

**Branchless replacement:**

```cpp
void CMP(uint8_t reg, uint8_t value) {
    ClearFlags(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
    auto result = reg - value;
    // Carry: reg >= value → no borrow
    _state.PS |= (reg >= value) ? PSFlags::Carry : 0;
    // Zero: result == 0 (equivalently reg == value)
    _state.PS |= (result == 0) ? PSFlags::Zero : 0;
    // Negative: bit 7 of result (PSFlags::Negative = 0x80 maps directly)
    _state.PS |= (result & 0x80);
}
```

**Call frequency:** CMP/CPX/CPY are among the most frequent NES instructions → **~5,000–20,000 calls/frame**.

**Also applies to NES `ADD` (Overflow/Carry), `BIT`, `ASL`/`LSR`/`ROL`/`ROR`** — same branchy patterns.

### NES `ADD` (ADC/SBC) — `NesCpu.h:313`

```cpp
// Current: two branches for Overflow and Carry
if (~(A() ^ value) & (A() ^ result) & 0x80) SetFlags(PSFlags::Overflow);
if (result > 0xFF) SetFlags(PSFlags::Carry);
```

**Branchless:**

```cpp
_state.PS |= ((~(A() ^ value) & (A() ^ result) & 0x80) != 0) ? PSFlags::Overflow : 0;
_state.PS |= (result > 0xFF) ? PSFlags::Carry : 0;
```

### NES `BIT` — `NesCpu.h:471`

```cpp
// Current: three branches
if ((A() & value) == 0) SetFlags(PSFlags::Zero);
if (value & 0x40) SetFlags(PSFlags::Overflow);
if (value & 0x80) SetFlags(PSFlags::Negative);
```

**Branchless:**

```cpp
_state.PS |= ((A() & value) == 0) ? PSFlags::Zero : 0;
_state.PS |= (value & 0x40);   // Overflow = 0x40, maps directly
_state.PS |= (value & 0x80);   // Negative = 0x80, maps directly
```

### NES Shift/Rotate — `NesCpu.h:373–418`

Each has `if (value & bit) SetFlags(PSFlags::Carry)` — one branch per call.

**Branchless (ASL example):**

```cpp
uint8_t ASL(uint8_t value) {
    ClearFlags(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
    _state.PS |= (value >> 7);  // Carry = bit 7 shifted to bit 0 (PSFlags::Carry = 0x01)
    uint8_t result = value << 1;
    SetZeroNegativeFlags(result);
    return result;
}
```

**Issue:** [216.3] Branchless NES CPU flag operations (`CMP`, `ADD`, `BIT`, shifts)

---

### Category D: Branchless SNES `Compare` Carry (LOW RISK)

The current `SnesCpu::Compare` at `SnesCpu.Instructions.h:326` uses if/else branching for 8-bit and 16-bit paths:

```cpp
if ((uint8_t)reg >= value) SetFlags(ProcFlags::Carry);
else ClearFlags(ProcFlags::Carry);
```

**Branchless replacement:**

```cpp
// ProcFlags::Carry = 0x01, so we can use setge directly
_state.PS = (_state.PS & ~ProcFlags::Carry) | ((uint8_t)reg >= value);
```

**Also applies to SNES `ShiftLeft`/`ShiftRight`/`RollLeft`/`RollRight`** — same if/else Carry pattern.

### SNES `Add8`/`Add16`/`Sub8`/`Sub16` — Overflow branches

```cpp
// Current: if/else for Overflow
if (~(_state.A ^ value) & (_state.A ^ result) & 0x80) SetFlags(ProcFlags::Overflow);
else ClearFlags(ProcFlags::Overflow);
```

**Branchless:**

```cpp
uint8_t overflow = ((~(_state.A ^ value) & (_state.A ^ result) & 0x80) != 0) ? ProcFlags::Overflow : 0;
_state.PS = (_state.PS & ~ProcFlags::Overflow) | overflow;
```

Note: The Carry branch in `Add8`/`Add16` follows `ClearFlags(Carry)` then conditional `SetFlags(Carry)` — already a clear/conditional-set pattern that compilers may optimize.

**Issue:** [216.4] Branchless SNES CPU flag operations (`Compare`, `Add`/`Sub` Overflow, shifts)

---

### Category E: GB PPU Config Caching (LOW RISK)

In `GbPpu::RunDrawCycle()` at `GbPpu.cpp:467`:

```cpp
GameboyConfig& cfg = _emu->GetSettings()->GetGameboyConfig();
```

This is called **every pixel** (23,040 visible pixels/frame + discarded pixels). It fetches through two pointer indirections (`_emu` → `GetSettings()` → `GetGameboyConfig()`).

**Optimization:** Cache the reference once at the start of each scanline in `ProcessVisibleScanline()` or at the start of `RunDrawCycle()` batch.

**Risk assessment:** The config doesn't change mid-scanline (settings are only written from the UI thread between frames). Caching per-scanline is safe.

**Issue:** [216.5] Cache `GameboyConfig` in GB PPU draw loop

---

## Benchmarking Requirements

### Existing Benchmarks (extend)

- `Core.Benchmarks/Gameboy/GbCpuBench.cpp` — Add `SetFlagState` branching vs branchless comparison
- `Core.Benchmarks/NES/NesCpuBench.cpp` — Add `CMP` with 3 branches vs branchless, `BIT` comparison
- `Core.Benchmarks/SNES/SnesCpuBench.cpp` — Add `Compare` Carry branching vs branchless

### New Benchmarks Needed

- `Core.Benchmarks/PPU/SnesPpuRenderBench.cpp` — Tile pixel extraction, window masking, color math (SNES PPU has no benchmarks yet)
- GB PPU config caching before/after

**Issue:** [216.6] Add benchmarks for Phase 2 optimizations

---

## Comparison Test Requirements

Following the Phase 1 pattern (exhaustive before/after comparison tests):

1. **GB `SetFlagState`**: All 256 values × 4 flags × 2 states (set/clear) × all prior flag combinations
2. **NES `CMP`**: All 256×256 reg/value pairs, verify C/Z/N identical
3. **NES `ADD`**: All 256×256 A/value pairs × 2 carry states, verify all flags identical
4. **NES `BIT`**: All 256×256 A/value pairs, verify Z/V/N identical
5. **NES `ASL`/`LSR`/`ROL`/`ROR`**: All 256 values × 2 carry states, verify C/Z/N identical
6. **SNES `Compare`**: 8-bit (256×256) + 16-bit (boundary sweeps), verify Carry identical
7. **SNES shifts**: 8-bit (256) + 16-bit (boundary sweeps), verify Carry identical

**Issue:** [216.7] Exhaustive comparison tests for Phase 2 optimizations

---

## Implementation Order

1. **[216.1] `[[likely]]`/`[[unlikely]]` Annotations** — Zero risk, no behavioral change, immediate commit
2. **[216.6] Add benchmarks** — Measure baselines before changing anything
3. **[216.2] GB `SetFlagState` branchless** — Low risk, benchmark first (HalfCarry lesson!)
4. **[216.3] NES branchless flags** — Low risk, broadest impact
5. **[216.4] SNES branchless flags** — Low risk, matches NES pattern
6. **[216.7] Comparison tests** — Verify correctness of all changes
7. **[216.5] GB PPU config caching** — Low risk, straightforward

---

## Risk Mitigation

- **ALWAYS benchmark before committing** — Learned from GB HalfCarry (36% slower when branchless)
- **Exhaustive comparison tests** — Every changed function gets a test covering all possible inputs
- **No accuracy regressions** — All changes are pure performance, same observable behavior
- **Incremental commits** — Each category is a separate commit tied to its issue
- **Existing test suites** — C++ 538 tests, C# 271 tests must all pass after each change

## Files Modified

| File | Categories |
|------|-----------|
| `Core/Gameboy/GbCpu.cpp` | B |
| `Core/Gameboy/GbPpu.cpp` | A, E |
| `Core/Gameboy/GbMemoryManager.cpp` | A |
| `Core/NES/NesCpu.h` | C |
| `Core/NES/NesCpu.cpp` | A |
| `Core/NES/NesPpu.cpp` | A |
| `Core/SNES/SnesCpu.cpp` | A |
| `Core/SNES/SnesCpu.Shared.h` | A |
| `Core/SNES/SnesCpu.Instructions.h` | A, D |
| `Core/SNES/SnesMemoryManager.cpp` | A |
| `Core.Benchmarks/Gameboy/GbCpuBench.cpp` | B benchmarks |
| `Core.Benchmarks/NES/NesCpuBench.cpp` | C benchmarks |
| `Core.Benchmarks/SNES/SnesCpuBench.cpp` | D benchmarks |
| `Core.Tests/Gameboy/GbCpuTests.cpp` (new) | B tests |
| `Core.Tests/NES/NesCpuTests.cpp` | C tests |
| `Core.Tests/SNES/SnesCpuTests.cpp` | D tests |
