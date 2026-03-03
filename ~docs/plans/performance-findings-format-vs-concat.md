# Performance Findings: std::format vs Alternatives on MSVC

## Benchmark Environment
- **Compiler**: MSVC v145 (VS 2026 Insiders), C++23 (`/std:c++latest`)
- **Configuration**: Release x64, full optimizations
- **CPU**: 12-core, 3696 MHz, 12MB L3

## Critical Finding

### std::format is SLOWER than operator+ for short strings

MSVC's `std::format` implementation has significant overhead that makes it **2–3.4x slower** than simple `operator+` concatenation for short string + integer patterns.

| Benchmark | `operator+` | `std::format` | Ratio |
|-----------|-------------|---------------|-------|
| `"Frame: " + to_string(n)` (3 strings) | 87.8 ns | 303 ns | format **3.4x slower** |
| `"[" + title + "] " + msg` | 91.5 ns | 164 ns | format **1.8x slower** |
| `"Underruns: " + to_string(n)` (3 strings) | 173 ns | 480 ns | format **2.8x slower** |
| `path + "\x1" + file + "\x1" + to_string(i)` | 92.2 ns | 193 ns | format **2.1x slower** |

### std::format is MUCH FASTER than stringstream

| Benchmark | `stringstream` | `std::format` | Ratio |
|-----------|----------------|---------------|-------|
| 3× formatted doubles (ss vs format) | 2526 ns | 664 ns | format **3.8x faster** |

### std::format is comparable to snprintf

For fixed-width numeric formatting (`%02u:%02u:%02u`), `std::format` and `snprintf` are similar — marginal difference either way.

## Guidelines

### DO use std::format for:
- Replacing `std::stringstream` / `std::ostringstream` (3.8x win)
- Fixed-width padding/formatting (`{:04X}`, `{:02d}`)
- Replacing `snprintf` + `string(buf)` patterns (cleaner + similar speed)
- Multi-field formatting where stringstream would normally be used

### DO NOT use std::format for:
- Replacing simple `"prefix" + std::to_string(n)` (2–3x regression!)
- Replacing `"[" + title + "] " + msg` (1.8x regression!)
- Any pattern that's currently just `operator+` with 1–3 string operands

### Preferred patterns (fastest to slowest):
1. **`reserve() + append()`** — Best for known-size concat (2.6x over operator+)
2. **`operator+`** — Good for simple 1–3 operand short strings (~90 ns)
3. **`std::format`** — Only for replacing stringstream or complex formatting
4. **`std::stringstream`** — Avoid entirely where possible (2500+ ns)

## Why This Matters for Nexen

Per-frame operations (HUD rendering, frame counters) execute 60 times/second minimum. Using `std::format` instead of `operator+` for HUD strings would add ~200ns × 3–4 counters × 60 fps = ~48μs/second of unnecessary overhead. For a 16.7ms frame budget, that's 0.3% wasted. Not catastrophic, but avoidable.

For cold paths (ROM loading, save states), the choice matters less. Use whichever is cleaner.
