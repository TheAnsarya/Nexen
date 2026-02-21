# Atari Lynx — Testing Guide

This document describes how to run and write tests for the Nexen Lynx emulator.

## Running Tests

### All Lynx Tests
```powershell
# Build first
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" `
	Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m

# Run all Lynx tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="Lynx*"

# Count tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="Lynx*" --gtest_list_tests | Select-String "^\s" | Measure-Object
```

### Specific Test Suites
```powershell
# Hardware reference tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="LynxHardwareRef*"

# Math coprocessor tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="LynxSuzy*"

# Timer tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="LynxTimer*"

# CPU tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="LynxCpu*"

# Hardware bugs
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="LynxHardwareBugs*"

# EEPROM tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="LynxEeprom*"

# Audio tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="LynxApu*"

# Memory/MAPCTL tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="LynxMemory*"
```

### Individual Tests
```powershell
# Run a single test
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="LynxHardwareRef.MasterClockIs16MHz"

# Run with verbose output
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter="LynxHardwareRef*" --gtest_print_time=1
```

## Running Benchmarks

### All Lynx Benchmarks
```powershell
# List all Lynx benchmarks
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="Lynx" --benchmark_list_tests

# Run all Lynx benchmarks
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="Lynx"

# Output as JSON
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="Lynx" --benchmark_out=lynx-bench.json --benchmark_out_format=json
```

### Specific Benchmark Suites
```powershell
# Memory/allocation benchmarks
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_LynxAlloc"

# Suzy/math benchmarks
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_LynxSuzy"

# CPU benchmarks
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_LynxCpu"
```

## Test Categories

### 1. Hardware Reference Tests (`LynxHardwareReferenceTests.cpp`)

**Purpose:** Validate emulation constants and structures against official Epyx/BLL documentation.

| Section | Tests | References |
|---------|-------|------------|
| Clock/Timing | 4 | 16 MHz master, 4 MHz CPU, 75 Hz refresh, cycles/frame |
| Display | 5 | 160x102 resolution, 4096 colors, 16 palette, buffer sizes |
| Memory Map | 5 | RAM size, Suzy/Mikey/ROM address ranges, MAPCTL |
| CPU 65SC02 | 5 | CMOS extensions (BRA, TRB, TSB, PHX/PLX, ZpgInd) |
| Timer System | 5 | 8 timers, prescaler masks, special assignments |
| Audio | 5 | 4 channels, LFSR taps, stereo attenuation, DAC mode |
| Sprite Engine | 5 | 8 types, BPP options, stretch/tilt, SCB linked list |
| Math Coprocessor | 5 | 16x16 multiply, accumulate, register layout |
| Controller/Input | 5 | 9 buttons, joypad bit layout, switch positions |
| EEPROM | 4 | 93C46 parameters (64 words, 16-bit, address bits) |
| Cart | 3 | Max ROM/header sizes, decryption key |
| State Integrity | 3 | State struct sizes, buffer inclusion |
| Hardware Bugs | 5 | Bugs 13.8, 13.9, 13.10, 13.12, sign-magnitude |
| UART | 2 | ComLynx baud rate, register count |
| Model Variants | 1 | Single model enum variant |

### 2. Math Coprocessor Tests (`LynxSuzyTests.cpp`)

Validates Suzy math operations:
- Unsigned/signed multiplication
- Division with remainder
- Accumulate mode
- Overflow detection
- Known hardware bugs (sign-magnitude behavior)
- Fuzz testing with random inputs

### 3. Benchmark Categories (`LynxAllocBench.cpp`)

| Category | Benchmarks | Purpose |
|----------|-----------|---------|
| State Sizes | 3 | Validate struct sizes for cache efficiency |
| Frame Buffer | 3 | Copy/fill performance for 160x102x2 |
| RAM Allocation | 3 | Stack vs heap vs vector for 64 KB |
| Save States | 2 | Serialization performance |
| Palette Lookup | 2 | LUT style (array vs shift) |
| Sprite Chain | 2 | SCB traversal patterns |
| Collision Buffer | 2 | Buffer clear/check patterns |
| Timer Batching | 2 | Per-timer vs batch tick cost |
| Audio Samples | 2 | Sample generation per frame |
| Input State | 2 | Copy vs read cost |

## Writing New Tests

### Test Template
```cpp
#include <gtest/gtest.h>
#include "Core/Lynx/LynxTypes.h"

TEST(LynxMyFeature, DescriptiveTestName) {
	// Arrange
	LynxSuzyState suzy{};

	// Act
	// ... test operations ...

	// Assert
	EXPECT_EQ(expected, actual);
}
```

### Conventions
- Test fixture name starts with `Lynx` prefix
- Test names describe WHAT is being verified
- Include hardware reference in comments:
  ```cpp
  // Per Epyx Developer's Guide, Section 13.8:
  // $8000 is treated as positive in sign-magnitude
  ```
- Use `EXPECT_*` (continue on failure) rather than `ASSERT_*` (abort on failure) unless subsequent tests depend on the check

### Adding to Build
Add new `.cpp` files to `Core.Tests/Core.Tests.vcxproj`:
```xml
<ClCompile Include="Lynx\YourNewTestFile.cpp" />
```

### Benchmark Template
```cpp
#include <benchmark/benchmark.h>
#include "Core/Lynx/LynxTypes.h"

static void BM_LynxFeature_Operation(benchmark::State& state) {
	// Setup outside timing loop
	LynxSuzyState suzy{};

	for (auto _ : state) {
		// Code to benchmark
		benchmark::DoNotOptimize(result);
	}
}
BENCHMARK(BM_LynxFeature_Operation);
```

## Test Statistics

As of the latest build:

| Metric | Count |
|--------|-------|
| Total Lynx tests | 828 |
| Hardware reference tests | 63 |
| All passing | ✅ Yes |
| Total Lynx benchmarks | 119 |
| Allocation benchmarks | 25 |
