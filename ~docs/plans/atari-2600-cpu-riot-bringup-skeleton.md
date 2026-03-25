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

## Execution Evidence: Issue #720

Issue: [#720](https://github.com/TheAnsarya/Nexen/issues/720)

Status: Completed (2026-03-17)

Implemented outcomes:

- Added RIOT direction-register semantics for Port A/Port B read paths.
- Added deterministic timer-divider behavior for 1/8/64/1024 prescale writes.
- Added timer underflow interrupt-edge tracking (`InterruptFlag`, `InterruptEdgeCount`) for deterministic checkpoint validation.
- Added focused RIOT phase tests for direction-masked reads and divider underflow edge progression.

Validation evidence:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600RiotPhaseATests.*:Atari2600CpuPhaseATests.*:Atari2600TimingSpikeHarnessTests.*:Atari2600MapperPhaseATests.*:Atari2600MapperPhaseBTests.*:Atari2600MapperPhaseCTests.* --gtest_brief=1
```

Result: 22 tests from 6 Atari suites passed.

## Execution Evidence: Issue #721

Issue: [#721](https://github.com/TheAnsarya/Nexen/issues/721)

Status: Completed (2026-03-17)

Implemented outcomes:

- Added WSYNC event counting to preserve deterministic scanline-boundary behavior tracking.
- Added HMOVE strobe/pending/apply semantics with deterministic blanking-burst color-clock advancement.
- Routed TIA HMOVE register writes (`$2a`) through bus decode into TIA timing logic.
- Added focused TIA phase tests for WSYNC boundary advancement and HMOVE edge behavior determinism.

Validation evidence:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600TiaPhaseATests.*:Atari2600RiotPhaseATests.*:Atari2600CpuPhaseATests.*:Atari2600TimingSpikeHarnessTests.*:Atari2600MapperPhaseATests.*:Atari2600MapperPhaseBTests.*:Atari2600MapperPhaseCTests.* --gtest_brief=1
```

Result: 25 tests from 7 Atari suites passed.

## Execution Evidence: Issue #722

Issue: [#722](https://github.com/TheAnsarya/Nexen/issues/722)

Status: Completed (2026-03-17)

Implemented outcomes:

- Replaced placeholder gradient renderer with deterministic TIA register-driven playfield and object-layer scaffold rendering.
- Added TIA register read/write handling for playfield colors/patterns, player graphics, and missile/ball enables.
- Routed low-address TIA writes through bus decode into render-state updates while preserving mapper/RIOT behavior.
- Added focused render phase tests validating playfield output changes and player/ball overlay determinism.

Validation evidence:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600RenderPhaseATests.*:Atari2600TiaPhaseATests.*:Atari2600RiotPhaseATests.*:Atari2600CpuPhaseATests.*:Atari2600TimingSpikeHarnessTests.*:Atari2600MapperPhaseATests.*:Atari2600MapperPhaseBTests.*:Atari2600MapperPhaseCTests.* --gtest_brief=1
```

Result: 28 tests from 8 Atari suites passed.

## Execution Evidence: Issue #726

Issue: [#726](https://github.com/TheAnsarya/Nexen/issues/726)

Status: Completed (2026-03-17)

Implemented outcomes:

- Expanded Atari platform serialization to cover CPU, RIOT, TIA, mapper state, and frame summary values.
- Added import/export state helpers for CPU, RIOT, TIA, and mapper to support save/load round-trip correctness.
- Added focused save-state determinism tests validating replay equivalence after restore and deterministic payload output for unchanged state.

Validation evidence:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600SaveStateDeterminismTests.*:Atari2600CompatibilityMatrixTests.*:Atari2600MapperPhaseDTests.*:Atari2600AudioPhaseATests.*:Atari2600RenderPhaseATests.*:Atari2600TiaPhaseATests.*:Atari2600RiotPhaseATests.*:Atari2600CpuPhaseATests.*:Atari2600TimingSpikeHarnessTests.*:Atari2600MapperPhaseATests.*:Atari2600MapperPhaseBTests.*:Atari2600MapperPhaseCTests.* --gtest_brief=1
```

Result: 38 tests from 12 Atari suites passed.

## Deferred Future-Work Linkage

Status: Future Work only. Do not start these issues until explicitly scheduled.

Completed from this deferred set:

- CPU follow-through: [#719](https://github.com/TheAnsarya/Nexen/issues/719)
- RIOT semantics follow-through: [#720](https://github.com/TheAnsarya/Nexen/issues/720)
- TIA timing follow-through: [#721](https://github.com/TheAnsarya/Nexen/issues/721)
- TIA render pipeline follow-through: [#722](https://github.com/TheAnsarya/Nexen/issues/722)
- Save-state determinism follow-through: [#726](https://github.com/TheAnsarya/Nexen/issues/726)

- Parent future-work epic: [#717](https://github.com/TheAnsarya/Nexen/issues/717)

## Dependencies

- [Atari 2600 Feasibility and Harness Plan](atari-2600-feasibility-and-harness-plan.md)
- [Atari 2600 Research Breakdown](../research/platform-parity/atari-2600/README.md)
