# Channel F Troubleshooting and Known-Issues Playbook

Parent issue: [#1021](https://github.com/TheAnsarya/Nexen/issues/1021)

## Purpose

Provide practical troubleshooting steps and known-issues guidance for Channel F runtime, debugger, TAS/movie, and persistence workflows.

## Quick Triage Flow

1. Verify Release build succeeds.
2. Verify core tests and any Channel F-focused filter pass.
3. Confirm BIOS path and ROM path are valid and readable.
4. Reproduce with a minimal configuration (single ROM, default input mapping).
5. Capture logs, failing command output, and issue references before escalating.

## Baseline Validation Commands

```powershell
# Release build
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m

# Core tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_brief=1

# Optional Channel F-focused filter (if available)
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=ChannelF* --gtest_brief=1
```

## Known Issues and Workarounds

| Symptom | Likely Cause | Workaround | Tracking |
|---|---|---|---|
| ROM boots but behavior diverges after initial input | Mapping preset mismatch for stick gestures or panel keys | Switch to a known-good preset and re-run input validation checklist | #1014, #1073 |
| Debugger memory/trace view appears incomplete for specific execution paths | Instrumentation parity gaps in some subsystems | Validate with latest debugger fixes and compare against disassembly plus runtime behavior | #1001 and follow-ups |
| Movie replay diverges from recorded behavior | Input encoding mismatch or rerecord branch drift | Recreate short deterministic recording and verify stable branch settings | #1011, #1012 |
| Save/load appears inconsistent during active debug sessions | State serialization coverage still being tightened for specific paths | Validate with clean restart and focused save/load loop; attach reproduction if mismatch persists | #981, #1006 |
| CI pass but local run fails | Local environment mismatch (toolchain or runtime prerequisites) | Re-run from clean Release build and compare command outputs line-by-line with CI artifacts | #1024 |

## Reporting Template

Use this template when opening or updating an issue:

```text
Environment:
- OS:
- Commit:
- Build config:

Repro steps:
1.
2.
3.

Expected:
Actual:

Evidence:
- Build/test command outputs
- Screenshot or trace snippet
- Related issue links
```

## Related Documents

- [Channel F Production Readiness Matrix](channelf-production-readiness-matrix.md)
- [Channel F Test Strategy Matrix](channelf-test-strategy-matrix-2026-03-30.md)
- [Channel F Debug and TAS Manual Test](../../docs/manual-testing/channelf-debug-and-tas-manual-test.md)
- [Channel F Controller Profile Presets](../../docs/manual-testing/channelf-controller-profile-presets.md)
