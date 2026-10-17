# AddressSanitizer (ASan) Configuration Guide

This guide explains how to use AddressSanitizer for detecting memory errors in Nexen.

## Overview

AddressSanitizer (ASan) is a fast memory error detector that catches:

- Buffer overflows (stack, heap, global)
- Use-after-free
- Use-after-return
- Double-free
- Memory leaks

## Enabling ASan

### Command Line Build

Build with ASan enabled using the `EnableASAN` property:

```powershell
# Build Debug with ASan
msbuild Core.Tests\Core.Tests.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:EnableASAN=true /m:1

# Build Core with ASan (for debugging emulation)
msbuild Core\Core.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:EnableASAN=true /m:1
```

### Visual Studio

1. Open project properties
2. Configuration: **Debug**
3. C/C++ → General → Enable Address Sanitizer: **Yes (/fsanitize=address)**

Or in the `.vcxproj`:

```xml
<EnableASAN>true</EnableASAN>
```

## Requirements

### Runtime Library

ASan requires the DLL runtime library:

- Debug: `MultiThreadedDebugDLL` (/MDd)
- Release: `MultiThreadedDLL` (/MD)

The test project is configured for this.

### Visual Studio Version

- Visual Studio 2019 16.4+ or VS 2022+
- Platform Toolset v142 or v143+

## Running with ASan

When ASan detects an error, it prints a detailed report:

```text
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x...
READ of size 4 at 0x... thread T0
	#0 0x... in function_name source.cpp:123
	#1 0x... in main main.cpp:45
	...
```

### Example Test Run

```powershell
# Build with ASan
msbuild Core.Tests\Core.Tests.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:EnableASAN=true /m:1

# Run tests
bin\win-x64\Debug\Core.Tests.exe
```

## Common Issues

### Crash on startup

If the app crashes immediately with ASan:

1. Ensure all dependencies are also built with ASan
2. Check for static initialization order issues

### False positives

Some patterns may trigger false positives:

- Custom allocators (may need annotations)
- Memory-mapped I/O regions

### Performance

ASan adds ~2x slowdown and ~2x memory overhead. Use for debugging, not production.

## Integration with CI

The CI pipeline can optionally run ASan tests:

```yaml
- name: Build with ASan
  run: |
	msbuild Core.Tests\Core.Tests.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:EnableASAN=true /m:1

- name: Run ASan Tests
  run: |
	bin\win-x64\Debug\Core.Tests.exe --gtest_output=xml:asan-results.xml
```

## Memory Errors Caught

| Error Type | Description |
| ------------ | ------------- |
| heap-buffer-overflow | Out-of-bounds heap access |
| stack-buffer-overflow | Out-of-bounds stack access |
| global-buffer-overflow | Out-of-bounds global access |
| heap-use-after-free | Access freed heap memory |
| stack-use-after-return | Access returned stack memory |
| double-free | Freeing already freed memory |
| alloc-dealloc-mismatch | new/delete vs malloc/free mismatch |

## Best Practices

1. **Run ASan in CI** - Catch issues early
2. **Fix all warnings** - ASan finds real bugs
3. **Test edge cases** - Boundary conditions often have bugs
4. **Profile separately** - Don't mix ASan with performance tests

## Related Documentation

- [Microsoft ASan Documentation](https://docs.microsoft.com/en-us/cpp/sanitizers/asan)
- [Testing Documentation](testing/)
- [CPP Development Guide](CPP-DEVELOPMENT-GUIDE.md)
