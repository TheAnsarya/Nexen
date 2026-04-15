# Issue #1295 Writable Config Fallback And Migration Plan

## Context

Issue: [#1295](https://github.com/TheAnsarya/Nexen/issues/1295)

Parent: #1291

Some users can run Nexen from folders where portable config storage exists but is not writable (for example protected install paths). In that scenario, settings persistence can fail unless the app falls back to a writable home path.

## Goals

- Detect when portable config path exists but is not writable.
- Fall back to a writable documents/appdata path automatically.
- Preserve settings by migrating `settings.json` when possible.
- Keep startup behavior deterministic and low-risk.

## Implemented Design

File: `UI/Config/ConfigManager.cs`

1. Added writability probe:

- `IsFolderWritable(string folder)`
- Creates a temp probe file in target folder and removes it.

2. Added migration helper:

- `TryMigrateSettingsFile(string sourceConfigFile, string destinationConfigFile)`
- Copies portable `settings.json` to documents path when destination does not already exist.

3. Added resolver:

- `ResolveHomeFolderPath(...)`
- Decision flow:
	- If no portable `settings.json`: use documents path.
	- If portable settings exists and portable folder is writable: use portable path.
	- If portable settings exists but portable is not writable and documents is writable: use documents path, migrate settings if needed.
	- If both are not writable: return portable as last-resort fallback with diagnostic logging.

4. Wired resolver into `HomeFolder` getter and added guarded directory creation fallback.

## Tests Added

File: `Tests/Config/ConfigManagerHomeFolderTests.cs`

Coverage:

- No portable settings uses documents path.
- Portable settings + writable portable uses portable path.
- Read-only portable + writable documents migrates and uses documents path.
- Read-only portable + writable documents + existing destination settings skips migration.
- Both paths non-writable falls back to portable path.

## Validation

- Release build (x64): pass
- `dotnet test Tests/Nexen.Tests.csproj -c Release`: pass (includes new tests)
- `Core.Tests.exe --gtest_brief=1`: pass
- `dotnet test --no-build -c Release`: pass

## Follow-Up

- Optionally surface an end-user UI notification when path fallback occurs, in addition to diagnostic logging.
