# C++ Testing Infrastructure Plan

> **Created:** January 29, 2026  
> **Status:** 📋 PLANNED  
> **Epic:** [#79](https://github.com/TheAnsarya/Nexen/issues/79)

## Overview

Establish comprehensive C++ unit testing for Nexen Core libraries using **Google Test (gtest)** framework.

## Framework Selection: Google Test

### Why Google Test?

| Criteria | Google Test | Catch2 |
| ---------- | ------------- | --------- |
| Industry Standard | ✅ Most widely used | ⚠️ Growing adoption |
| Visual Studio Integration | ✅ Excellent (Test Explorer) | ✅ Good |
| Performance | ✅ Fast compilation | ⚠️ Slower (header-only) |
| Parameterized Tests | ✅ Built-in | ⚠️ Requires macros |
| Mocking Support | ✅ Google Mock included | ⚠️ Separate library |
| Documentation | ✅ Comprehensive | ✅ Good |
| vcpkg Support | ✅ Yes | ✅ Yes |
| Death Tests | ✅ Yes | ✅ Yes |

**Decision:** Google Test - better VS integration, faster compile times, comprehensive mocking support.

## Project Structure

```text
Nexen/
├── Core.Tests/					# NEW - C++ test project
│   ├── Core.Tests.vcxproj		 # VS project file
│   ├── pch.h					  # Precompiled header
│   ├── pch.cpp
│   ├── Shared/					# Tests for Core/Shared
│   │   ├── ColorUtilitiesTests.cpp
│   │   ├── CrcTests.cpp
│   │   ├── CpuTypeTests.cpp
│   │   └── ...
│   ├── Utilities/				 # Tests for Utilities
│   │   ├── HexUtilitiesTests.cpp
│   │   ├── StringUtilitiesTests.cpp
│   │   └── ...
│   ├── NES/					   # Tests for NES emulation
│   │   ├── NesCpuTests.cpp
│   │   ├── NesPpuTests.cpp
│   │   └── ...
│   └── main.cpp				   # gtest main entry point
├── Utilities.Tests/			   # FUTURE - Utilities library tests
├── vcpkg.json					 # NEW - Package manifest
└── vcpkg-configuration.json	   # NEW - vcpkg config
```

## Implementation Phases

### Phase 1: Infrastructure Setup (Epic #79)

**Estimated Time:** 2-3 hours

1. **Install vcpkg (if not already available)**

	```powershell
	# Clone vcpkg repository
	git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
	cd C:\vcpkg
	.\bootstrap-vcpkg.bat
	```

2. **Create vcpkg.json manifest**

	```json
	{
		"name": "Nexen",
		"version": "2.0.0",
		"dependencies": [
			"gtest"
		]
	}
	```

3. **Create Core.Tests project**
	- Platform Toolset: v145 (VS 2026)
	- Language Standard: C++23 (`/std:c++latest`)
	- Include paths: $(SolutionDir)Core, $(SolutionDir)Utilities
	- Link to Core.lib and Utilities.lib
	- Configure Test Adapter for Google Test

4. **Configure Visual Studio Test Explorer**
	- Install "Test Adapter for Google Test" extension
	- Configure test discovery settings

### Phase 2: Initial Test Coverage (Week 1)

**Priority:** HIGH - Zero-cost abstractions, utilities

#### Utilities Tests (LOW RISK - No hot paths)

- `HexUtilitiesTests.cpp` - hex parsing, formatting
- `StringUtilitiesTests.cpp` - string manipulation
- `FolderUtilitiesTests.cpp` - file operations
- `PlatformUtilitiesTests.cpp` - platform detection
- `TimerTests.cpp` - timing utilities

#### Core/Shared Tests (MEDIUM RISK - Profile first)

- `ColorUtilitiesTests.cpp` - constexpr color functions
- `CrcTests.cpp` - CRC32 calculations
- `Crc32Tests.cpp` - new C++20 CRC (verify accuracy)
- `CpuTypeTests.cpp` - enum operations
- `MemoryTypeTests.cpp` - enum operations
- `BatteryManagerTests.cpp` - std::span API

### Phase 3: Emulation Core Tests (Week 2)

**Priority:** MEDIUM - Requires ROM test data

#### NES Tests

- `NesCpuTests.cpp` - CPU instruction accuracy
- `NesPpuTests.cpp` - PPU rendering correctness
- `NesApuTests.cpp` - Audio synthesis
- `NesMapperTests.cpp` - Mapper implementations

#### SNES Tests

- `SnesCpuTests.cpp` - 65816 opcodes
- `SnesPpuTests.cpp` - Mode 0-7 rendering
- `SnesDspTests.cpp` - DSP chip emulation

### Phase 4: Regression Tests (Ongoing)

**Priority:** HIGH - Prevent regressions

- Known bug fixes → regression tests
- Edge cases discovered during development
- Performance regressions

## Test Categories

### Unit Tests

Test individual functions/classes in isolation.

```cpp
// Example: ColorUtilities
TEST(ColorUtilitiesTests, Rgb555To888_ValidColor_ReturnsCorrect) {
	uint32_t result = ColorUtilities::Rgb555ToArgb(0x7FFF);
	EXPECT_EQ(result, 0xFFF8F8F8);
}

TEST(ColorUtilitiesTests, Constexpr_CompileTime) {
	constexpr uint32_t color = ColorUtilities::Rgb555ToArgb(0x001F);
	EXPECT_EQ(color, 0xFFF80000);
}
```

### Integration Tests

Test multiple components working together.

```cpp
// Example: BatteryManager + Savestate
TEST(BatteryManagerTests, SaveAndLoad_RoundTrip_DataMatches) {
	std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
	BatteryManager::SaveBattery(".sav", data);
	
	std::vector<uint8_t> loaded(4);
	BatteryManager::LoadBattery(".sav", loaded);
	
	EXPECT_EQ(data, loaded);
}
```

### Parameterized Tests

Test same logic with multiple inputs.

```cpp
class ColorConversionTest : public testing::TestWithParam<std::tuple<uint16_t, uint32_t>> {};

TEST_P(ColorConversionTest, Rgb555ToArgb) {
	auto [input, expected] = GetParam();
	EXPECT_EQ(ColorUtilities::Rgb555ToArgb(input), expected);
}

INSTANTIATE_TEST_SUITE_P(
	ValidColors,
	ColorConversionTest,
	testing::Values(
		std::make_tuple(0x0000, 0xFF000000),
		std::make_tuple(0x7FFF, 0xFFF8F8F8),
		std::make_tuple(0x001F, 0xFFF80000)
	)
);
```

### Death Tests

Test that code crashes/asserts as expected.

```cpp
TEST(BatteryManagerDeathTest, InvalidSpan_Crashes) {
	EXPECT_DEATH({
		std::span<uint8_t> invalid(nullptr, 100);
		BatteryManager::SaveBattery(".sav", invalid);
	}, ".*");
}
```

## Benchmarking Integration

Tests can also serve as micro-benchmarks:

```cpp
TEST(ColorUtilitiesPerf, Rgb555ToArgb_1Million) {
	auto start = std::chrono::high_resolution_clock::now();
	
	for (int i = 0; i < 1'000'000; i++) {
		volatile uint32_t result = ColorUtilities::Rgb555ToArgb(i & 0x7FFF);
	}
	
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	
	// Should complete in < 10ms (debug) or < 1ms (release)
	EXPECT_LT(duration.count(), 10'000);
}
```

## Test Data Management

### Small Test Data

Embed directly in test code:

```cpp
const uint8_t kNesTestRom[] = {
	0x4E, 0x45, 0x53, 0x1A, // "NES\x1A"
	0x02, 0x01, 0x00, 0x00, // 2 PRG, 1 CHR, mapper 0
	// ...
};
```

### Large Test ROMs

Store in `Core.Tests/TestData/` (gitignored):

```cpp
TEST(NesEmulationTests, NesTest_Passes) {
	std::ifstream rom("TestData/nestest.nes", std::ios::binary);
	ASSERT_TRUE(rom.is_open());
	// Run test ROM, verify output
}
```

## Continuous Integration

### Local Testing

```powershell
# Build and run tests
msbuild Nexen.sln /t:Core.Tests /p:Configuration=Release
vstest.console.exe Core.Tests\bin\Release\Core.Tests.dll

# Or via Test Explorer in Visual Studio
# Test → Run All Tests
```

### GitHub Actions (Future)

```yaml
name: C++ Tests

on: [push, pull_request]

jobs:
  test:
	runs-on: windows-latest
	steps:
	  - uses: actions/checkout@v3
	  - name: Setup vcpkg
		run: |
		  git clone https://github.com/microsoft/vcpkg.git
		  .\vcpkg\bootstrap-vcpkg.bat
	  - name: Build
		run: msbuild Nexen.sln /p:Configuration=Release
	  - name: Test
		run: vstest.console.exe Core.Tests\bin\Release\Core.Tests.dll
```

## Coverage Goals

| Component | Target Coverage | Priority |
| ----------- | ---------------- | ---------- |
| Utilities | 80% | HIGH |
| Core/Shared | 60% | HIGH |
| CPU Emulation | 40% | MEDIUM |
| PPU Emulation | 30% | MEDIUM |
| Mappers | 20% | LOW |

## Success Metrics

- ✅ Test project builds without errors
- ✅ Tests discoverable in VS Test Explorer
- ✅ All tests pass in Release build
- ✅ < 1 second test suite execution (initial suite)
- ✅ Zero false positives
- ✅ Easy to add new tests

## Open Questions

1. **Test ROM Licensing**: Can we include test ROMs in repo? (Likely NO)
2. **Performance Impact**: Should we profile before writing CPU tests?
3. **Mock Framework**: Do we need Google Mock, or just gtest?
4. **Test Data Size**: Should we download large test ROMs on-demand?

## Related Documents

- [Epic #79: C++ Unit Testing Infrastructure](https://github.com/TheAnsarya/Nexen/issues/79)
- [Epic #80: C++ Performance Benchmarking](https://github.com/TheAnsarya/Nexen/issues/80)
- [CPP-ISSUES-TRACKING.md](../modernization/CPP-ISSUES-TRACKING.md)
- [cpp-modernization-opportunities.md](../modernization/cpp-modernization-opportunities.md)
