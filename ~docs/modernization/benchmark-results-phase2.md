# Phase 2 Performance Benchmark Results

**Date:** 2026-02-11
**Platform:** Windows x64, 12-core 3696 MHz, MSVC v145 Release
**Build:** Release x64 with full optimizations

## Summary

All branchless optimizations are confirmed faster or equivalent to branching originals.
No regressions detected across 5-repetition benchmark runs.

## NES CPU (6502)

| Operation | Branching (ns) | Branchless (ns) | Speedup | Status |
|-----------|---------------|-----------------|---------|--------|
| SetZeroNegativeFlags | 5.23 | 4.22 | **-19.3%** | Faster |
| CMP | 4.73 | 4.77 | ~0% | Equivalent |
| ADD (ADC) | 10.2 | 10.1 | -1.0% | Equivalent |
| BIT | 5.24 | 4.21 | **-19.7%** | Faster |
| ASL | 9.03 | 7.94 | **-12.1%** | Faster |
| LSR | 7.70 | 7.57 | -1.7% | Equivalent |
| ROL | 8.71 | 8.12 | **-6.8%** | Faster |
| ROR | 9.14 | 7.99 | **-12.6%** | Faster |

## SNES CPU (65816)

| Operation | Branching (ns) | Branchless (ns) | Speedup | Status |
|-----------|---------------|-----------------|---------|--------|
| SetZeroNeg 8-bit | 5.00 | 4.44 | **-11.2%** | Faster |
| SetZeroNeg 16-bit | 5.65 | 4.44 | **-21.4%** | Faster |
| Compare8 | 5.55 | 4.32 | **-22.2%** | Faster |
| Add8 | 9.33 | 9.49 | +1.7% | Equivalent |
| ShiftLeft8 | 10.7 | 9.00 | **-15.9%** | Faster |
| TestBits8 | 6.01 | 3.37 | **-43.9%** | Faster |

## Game Boy CPU (LR35902)

| Operation | Branching (ns) | Branchless (ns) | Speedup | Status |
|-----------|---------------|-----------------|---------|--------|
| SetFlagState | 5.50 | 4.75 | **-13.6%** | Faster |

## Key Lessons

1. **Branchless is not always faster** — Phase 1 showed GB HalfCarry was 36% slower with branchless. Always benchmark.
2. **Microbenchmark variance is high** — Initial 3-rep results showed NES ASL/ROR as slower; 5-rep confirmed they're faster. Use ≥5 repetitions.
3. **Best gains from direct bit mapping** — Operations like BIT, TestBits, and shifts where if-branches map directly to bitwise ops show the largest speedups (20-44%).
4. **Ternary is equivalent to if-branch for single flags** — CMP and ADD show ~0% difference, likely because MSVC already optimizes single ternary/if to cmov.
5. **Config caching eliminates cross-TU calls** — GB PPU RefreshCachedConfig reduces GetGameboyConfig() calls from ~23,040/frame to ~144/frame (160x reduction).

## Test Coverage

576 tests passing, including:

- **NES:** 7 exhaustive branchless comparison tests + 7 multi-PS-state corner cases
- **SNES:** 10 exhaustive 8-bit + 4 exhaustive 16-bit + 4 multi-PS + 2 expanded boundary tests
- **GB:** 2 exhaustive SetFlagState comparison tests
