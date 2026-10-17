# Issue #1289 Notification Box Bottom-Left Layout Plan

## Context

Issue: [#1289](https://github.com/TheAnsarya/Nexen/issues/1289)

Save-state and action notification boxes were rendered near the center of the viewport, which can obstruct gameplay-critical visuals. The request is to move those message boxes to the bottom-left with clear margins.

## Goals

- Move save-state notification box placement to bottom-left.
- Move action notification box placement to bottom-left.
- Use explicit left/bottom margins for consistency.
- Keep existing text rendering, fade behavior, and styling unchanged.
- Add deterministic tests for layout math and edge clamping.

## Implemented Design

Files:

- `Core/Shared/Video/SystemHud.h`
- `Core/Shared/Video/SystemHud.cpp`

1. Added fixed placement margins in `SystemHud`:

- `NotificationLeftMargin = 10`
- `NotificationBottomMargin = 14`

2. Added a shared helper:

- `GetBottomLeftBoxPosition(screenWidth, screenHeight, boxWidth, boxHeight, leftMargin, bottomMargin)`
- Returns an anchored `(x, y)` box position with viewport clamping to avoid overflow on small framebuffers.

3. Rewired box placement callsites:

- `DrawSaveStateNotification(...)` now uses `GetBottomLeftBoxPosition(...)`.
- `DrawActionNotification(...)` now uses `GetBottomLeftBoxPosition(...)`.

4. Preserved non-layout behavior:

- Fade timing, text content, dimensions, and draw order are unchanged.

## Tests Added

File: `Core.Tests/Shared/SystemHudLayoutTests.cpp`

Coverage:

- Bottom-left position uses configured margins.
- X coordinate clamps when box width would overflow viewport width.
- Y coordinate clamps when box height would overflow viewport height.

Project wiring:

- Added test compilation item to `Core.Tests/Core.Tests.vcxproj`.

## Validation

- `Core.Tests.exe --gtest_brief=1`: pass
	- 3690 tests, 0 failed
- `dotnet test Tests/Nexen.Tests.csproj --no-build -c Release --nologo -v m`: pass
	- 1434 tests, 0 failed

## Scope Notes

- This change targets the boxed save/action notification path that was centered.
- Existing standard OSD text stacking behavior remains unchanged.
