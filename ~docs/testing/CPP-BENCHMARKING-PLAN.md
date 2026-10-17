# C++ Performance Benchmarking Plan

> **Created:** January 29, 2026  
> **Status:** 📋 PLANNED  
> **Epic:** [#80](https://github.com/TheAnsarya/Nexen/issues/80)

## Overview

Establish performance benchmarking infrastructure to measure the impact of C++ modernizations and prevent performance regressions.

## Goals

1. **Validate Zero-Cost Abstractions**: Ensure modernizations don't degrade performance
2. **Regression Detection**: Catch performance regressions before merge
3. **Optimization Guidance**: Identify hotspots for future optimization
4. **Release Performance**: Focus on Release build metrics (debug perf not critical)

## Framework Selection: Google Benchmark

### Why Google Benchmark?

| Criteria | Google Benchmark | Custom Timing | nanobench |
| ---------- | ------------------ | --------------- | ----------- |
| Industry Standard | ✅ Widely used | ❌ Reinventing wheel | ⚠️ Less known |
| Statistical Analysis | ✅ Mean, median, stddev | ❌ Manual | ✅ Yes |
| Visual Studio Integration | ✅ Good | ⚠️ None | ⚠️ Limited |
| Parametric Benchmarks | ✅ Built-in | ❌ Manual | ✅ Yes |
| Regression Testing | ✅ Compare mode | ❌ Manual | ✅ Yes |
| vcpkg Support | ✅ Yes | N/A | ⚠️ No |
| Warmup/Cooldown | ✅ Automatic | ❌ Manual | ✅ Yes |

**Decision:** Google Benchmark - battle-tested, excellent VS integration, statistical rigor.

## Project Structure

```text
Nexen/
├── Core.Benchmarks/			   # NEW - C++ benchmark project
│   ├── Core.Benchmarks.vcxproj	# VS project file
│   ├── pch.h					  # Precompiled header
│   ├── pch.cpp
│   ├── Shared/					# Benchmarks for Core/Shared
│   │   ├── ColorUtilitiesBench.cpp
│   │   ├── CrcBench.cpp
│   │   └── ...
│   ├── Utilities/				 # Benchmarks for Utilities
│   │   ├── HexUtilitiesBench.cpp
│   │   └── ...
│   ├── Emulation/				 # Emulation core benchmarks
│   │   ├── NesCpuBench.cpp
│   │   ├── SnesCpuBench.cpp
│   │   └── ...
│   ├── main.cpp				   # benchmark::Initialize() entry point
│   └── baseline/				  # Baseline results (gitignored)
│	   ├── ColorUtilities_baseline.json
│	   └── ...
└── ~docs/testing/
	└── BENCHMARK-RESULTS.md	   # Latest benchmark results
```

## Benchmark Categories

### Micro-Benchmarks (Hot Path Validation)

Measure individual functions that are called millions of times per frame.

#### Example: ColorUtilities::Rgb555ToArgb (constexpr)

```cpp
#include <benchmark/benchmark.h>
#include "Shared/ColorUtilities.h"

static void BM_ColorUtilities_Rgb555ToArgb(benchmark::State& state) {
	uint16_t color = 0x7FFF;
	for (auto _ : state) {
		benchmark::DoNotOptimize(ColorUtilities::Rgb555ToArgb(color));
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ColorUtilities_Rgb555ToArgb);

static void BM_ColorUtilities_Rgb555ToArgb_Loop(benchmark::State& state) {
	for (auto _ : state) {
		for (uint16_t i = 0; i < 0x8000; i++) {
			benchmark::DoNotOptimize(ColorUtilities::Rgb555ToArgb(i));
		}
	}
	state.SetItemsProcessed(state.iterations() * 0x8000);
}
BENCHMARK(BM_ColorUtilities_Rgb555ToArgb_Loop);
```

**Expected Results:**

- **Before constexpr:** ~5-10 ns/op
- **After constexpr:** ~2-5 ns/op (or compile-time if used with literals)

#### Example: std::span vs Raw Pointers

```cpp
static void BM_BatteryManager_SaveBattery_RawPointer(benchmark::State& state) {
	std::vector<uint8_t> data(state.range(0), 0xAA);
	
	for (auto _ : state) {
		// Simulate old implementation
		BatteryManager_Legacy::SaveBattery(".sav", data.data(), data.size());
	}
	state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_BatteryManager_SaveBattery_RawPointer)->Range(1024, 1<<20);

static void BM_BatteryManager_SaveBattery_Span(benchmark::State& state) {
	std::vector<uint8_t> data(state.range(0), 0xAA);
	
	for (auto _ : state) {
		BatteryManager::SaveBattery(".sav", std::span(data));
	}
	state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_BatteryManager_SaveBattery_Span)->Range(1024, 1<<20);
```

**Expected Results:**

- **Release build:** Identical performance (zero-cost abstraction)
- **Debug build:** std::span may be slower (acceptable per user directive)

### Macro-Benchmarks (System-Level Performance)

Measure larger subsystems like a full frame of emulation.

#### Example: NES Frame Rendering

```cpp
static void BM_NES_RenderFrame(benchmark::State& state) {
	// Load test ROM (Super Mario Bros)
	auto console = CreateTestConsole("TestData/smb1.nes");
	
	for (auto _ : state) {
		console->RunSingleFrame();
	}
	
	// Report as frames per second
	state.counters["FPS"] = benchmark::Counter(
		state.iterations(),
		benchmark::Counter::kIsRate
	);
}
BENCHMARK(BM_NES_RenderFrame)->Unit(benchmark::kMillisecond);
```

**Expected Results:**

- **Target:** > 1000 FPS (1ms per frame) for NES on modern CPU
- **Regression threshold:** < 5% variance

### Comparison Benchmarks (Before/After Modernization)

Compare old implementation vs new modernized version.

#### Example: [[unlikely]] Branch Hints

```cpp
// Old version (no hints)
uint8_t ReadMemory_Old(uint16_t addr) {
	if (addr < 0x2000) {
		return ram[addr & 0x7FF];
	} else if (addr < 0x4000) {
		return ppu->ReadRegister(addr);
	} else {
		return io->ReadRegister(addr);
	}
}

// New version (with [[unlikely]])
uint8_t ReadMemory_New(uint16_t addr) {
	if (addr < 0x2000) {
		return ram[addr & 0x7FF];
	} else if (addr < 0x4000) [[unlikely]] {
		return ppu->ReadRegister(addr);
	} else [[unlikely]] {
		return io->ReadRegister(addr);
	}
}

static void BM_MemoryRead_NoHints(benchmark::State& state) {
	// Mostly RAM reads (typical case)
	std::vector<uint16_t> addresses;
	for (int i = 0; i < 1000; i++) {
		addresses.push_back(i % 0x2000); // 100% RAM
	}
	
	for (auto _ : state) {
		for (auto addr : addresses) {
			benchmark::DoNotOptimize(ReadMemory_Old(addr));
		}
	}
}
BENCHMARK(BM_MemoryRead_NoHints);

static void BM_MemoryRead_WithHints(benchmark::State& state) {
	std::vector<uint16_t> addresses;
	for (int i = 0; i < 1000; i++) {
		addresses.push_back(i % 0x2000); // 100% RAM
	}
	
	for (auto _ : state) {
		for (auto addr : addresses) {
			benchmark::DoNotOptimize(ReadMemory_New(addr));
		}
	}
}
BENCHMARK(BM_MemoryRead_WithHints);
```

**Expected Results:**

- **With [[unlikely]]:** 5-15% faster for typical case (RAM reads)
- **Without hints:** More branch mispredictions

## Benchmark Targets

### High Priority (Hot Paths)

| Component | Function | Iterations/Frame | Priority |
| ----------- | ---------- | ------------------ | ---------- |
| NesCpu | ExecuteInstruction | ~30,000 | 🔴 CRITICAL |
| NesPpu | RenderPixel | ~61,440 | 🔴 CRITICAL |
| SnesCpu | ExecuteOp | ~89,341 | 🔴 CRITICAL |
| Memory | ReadByte / WriteByte | ~100,000 | 🔴 CRITICAL |
| ColorUtilities | Rgb555ToArgb | ~61,440 | 🟡 HIGH |

### Medium Priority (Warm Paths)

| Component | Function | Frequency | Priority |
| ----------- | ---------- | ----------- | ---------- |
| BatteryManager | SaveBattery | Per save | 🟢 MEDIUM |
| Crc32 | CalculateCRC | Per ROM load | 🟢 MEDIUM |
| HexUtilities | ToHex | Debug only | 🟢 MEDIUM |

### Low Priority (Cold Paths)

| Component | Function | Frequency | Priority |
| ----------- | ---------- | ----------- | ---------- |
| StringUtilities | Format | UI updates | 🔵 LOW |
| FolderUtilities | CreateFolder | Setup only | 🔵 LOW |

## Regression Thresholds

Define acceptable performance variance:

```cpp
// Benchmark with custom threshold
BENCHMARK(BM_CriticalPath)
	->Unit(benchmark::kNanosecond)
	->MinTime(2.0)  // Run for at least 2 seconds
	->ComputeStatistics("max", [](const std::vector<double>& v) {
		return *std::max_element(v.begin(), v.end());
	})
	->ComputeStatistics("min", [](const std::vector<double>& v) {
		return *std::min_element(v.begin(), v.end());
	});
```

### Thresholds

- **Critical paths (CPU/PPU):** < 2% regression
- **Hot paths (utilities):** < 5% regression
- **Warm paths (I/O):** < 10% regression
- **Cold paths (UI):** No threshold

## Baseline Management

### Establishing Baselines

```powershell
# Run benchmarks and save baseline
.\Core.Benchmarks.exe --benchmark_out=baseline/baseline.json --benchmark_out_format=json

# Save git commit hash
git rev-parse HEAD > baseline/commit.txt
```

### Comparing Against Baseline

```powershell
# Run current benchmarks
.\Core.Benchmarks.exe --benchmark_out=current.json --benchmark_out_format=json

# Compare using benchmark tool
python tools/compare.py baseline/baseline.json current.json
```

### Example Compare Output

```text
Comparing baseline/baseline.json to current.json
Benchmark								  Time	   CPU	Time Old	Time New
---------------------------------------------------------------------------------------
BM_ColorUtilities_Rgb555ToArgb		   -15 ns	-15 ns		 20		 5
BM_NesCpu_ExecuteInstruction			  +2 ns	 +2 ns		100	   102  ⚠️ REGRESSION
BM_BatteryManager_SaveBattery_Span		+0 ns	 +0 ns	   5000	  5000  ✅ SAME
```

## CI Integration (Future)

### GitHub Actions Workflow

```yaml
name: Performance Benchmarks

on:
  pull_request:
	branches: [cpp-modernization, master]

jobs:
  benchmark:
	runs-on: windows-latest
	steps:
	  - uses: actions/checkout@v3
		with:
		  fetch-depth: 0  # Need history for baseline
	  
	  - name: Get baseline commit
		run: |
		  git checkout main
		  git rev-parse HEAD > baseline_commit.txt
	  
	  - name: Build baseline
		run: msbuild Nexen.sln /p:Configuration=Release /t:Core.Benchmarks
	  
	  - name: Run baseline benchmarks
		run: |
		  .\Core.Benchmarks\bin\Release\Core.Benchmarks.exe --benchmark_out=baseline.json
	  
	  - name: Checkout PR
		run: git checkout ${{ github.sha }}
	  
	  - name: Build PR
		run: msbuild Nexen.sln /p:Configuration=Release /t:Core.Benchmarks
	  
	  - name: Run PR benchmarks
		run: |
		  .\Core.Benchmarks\bin\Release\Core.Benchmarks.exe --benchmark_out=current.json
	  
	  - name: Compare results
		run: python tools/benchmark_compare.py baseline.json current.json --threshold 5
	  
	  - name: Comment on PR
		uses: actions/github-script@v6
		with:
		  script: |
			// Post comparison results as PR comment
```

## Test ROM Selection

### Ideal Test ROMs

1. **nestest.nes** - CPU instruction test
2. **ppu_vbl_nmi.nes** - PPU timing test
3. **Super Mario Bros 1** - Real-world gameplay
4. **Mega Man 2** - Complex graphics
5. **Castlevania** - Intensive rendering

### Requirements

- Small file size (< 1MB)
- Deterministic behavior
- Representative of real games
- Publicly available or homebrew

## Benchmark Naming Convention

```text
BM_<Component>_<Function>_<Variant>

Examples:
- BM_ColorUtilities_Rgb555ToArgb
- BM_ColorUtilities_Rgb555ToArgb_Constexpr
- BM_NesCpu_ExecuteInstruction_AddressingMode_Immediate
- BM_BatteryManager_SaveBattery_Span
- BM_BatteryManager_SaveBattery_RawPointer
```

## Output Formats

### Console Output (Default)

```text
Run on (16 X 3600 MHz CPU s)
CPU Caches:
  L1 Data 32K (x8)
  L1 Instruction 32K (x8)
  L2 Unified 256K (x8)
  L3 Unified 16384K (x1)
-------------------------------------------------------------------------
Benchmark							   Time			 CPU   Iterations
-------------------------------------------------------------------------
BM_ColorUtilities_Rgb555ToArgb	   4.15 ns		 4.14 ns	168872727
BM_NesCpu_ExecuteInstruction		  102 ns		  102 ns	  6862745
```

### JSON Output (For Regression Tracking)

```json
{
  "context": {
	"date": "2026-01-29T12:34:56Z",
	"host_name": "DESKTOP-ABC123",
	"executable": "Core.Benchmarks.exe",
	"num_cpus": 16,
	"mhz_per_cpu": 3600
  },
  "benchmarks": [
	{
	  "name": "BM_ColorUtilities_Rgb555ToArgb",
	  "iterations": 168872727,
	  "real_time": 4.15,
	  "cpu_time": 4.14,
	  "time_unit": "ns"
	}
  ]
}
```

## Success Metrics

- ✅ Benchmark project builds without errors
- ✅ Benchmarks run in < 5 minutes (full suite)
- ✅ Results reproducible (< 5% variance between runs)
- ✅ Zero performance regressions on critical paths
- ✅ Baselines established for all modernizations

## Implementation Checklist

### Phase 1: Infrastructure

- [ ] Install Google Benchmark via vcpkg
- [ ] Create Core.Benchmarks project
- [ ] Configure Release-only builds
- [ ] Add benchmark::Initialize() main
- [ ] Verify benchmark discovery in VS

### Phase 2: Initial Benchmarks

- [ ] ColorUtilities benchmarks
- [ ] CRC32 benchmarks
- [ ] BatteryManager std::span comparison
- [ ] Establish baselines (pre-modernization)

### Phase 3: Modernization Validation

- [ ] [[unlikely]] branch hint comparisons
- [ ] constexpr function comparisons
- [ ] [[nodiscard]] (should be zero cost)
- [ ] std::span vs raw pointer comparisons

### Phase 4: Emulation Benchmarks

- [ ] NES CPU instruction benchmarks
- [ ] NES PPU frame rendering
- [ ] SNES CPU benchmarks
- [ ] Full frame benchmarks (all systems)

## Related Documents

- [Epic #80: C++ Performance Benchmarking](https://github.com/TheAnsarya/Nexen/issues/80)
- [Epic #79: C++ Unit Testing Infrastructure](https://github.com/TheAnsarya/Nexen/issues/79)
- [CPP-TESTING-PLAN.md](CPP-TESTING-PLAN.md)
- [cpp-modernization-opportunities.md](../modernization/cpp-modernization-opportunities.md)
