# Debugger Performance Optimization

## Overview

Nexen's debugger pipeline runs on every emulated CPU instruction when debugging features are active. This document describes the optimizations applied to minimize debugger overhead while preserving full functionality.

## Performance Improvements

### Profiler: Cached Pointer Optimization (6-9x faster)

**Issue:** [#426](https://github.com/TheAnsarya/Nexen/issues/426)

The Profiler tracks per-function cycle counts by maintaining a call stack and updating inclusive/exclusive cycle counters on every instruction. Previously, `UpdateCycles()` performed an `unordered_map` hash lookup for every level of the function call stack — at call depths of 5-50, this cost 47-350ns per call.

**Optimization:** Cache `ProfiledFunction*` pointers alongside the function stack. Since `unordered_map` guarantees pointer stability on insertion, we can safely store and dereference pointers to map entries, replacing O(n × hash_lookup) with O(n × pointer_deref).

| Call Depth | Before (ns) | After (ns) | Speedup |
|-----------|-------------|------------|---------|
| 5 | 50 | 8 | **6.0x** |
| 10 | 82 | 14 | **6.0x** |
| 20 | 144 | 18 | **8.2x** |
| 50 | 346 | 37 | **9.4x** |

### CallstackManager: Ring Buffer (2x faster)

**Issue:** [#427](https://github.com/TheAnsarya/Nexen/issues/427)

The CallstackManager tracks function call/return addresses for the debugger's call stack display and return address validation. Previously, it used a `deque<StackFrameInfo>` with a max of 511 entries.

**Optimization:** Replace the deque with a fixed `std::array<StackFrameInfo, 512>` ring buffer. Contiguous memory layout enables hardware prefetching during `IsReturnAddrMatch()` reverse scans (called on every RTS/return instruction).

| Stack Depth | Before (ns) | After (ns) | Speedup |
|-------------|-------------|------------|---------|
| 5 | 11 | 7 | **1.5x** |
| 20 | 31 | 15 | **2.0x** |
| 100 | 143 | 67 | **2.1x** |
| 511 | 670 | 310 | **2.2x** |

## Benchmark Suite

**Issue:** [#428](https://github.com/TheAnsarya/Nexen/issues/428)

30 micro-benchmarks in `Core.Benchmarks/Debugger/DebuggerPipelineBench.cpp` cover all major debugger pipeline components:

- **CDL** — Code/Data Logger floor baselines
- **MemoryAccessCounter** — AoS vs SoA memory layouts
- **Profiler** — HashMap vs FlatArray, UpdateCycles hash vs cached pointers
- **CallstackManager** — Deque vs RingBuffer, push/pop and IsReturnMatch
- **BreakpointManager** — Bool fast-path rejection
- **FrozenAddress** — Empty hashset fast path
- **EventManager** — Vector push overhead
- **Full Pipeline** — Composite instruction simulation

### Running Benchmarks

```powershell
# Build benchmarks
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" `
	Core.Benchmarks\Core.Benchmarks.vcxproj /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m

# Run all debugger benchmarks
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_"

# Run specific component
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="Profiler"

# Run with repetitions for statistical rigor
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="Profiler|Callstack" --benchmark_repetitions=5
```

## Decisions Not Implemented

The benchmark suite also measured several approaches that were **not** beneficial:

| Approach | Result | Reason |
|----------|--------|--------|
| MemoryAccessCounter SoA layout | 0.95x composite | Reads faster but writes/exec slower; net wash |
| Breakpoint bitmap fast-reject | 10.5ns vs 2.9ns current | Current bool array already optimal |
| FrozenAddress bitset | 11.0ns vs 3.5ns current | Worse for common empty-set case |

These benchmarks remain in the suite for future reference if cache patterns change.

## Related Issues

- [#418](https://github.com/TheAnsarya/Nexen/issues/418) — Performance Investigation Epic
- [#425](https://github.com/TheAnsarya/Nexen/issues/425) — Lightweight CDL Recorder
- [#426](https://github.com/TheAnsarya/Nexen/issues/426) — Profiler cached pointer optimization
- [#427](https://github.com/TheAnsarya/Nexen/issues/427) — CallstackManager ring buffer
- [#428](https://github.com/TheAnsarya/Nexen/issues/428) — Debugger pipeline benchmarks
