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
- [#431](https://github.com/TheAnsarya/Nexen/issues/431) — ReverbFilter ring buffer optimization
- [#433](https://github.com/TheAnsarya/Nexen/issues/433) — SDL ring buffer atomic fix
- [#434](https://github.com/TheAnsarya/Nexen/issues/434) — AudioConfig const reference
- [#435](https://github.com/TheAnsarya/Nexen/issues/435) — Rewind audio ring buffer
- [#436](https://github.com/TheAnsarya/Nexen/issues/436) — AddHistoryBlock O(1) memory tracking
- [#437](https://github.com/TheAnsarya/Nexen/issues/437) — CompressionHelper direct compression
- [#438](https://github.com/TheAnsarya/Nexen/issues/438) — Video frame buffer reuse

## Metadata Recording Benchmarks (CDL & Pansy)

Recent benchmarks confirm:

- **Disabled:** 0.000ns (no cost)
- **Enabled:** CDL ~0.26ns/call, Pansy ~0.25ns/call, combined ~0.75ns/call
- All hooks are properly conditional; no unconditional cost in hot path.

### Next Steps

- Profile real-world metadata recording in large games
- Investigate lock-free ring buffers, flat arrays, and intrusive lists for future scalability
- Track all findings in [#429](https://github.com/TheAnsarya/Nexen/issues/429)

Benchmarks: `Core.Benchmarks/Debugger/MetadataRecordingBench.cpp`

## Audio & Timing Benchmarks

Recent benchmarks:

- **SoundMixer Mix:** ~343ns (1024), ~1425ns (4096)
- **SoundMixer Output:** ~1394ns (1024), ~5281ns (4096)
- **FrameAdvance/Turbo:** 0.000ns (dummy)

### Next Steps

- Profile real SoundMixer output for blocking/latency
- Investigate SIMD mixing, async output, lock-free buffers
- Track all findings in [#430](https://github.com/TheAnsarya/Nexen/issues/430)

Benchmarks: `Core.Benchmarks/Debugger/AudioTimingBench.cpp`

## Audio Pipeline Optimizations

A deep audit of the audio pipeline (SoundMixer → SoundResampler → Effects → AudioDevice) identified several performance and correctness issues. Three have been implemented:

### ReverbFilter: Ring Buffer (deque → vector)

**Issue:** [#431](https://github.com/TheAnsarya/Nexen/issues/431)

The ReverbFilter maintains 10 delay lines, each using `std::deque<int16_t>` with per-sample `push_back()`/`pop_front()` — approximately 8,000 heap operations per frame when reverb is enabled. `std::deque` stores data in scattered 512-byte chunks, causing cache misses on every access.

**Optimization:** Replace each `std::deque` with a contiguous `std::vector<int16_t>` ring buffer using modulo arithmetic for read/write positions. `SetParameters()` pre-allocates `delay + 8192` capacity.

- Eliminates per-sample heap allocations
- Cache-friendly contiguous memory access
- Estimated 20-40μs/frame savings when reverb is enabled
- Zero cost when reverb is disabled

### SDL Ring Buffer: Atomic Fix

**Issue:** [#433](https://github.com/TheAnsarya/Nexen/issues/433)

The SDL audio backend (`SdlSoundManager`) shares a ring buffer between the SDL callback thread (reads) and the emulation thread (writes). The `_writePosition` and `_readPosition` members were plain `uint32_t` with no synchronization — a data race (undefined behavior in C++).

**Fix:** Changed both to `std::atomic<uint32_t>`. On x86 this compiles to identical instructions (natural alignment provides atomicity) but provides the required memory ordering guarantees for correct behavior on all architectures.

### AudioConfig Const Reference

**Issue:** [#434](https://github.com/TheAnsarya/Nexen/issues/434)

`SoundMixer::PlayAudioBuffer()` and `GetMasterVolume()` copied the entire `AudioConfig` struct (~224 bytes) by value on every call. Changed to `const AudioConfig&` to avoid unnecessary per-frame copies.

### Remaining Findings (Not Yet Implemented)

| Finding | File | Priority | Notes |
|---------|------|----------|-------|
| Equalizer 20-band double-precision | Equalizer.cpp | Medium | Disabled by default; SIMD would help |
| HermiteResampler vector realloc | HermiteResampler.h | Medium | Rare in practice |
| CrossFeedFilter int16_t overflow | CrossFeedFilter.cpp | Low | Correctness bug, not performance |
| DirectSound deprecated API | SoundManager.cpp | Low | Works fine; WASAPI migration future work |

## Rewind System Optimizations

A performance audit of the rewind system identified multiple heap allocation storms and O(n) operations that cause stuttering during rewind, especially at turbo speed.

### Audio History Ring Buffer

**Issue:** [#435](https://github.com/TheAnsarya/Nexen/issues/435)

Replaced `std::deque<int16_t>` audio history with a pre-allocated 1M-sample contiguous ring buffer. Eliminates ~2000 individual `pop_back()` calls per audio callback and O(n) front-insertion. Uses bulk `memcpy` for insert and sequential index access for playback.

### AddHistoryBlock O(1) Memory Tracking

**Issue:** [#436](https://github.com/TheAnsarya/Nexen/issues/436)

`AddHistoryBlock()` and `GetStats()` iterated the entire history deque to sum memory usage — O(n) every 30 frames. Replaced with a running `_totalMemoryUsage` counter that increments on add and decrements on remove. Both operations now O(1).

### CompressionHelper Direct Compression

**Issue:** [#437](https://github.com/TheAnsarya/Nexen/issues/437)

`CompressionHelper::Compress()` used naked `new/delete` for a temporary buffer, then byte-by-byte inserted into the output vector. Now pre-sizes the output vector with `compressBound()` and compresses directly into it — zero temporary allocations.

### Video Frame Buffer Reuse

**Issue:** [#438](https://github.com/TheAnsarya/Nexen/issues/438)

During rewind, `ProcessFrame()` allocated a new `vector<uint32_t>` (~246KB) for every frame. Now uses `VideoFrame::CopyFrom()` which reuses existing buffer capacity via `resize()` + `memcpy`, and `std::move` when pushing into the video history deque.
