# C++ Development Guide

This document covers C++ development practices for Nexen.

## Build System

### Platform Toolset

- **Windows:** Visual Studio 2026 (v145)
- **Linux:** GCC/Clang with C++23 support

### Language Standard

- C++23 (`/std:c++23` on Windows, `-std=c++23` on Linux)

### Warning Level

- Windows: `/W4` (Level 4)
- Linux: `-Wall -Wextra -Wpedantic`

## Code Quality Tools

### clang-tidy

Static analysis tool for C++ code. Configuration in `.clang-tidy`.

#### Running clang-tidy

**Single file:**

```bash
clang-tidy --config-file=.clang-tidy Core/MyFile.cpp
```

**All files in a directory:**

```bash
# Generate compile_commands.json first (CMake or Bear)
clang-tidy -p build Core/*.cpp
```

**Fix issues automatically:**

```bash
clang-tidy --config-file=.clang-tidy --fix Core/MyFile.cpp
```

#### Enabled Checks

| Category | Description |
| ---------- | ------------- |
| `modernize-*` | C++11/14/17/20/23 modernization |
| `cppcoreguidelines-*` | C++ Core Guidelines |
| `performance-*` | Performance improvements |
| `readability-*` | Code readability |
| `bugprone-*` | Bug-prone patterns |
| `clang-analyzer-*` | Static analysis |

#### Suppressing Warnings

**In code:**

```cpp
// NOLINTNEXTLINE(modernize-use-nullptr)
void* ptr = 0;

void* ptr2 = 0;  // NOLINT(modernize-use-nullptr)
```

**In .clang-tidy:**
Add to disabled checks list (prefix with `-`).

### clang-format

Code formatting tool. Configuration in `.clang-format`.

**Format file:**

```bash
clang-format -i Core/MyFile.cpp
```

**Format all files:**

```bash
find Core Utilities Windows -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

**Check formatting (CI):**

```bash
clang-format --dry-run --Werror Core/MyFile.cpp
```

## C++ Style Guide

### Naming Conventions

| Element | Style | Example |
| --------- | ------- | --------- |
| Class | PascalCase | `NesCpu` |
| Function | PascalCase | `ExecuteInstruction` |
| Variable | camelCase | `frameCount` |
| Private member | _prefix | `_registers` |
| Constant | UPPER_CASE | `MAX_SPRITES` |
| Enum constant | UPPER_CASE | `READ_ONLY` |

### Modern C++ Practices

#### Smart Pointers

```cpp
// Prefer unique_ptr for ownership
std::unique_ptr<Console> _console = std::make_unique<Console>();

// shared_ptr when ownership is truly shared
std::shared_ptr<IVideoDevice> _video;
```

#### std::span for Buffers

```cpp
// Instead of pointer + size
void ProcessData(std::span<const uint8_t> data) {
	for (auto byte : data) { /* ... */ }
}
```

#### std::optional for Optional Values

```cpp
std::optional<uint32_t> FindAddress(const std::string& label) {
	auto it = _symbols.find(label);
	if (it != _symbols.end()) {
		return it->second;
	}
	return std::nullopt;
}
```

#### std::format for Formatting

```cpp
// Instead of sprintf
std::string msg = std::format("Address: ${:04x}", address);
```

#### Range-based algorithms

```cpp
// Instead of raw loops
auto count = std::ranges::count_if(sprites, [](auto& s) { 
	return s.visible; 
});
```

## Testing

See [CPP-MODERNIZATION-ROADMAP.md](modernization/CPP-MODERNIZATION-ROADMAP.md) for testing infrastructure plans.

## Performance

### Profiling

- **Windows:** Visual Studio Profiler, VTune
- **Linux:** perf, Valgrind

### AddressSanitizer

Enable for debug builds to catch memory errors:

**Windows (VS2022+):**

```xml
<EnableASAN>true</EnableASAN>
```

**Linux:**

```bash
-fsanitize=address -fno-omit-frame-pointer
```

## Related Documentation

- [CPP-MODERNIZATION-ROADMAP.md](modernization/CPP-MODERNIZATION-ROADMAP.md) - Full modernization plan
- [CPP-ISSUES-TRACKING.md](modernization/CPP-ISSUES-TRACKING.md) - GitHub issue tracking
