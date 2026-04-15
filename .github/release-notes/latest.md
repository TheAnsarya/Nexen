Nexen v1.4.32 is a stability-focused release that addresses settings persistence reliability, startup configuration fallback behavior, and menu/config pause semantics during ROM load. It also improves OSD notification placement and extends regression coverage for these flows.

## Highlights

- **Settings reliability hardening** — prevents silent state loss when settings writes fail.
- **Read-only portable config fallback** — automatic writable-path fallback and migration when startup folder is not writable.
- **Pause behavior correctness** — menu/config pause no longer triggers unexpectedly during ROM load/startup transitions.
- **Notification UX refinement** — save/action notification boxes moved to bottom-left with margin-based positioning and clamping.
- **Validation depth increased** — new C++ and managed tests added for layout and pause/config decision logic.

## Fixed

- **Settings not persisted after relaunch in write-failure scenarios** (#1291)
	- Config save flow now preserves in-memory settings unless write succeeds.
	- Adds diagnostic logging for failed writes without corrupting runtime state.

- **Portable settings path unwritable edge case** (#1295)
	- Startup now detects unwritable portable config locations.
	- Automatically falls back to writable user documents path.
	- Migrates `settings.json` when safe and needed.

- **Pause in menu/config incorrectly affecting startup game load** (#1290)
	- Menu/config pause now requires an active running game.
	- ROM load transitions no longer enter unexpected paused state.

## Changed

- **Boxed save/action OSD placement** (#1289)
	- Repositioned to bottom-left with explicit left/bottom margins.
	- Added viewport clamping for small render sizes to avoid overflow.

- **Release metadata sync**
	- Version metadata and release-facing download references updated for v1.4.32.

## Added

- **C++ layout regression tests** (#1289)
	- New `SystemHudLayoutTests` coverage for bottom-left placement and clamping.

- **Managed pause behavior regression tests** (#1290)
	- New `MainWindowPauseBehaviorTests` verifies pause gating by running state and context.

- **Managed config fallback tests** (#1295)
	- Added/expanded writable-path fallback and migration coverage in `ConfigManagerHomeFolderTests`.

## Validation Summary

- Release x64 build: success
- C++ tests: 3690 passed, 0 failed
- Managed tests: 1439 passed, 0 failed
- Local warning scan (clean Release build log): no warning lines detected

## Downloads

### Windows

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-Windows-x64-v1.4.32.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.32/Nexen-Windows-x64-v1.4.32.exe) | Single-file, recommended |
| **Native AOT** | [Nexen-Windows-x64-AoT-v1.4.32.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.32/Nexen-Windows-x64-AoT-v1.4.32.exe) | Faster startup |

### Linux

| Build | Download | Notes |
|-------|----------|-------|
| **AppImage x64** | [Nexen-Linux-x64-v1.4.32.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.32/Nexen-Linux-x64-v1.4.32.AppImage) | Recommended |
| **AppImage ARM64** | [Nexen-Linux-ARM64-v1.4.32.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.32/Nexen-Linux-ARM64-v1.4.32.AppImage) | Raspberry Pi, etc. |
| Binary x64 (clang) | [Nexen-Linux-x64-v1.4.32.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.32/Nexen-Linux-x64-v1.4.32.tar.gz) | Tarball, requires SDL2 |
| Binary x64 (gcc) | [Nexen-Linux-x64-gcc-v1.4.32.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.32/Nexen-Linux-x64-gcc-v1.4.32.tar.gz) | Tarball, requires SDL2 |
| Binary ARM64 (clang) | [Nexen-Linux-ARM64-v1.4.32.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.32/Nexen-Linux-ARM64-v1.4.32.tar.gz) | Tarball, requires SDL2 |
| Binary ARM64 (gcc) | [Nexen-Linux-ARM64-gcc-v1.4.32.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.32/Nexen-Linux-ARM64-gcc-v1.4.32.tar.gz) | Tarball, requires SDL2 |
| Native AOT x64 | [Nexen-Linux-x64-AoT-v1.4.32.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.32/Nexen-Linux-x64-AoT-v1.4.32.tar.gz) | Faster startup |

### macOS (Apple Silicon)

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-macOS-ARM64-v1.4.32.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.32/Nexen-macOS-ARM64-v1.4.32.zip) | App bundle |
| ~~Native AOT~~ | *Temporarily unavailable* | .NET 10 ILC compiler bug |

> **Notes:**
> - Linux requires SDL2 (`sudo apt install libsdl2-2.0-0`)
> - macOS: Right-click -> Open on first launch to bypass Gatekeeper
