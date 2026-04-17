# Epic 16 Plan - Cross-Console Load Freeze & Paused-Start Regression

## Problem Statement

Reported sequence:

- Load FF2 on WSC (starts paused unexpectedly)
- Load DQ on GBC/NES
- Reload FF2 on WSC
- UI freezes and second load never completes

## Findings From Logs + Code

- `nexen-log.txt` shows successful `LoadRomHelper.LoadRom` calls and no explicit error/failure entries for the reproductions.
- `LoadRom` path sets `IsRomLoadInProgress` while loading.
- `LoadRecentGame` path previously did not set `IsRomLoadInProgress`.
- Main window pause logic intentionally skips menu/config pausing only when `IsRomLoadInProgress` is true.
- Mismatch can allow pause transitions during `LoadRecentGame` operations, leading to paused-start behavior and potential UI/load contention.

## Fix Strategy

- Introduce scoped load-operation tokens in `LoadRomHelper`:
- Increment in-progress counter on operation start.
- Decrement reliably in `finally` on operation end.
- Apply to both `LoadRom` and `LoadRecentGame` paths.
- Add detailed logging for operation start/end and `LoadRecentGame` parameters.

## Verification Strategy

- Unit tests for scoped load-operation counter behavior:
- Start/end flag transitions.
- Nested operation reference counting.
- Double-dispose safety.
- Existing pause-gate tests already verify `isRomLoadInProgress => no menu/config pausing`.
- Add benchmark for pause-gate decision path under load/non-load conditions.
- Build + full .NET test runs.
- Launch Nexen executable for manual verification.

## Follow-up

- Continue Epic 15.3 (piano-roll virtualization slice) after shipping this fix.
