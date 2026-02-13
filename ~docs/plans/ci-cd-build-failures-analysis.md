# CI/CD Build Failures Analysis

**Issue:** [#238 - Research CI/CD Build Failures for v1.1.0 Release](https://github.com/TheAnsarya/Nexen/issues/238)
**Date:** 2026-02-13
**Analysis Scope:** v1.1.0 and v1.1.1 release builds
**Status:** RESOLVED in v1.1.2

---

## Executive Summary

The automated release pipeline was **blocking on a single failing macOS AOT build**, causing **ZERO release assets** to be published despite 10 out of 11 build configurations passing. The release job depends on ALL jobs completing successfully before it runs, so one failure prevents any uploads.

**Resolution:** macOS AOT builds temporarily disabled in v1.1.2.

### Current State (Post-Fix)

| Component | Status | Details |
|-----------|--------|---------|
| Windows Builds | ✅ PASSING | Both Standard and AOT compile successfully |
| Linux Builds | ✅ PASSING | All 5 configurations (gcc, clang, AOT) pass |
| AppImage Builds | ✅ PASSING | Both x64 and ARM64 AppImages build |
| macOS Standard | ✅ PASSING | ARM64 clang build works |
| macOS AOT | ⏸️ DISABLED | Temporarily removed from build matrix |
| Release Job | ✅ RUNNING | No longer blocked |

---

## Detailed Build Status (Run 21988635938)

### Passing Builds (10/11)

| Build | Runner | Compiler | Status |
|-------|--------|----------|--------|
| Windows Standard | windows-latest | MSVC | ✅ Success |
| Windows AOT | windows-latest | MSVC | ✅ Success |
| Linux x64 | ubuntu-22.04 | gcc-13 | ✅ Success |
| Linux ARM64 | ubuntu-22.04-arm | gcc-13 | ✅ Success |
| Linux x64 | ubuntu-22.04 | clang-18 | ✅ Success |
| Linux ARM64 | ubuntu-22.04-arm | clang-18 | ✅ Success |
| Linux x64 AOT | ubuntu-22.04 | clang-18 | ✅ Success |
| AppImage x64 | ubuntu-22.04 | clang-18 | ✅ Success |
| AppImage ARM64 | ubuntu-22.04-arm | clang-18 | ✅ Success |
| macOS ARM64 | macos-14 | llvm/clang | ✅ Success |

### Failing Build (1/11)

| Build | Runner | Error | Impact |
|-------|--------|-------|--------|
| macOS ARM64 AOT | macos-14 | Segmentation fault in ILC | Blocks release |

---

## Blocking Issue #1: macOS AOT Compiler Crash

### Error Details

```text
/var/folders/.../ilc" @"obj/Release/native/Nexen.ilc.rsp"" exited with code 139
```

**Exit Code 139** = SIGSEGV (Segmentation Fault) in the .NET IL Compiler

### Root Cause Analysis

The .NET 10.0.3 AOT compiler (`Microsoft.DotNet.ILCompiler`) crashes during native code generation on macOS ARM64. This is a bug in the compiler itself, not in Nexen code.

**Contributing factors:**

- Large codebase with extensive reflection usage (ReactiveUI, Avalonia bindings)
- Many trim analysis warnings indicating reflection-heavy patterns
- The ILC must process all Avalonia XAML bindings, DataGrid extensions, etc.

### Previous macOS Issues (Resolved)

These issues were fixed in earlier commits but are documented for reference:

1. **libc++ ABI Mismatch** (Fixed in 93942a32)
	- Homebrew LLVM's libc++ headers had incompatible ABI tags
	- Fixed by passing `-L/opt/homebrew/opt/llvm/lib/c++ -Wl,-rpath,...` flags

2. **Code Signing Failure** (Fixed in 0bac6b09)
	- Build failed when `MACOS_CERTIFICATE` secret was not configured
	- Fixed by making signing optional

---

## Blocking Issue #2: Release Job Architecture

### Current Workflow Structure

```yaml
release:
  if: startsWith(github.ref, 'refs/tags/v')
  needs: [windows, linux, appimage, macos]  # <-- ALL must pass
```

The release job uses `needs: [windows, linux, appimage, macos]`, meaning:

- **ALL** platform builds must succeed
- **ANY** single failure prevents release
- **ZERO** artifacts are uploaded even if 10/11 builds pass

### Impact

When any macOS build fails:

1. Release step is skipped entirely
2. No artifacts are copied to `release-assets/`
3. `softprops/action-gh-release` never runs
4. GitHub Release has no downloadable files

---

## README Download Links Status

The README.md advertises download links that currently 404:

| Platform | Link | Status |
|----------|------|--------|
| Windows Standard | `.../Nexen-Windows-x64.exe` | ❌ 404 |
| Windows AOT | `.../Nexen-Windows-x64-AoT.exe` | ❌ 404 |
| Linux AppImage x64 | `.../Nexen-Linux-x64.AppImage` | ❌ 404 |
| Linux AppImage ARM64 | `.../Nexen-Linux-ARM64.AppImage` | ❌ 404 |
| Linux Binary x64 | `.../Nexen-Linux-x64` | ❌ 404 |
| Linux Binary ARM64 | `.../Nexen-Linux-ARM64` | ❌ 404 |
| macOS Standard | `.../Nexen-macOS-ARM64.zip` | ❌ 404 |
| macOS AOT | `.../Nexen-macOS-ARM64-AoT.zip` | ❌ 404 |

### Actually Available Assets

| Release | Assets | Source |
|---------|--------|--------|
| v1.1.1 | `Nexen-v1.1.1-Windows-x64.zip` | Manual upload |
| v1.1.0 | `Nexen-v1.1.0-Windows-x64.zip` | Manual upload |
| v1.0.1 | `Nexen-1.0.1-win-x64.zip` | Unknown |

---

## Applied Fix

### Fix: Disable macOS AOT Temporarily

The .NET 10 ILC bug was resolved by removing macOS AOT from the build matrix:

```yaml
macos:
  strategy:
    matrix:
      # NOTE: clang_aot temporarily disabled - .NET 10 ILC crashes on macOS ARM64
      # See issue #238 for details. Re-enable when .NET fixes ILC segfault.
      compiler: [clang]
```

This was implemented in commit for v1.1.2.

---

## Investigation Notes

### GitHub Actions Run History

All recent runs show the same pattern:

- 10 jobs pass
- macOS AOT fails with ILC segfault
- Release job skips

| Run ID | Date | Windows | Linux | AppImage | macOS | macOS AOT | Release |
|--------|------|---------|-------|----------|-------|-----------|---------|
| 21988635938 | 2026-02-13 | ✅ | ✅ | ✅ | ✅ | ❌ | ⏭️ Skip |
| 21961664292 | 2026-02-12 | ✅ | ✅ | ✅ | ✅ | ❌ | ⏭️ Skip |
| 21961575521 | 2026-02-12 | ✅ | ✅ | ✅ | ✅ | ❌ | ⏭️ Skip |

### .NET 10 ILC Issues

The ILC crash appears to be triggered by:

- Large amounts of AOT warnings (trim analysis IL2026, IL2070, etc.)
- Reflection-heavy patterns in ReactiveUI/Avalonia
- Anonymous types with JSON serialization

Relevant .NET issue to track:

- Search GitHub `dotnet/runtime` for "ILC segfault macOS ARM64"

---

## Future Work

### Re-enabling macOS AOT

When .NET releases an ILC fix:

1. Re-add `clang_aot` to the macOS build matrix
2. Uncomment the release asset copy line
3. Update README to show macOS AOT as available
4. Test with a pre-release tag first

---

## Files Modified

| File | Change |
|------|--------|
| `.github/workflows/build.yml` | Removed macOS AOT from build matrix |
| `README.md` | Updated macOS AOT row to show unavailable |
| `CHANGELOG.md` | Added v1.1.2 release notes |

---

## References

- [Issue #238](https://github.com/TheAnsarya/Nexen/issues/238)
- [GitHub Actions Run 21988635938](https://github.com/TheAnsarya/Nexen/actions/runs/21988635938)
- [Build workflow](.github/workflows/build.yml)
