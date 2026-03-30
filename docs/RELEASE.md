# Nexen Release Guide

This document describes how to prepare, build, and publish releases of Nexen for all supported platforms.

## Supported Platforms

| Platform | Architecture | Format | Notes |
| ---------- | -------------- | -------- | ------- |
| Windows | x64 | Portable EXE | Single-file, AoT available |
| Linux | x64, ARM64 | AppImage, Binary | Static linking |
| macOS | ARM64 (Apple Silicon) | .app bundle | Code-signed |

## Build Configurations

### Windows Builds

| Configuration | Framework | Type | Use Case |
| --------------- | ----------- | ------ | ---------- |
| Standard | .NET 10 | Single-file | Most users |
| AoT | .NET 10 | Native AOT | Faster startup |

### Linux Builds

| Configuration | Compiler | Type | Notes |
| --------------- | ---------- | ------ | ------- |
| Clang | LLVM/Clang | Binary | Recommended (faster) |
| GCC | GNU GCC | Binary | Alternative |
| AppImage | Clang | Portable | Self-contained |

### macOS Builds

| Configuration | Architecture | Type | Notes |
| --------------- | -------------- | ------ | ------- |
| Apple Silicon | ARM64 | .app | macOS 14+ |
| ~~AoT~~ | ~~ARM64~~ | ~~.app~~ | Disabled (.NET 10 ILC bug) |

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

All builds are automated via GitHub Actions using `workflow_dispatch`. The workflow:

1. **Manual dispatch** → Triggers `build.yml` with `run_release` and `release_tag` inputs
2. **Build artifacts** → All platform binaries uploaded as CI artifacts
3. **Release job** → Downloads all artifacts, renames with version, uploads to GitHub Release

### Creating a Release

1. **Tag the commit:**

   ```bash
   git tag -a v1.0.0 -m "Nexen 1.0.0"
   git push origin v1.0.0
   ```

2. **Dispatch the build workflow** with release inputs:

   ```powershell
   gh workflow run build.yml --ref <branch-or-tag> -f run_release=true -f release_tag=vX.Y.Z
   ```

3. **CI automatically:**
   - Builds all platform binaries (Windows, Linux, macOS)
   - Creates tarballs for Linux binaries
   - Uploads all assets to the GitHub Release via `softprops/action-gh-release@v2`
   - Uses `overwrite_files: true` to update existing releases

### Artifact Naming Convention

| Platform | CI Artifact Name | Release Asset Name |
| ---------- | --------------- | --------------- |
| Windows (standard) | `Nexen (Windows)` | `Nexen-Windows-x64-vX.Y.Z.exe` |
| Windows (AoT) | `Nexen (Windows - AoT)` | `Nexen-Windows-x64-AoT-vX.Y.Z.exe` |
| Linux x64 (Clang) | `Nexen (Linux - ubuntu-22.04 - clang)` | `Nexen-Linux-x64-vX.Y.Z.tar.gz` |
| Linux x64 (GCC) | `Nexen (Linux - ubuntu-22.04 - gcc)` | `Nexen-Linux-x64-gcc-vX.Y.Z.tar.gz` |
| Linux ARM64 (Clang) | `Nexen (Linux - ubuntu-22.04-arm - clang)` | `Nexen-Linux-ARM64-vX.Y.Z.tar.gz` |
| Linux ARM64 (GCC) | `Nexen (Linux - ubuntu-22.04-arm - gcc)` | `Nexen-Linux-ARM64-gcc-vX.Y.Z.tar.gz` |
| Linux AoT x64 | `Nexen (Linux - ubuntu-22.04 - clang_aot)` | `Nexen-Linux-x64-AoT-vX.Y.Z.tar.gz` |
| Linux AppImage x64 | `Nexen (Linux x64 - AppImage)` | `Nexen-Linux-x64-vX.Y.Z.AppImage` |
| Linux AppImage ARM64 | `Nexen (Linux ARM64 - AppImage)` | `Nexen-Linux-ARM64-vX.Y.Z.AppImage` |
| macOS Apple Silicon | `Nexen (macOS - macos-14 - clang)` | `Nexen-macOS-ARM64-vX.Y.Z.zip` |

## Platform-Specific Notes

### Windows

- **Runtime:** .NET 10+ required unless using AoT build
- **Single-file:** Native DLLs bundled via `IncludeNativeLibrariesForSelfExtract`
- **AoT:** Faster startup, no .NET runtime needed

### Linux

- **Dependencies:** SDL2 (`libsdl2-2.0-0`)
- **AppImage:** Self-contained, no dependencies
- **Static linking:** Most dependencies bundled

### macOS

- **Code signing:** Required for Gatekeeper
- **Entitlements:** JIT compilation, network access
- **Notarization:** Recommended for distribution
- **Apple Silicon only:** Intel (x64) builds are no longer provided
- **AoT disabled:** Due to .NET 10 ILC compiler crash (see [#238](https://github.com/TheAnsarya/Nexen/issues/238))

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
| Windows | `Nexen-Windows-x64-vX.Y.Z.exe` |
| Windows (AoT) | `Nexen-Windows-x64-AoT-vX.Y.Z.exe` |
| Linux (AppImage x64) | `Nexen-Linux-x64-vX.Y.Z.AppImage` |
| Linux (AppImage ARM64) | `Nexen-Linux-ARM64-vX.Y.Z.AppImage` |
| macOS (Apple Silicon) | `Nexen-macOS-ARM64-vX.Y.Z.zip` |
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
