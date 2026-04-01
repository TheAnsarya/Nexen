# Channel F Test Strategy Matrix (2026-03-30)

Parent issue: [#1015](https://github.com/TheAnsarya/Nexen/issues/1015)

## Purpose

Define consolidated automated and manual quality gates for Fairchild Channel F across core emulation, UI, debugger, TAS/movie workflows, and persistence.

## Scope Areas

- Core emulation behavior and runtime stability.
- UI surfaces and platform discoverability.
- Debugger behavior and consistency.
- TAS and movie edit and replay workflows.
- Persistence behavior (save state and movie roundtrip).

## Validation Commands

```powershell
# Build Release x64
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m

# Run full C++ tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_brief=1

# Run full .NET tests
dotnet test --no-build -c Release --nologo -v m
```

## Quality Gate Matrix

| Area | Gate | Automated Evidence | Manual Evidence | Status |
| --- | --- | --- | --- | --- |
| Core boot/run | Channel F ROM opens and runs stable frame loops | Existing core test suites and smoke execution in Release build | Stage 1.1 in manual checklist | Partial |
| Core timing/state | Frame progression and runtime state transitions remain stable under pause/resume and state restore | Existing emulator core regression suites | Stage 1.2 in manual checklist | Partial |
| UI discoverability | Channel F appears in standard ROM-open and runtime workflows with no platform-specific dead ends | Shared UI test suites | Stage 1.1 and Stage 3.1 in manual checklist | Partial |
| Debugger break/step | Breakpoints and single-step are stable and deterministic in Channel F sessions | Shared debugger tests where applicable | Stage 2.1 in manual checklist | Partial |
| Debugger coherence | Memory and trace views agree on observed execution effects | Shared debugger and trace tests where applicable | Stage 2.2 in manual checklist | Partial |
| TAS editor stability | Channel F TAS editor opens, edits, and replays without crash | Existing TAS editor managed test suites | Stage 3.1 in manual checklist | Partial |
| TAS branch/rerecord | Branching and rerecord loops preserve timeline integrity | Existing TAS workflow tests | Stage 3.2 in manual checklist | Partial |
| Movie persistence | Channel F movie save/load preserves key input timeline state | Existing movie serialization tests | Stage 3.3 in manual checklist | Partial |
| Save-state persistence | Save/load roundtrips are stable in active Channel F sessions | Existing save-state determinism suites | Stage 1.2 in manual checklist | Partial |

## Manual Checklist Link

- [docs/manual-testing/channelf-debug-and-tas-manual-test.md](../../docs/manual-testing/channelf-debug-and-tas-manual-test.md)

## Completion Criteria

Mark this matrix complete when:

1. Channel F-specific automated test filters are added and passing in CI.
2. The manual checklist has a full green pass recorded in an active session log.
3. All rows above are promoted from `Partial` to `Complete` with linked evidence.
