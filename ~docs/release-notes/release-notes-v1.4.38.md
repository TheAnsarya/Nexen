# Nexen v1.4.38 Testing Release - Do Not Use In Production

> ⚠️ Warning: This is a bad release for testing and validation only. Do not use this version as a daily build. Please watch for v1.5.0 for the recommended forward release.

Nexen v1.4.38 is a stabilization checkpoint focused on cross-platform dialog ownership safety, TAS input preview correctness, and release-gate determinism. This release is intentionally published as a testing milestone so teams can validate behavior, packaging, and CI artifact integrity before the v1.5.0 rollout.

## 🌟 Release Title

Nexen v1.4.38 - Test Flight Stabilization Pass

## 🧭 Scope Of This Build

| Area | Outcome |
|---|---|
| 🪟 Dialog ownership safety | Hardened parent window resolution for cross-platform dialogs and file pickers |
| 🧩 UI callsite cleanup | Removed risky parent usage patterns in settings/firmware/palette workflows |
| 🎮 TAS correctness | Fixed stale button-layout reuse during preview updates |
| ✅ Release gates | Repeated release build and test validation passes with deterministic outcomes |
| 📦 Artifact readiness | Release workflow prepared to emit multi-platform binaries for tag v1.4.38 |

## 🔧 Fix Highlights

### Linux/AppImage settings reset hardening

- Fixed reset confirmation ownership to avoid invalid parent window failures during settings reset flows.
- Centralized owner resolution fallback so dialogs remain valid even when controls are detached or re-parented.

### Full parent-resolution sweep in UI surfaces

- Hardened shared message-box and file-dialog helper behavior.
- Updated control callsites to provide safe owners instead of fragile visual roots.
- Reduced platform-specific dialog regressions in firmware and palette configuration paths.

### TAS input preview determinism and correctness

- Corrected in-place update logic so layout identity is validated before button-state reuse.
- Prevented stale SNES button IDs from persisting when switching to Genesis layouts with equal counts.

## 🧪 Validation Summary

- Release build gate passed for Release x64.
- C++ gate passed: Core.Tests.exe --gtest_brief=1 => 3706/3706.
- .NET gate passed: dotnet test Tests/Nexen.Tests.csproj -c Release --nologo => 1716/1716.

## 📦 Expected Release Assets (v1.4.38)

- Nexen-Windows-x64-v1.4.38.exe
- Nexen-Windows-x64-AoT-v1.4.38.exe
- Nexen-Linux-x64-v1.4.38.AppImage
- Nexen-Linux-ARM64-v1.4.38.AppImage
- Nexen-Linux-x64-v1.4.38.tar.gz
- Nexen-Linux-x64-gcc-v1.4.38.tar.gz
- Nexen-Linux-ARM64-v1.4.38.tar.gz
- Nexen-Linux-ARM64-gcc-v1.4.38.tar.gz
- Nexen-Linux-x64-AoT-v1.4.38.tar.gz
- Nexen-macOS-ARM64-v1.4.38.zip

## 🔗 Links

- Release tag: https://github.com/TheAnsarya/Nexen/releases/tag/v1.4.38
- Compare: https://github.com/TheAnsarya/Nexen/compare/v1.4.37...v1.4.38
