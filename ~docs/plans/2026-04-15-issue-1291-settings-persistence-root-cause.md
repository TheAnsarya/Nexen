# Issue #1291 Root Cause + Fix Plan

## Context

Issue: [#1291](https://github.com/TheAnsarya/Nexen/issues/1291)

User symptom:

- Settings appear to apply during a session.
- After exiting and relaunching, many settings revert (video, filters, controllers, shortcuts).

## Root Cause Identified

In `UI/Config/Configuration.cs`, `Serialize(string configFile)` updated `_fileData` even when `FileHelper.WriteAllText(...)` failed.

Observed behavior in pre-fix logic:

1. Serialization computes `cfgData`.
2. Write is attempted through `FileHelper.WriteAllText(...)`.
3. Return value (`bool`) was ignored.
4. `_fileData = cfgData` executed unconditionally.

Impact:

- If write fails (permissions, lock, transient I/O issue), disk config remains unchanged.
- In-memory `_fileData` now matches current config anyway.
- Subsequent saves in the same session are skipped due to `_fileData == cfgData` check.
- Restart reloads stale settings from disk, causing apparent "settings not saved" behavior.

## Implemented Fix

File: `UI/Config/Configuration.cs`

- Capture `writeSucceeded = FileHelper.WriteAllText(configFile, cfgData)`.
- Update `_fileData` only when `writeSucceeded == true`.
- Add explicit UI/core log entries when write or serialization fails:
	- `[UI] Failed to save settings to disk: ...`
	- `[UI] Failed to serialize settings: ...`

## Validation

- Release build (x64): pass
- C++ tests: `Core.Tests.exe --gtest_brief=1` pass
- .NET tests: `dotnet test --no-build -c Release` pass

## Follow-Up Scope (Separate Issue)

Potential additional hardening:

- If portable `settings.json` exists in a non-writable location, automatically migrate/fallback to Documents/AppData config path and notify user.
- Add explicit startup health check for config path writability.

This is tracked as a separate follow-up issue to keep #1291 focused and low risk.
