# 🔧 Nexen C++ Core Modernization Roadmap

> **Branch:** `cpp-modernization` (to be created from `pansy-export`)
> **Document Created:** January 28, 2026
> **Last Updated:** January 28, 2026
> **Status:** 📋 **PLANNING PHASE**

## 📋 Executive Summary

This roadmap outlines the comprehensive modernization of Nexen's C++ core emulation components. The goal is to update to the latest C++ standards (C++23/26), modernize the codebase with best practices, implement testing, optimize performance, and improve documentation.

## 🎯 Goals

1. **Modern C++ Standards** - Upgrade from C++17/20 to C++23 (with C++26 features where supported)
2. **Modern Libraries** - Use standard library features, consider ranges, std::span, std::format
3. **Testing Framework** - Implement unit tests for core components using Google Test or Catch2
4. **Benchmarking** - Create performance benchmarks for critical emulation paths
5. **Code Quality** - Apply clang-tidy, address sanitizers, modernize idioms
6. **Documentation** - Comprehensive documentation with Doxygen

## 📊 Current State Analysis

### C++ Projects in Solution

| Project | Purpose | Files | Priority |
| --------- | --------- | ------- | ---------- |
| **Core** | Main emulation engines (NES, SNES, GB, GBA, SMS, PCE, WS) | ~500+ | High |
| **Utilities** | Shared utility code | ~50 | Medium |
| **Windows** | Windows-specific code | ~20 | Low |
| **InteropDLL** | C#/C++ interop layer | ~30 | Medium |
| **Lua** | Lua scripting integration | ~100 | Low |
| **Sdl** | SDL-based rendering/audio | ~20 | Low |
| **SevenZip** | 7-Zip compression library | ~200 | Low (external) |

### Core Subdirectories

```text
Core/
├── Shared/		 # Shared emulation infrastructure
├── Debugger/	   # Debugging support
├── NES/			# NES emulation
├── SNES/		   # SNES emulation
├── Gameboy/		# Game Boy emulation
├── GBA/			# Game Boy Advance emulation
├── SMS/			# Sega Master System emulation
├── PCE/			# PC Engine emulation
├── WS/			 # WonderSwan emulation
└── Netplay/		# Network play support
```

## 🗺️ Phases

### Phase 1: Assessment & Planning (1-2 weeks)

**Objective:** Analyze current codebase and create detailed modernization plan

#### Tasks

- [ ] Analyze current C++ standard usage across all projects
- [ ] Identify deprecated patterns and APIs
- [ ] Catalog compiler warnings (enable /W4 on MSVC, -Wall -Wextra on GCC/Clang)
- [ ] Identify memory management patterns (raw pointers vs smart pointers)
- [ ] Document existing code patterns and conventions
- [ ] Create detailed task breakdown for each module

#### Deliverables

- Code analysis report
- Prioritized modernization backlog
- Estimated effort per module

### Phase 2: Build System & Tooling Update (1 week)

**Objective:** Modernize build configuration and enable modern tooling

#### Tasks

- [ ] Update Visual Studio project files to VS2026 (v143 → v144 platform toolset)
- [ ] Enable C++23 standard (`/std:c++23` or `/std:c++latest`)
- [ ] Configure clang-tidy integration
- [ ] Enable AddressSanitizer (ASan) for debug builds
- [ ] Enable UndefinedBehaviorSanitizer (UBSan) for debug builds
- [ ] Configure code coverage tools
- [ ] Update CMake/makefile for Linux builds

#### .vcxproj Updates

```xml
<!-- Update platform toolset -->
<PlatformToolset>v144</PlatformToolset>

<!-- Enable C++23 -->
<LanguageStandard>stdcpp23</LanguageStandard>

<!-- Enable modern warnings -->
<WarningLevel>Level4</WarningLevel>
<TreatWarningAsError>true</TreatWarningAsError>
```

### Phase 3: Testing Framework Setup (1-2 weeks)

**Objective:** Establish unit testing infrastructure

#### Tasks

- [ ] Integrate Google Test (gtest) or Catch2 framework
- [ ] Create test project structure
- [ ] Write initial tests for critical components:
	- [ ] CPU instruction tests (6502, 65816, Z80, ARM7)
	- [ ] Memory mapping tests
	- [ ] PPU timing tests
	- [ ] APU tests
- [ ] Set up CI integration for tests

#### Test Categories

| Category | Description | Priority |
| ---------- | ------------- | ---------- |
| CPU Instructions | Verify each CPU opcode | High |
| Memory Mapping | Bank switching, mirroring | High |
| PPU Timing | Scanline timing accuracy | High |
| APU | Audio generation | Medium |
| Mappers | NES/SNES cartridge mappers | High |
| Save States | State save/restore integrity | Medium |

### Phase 4: Memory Safety Modernization (2-3 weeks)

**Objective:** Replace raw pointers with smart pointers and modern containers

#### Tasks

- [ ] Replace `new`/`delete` with `std::make_unique`/`std::make_shared`
- [ ] Convert raw arrays to `std::array`, `std::vector`, or `std::span`
- [ ] Use `std::string_view` for non-owning string references
- [ ] Replace C-style casts with C++ casts
- [ ] Add `[[nodiscard]]` attributes where appropriate
- [ ] Use `constexpr` and `consteval` where possible

#### Example Modernizations

```cpp
// Before
uint8_t* buffer = new uint8_t[size];
// ...
delete[] buffer;

// After
auto buffer = std::make_unique<uint8_t[]>(size);
// Automatic cleanup

// Before
void ProcessData(uint8_t* data, size_t size);

// After
void ProcessData(std::span<const uint8_t> data);
```

### Phase 5: Standard Library Modernization (2 weeks)

**Objective:** Leverage modern C++ standard library features

#### Tasks

- [ ] Replace manual loops with `<algorithm>` and `<ranges>`
- [ ] Use `std::format` instead of sprintf/printf
- [ ] Use `std::filesystem` for file operations
- [ ] Use `std::optional` for optional values
- [ ] Use `std::variant` for type-safe unions
- [ ] Use structured bindings where appropriate
- [ ] Apply `[[likely]]` and `[[unlikely]]` attributes

#### Example Modernizations

```cpp
// Before
for (int i = 0; i < count; i++) {
	if (data[i] == target) return i;
}

// After
auto it = std::ranges::find(data, target);
return it != data.end() ? std::distance(data.begin(), it) : -1;

// Before
char buffer[256];
sprintf(buffer, "Value: %d", value);

// After
auto result = std::format("Value: {}", value);
```

### Phase 6: Performance Optimization (2-3 weeks)

**Objective:** Profile and optimize critical emulation paths

#### Tasks

- [ ] Profile emulation hot paths using VTune, perf, or Tracy
- [ ] Optimize memory access patterns for cache efficiency
- [ ] Consider SIMD optimizations using `<simd>` or intrinsics
- [ ] Evaluate and optimize branch prediction
- [ ] Create benchmarks for critical components
- [ ] Document performance characteristics

#### Benchmark Targets

| Component | Metric | Target |
| ----------- | -------- | -------- |
| CPU Core | Instructions/sec | Baseline + 10% |
| PPU Render | Frames/sec | Maintain 60 FPS |
| Audio Mix | Latency | < 10ms |
| State Save | Time | < 100ms |

### Phase 7: Documentation & Code Quality (Ongoing)

**Objective:** Comprehensive documentation and consistent code style

#### Tasks

- [ ] Add Doxygen comments to all public APIs
- [ ] Document emulation accuracy notes
- [ ] Create architecture documentation
- [ ] Apply consistent code formatting (clang-format)
- [ ] Remove dead code and unused files
- [ ] Consolidate duplicate code

#### Documentation Requirements

- Every public class/function needs documentation
- Complex algorithms need inline comments
- Platform-specific quirks need documentation
- Performance-critical code needs optimization notes

## 📋 GitHub Issues Structure

### Epic Issues

1. **C++ Build System Modernization** (Epic)
   - VS2026 platform toolset update
   - C++23 standard enablement
   - Clang-tidy integration
   - Sanitizer configuration

2. **C++ Testing Infrastructure** (Epic)
   - Test framework integration
   - CPU instruction tests
   - PPU tests
   - Integration tests

3. **C++ Memory Safety** (Epic)
   - Smart pointer migration
   - Container modernization
   - Span adoption

4. **C++ Standard Library Modernization** (Epic)
   - Algorithm/ranges adoption
   - std::format migration
   - filesystem migration

5. **C++ Performance Optimization** (Epic)
   - Profiling infrastructure
   - Hot path optimization
   - SIMD opportunities

6. **C++ Documentation** (Epic)
   - Doxygen setup
   - API documentation
   - Architecture docs

## 🔍 Code Examples by Module

### NES CPU (Core/NES/)

Current patterns to modernize:

- Raw pointer memory access
- Manual instruction dispatch tables
- C-style type punning

### SNES CPU (Core/SNES/)

Current patterns to modernize:

- Complex memory mapping with raw pointers
- Bank switching logic
- DMA handling

### Shared Components (Core/Shared/)

Current patterns to modernize:

- Base emulator interface
- Memory management
- Timing infrastructure

## 📈 Success Metrics

| Metric | Target |
| -------- | -------- |
| Compiler Warnings | 0 at /W4 |
| Test Coverage | > 70% for core |
| Memory Leaks | 0 (ASan clean) |
| UB Detected | 0 (UBSan clean) |
| Documentation | 100% public APIs |
| Performance | ≥ baseline |

## 🚨 Risks & Mitigations

| Risk | Mitigation |
| ------ | ------------ |
| Performance regression | Continuous benchmarking, profile-guided optimization |
| Breaking changes | Incremental changes with extensive testing |
| Platform compatibility | Test on all platforms (Windows, Linux, macOS) |
| Large scope | Prioritize high-impact changes, iterative approach |

## 📅 Timeline Estimate

| Phase | Duration | Dependencies |
| ------- | ---------- | -------------- |
| Phase 1: Assessment | 1-2 weeks | None |
| Phase 2: Build System | 1 week | Phase 1 |
| Phase 3: Testing | 1-2 weeks | Phase 2 |
| Phase 4: Memory Safety | 2-3 weeks | Phase 3 |
| Phase 5: Standard Library | 2 weeks | Phase 4 |
| Phase 6: Performance | 2-3 weeks | Phase 5 |
| Phase 7: Documentation | Ongoing | All phases |

**Total Estimate:** 10-14 weeks for full modernization

## 📚 References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Clang-Tidy Checks](https://clang.llvm.org/extra/clang-tidy/checks/list.html)
- [Google Test](https://google.github.io/googletest/)
- [Catch2](https://github.com/catchorg/Catch2)
- [Tracy Profiler](https://github.com/wolfpld/tracy)
