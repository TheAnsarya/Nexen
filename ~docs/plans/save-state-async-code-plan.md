# Save State Async I/O — Code Plan

**Issue:** [#422](https://github.com/TheAnsarya/Nexen/issues/422) — Save state operations block emulation thread synchronously
**Parent:** [#418](https://github.com/TheAnsarya/Nexen/issues/418) — Performance: Audio Stuttering, Slowdowns & Fast-Forward Limitations

## Current Architecture

### Save Pipeline (all under emulator lock)

```text
AcquireLock()
  → GetSaveStateHeader(stream)
    → SaveVideoData(stream)        // zlib compress framebuffer (~150KB) at MZ_BEST_SPEED
    → Write ROM name/version/etc
  → Serialize(stream, false)
    → Serializer::Stream(console)   // Serialize all state to in-memory _data buffer (~320KB)
    → Serializer::SaveTo(stream)    // zlib compress _data, write to stream
ReleaseLock()
```

### Key Files

- `Core/Shared/SaveStateManager.cpp:148-165` — `SaveState(filepath)`: acquires lock, serializes, writes file
- `Core/Shared/SaveStateManager.cpp:143-145` — `SaveState(ostream&)`: header + serialize
- `Core/Shared/SaveStateManager.cpp:183-201` — `SaveVideoData`: zlib compress framebuffer
- `Core/Shared/Emulator.cpp:870-878` — `Serialize`: create Serializer, stream console, SaveTo
- `Utilities/Serializer.cpp:228-251` — `SaveTo`: compress _data, write to ostream
- `Core/Shared/Emulator.cpp:167-180` — `ProcessAutoSaveState`: called every frame

### Callers

1. **Auto-save** (every 20-30 min): `ProcessAutoSaveState()` → `SaveState(AutoSaveStateIndex)`
2. **Quick save** (user F1/Ctrl+S): `SaveState(slotIndex)` or `QuickSave()` → `SaveState(filepath)`
3. **Recent play** (every 5 min): UI timer calls `RecordRecentPlayState()`
4. **SaveRecentGame** (on ROM unload): Full ZIP creation

### Timing Analysis

- Serializer reserves 320KB (`0x50000`), actual state size varies by console
- SaveVideoData: compress ~150KB framebuffer at MZ_BEST_SPEED (~0.5-1ms)
- Serializer: serialize state (~1-3ms) + compress (~1-2ms)
- File write: ~0.5-2ms (SSD), ~5-20ms (HDD)
- **Total under lock: ~3-8ms** (NES frame budget: 16.7ms at 60fps)

## Proposed Solution: Snapshot + Background Write

### Phase 1: Snapshot Under Lock, Compress+Write in Background

The key insight is that `Serializer::Stream()` already serializes to an in-memory buffer (`_data`). We can split the pipeline:

**Under lock (fast, ~1-3ms):**

1. Serialize state to in-memory buffer (Serializer::Stream + capture _data)
2. Copy framebuffer for screenshot

**Off lock (background thread):**

1. Compress framebuffer (zlib)
2. Compress serialized state (zlib)
3. Write header + compressed data to file
4. File close

### Implementation Steps

#### Step 1: Add `SerializeToBuffer()` to Emulator

```cpp
// Returns a buffer containing the serialized (uncompressed) state
vector<uint8_t> Emulator::SerializeToBuffer() {
    Serializer s(SaveStateManager::FileFormatVersion, true);
    s.Stream(_console, "");
    return s.GetData(); // New method: returns std::move(_data)
}
```

#### Step 2: Add `GetData()` to Serializer

```cpp
// Move the serialized data out (for async writing)
vector<uint8_t> Serializer::GetData() {
    return std::move(_data);
}
```

#### Step 3: Capture framebuffer snapshot

```cpp
struct SaveStateSnapshot {
    vector<uint8_t> stateData;     // Serialized (uncompressed) state
    vector<uint8_t> frameBuffer;   // Raw framebuffer copy
    uint32_t frameWidth;
    uint32_t frameHeight;
    double frameScale;
    uint32_t emuVersion;
    ConsoleType consoleType;
    string romName;
    string filepath;
    bool showSuccessMessage;
};
```

#### Step 4: Background writer thread

```cpp
class SaveStateManager {
    // ... existing ...
    std::jthread _writeThread;
    std::queue<SaveStateSnapshot> _writeQueue;
    std::mutex _writeMutex;
    std::condition_variable _writeCv;

    void BackgroundWriteLoop();
    void EnqueueWrite(SaveStateSnapshot&& snapshot);
};
```

#### Step 5: Update SaveState(filepath) → snapshot + enqueue

```cpp
bool SaveStateManager::SaveState(const string& filepath, bool showSuccessMessage) {
    SaveStateSnapshot snapshot;
    {
        auto lock = _emu->AcquireLock();
        // Capture state under lock (~1-3ms)
        snapshot.stateData = _emu->SerializeToBuffer();
        PpuFrameInfo frame = _emu->GetPpuFrame();
        snapshot.frameBuffer.assign(
            (uint8_t*)frame.FrameBuffer,
            (uint8_t*)frame.FrameBuffer + frame.FrameBufferSize);
        snapshot.frameWidth = frame.Width;
        snapshot.frameHeight = frame.Height;
        snapshot.frameScale = _emu->GetVideoDecoder()->GetLastFrameScale();
        snapshot.emuVersion = _emu->GetSettings()->GetVersion();
        snapshot.consoleType = _emu->GetConsoleType();
        RomInfo romInfo = _emu->GetRomInfo();
        snapshot.romName = FolderUtilities::GetFilename(romInfo.RomFile.GetFileName(), true);
        snapshot.filepath = filepath;
        snapshot.showSuccessMessage = showSuccessMessage;
        _emu->ProcessEvent(EventType::StateSaved);
    }
    // Enqueue for background compression + write (~0ms, just moves data)
    EnqueueWrite(std::move(snapshot));
    return true;
}
```

### Phase 2: Optimize Rewind (Future)

Rewind saves are frequent (every 30 frames) and use a different path (RewindManager). This phase would apply similar async patterns to rewind but is significantly more complex due to the need for immediate load-back capability.

## Risk Assessment

### Safe

- File-based saves (auto, quick, recent) — no one reads these back immediately
- Background thread only does compression + file I/O (no shared state mutation)
- Emulator lock duration reduced from ~5-10ms to ~1-3ms

### Risky

- `SaveRecentGame` (on ROM unload) — need to ensure background writes complete before process exit
- Rewind — needs immediate access to saved state, can't be fully async

### Mitigations

- Add `FlushPendingWrites()` method called during ROM unload / shutdown
- Keep rewind path synchronous for now (separate issue)
- Guard _writeQueue with mutex, use condition_variable for signaling

## Testing Plan

1. Verify all existing save state tests pass
2. Manual: Quick save during gameplay → verify file created and loadable
3. Manual: Auto-save triggers → verify save state valid
4. Manual: Close ROM → verify all pending writes complete
5. Benchmark: Measure lock hold time before/after (expect ~3-5ms reduction)

## Acceptance Criteria

- [ ] SaveState(filepath) holds lock only during state snapshot (~1-3ms)
- [ ] Compression + file write happens on background thread
- [ ] FlushPendingWrites() called on ROM unload and shutdown
- [ ] All save states remain loadable (format unchanged)
- [ ] All existing tests pass
- [ ] Lock hold time measured and documented
