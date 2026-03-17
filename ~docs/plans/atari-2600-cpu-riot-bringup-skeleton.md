# Atari 2600 CPU/RIOT Bring-Up Skeleton Plan

Issue linkage: [#697](https://github.com/TheAnsarya/Nexen/issues/697)

Parent issue: [#695](https://github.com/TheAnsarya/Nexen/issues/695)

## Goal

Create a buildable Atari 2600 core skeleton with explicit CPU, memory-map, and RIOT boundaries that can step frames on a smoke ROM without crashing.

## Scope

- Platform shell and startup wiring for Atari 2600.
- 6507-compatible execution boundary (via existing 6502 strategy constraints).
- Minimal RIOT timer and input/output state object with deterministic stepping entry points.
- No-op hooks for TIA and mapper routing.

## Proposed Architecture

| Area | Boundary |
|---|---|
| `Atari2600Console` shell | Owns CPU, RIOT, TIA placeholder, mapper placeholder, and frame-step loop |
| CPU adapter | Exposes `Reset()`, `StepCycles(n)`, and bus read/write callbacks |
| RIOT core skeleton | Owns timer and port state with `StepCpuCycle()` |
| Bus decode layer | Routes mirrored TIA/RIOT areas and cartridge window |
| TIA placeholder | Provides minimal interface required by frame stepper |

## Build-Time Scaffolding Checklist

1. Add Atari 2600 platform registration and factory wiring.
2. Add skeleton classes for CPU adapter, RIOT, and platform bus.
3. Add no-op TIA and mapper interfaces used by the shell.
4. Add frame-step function that advances CPU and RIOT deterministically.
5. Add smoke test scaffold that loads a ROM and runs a fixed number of frames.

## Validation Commands (Template)

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600Bringup* --gtest_brief=1
```

## Exit Criteria

- Release x64 build succeeds with new Atari 2600 skeleton code included.
- Smoke test can step frames without crash.
- Test scaffold exists for CPU/RIOT wiring assertions.

## Execution Evidence: Issue #719

Issue: [#719](https://github.com/TheAnsarya/Nexen/issues/719)

Status: Completed (2026-03-17)

Implemented outcomes:

- Replaced one-byte stub stepping in Atari CPU adapter with instruction-level decode and deterministic cycle accounting.
- Added cycle-aware branching and arithmetic/control-path support used by scaffold ROMs and harness paths.
- Added focused CPU phase tests that validate instruction-cycle boundaries and branch-selected side effects through RIOT port writes.

Validation evidence:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600CpuPhaseATests.*:Atari2600TimingSpikeHarnessTests.*:Atari2600MapperPhaseATests.*:Atari2600MapperPhaseBTests.*:Atari2600MapperPhaseCTests.* --gtest_brief=1
```

Result: 20 tests from 5 Atari suites passed.

## Deferred Future-Work Linkage

Status: Future Work only. Do not start these issues until explicitly scheduled.

Completed from this deferred set:

- CPU follow-through: [#719](https://github.com/TheAnsarya/Nexen/issues/719)

- Parent future-work epic: [#717](https://github.com/TheAnsarya/Nexen/issues/717)
- RIOT semantics follow-through: [#720](https://github.com/TheAnsarya/Nexen/issues/720)
- Save-state determinism follow-through: [#726](https://github.com/TheAnsarya/Nexen/issues/726)

## Dependencies

- [Atari 2600 Feasibility and Harness Plan](atari-2600-feasibility-and-harness-plan.md)
- [Atari 2600 Research Breakdown](../research/platform-parity/atari-2600/README.md)
