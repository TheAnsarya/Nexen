# Automated Accuracy Testing Plan

> Infrastructure design for running hardware accuracy test ROMs automatically in Nexen.

## Overview

Nexen already has robust automation infrastructure. This plan describes how to leverage existing capabilities to build a comprehensive accuracy test pipeline using community test ROMs.

---

## Existing Infrastructure

### 1. TestApiWrapper (C++ Interop)

**File:** `InteropDLL/TestApiWrapper.cpp`

Two key entry points:

#### `RunTest(filename, address, memType)` — Memory-Read Tests

```cpp
// Creates background emulator, loads ROM, runs 500 frames, reads memory
uint64_t RunTest(char* filename, uint32_t address, MemoryType memType);
```

- Creates isolated `Emulator` instance
- Sets `EmulationFlags::TestMode` + `EmulationFlags::MaximumSpeed`
- Loads ROM and runs 500 frames
- Reads 8 bytes from specified address/memory type
- Returns result as `uint64_t`
- **Ideal for:** Test ROMs that write pass/fail status to a known memory address (blargg tests write to $6000)

#### `RunRecordedTest(filename, inBackground)` — Frame-Hash Tests

```cpp
// Plays back recorded test, comparing frame MD5 hashes
RomTestResult RunRecordedTest(char* filename, bool inBackground);
```

- Loads previously recorded test (frame hashes + input)
- Replays and compares each frame's MD5 hash
- Returns `Passed`, `Failed`, or `PassedWithWarnings` (≤10 bad frames tolerated)
- **Ideal for:** Visual regression testing — detect rendering changes

### 2. Lua Scripting API

**File:** `Core/Debugger/LuaApi.h/cpp`

Full scripting with:

- **Memory:** `emu.read()`, `emu.write()` (8/16/32-bit, any memory type)
- **Screen:** `emu.getScreenBuffer()`, `emu.setScreenBuffer()`, `emu.takeScreenshot()`
- **Input:** `emu.getInput()`, `emu.setInput()`
- **State:** `emu.getState()`, `emu.setState()`, `emu.createSavestate()`, `emu.loadSavestate()`
- **Events:** `emu.addEventCallback()` for frame end, input polled, memory read/write, etc.
- **Control:** `emu.reset()`, `emu.stop()`, `emu.step()`, `emu.exit(code)`
- **Info:** `emu.getRomInfo()`, `emu.log()`

### 3. Command-Line Interface

**File:** `UI/Utilities/CommandLineHelper.cs`

| Flag | Purpose |
|------|---------|
| `--novideo` | Headless mode (no video output) |
| `--noaudio` | Disable audio |
| `--noinput` | Disable user input |
| `--timeout=N` | Auto-exit after N seconds (default: 100) |
| `.lua` file arg | Auto-load Lua script |
| ROM file arg | Auto-load ROM |

**Headless test execution:**

```powershell
Nexen.exe "test.nes" "test-script.lua" --novideo --noaudio --noinput --timeout=30
```

### 4. Movie System

**File:** `Core/Shared/Movies/NexenMovie.h/cpp`

- `.nexen-movie` format (ZIP with `movie.json`, `input.txt`, optional savestate/scripts)
- `Play(file, forTest)` — silent playback mode for testing
- Deterministic input replay for reproducible test execution

### 5. Existing Test Pattern — boot-smoke.lua

**File:** `~scripts/lynx/boot-smoke.lua`

Established pattern:

- Sample screen buffer periodically via `emu.getScreenBuffer()`
- Track metrics (non-zero pixels, unique colors)
- Apply scripted input at specific frames
- Exit with code: 0=pass, 2=fail, 90=wrong console
- Event-driven via `emu.addEventCallback()`

---

## Test Automation Strategies

### Strategy 1: Memory-Read Tests (Highest Priority)

For test ROMs that write results to known memory locations.

**Applicable to:** blargg's NES tests ($6000), AccuracyCoin (RAM locations), Mooneye (register protocol)

**Implementation:**

```
RunTest("blargg/cpu_instrs.nes", 0x6000, MemoryType::NesWorkRam)
→ Check result byte: 0x00 = running, 0x01-0x80 = pass, 0x81+ = fail
```

**Extension needed:** Make frame count configurable (some tests need >500 frames).

### Strategy 2: Lua Script Tests (Most Flexible)

For tests requiring complex pass/fail detection or input sequences.

**Template pattern (based on boot-smoke.lua):**

```lua
-- Generic accuracy test Lua template
local testName = "cpu_instrs"
local maxFrames = 3000
local resultAddress = 0x6000
local resultMemType = "nesWorkRam"
local expectedResult = 0x00  -- or check for specific pass value

local function checkResult()
    local result = emu.read(resultAddress, resultMemType)
    if result == expectedResult then
        emu.log(testName .. ": PASS")
        emu.exit(0)
    else
        emu.log(testName .. ": FAIL (result=" .. tostring(result) .. ")")
        emu.exit(1)
    end
end

local function onFrame()
    local state = emu.getState()
    local frame = state["frameCount"]
    if frame >= maxFrames then
        checkResult()
    end
end

emu.addEventCallback(onFrame, emu.eventType.endFrame)
```

**For screen-based tests (screenshot comparison):**

```lua
local function compareScreen(referenceHash)
    local buffer = emu.getScreenBuffer()
    -- Compute hash of screen buffer
    -- Compare against known-good reference hash
end
```

### Strategy 3: Recorded Tests (Regression Testing)

For visual regression — detect any rendering changes between builds.

**Workflow:**

1. Record golden reference: `RomTestRecord("test.nes")`
2. Play through the test manually (or via script)
3. Stop recording: `RomTestStop()`
4. On future builds: `RunRecordedTest("test.mrt", true)` → Pass/Fail

### Strategy 4: Screenshot Comparison

For tests with known reference screenshots (acid2 tests, visual tests).

**Implementation:** Use Lua `emu.getScreenBuffer()` to capture pixel data, hash it, compare against golden reference hashes stored in a JSON file.

---

## Architecture: Test Runner Framework

### Phase 1: Lua Test Runner (MVP)

A master Lua script that:

1. Reads a test manifest (JSON file)
2. For each test ROM: loads it, waits N frames, checks pass/fail condition
3. Writes results to a log file
4. Returns overall exit code

**Test manifest format:**

```json
{
    "tests": [
        {
            "name": "blargg_cpu_instrs",
            "rom": "nes/blargg/cpu_instrs/cpu_instrs.nes",
            "type": "memory_read",
            "address": "0x6000",
            "memType": "nesWorkRam",
            "expectedValue": 0,
            "maxFrames": 3000,
            "system": "nes"
        },
        {
            "name": "mooneye_acceptance_timer",
            "rom": "gb/mooneye/acceptance/timer/timer_basic.gb",
            "type": "register_check",
            "expectedRegisters": {"B": 3, "C": 5, "D": 8, "E": 13, "H": 21, "L": 34},
            "maxFrames": 1000,
            "system": "gb"
        },
        {
            "name": "accuracycoin_all",
            "rom": "nes/accuracycoin/AccuracyCoin.nes",
            "type": "lua_script",
            "script": "scripts/nes/accuracycoin-runner.lua",
            "movie": "movies/nes/accuracycoin-run-all.nexen-movie",
            "system": "nes"
        }
    ]
}
```

### Phase 2: C++ Test Harness

Extend `TestApiWrapper.cpp` with:

- Configurable frame count (not hardcoded 500)
- Multiple memory reads per test
- Screenshot capture and hash comparison
- Batch test execution
- JSON result output

### Phase 3: CI Integration

Extend `.github/workflows/tests.yml`:

```yaml
accuracy-tests:
    runs-on: windows-2025-vs2026
    steps:
        - name: Build Nexen Release
          run: MSBuild Nexen.sln /p:Configuration=Release /p:Platform=x64

        - name: Download test ROMs
          run: |
              # Download AccuracyCoin, mooneye prebuilts, blargg archives, etc.
              scripts/download-test-roms.ps1

        - name: Run accuracy tests
          run: |
              bin/win-x64/Release/Nexen.exe --run-accuracy-tests --test-manifest tests/accuracy/manifest.json --novideo --noaudio

        - name: Upload results
          uses: actions/upload-artifact@v4
          with:
              name: accuracy-results
              path: tests/accuracy/results/
```

---

## Test Categories and Frame Budgets

| Category | Typical Frame Budget | Strategy |
|----------|---------------------|----------|
| CPU instruction tests | 500-3000 frames | Memory read |
| PPU timing tests | 100-1000 frames | Memory read or screen hash |
| APU tests | 1000-5000 frames | Memory read (audio tests need longer) |
| Mapper tests | 500-2000 frames | Memory read or screen hash |
| Full test suites (AccuracyCoin) | 10000+ frames | Movie + Lua script |
| Mooneye tests | 500-2000 frames | Register check (Fibonacci protocol) |
| Acid2 visual tests | 100-500 frames | Screenshot hash |
| Boot smoke tests | 600-900 frames | Lua screen sampling |

---

## Required Extensions to Nexen

### Must-Have (Phase 1)

1. **Configurable frame budget** in `RunTest()` — currently hardcoded to 500
2. **Batch test runner** — PowerShell script calling Nexen per-ROM with Lua scripts
3. **Exit code propagation** — ensure `emu.exit(code)` propagates to process exit
4. **Test manifest parser** — JSON manifest → test execution

### Nice-to-Have (Phase 2+)

5. **Multiple memory reads** — read several addresses per test
6. **Screen buffer hashing** — built-in MD5/CRC32 of frame buffer
7. **CI workflow integration** — GitHub Actions with test ROM download step
8. **Result dashboard** — HTML report of pass/fail per system per test

---

## Related Documents

- [Accuracy Test ROM Catalog](accuracy-test-rom-catalog.md) — Complete catalog of available test ROMs
- [In-House Test ROM Development Plan](in-house-test-rom-plan.md) — Custom test ROM development with Flower Toolchain
- [Accuracy Testing Pipeline](accuracy-testing-pipeline.md) — Step-by-step commands
