# Compiling Nexen

Nexen uses modern C++23 and .NET 10 for improved performance and code quality.

## Requirements

| Component | Version | Notes |
| ----------- | --------- | ------- |
| C++ Standard | C++23 | `/std:c++latest` on MSVC |
| .NET SDK | 10.0+ | [Download](https://dotnet.microsoft.com/download) |
| Platform Toolset | v145 | Visual Studio 2026 |
| SDL2 | Latest | Required for Linux/macOS |

## Windows

### Requirements

- **Compiler:** MSVC (Microsoft Visual C++) from Visual Studio 2026
- **IDE:** Visual Studio 2026 or later with C++ Desktop workload
- **Platform Toolset:** v145

### Building with Visual Studio

1. Open `Nexen.sln` in Visual Studio 2026 (or later)
2. Compile as `Release` / `x64`
3. Set the startup project to `UI` and run

**Note:** Nexen requires Visual Studio 2026 with platform toolset v145 for full C++23 support.

### Building from Command Line

```powershell
# Using MSBuild
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" `
    Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m:1

# Run tests
.\bin\win-x64\Release\Core.Tests.exe
```

## Linux

To build under Linux you need a version of Clang or GCC that supports C++23.

Additionally, SDL2 and the [.NET 10 SDK](https://learn.microsoft.com/en-us/dotnet/core/install/linux) must also be installed.

Once SDL2 and the .NET 10 SDK are installed, run `make` to compile with Clang.
To compile with GCC instead, use `USE_GCC=true make`.

**Note:** Nexen usually runs faster when built with Clang instead of GCC.

## macOS

To build on macOS, install SDL2 (i.e via Homebrew) and the [.NET 10 SDK](https://dotnet.microsoft.com/download).

Once SDL2 and the .NET 10 SDK are installed, run `make`.

## Key Features

Nexen includes:

- C++23 modernization (smart pointers, std::format, ranges)
- 🌼 Pansy metadata export for disassembly
- TAS Editor with piano roll and greenzone
- Infinite save states with visual picker
- Comprehensive unit tests (Google Test)
- Doxygen API documentation

See [FORMATTING.md](FORMATTING.md) for code style guidelines.
