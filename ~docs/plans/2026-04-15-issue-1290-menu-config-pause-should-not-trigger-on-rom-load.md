# Issue #1290 Menu/Config Pause Should Not Trigger On ROM Load

## Context

Issue: [#1290](https://github.com/TheAnsarya/Nexen/issues/1290)

When `Pause when in menu and config dialogs` is enabled, loading a ROM can leave emulation paused immediately at startup. The expected behavior is to pause only while an actively running game is in a menu/config context, not during game load transitions.

## Root Cause

File: `UI/Windows/MainWindow.axaml.cs`

In `timerUpdateBackgroundFlag(...)`, pause evaluation for menu/config windows did not require an active game state. During ROM load or transition windows, this could still set `needPause`, call `EmuApi.Pause()`, and persist into newly loaded gameplay.

## Goals

- Restrict menu/config auto-pause to active gameplay only.
- Preserve existing behavior while a game is running.
- Add regression tests for this decision logic.

## Implemented Fix

File: `UI/Windows/MainWindow.axaml.cs`

1. Added active game-state guard in pause evaluation:

- Captured `isGameRunning = EmuApi.IsRunning()` in `timerUpdateBackgroundFlag(...)`.
- Replaced inline menu/config pause conditions with helper:
	- `ShouldPauseInMenusAndConfig(...)`

2. Added testable helper:

- `internal static bool ShouldPauseInMenusAndConfig(bool pauseWhenInMenusAndConfig, bool isGameRunning, bool isConfigWindow, bool isMainMenuOpen)`
- Returns `true` only when:
	- option is enabled,
	- a game is running,
	- and either config dialog is open or main menu is open.

## Tests Added

File: `Tests/Windows/MainWindowPauseBehaviorTests.cs`

Coverage:

- Disabled option returns false.
- No active game + menu open returns false.
- No active game + config window open returns false.
- Active game + menu open returns true.
- Active game + config window open returns true.

## Validation

Before change:

- `Core.Tests.exe --gtest_brief=1`: pass (3690 tests, 0 failed)
- `dotnet test Tests/Nexen.Tests.csproj --no-build -c Release --nologo -v m`: pass (1434 tests, 0 failed)

After change:

- `Core.Tests.exe --gtest_brief=1`: pass (3690 tests, 0 failed)
- `dotnet test Tests/Nexen.Tests.csproj -c Release --nologo -v m`: pass (1439 tests, 0 failed)

## Scope Notes

- Fix is intentionally narrow to menu/config pause logic.
- Background pause behavior (`PauseWhenInBackground`) is unchanged.
