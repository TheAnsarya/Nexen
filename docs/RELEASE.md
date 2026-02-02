# Nexen Release Guide

This document describes how to prepare, build, and publish releases of Nexen for all supported platforms.

## Supported Platforms

| Platform | Architecture | Format | Notes |
| ---------- | -------------- | -------- | ------- |
| Windows | x64 | Portable EXE | Single-file, AoT available |
| Linux | x64, ARM64 | AppImage, Binary | Static linking |
| macOS | x64 (Intel), ARM64 (Apple Silicon) | .app bundle | Code-signed |

## Build Configurations

### Windows Builds

| Configuration | Framework | Type | Use Case |
| --------------- | ----------- | ------ | ---------- |
| Standard | .NET 8 | Single-file | Most users |
| AoT | .NET 8 | Native AOT | Maximum performance |
| Legacy | .NET 6 | Single-file | Compatibility |

### Linux Builds

| Configuration | Compiler | Type | Notes |
| --------------- | ---------- | ------ | ------- |
| Clang | LLVM/Clang | Binary | Recommended (faster) |
| GCC | GNU GCC | Binary | Alternative |
| AppImage | Clang | Portable | Self-contained |

### macOS Builds

| Configuration | Architecture | Type | Notes |
| --------------- | -------------- | ------ | ------- |
| Intel | x64 | .app | macOS 13+ |
| Apple Silicon | ARM64 | .app | macOS 14+ |
| AoT | Both | .app | Native compilation |

## Pre-Release Checklist

### Code Quality

- [ ] All unit tests passing (`bin\win-x64\Release\Core.Tests.exe`)
- [ ] No compiler warnings (Release build)
- [ ] No memory leaks (AddressSanitizer pass)
- [ ] Benchmarks show no regressions

### Documentation

- [ ] README.md up to date with all features
- [ ] CHANGELOG.md updated with new changes
- [ ] All user-facing docs complete
- [ ] Doxygen documentation generated

### Testing

- [ ] Manual testing on Windows
- [ ] Manual testing on Linux (if available)
- [ ] Manual testing on macOS (if available)
- [ ] All emulation cores tested
- [ ] TAS features tested
- [ ] Pansy export tested
- [ ] Save states tested

## Building Release Binaries

### Windows (Local Build)

```powershell
# Clean build
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" `
    Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Clean,Build /m

# Publish single-file
dotnet publish -c Release -r win-x64 --self-contained -p:PublishSingleFile=true `
    UI/UI.csproj -o publish/win-x64

# Run tests
.\bin\win-x64\Release\Core.Tests.exe
```

### Linux (Local Build)

```bash
# Clean build with Clang (recommended)
make clean
make -j$(nproc) LTO=true STATICLINK=true

# Or with GCC
USE_GCC=true make -j$(nproc) LTO=true STATICLINK=true

# AppImage
Linux/appimage/appimage.sh
```

### macOS (Local Build)

```bash
# Clean build
make clean
make -j$(sysctl -n hw.logicalcpu)

# Output: bin/osx-{arch}/Release/osx-{arch}/publish/Nexen.app
```

## GitHub Actions CI/CD

All builds are automated via GitHub Actions. The workflow:

1. **Push to branch** → Triggers `build.yml`
2. **Build artifacts** → Uploaded for each platform
3. **Release tag** → Triggers release workflow

### Creating a Release

1. **Tag the commit:**

   ```bash
   git tag -a v1.0.0 -m "Nexen 1.0.0"
   git push origin v1.0.0
   ```

2. **GitHub Actions builds** all platform binaries automatically

3. **Create GitHub Release:**
   - Go to Releases → Draft new release
   - Select the tag
   - Add release notes from CHANGELOG
   - Download artifacts from Actions and attach
   - Publish release

### Artifact Naming Convention

| Platform | Artifact Name |
| ---------- | --------------- |
| Windows (standard) | `Nexen (Windows - net8.0)` |
| Windows (AoT) | `Nexen (Windows - net8.0 - AoT)` |
| Linux x64 (Clang) | `Nexen (Linux - ubuntu-22.04 - clang)` |
| Linux ARM64 | `Nexen (Linux - ubuntu-22.04-arm - gcc)` |
| Linux AppImage x64 | `Nexen (Linux x64 - AppImage)` |
| Linux AppImage ARM64 | `Nexen (Linux ARM64 - AppImage)` |
| macOS Intel | `Nexen (macOS - macos-13 - clang)` |
| macOS Apple Silicon | `Nexen (macOS - macos-14 - clang)` |

## Platform-Specific Notes

### Windows

- **Runtime:** .NET 8+ required unless using AoT build
- **Single-file:** Extracts to temp folder on first run
- **AoT:** Larger binary but no .NET runtime needed

### Linux

- **Dependencies:** SDL2 (`libsdl2-2.0-0`)
- **AppImage:** Self-contained, no dependencies
- **Static linking:** Most dependencies bundled

### macOS

- **Code signing:** Required for Gatekeeper
- **Entitlements:** JIT compilation, network access
- **Notarization:** Recommended for distribution
- **Universal binary:** Not currently built (separate Intel/ARM)

## Version Numbering

Nexen uses semantic versioning: `MAJOR.MINOR.PATCH`

- **MAJOR:** Breaking changes or major new features
- **MINOR:** New features, backward compatible
- **PATCH:** Bug fixes, small improvements

## Release Notes Template

```markdown
# Nexen vX.Y.Z

Release date: YYYY-MM-DD

## Highlights

- Feature 1
- Feature 2

## New Features

- Detailed feature list

## Improvements

- Performance improvements
- UI improvements

## Bug Fixes

- Bug fix list

## Breaking Changes

- Any breaking changes

## Known Issues

- Any known issues

## Downloads

| Platform | Download |
| -------- | -------- |
| Windows | [Nexen-vX.Y.Z-win-x64.zip]() |
| Linux (AppImage) | [Nexen-vX.Y.Z-x86_64.AppImage]() |
| macOS (Intel) | [Nexen-vX.Y.Z-macos-x64.zip]() |
| macOS (Apple Silicon) | [Nexen-vX.Y.Z-macos-arm64.zip]() |
```

## Troubleshooting

### Windows

- **"VCRUNTIME140.dll not found":** Install Visual C++ Redistributable
- **Antivirus false positive:** Add exception for Nexen.exe

### Linux

- **"libSDL2.so.0 not found":** Install SDL2 (`sudo apt install libsdl2-2.0-0`)
- **AppImage won't run:** `chmod +x Nexen.AppImage` and install FUSE

### macOS

- **"App is damaged":** `xattr -cr Nexen.app` to clear quarantine
- **Gatekeeper blocks:** Open from right-click menu → Open
