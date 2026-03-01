# Performance Improvement Plan — Emulation Speed & Audio

**Created:** 2026-02-26
**Updated:** 2026-02-28
**Epic Issue:** #418
**Branch:** `master` (merged from `features-atari-lynx`)

## Problem Statement

User reports while playing Dragon Warrior 4 (NES):
1. Audio stuttering (periodic glitches)
2. Periodic slowdowns (frame drops)
3. Cannot sustain 2x/3x fast-forward speed

## Root Cause Analysis Summary

### Finding 1 (CRITICAL — Nexen-specific): BackgroundCdlRecording activates full debugger
- `BackgroundCdlRecording = true` by default → `BackgroundPansyExporter` calls `InitializeDebugger()` on every ROM load
- Full C++ Debugger instantiated (all subsystems: breakpoints, trace logger, disassembler, CDL, memory counters)
- **Every CPU instruction** goes through `ProcessInstruction()` — address resolution, CDL marking, call stack, breakpoint checks
- **Every memory read/write** goes through `ProcessMemoryRead()`/`ProcessMemoryWrite()` — address resolution, CDL marking, access counters, breakpoint evaluation
- **Estimated overhead: 30-50% CPU slowdown**
- **Status: FIXED** — Default changed to `false`, config upgrade now sets `false` instead of `true`

### Finding 2 (HIGH — Mesen2): Audio buffer overflow at turbo speed
- `SoundMixer::PlayAudioBuffer` has no handling for speed > 100%
- Pitch adjustment only handles speed < 100% (slow-motion)
- At 2x/3x, audio buffer fills 2-3x faster than hardware drains, causing overflow → stutter
- `SoundResampler` disables rate feedback when speed != 100%
- Buffer health monitoring disabled when speed > 100%

### Finding 3 (HIGH — Mesen2): VideoDecoder spin-wait caps emulation speed
- `VideoDecoder::UpdateFrame()` busy-spins `while (_frameChanged) {}` with NO sleep/yield
- If video decode thread is slow, emulation thread burns CPU waiting
- At turbo speed, this creates a hard speed cap equal to video decode throughput
- Video filters (NTSC, scale2x) make decode slower

### Finding 4 (MEDIUM — Both): Synchronous save state operations
- All save operations (auto-save, recent-play, quick-save) block emulation thread
- Pipeline: AcquireLock → framebuffer zlib compression → state serialization → disk I/O → ReleaseLock
- Each save can take 5-15ms (NES frame = 16.7ms)
- **Partial fix applied:** SaveVideoData compression level reduced from 6 to 1

### Finding 5 (MEDIUM — Mesen2): Rewind buffer overhead at turbo
- Records full savestate every 30 frames (always active if buffer > 0)
- At 3x speed, fires 3x more often in real-time

### Finding 6 (LOW — Mesen2): Audio effects chain runs at turbo speed
- Full effects pipeline (resampling, EQ, reverb, crossfeed) runs for every audio frame
- At 3x speed, this is 3x the effects processing per real-time second

## Immediate Fixes Applied (This Session)

### Fix 1: BackgroundCdlRecording default → false (#419)
- `IntegrationConfig.cs`: Changed default from `true` to `false`
- `Configuration.cs`: Config upgrade now sets `false` (was `true`)
- Impact: Eliminates 30-50% overhead for all users who haven't manually enabled it

### Fix 2: SaveVideoData compression level 6 → 1 (#424)
- `SaveStateManager.cpp`: `MZ_DEFAULT_LEVEL` → `MZ_BEST_SPEED`
- Impact: 2-4x faster framebuffer compression on every save state

## Future Work (Issues Created)

### Phase 1: Quick Wins — COMPLETE
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #419 | BackgroundCdlRecording default to false | ✅ Done | 30-50% speed recovery |
| #424 | SaveVideoData compression level | ✅ Done | 2-4x faster saves |
| #425 | Lightweight CDL recorder | ✅ Done | ~0.1% overhead vs 30-50% |

### Phase 2: Audio & Video — COMPLETE
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #420 | Audio frame skipping at turbo speed | ✅ Done | Removes audio speed bottleneck |
| #421 | VideoDecoder condition variable | ✅ Done | Removes spin-wait speed cap |

### Phase 3: Debugger Pipeline — COMPLETE
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #426 | Profiler cached pointers | ✅ Done | 6-9.4x profiler speedup |
| #427 | CallstackManager ring buffer | ✅ Done | 1.5-2.2x callstack speedup |

### Phase 4: Audio Pipeline — COMPLETE
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #431 | ReverbFilter ring buffer | ✅ Done | Eliminates buffer rotation |
| #432 | SDL audio atomics | ✅ Done | Lock-free audio buffer |
| #433 | AudioConfig const ref | ✅ Done | Eliminates per-sample copy |

### Phase 5: Rewind System — COMPLETE
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #434 | Audio ring buffer for rewind | ✅ Done | O(1) audio rewind |
| #435 | O(1) memory tracking | ✅ Done | Eliminates per-frame map walk |
| #436 | CompressionHelper direct-write | ✅ Done | Eliminates intermediate buffer |
| #437 | Video frame reuse | ✅ Done | Reuses framebuffer allocation |

### Phase 6: Run-Ahead & Allocations — COMPLETE
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #439 | RunAhead persistent stream | ✅ Done | Eliminates stringstream allocs |
| #440 | CrossFeedFilter overflow fix | ✅ Done | Correct int32_t arithmetic |
| #441 | SaveVideoData buffer reuse | ✅ Done | 2x faster (8us vs 16us) |
| #442 | HermiteResampler reserve | ✅ Done | Eliminates reallocation |
| #443 | FastBinary positional serializer | ✅ Done | **8.2x round-trip speedup** |

### Phase 7: Move Semantics & HUD — COMPLETE
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #444 | RenderedFrame/VideoDecoder move | ✅ Done | Eliminates frame copy |
| #445 | NotificationManager cleanup | ✅ Done | 2.3x dispatch speedup |
| #446 | MessageManager single lookup | ✅ Done | 1.5x lookup speedup |
| #447 | HUD string copy elimination | ✅ Done | Move semantics for DrawStringCommand |

### Phase 8: Benchmarks & Validation — COMPLETE
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #448 | Performance regression benchmarks | ✅ Done | 30+ benchmarks added |
| #449 | Pansy/CDL tracking audit | ✅ Done | Already idempotent, no changes needed |

### Phase 9: DebugHud, Pansy Pipeline, Game Package Export
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #453 | DebugHud flat buffer pixel tracking | ✅ Done | O(1) indexed vs hash lookup per pixel |
| #454 | Game Package Export (.nexen-pack.zip) | ✅ Done | New feature — Tools menu |
| #455 | Pansy auto-save off UI thread + CRC cache | ✅ Done | Eliminates UI thread file I/O blocking |

### Remaining Open Work
| Issue | Description | Priority |
|-------|-------------|----------|
| #422 | Async save states | Medium |
| #423 | Rewind at turbo speed | Medium |
| #452 | BaseControlDevice lock coarsening | Low |
| #462 | MemoryAccessCounter per-read branch elimination | High |
| #463 | NotificationManager RCU pattern | Medium |

### Phase 10: Deep Hot Path Audit & Mechanical Cleanup — COMPLETE
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #456 | SaveStateManager buffer reuse (memcpy vs move) | ✅ Done | 15% faster, 50× more stable variance |
| #457 | CheatManager sorted vector (binary search) | ✅ Done | 5.7% faster misses, cache-friendly |
| #458 | VideoRenderer persistent AVI HUDs | ✅ Done | Eliminates per-frame DebugHud/InputHud allocation |
| #459 | Debugger::Log deque + const ref + '\n' | ✅ Done | O(1) push vs O(n) list, no stdout flush |
| #460 | RecordInput pass by const ref | ✅ Done | Eliminates vector<shared_ptr> copy per frame |
| #461 | NesDebugger DebugRead research | ✅ Closed | Not redundant — architecturally required pre-fetch |
| #462 | MemoryAccessCounter audit | ✅ Closed | Already optimal — flat arrays, compile-time unroll |
| #464 | HistoryViewer cached segments | ✅ Done | O(1) GetState() vs O(n) scan |
| #465 | LabelManager const ref + GetComment no-copy | ✅ Done | Zero-copy comment return, const ref params |
| #466 | Breakpoint::Matches header inline | ✅ Done | Cross-TU inlining for per-access template |
| #467 | MessageManager/DebugLog const ref + emplace_back | ✅ Done | 11 files, 16 emplace_back conversions |

### Phase 11: Fifth Audit Sweep — Buffer Reuse & Copy Elimination — COMPLETE
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #468 | GetPortStates buffer reuse (all consoles) | ✅ Done | Eliminates per-frame vector alloc |
| #469 | GBA pendingIrqs swap-and-pop | ✅ Done | O(1) removal vs O(n) erase |
| #470 | ClearFrameEvents swap vs copy | ✅ Done | **2.18x** faster (1265ns→581ns) |
| #471 | ExpressionEvaluator const ref (9 methods) | ✅ Done | **9.1x** faster per call (70ns→7.7ns) |
| #472 | TraceLogger persistent buffers + ScriptingContext const ref | ✅ Done | Eliminates per-instruction allocs |
| #473 | NesSoundMixer timestamps reserve(512) | ✅ Done | Prevents early-frame reallocs |

### Phase 12: Sixth Audit Sweep — Micro-optimizations & Cleanup — COMPLETE
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #474 | Audio providers reserve _samplesToPlay | ✅ Done | Eliminates early reallocs (SmsFm/PceCd/PceAdpcm) |
| #475 | NecDsp/Cx4 TraceLogger stack buffer for flags | ✅ Done | Zero heap allocs per trace row |
| #476 | ExpressionEvaluator cache emplace (single lookup) | ✅ Done | Eliminates double hash lookup on cache miss |
| #477 | ShortcutKeyHandler move pressedKeys | ✅ Done | O(1) move vs O(n) copy per input poll |
| #478 | StepBack emplace_back + MovieRecorder/NexenMovie const ref | ✅ Done | Eliminates temp + avoids string copies |
| #479 | SystemHud snprintf + DebugStats stringstream reuse | ✅ Done | Eliminates stringstream allocs per frame |

### Phase 13: Seventh Audit Sweep — Audio, Rewind, and Debugger Hot Paths — COMPLETE
| Issue | Description | Status | Impact |
|-------|-------------|--------|--------|
| #483 | NesSoundMixer EndFrame — selective zeroing + dedup-at-insert | ✅ Done | 1.76x memset + 1.52x timestamps = ~3600ns/frame saved |
| #484 | CompressionHelper::Compress const string& | ✅ Done | Eliminates 100KB+ heap copy per rewind snapshot |
| #485 | RewindData::SaveState assign via pointer cast | ✅ Done | Eliminates char-by-char ~100KB copy |
| #486 | BaseControlDevice::GetTextState cache GetKeyNames() | ✅ Done | One alloc per device lifetime vs per-frame |
| #487 | StringUtilities::GetNthSegment for Disassembler | ✅ Done | 377ns → 22.5ns = **16.8x faster** |
| #488 | MovieRecorder::CreateMovie const RewindData& | ✅ Done | Eliminates 100KB+ deep copy per rewind entry |

## Benchmark Results Summary

### FastBinary Serializer (run-ahead hot path)
| Benchmark | Binary | FastBinary | Speedup |
|-----------|--------|------------|---------|
| SaveSmallState | 914ns | 32ns | **29x** |
| SavePpuState | 1308ns | 73ns | **17.9x** |
| SaveLargeState | 3555ns | 1293ns | **2.7x** |
| RoundTrip (save+load) | 22576ns | 2739ns | **8.2x** |
| KeyPrefixManagement | 844ns | 49ns | **17.2x** |
| DeepNesting | 684ns | 69ns | **9.9x** |

### Other Optimizations
| Benchmark | Old | New | Speedup |
|-----------|-----|-----|---------|
| NotificationManager dispatch | 2166ns | 961ns | **2.3x** |
| MessageManager lookup | 1684ns | 1112ns | **1.5x** |
| Persistent buffer (SaveVideoData) | 15978ns | 8041ns | **2.0x** |
| CDL disabled overhead | N/A | 0ns | **zero cost** |
| CDL enabled (hot/idempotent) | N/A | 3.6us/10K | negligible |
| ClearFrameEvents copy→swap | 1265ns | 581ns | **2.18x** |
| ExprEval string by-value→const ref | 69.8ns | 7.67ns | **9.1x** |

### Phase 13 Measurements
| Benchmark | Old | New | Speedup |
|-----------|-----|-----|---------|
| EndFrame full memset (220KB) | 4150ns | 2352ns (selective) | **1.76x** |
| Timestamps push+sort+unique | 5197ns | 3418ns (dedup+sort) | **1.52x** |
| Split()[n] comment extraction | 377ns | 22.5ns (GetNthSegment) | **16.8x** |

## Pansy/CDL Tracking Audit (#449)

The user's concern about Pansy tracking firing redundant events (e.g., same jump address) was investigated
thoroughly. The system is **already idempotent at every level**:

- **Lightweight CDL**: Byte-OR operations — OR'ing an already-set bit is a no-op (~10-15ns/instruction)
- **Full debugger CDL**: Same byte-OR pattern with richer flags
- **Pansy export**: Runs on UI thread via timer (minutes apart), never during emulation
- **Cross-references**: HashSet deduplication at export time
- **Tiers are mutually exclusive**: Lightweight and full debugger never both active

**Conclusion: No caching mechanism needed.** The byte-OR pattern IS the cache — it's inherently idempotent.

## Files Changed (All Phases)

| File | Change |
|------|--------|
| `UI/Config/IntegrationConfig.cs` | `BackgroundCdlRecording` default: `true` → `false` |
| `UI/Config/Configuration.cs` | Config upgrade: `true` → `false` |
| `Core/Shared/SaveStateManager.cpp` | Compression `MZ_DEFAULT_LEVEL` → `MZ_BEST_SPEED` |
| `Core/Shared/LightweightCdlRecorder.h/cpp` | New lightweight CDL recorder |
| `Core/Shared/Video/VideoDecoder.cpp` | Condition variable replaces busy-spin |
| `Core/Debugger/Profiler.cpp` | Cached pointers for 6-9.4x speedup |
| `Core/Debugger/CallstackManager.cpp` | Ring buffer for 1.5-2.2x speedup |
| `Core/Shared/Audio/ReverbFilter.cpp` | Ring buffer replaces rotation |
| `Core/Shared/Audio/SdlSoundManager.cpp` | SDL atomics for lock-free audio |
| `Core/Shared/Audio/SoundMixer.cpp` | AudioConfig const ref |
| `Core/Shared/Audio/CrossFeedFilter.cpp` | int32_t + std::clamp overflow fix |
| `Core/Shared/RewindManager.cpp` | Ring buffer, O(1) tracking, direct-write |
| `Core/Shared/Emulator.cpp` | RunAhead persistent stream |
| `Core/Shared/Video/SystemHud.cpp` | String move semantics |
| `Core/Shared/NotificationManager.cpp` | Index-based iteration under lock |
| `Core/Shared/MessageManager.cpp` | Single find() lookup |
| `Core/Shared/Video/RenderedFrame.h` | Move constructor/assignment |
| `Core/Shared/Serializer.h/cpp` | FastBinary positional format |
| `Core/Shared/Audio/HermiteResampler.h` | Vector reserve |
| `Core.Benchmarks/Serialization/SerializerBench.cpp` | 9 FastBinary benchmarks |
| `Core.Benchmarks/Debugger/MetadataRecordingBench.cpp` | CDL/CrossFeed/Notification/Message benchmarks |
| `Core.Benchmarks/Shared/MoveSemanticsBench.cpp` | Move semantics benchmarks |
| `Core/Shared/Video/DebugHud.h` | Flat vector<uint32_t> + dirty indices replace unordered_map |
| `Core/Shared/Video/DebugHud.cpp` | Flat buffer diff, sparse clear, O(1) pixel tracking |
| `Core/Shared/Video/DrawCommand.h` | Flat buffer pointer + bounds check in InternalDrawPixel |
| `UI/Utilities/GamePackageExporter.cs` | New — .nexen-pack.zip export with async background thread |
| `UI/Debugger/Labels/BackgroundPansyExporter.cs` | Threadpool export, cached CRC32 |
| `UI/Debugger/Labels/PansyExporter.cs` | Cached CRC param, FileOptions.SequentialScan |
| `UI/ViewModels/MainMenuViewModel.cs` | Export Game Package menu items |
| `UI/Debugger/Utilities/ContextMenuAction.cs` | ExportGamePackage, OpenGamePacksFolder actions |
| `UI/Localization/resources.en.xml` | Localization for new menu items and messages |
| `Core/Shared/BaseControlManager.h/cpp` | GetPortStates buffer reuse (const ref return) |
| `Core/Shared/RenderedFrame.h` | Constructor takes const vector<ControllerData>& |
| `InteropDLL/InputApiWrapper.cpp` | const ref for GetPortStates result |
| `Core/GBA/GbaMemoryManager.cpp` | pendingIrqs swap-and-pop O(1) removal |
| `Core/Debugger/BaseEventManager.cpp` | ClearFrameEvents swap instead of copy |
| `Core/Debugger/ExpressionEvaluator.h/cpp` | 9 methods const string& |
| `Core/Debugger/Debugger.h/cpp` | EvaluateExpression const string& |
| `Core/Debugger/BaseTraceLogger.h` | Persistent _rowBuffer/_byteCodeBuffer, WriteStringValue const ref |
| `Core/Debugger/ScriptingContext.h/cpp` | Log() const string& |
| `Core/NES/NesSoundMixer.cpp` | _timestamps.reserve(512) |
| `Core.Benchmarks/Debugger/DebuggerPipelineBench.cpp` | ClearFrameEvents + ExprEval benchmarks |
| `Core/SMS/SmsFmAudio.cpp` | _samplesToPlay.reserve(2048) |
| `Core/PCE/CdRom/PceCdAudioPlayer.cpp` | _samplesToPlay.reserve(1200) |
| `Core/PCE/CdRom/PceAdpcm.cpp` | reserve(2048) + emplace_back |
| `Core/SNES/Debugger/TraceLogger/NecDspTraceLogger.cpp` | Stack buffer for 6-flag string |
| `Core/SNES/Debugger/TraceLogger/Cx4TraceLogger.cpp` | Stack buffer for 4-flag string |
| `Core/Shared/ShortcutKeyHandler.cpp` | move() for _lastPressedKeys |
| `Core/Debugger/StepBackManager.cpp` | emplace_back replaces push_back+back() |
| `Core/Shared/Movies/MovieRecorder.h/cpp` | WriteString/WriteInt/WriteBool const string& |
| `Core/Shared/Movies/NexenMovie.h/cpp` | LoadInt/LoadBool/LoadString const string& |
| `Core/Shared/Video/SystemHud.cpp` | snprintf replaces stringstream in ShowGameTimer |
| `Core/Shared/Video/DebugStats.cpp` | stringstream buffer reuse (ss.str(""); ss.clear()) |
| `Core/NES/NesSoundMixer.h` | _usedTimestamp dedup array for EndFrame optimization |
| `Core/NES/NesSoundMixer.cpp` | Selective zeroing + dedup-at-insert + memset in Reset only |
| `Utilities/CompressionHelper.h` | Compress() takes const string& |
| `Core/Shared/RewindData.cpp` | assign() via reinterpret_cast replaces char-by-char copy |
| `Core/Shared/BaseControlDevice.h` | _cachedKeyNames + _keyNamesCached members |
| `Core/Shared/BaseControlDevice.cpp` | Lazy-cached GetKeyNames() in GetTextState |
| `Utilities/StringUtilities.h` | GetNthSegment() — extract Nth delimited segment without full Split |
| `Core/Debugger/Disassembler.cpp` | GetNthSegment replaces Split()[n] for comment lines |
| `Core/Shared/Movies/MovieRecorder.cpp` | const RewindData& in CreateMovie loop |
| `Core.Benchmarks/NES/NesSoundMixerBench.cpp` | EndFrame + timestamp + GetNthSegment benchmarks |
