# Nexen v1.4.38 - Test Flight Stabilization Pass

> ⚠️ Warning: This is a bad release for testing and validation only. Do not use this version as a daily build. Please watch for v1.5.0 for the recommended forward release.

Nexen v1.4.38 is a focused stabilization build intended to verify cross-platform safety, deterministic release gating, and packaging integrity before the next feature-forward cycle.

## 🚨 Important Notice

- This release is intentionally labeled as test-only.
- Expect rough edges and ongoing refinement.
- Please target v1.5.0 for broader day-to-day usage.

## ✨ Highlights

| Area | Details |
|---|---|
| 🪟 Dialog Safety | Parent-window resolution hardened across settings, message-box, and picker surfaces |
| 🧭 Cross-Platform UX | Linux/AppImage reset-flow invalid-parent failures fixed |
| 🎮 TAS Accuracy | Input preview now correctly rebuilds when layout identity changes |
| 🧪 Gate Reliability | Native and managed release gates re-run and verified on current master |
| 📦 Release Pipeline | Multi-platform artifact workflow configured for release attachment |

## 🔧 Detailed Changes

### 🪟 Dialog parent ownership hardening

- Reworked ownership resolution for dialog and file-picker paths so controls resolve to a valid window parent.
- Removed fragile invalid-parent behavior from helper wrappers.
- Updated related UI callsites in config/firmware/palette paths to use safe ownership sources.

### 🎮 TAS input preview stale-layout fix

- Corrected preview update behavior so button IDs are not reused across incompatible layouts.
- Prevents stale SNES input labels from leaking into Genesis preview views when button counts match.

### 🧹 Warning stabilization pass

- Addressed additional low-risk warning locations in utility/vendor boundary code paths.
- Left high-risk numeric conversion warnings in emulator-sensitive third-party code untouched to preserve behavior.

## ✅ Validation Results

- Release build passed (Nexen.sln, Release x64).
- C++ gate passed: Core.Tests.exe --gtest_brief=1 => 3706/3706.
- .NET gate passed: dotnet test Tests/Nexen.Tests.csproj -c Release --nologo => 1716/1716.

## 📦 Expected Assets For v1.4.38

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

## 🔗 Project Links

- Changelog: https://github.com/TheAnsarya/Nexen/blob/master/CHANGELOG.md
- Compare: https://github.com/TheAnsarya/Nexen/compare/v1.4.37...v1.4.38
