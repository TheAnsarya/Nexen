# Nexen v1.4.38 Stable Release: Dialog Parent Hardening and TAS Preview Fixes

Nexen v1.4.38 focuses on release stability and cross-platform dialog safety, with a targeted Linux/AppImage reset-flow fix, a full dialog-parent hardening sweep, and a TAS input-preview correctness fix discovered during release validation.

## What Changed In v1.4.38

| Area | Summary |
|---|---|
| Settings and dialogs | Fixed reset-flow invalid parent ownership and hardened parent resolution for dialogs/file pickers |
| UI stability | Updated firmware/palette/config dialog call sites to pass valid owners |
| TAS editor correctness | Fixed stale SNES button reuse in Genesis input preview when layouts share button counts |
| Release readiness | Completed release build + C++ + .NET validation gates |

## Fix Highlights

### Linux/AppImage reset dialog stability

- Fixed settings reset confirmation ownership to prevent `Invalid parent window` failures in Linux/AppImage flows.
- Added robust owner resolution fallback in shared UI utilities to avoid invalid dialog/file picker parents.

### Full dialog-parent hardening sweep

- Centralized parent resolution in UI helper utilities.
- Removed invalid-parent throw behavior from message-box and file-dialog wrappers.
- Updated firmware and palette configuration surfaces to use safe parent references.

### TAS input preview correctness

- Fixed in-place preview update logic to rebuild when layout identity changes, even if button counts match.
- Prevents stale SNES button IDs from appearing in Genesis preview grids.

## Validation Summary

- Release build: `Build Nexen Release x64` passed.
- C++ tests: `Core.Tests.exe --gtest_brief=1` passed (`3706/3706`).
- .NET tests: `dotnet test Tests/Nexen.Tests.csproj -c Release --nologo` passed (`1716/1716`).

## Release Assets

Assets for tag v1.4.38 are expected on the release page:

- `Nexen-Windows-x64-v1.4.38.exe`
- `Nexen-Windows-x64-AoT-v1.4.38.exe`
- `Nexen-Linux-x64-v1.4.38.AppImage`
- `Nexen-Linux-ARM64-v1.4.38.AppImage`
- `Nexen-Linux-x64-v1.4.38.tar.gz`
- `Nexen-Linux-x64-gcc-v1.4.38.tar.gz`
- `Nexen-Linux-ARM64-v1.4.38.tar.gz`
- `Nexen-Linux-ARM64-gcc-v1.4.38.tar.gz`
- `Nexen-Linux-x64-AoT-v1.4.38.tar.gz`
- `Nexen-macOS-ARM64-v1.4.38.zip`

## Links

- Release tag: https://github.com/TheAnsarya/Nexen/releases/tag/v1.4.38
- Compare: https://github.com/TheAnsarya/Nexen/compare/v1.4.37...v1.4.38
