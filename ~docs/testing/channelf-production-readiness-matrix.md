# Channel F Production Readiness Matrix

Parent issue: [#1023](https://github.com/TheAnsarya/Nexen/issues/1023)

## Purpose

Define explicit go/no-go criteria for first-class Channel F readiness across accuracy, debugger/tooling behavior, TAS/movie workflows, persistence, and documentation.

## Required Validation Commands

```powershell
# Build Release x64
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m

# C++ tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_brief=1

# Managed tests
dotnet test --no-build -c Release --nologo -v m
```

## Readiness Gates

| Area | Gate | Evidence Source | Status |
|---|---|---|---|
| Core runtime | BIOS boot and cartridge loop stability confirmed | Channel F runtime smoke evidence in session logs and CI artifacts | In Progress |
| CPU/bus behavior | Deterministic execution for core instruction and bus paths | Channel F CPU and bus regression tests | In Progress |
| Video/audio | Frame progression and tone output are stable over extended playback | Core tests + manual workflow captures | In Progress |
| Input fidelity | Stick twist/pull/push plus front-panel buttons map correctly | Manual testing checklist + input traces | In Progress |
| Debugger parity | Break/step/disassembly/memory views are coherent for Channel F sessions | Debugger validation checklist and issue-linked evidence | In Progress |
| TAS/movie parity | Record/replay/rerecord workflows are deterministic | TAS/manual tests + movie roundtrip checks | In Progress |
| Persistence | Save-state and movie/load paths preserve expected state | Save/load determinism checks | In Progress |
| Docs completeness | User and developer docs include Channel F run/debug/test guidance | docs/manual-testing + ~docs/testing indexes | In Progress |
| CI gating | Build/test lanes include Channel F readiness checks and fail on regressions | CI workflow runs and problem-match evidence | In Progress |

## Go/No-Go Rule

Release readiness is green only when all rows are set to Complete and each row has linked evidence in a session log, issue comment, or CI run.

## Related Documents

- [Channel F Test Strategy Matrix](channelf-test-strategy-matrix-2026-03-30.md)
- [Channel F Debug and TAS Manual Test](../../docs/manual-testing/channelf-debug-and-tas-manual-test.md)
- [Channel F Ordered Prompt Pack](../plans/channel-f-implementation-prompts.md)
