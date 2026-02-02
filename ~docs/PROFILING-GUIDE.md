# C++ Profiling Guide for Nexen

This guide covers profiling infrastructure and workflows for performance analysis of the C++ Core library.

## Profiling Options

### 1. Visual Studio 2026 Profiler (Recommended for Windows)

Visual Studio's built-in profiler integrates seamlessly with the Nexen solution.

**Pros:**

- Zero configuration required
- Deep integration with VS debugger
- CPU, memory, GPU profiling
- Works with Release and Profile builds

**How to Use:**

1. Open `Nexen.sln` in Visual Studio 2026
2. Build in **Release** or **PGO** configuration
3. **Debug → Performance Profiler** (Alt+F2)
4. Select profiling type:
   - **CPU Usage** - For emulation performance
   - **Memory Usage** - For allocation analysis
   - **.NET Object Allocation** - For managed code
5. Click **Start** and run your test scenario
6. Stop profiling and analyze results

**CPU Usage Analysis:**

- Look for hot paths in CPU cores (`NesCpu::Exec`, `SnesCpu::Exec`, etc.)
- Check PPU rendering functions
- Identify memory access bottlenecks

### 2. Intel VTune Profiler

Professional-grade profiler with detailed microarchitecture analysis.

**Installation:**
Download from [Intel VTune Profiler](https://www.intel.com/content/www/us/en/developer/tools/oneapi/vtune-profiler.html)

**Configuration:**

1. Build Nexen with debug info: `Release|x64` configuration
2. Launch VTune and create new project
3. Set application path to `bin\win-x64\Release\Nexen.exe`

**Useful Analysis Types:**

- **Hotspots** - Find time-consuming functions
- **Microarchitecture Exploration** - CPU pipeline analysis
- **Memory Access** - Cache efficiency
- **Threading** - Parallelism analysis

### 3. Tracy Profiler (Real-time)

Lightweight, real-time profiler excellent for frame-by-frame analysis.

**Integration (Future Work):**

```cpp
// Add to pch.h when integrating Tracy
// #include <tracy/Tracy.hpp>

// Mark hot zones
// ZoneScoped;
// ZoneScopedN("PPU Render");
```

**Benefits:**

- Real-time visualization
- Frame-based analysis
- Low overhead (~2%)
- GPU profiling support

### 4. Superluminal Profiler

Modern Windows profiler with excellent UX.

**Features:**

- Sub-microsecond resolution
- Live profiling
- Excellent call tree visualization
- Works with unmodified Release builds

## Profile Build Configuration

For accurate profiling, use these build settings:

**Release with Debug Info:**

```xml
<Optimization>Full</Optimization>
<GenerateDebugInformation>true</GenerateDebugInformation>
<InlineFunctionExpansion>AnySuitable</InlineFunctionExpansion>
<FavorSizeOrSpeed>Speed</FavorSizeOrSpeed>
```

**Profile-Guided Optimization (PGO) Build:**

```bash
# Windows (use buildPGO script when available)
# 1. Instrument build
# 2. Run training scenarios
# 3. Optimize build
```

## Profiling Workflow

### Step 1: Establish Baseline

1. Run `Core.Benchmarks.exe` to get baseline metrics
2. Save results to `~docs/benchmarks/baseline-YYYY-MM-DD.json`

### Step 2: Profile Specific Scenario

1. Load ROM that exercises target code path
2. Enable FPS counter (F11)
3. Start profiler
4. Play for 30-60 seconds in consistent scenario
5. Stop profiler

### Step 3: Analyze Results

1. Sort by "Self CPU Time" or "Inclusive CPU Time"
2. Look for unexpected hot spots
3. Check cache miss rates
4. Identify memory allocation patterns

### Step 4: Iterate

1. Make targeted optimization
2. Re-run benchmark
3. Compare results
4. Document changes

## Key Performance Areas to Profile

### CPU Emulation

- `NesCpu::Exec()` - NES CPU main loop
- `SnesCpu::Exec()` - SNES CPU main loop
- `GbCpu::Exec()` - Game Boy CPU
- `GbaCpu::Exec()` - GBA ARM/Thumb

### PPU/Video

- `NesPpu::Run()` - NES PPU rendering
- `SnesPpu::Render*()` - SNES PPU layers
- `GbPpu::Exec()` - GB PPU

### Memory

- `NesMemoryManager::Read/Write()`
- `SnesMemoryManager::Read/Write()`
- Mapper implementations

### Audio

- APU mixing loops
- DSP processing (SNES SPC700)

## Benchmark Integration

The `Core.Benchmarks` project uses Google Benchmark for microbenchmarks:

```bash
# Run all benchmarks
bin\win-x64\Release\Core.Benchmarks.exe

# Run specific benchmark
bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter=ColorUtilities

# Output JSON for comparison
bin\win-x64\Release\Core.Benchmarks.exe --benchmark_format=json > results.json

# Compare two runs
compare.py results_before.json results_after.json
```

## Performance Targets

| Component | Target | Notes |
| ----------- | -------- | ------- |
| NES @ 60fps | <2ms/frame | 16.7ms budget |
| SNES @ 60fps | <8ms/frame | 16.7ms budget |
| GB @ 60fps | <4ms/frame | 16.7ms budget |
| GBA @ 60fps | <10ms/frame | 16.7ms budget |

## Tips

1. **Always profile Release builds** - Debug builds have different characteristics
2. **Profile representative scenarios** - Use games that stress target subsystems
3. **Profile on target hardware** - Results vary by CPU
4. **Use deterministic inputs** - TAS recordings for reproducible tests
5. **Check memory bandwidth** - Often the bottleneck in emulation
6. **Watch for branch mispredictions** - Common in interpreter loops

## Related Documentation

- [COMPILING.md](../COMPILING.md) - Build instructions
- [Core.Benchmarks](../Core.Benchmarks/) - Microbenchmark suite
- [buildPGO.sh](../buildPGO.sh) - PGO build script (Linux)

