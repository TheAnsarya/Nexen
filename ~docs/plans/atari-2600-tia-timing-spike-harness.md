# Atari 2600 TIA Timing Spike and Smoke-Test Harness Plan

Issue linkage: [#698](https://github.com/TheAnsarya/Nexen/issues/698)

Parent issue: [#695](https://github.com/TheAnsarya/Nexen/issues/695)

## Goal

Define and implement a deterministic TIA timing spike with scriptable pass/fail outputs for a baseline smoke ROM set.

## Scope

- Scanline stepping model with explicit cycle accounting.
- Minimal visibility checkpoints for player, missile, and ball timing.
- Deterministic harness output format.
- Documented run commands and expected output signatures.

## Timing Checkpoint Matrix

| Checkpoint ID | Assertion | Output |
|---|---|---|
| TIA-CP-01 | Scanline counter increments exactly as expected across frame boundary | `PASS TIA-CP-01` |
| TIA-CP-02 | WSYNC hold/release behavior preserves cycle ordering | `PASS TIA-CP-02` |
| TIA-CP-03 | Object visibility checkpoint appears at expected cycle bucket | `PASS TIA-CP-03` |
| TIA-CP-04 | Repeat runs produce identical summary digest | `PASS TIA-CP-04` |

## Harness Output Contract

- Machine-readable line format:
- `CHECKPOINT <id> <PASS|FAIL> <context>`
- Final summary line:
- `HARNESS_SUMMARY PASS=<n> FAIL=<n> DIGEST=<hash>`
- Baseline ROM set line format:
- `ROM_RESULT <rom-name> <PASS|FAIL> PASS=<n> FAIL=<n> DIGEST=<hash>`
- Baseline ROM set summary line:
- `ROM_SET_SUMMARY PASS=<n> FAIL=<n> DIGEST=<hash>`

## Run Command Templates

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600TimingSpikeHarnessTests.TimingSpikeHarnessHasStableScanlineDeltas --gtest_brief=1
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600TimingSpikeHarnessTests.* --gtest_brief=1
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600TimingSpikeHarnessTests.BaselineRomSet* --gtest_brief=1
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=GenesisM68kBoundaryScaffoldTests.*:Atari2600TimingSpikeHarnessTests.* --gtest_brief=1
```

## Expected Outcomes

- All baseline checkpoints return `PASS`.
- Digest remains stable across repeated runs on unchanged code.
- Failing checkpoint reports include cycle/scanline context.
- Baseline ROM set run emits one `ROM_RESULT` line per ROM and a final `ROM_SET_SUMMARY` line.
- Current focused run prints:
	- `[==========] 10 tests from 2 test suites ran.`
	- `TIMING_SPIKE SUMMARY STABLE=true DIGEST=<stable-hash>` (from harness output lines)

## Execution Evidence: Promoted TIA Follow-Through

Status: Completed from deferred backlog (2026-03-17)

### Issue [#721](https://github.com/TheAnsarya/Nexen/issues/721)

- Added deterministic WSYNC edge counting and HMOVE strobe/apply timing behavior.
- Added focused tests in `Atari2600TiaPhaseATests` for WSYNC/HMOVE timing progression.

### Issue [#722](https://github.com/TheAnsarya/Nexen/issues/722)

- Replaced placeholder debug gradient with deterministic register-driven playfield/object rendering scaffold.
- Added focused tests in `Atari2600RenderPhaseATests` for playfield changes and layer overlays.

### Issue [#723](https://github.com/TheAnsarya/Nexen/issues/723)

- Implemented deterministic two-channel TIA audio stepping and mixer accumulator state.
- Added AUDC/AUDF/AUDV register read/write behavior and audio metadata integration through console audio API.
- Added focused tests in `Atari2600AudioPhaseATests` for register semantics, deterministic output, and mixer reset behavior.

### Issue [#725](https://github.com/TheAnsarya/Nexen/issues/725)

- Added a title-targeted compatibility matrix harness path (`RunCompatibilityMatrix`) with deterministic per-title checkpoints.
- Added machine-readable compatibility output lines:
	- `COMPAT_CHECK <id> <PASS|FAIL> <context>`
	- `COMPAT_RESULT <title> <PASS|FAIL> PASS=<n> FAIL=<n> DIGEST=<hash>`
	- `COMPAT_MATRIX_SUMMARY PASS=<n> FAIL=<n> DIGEST=<hash>`
- Added focused tests in `Atari2600CompatibilityMatrixTests` covering deterministic digest stability and failure-path behavior for empty ROM entries.

### Issue [#727](https://github.com/TheAnsarya/Nexen/issues/727)

- Added performance gate harness path (`RunPerformanceGate`) for per-title budget checks following correctness gates.
- Added machine-readable performance output lines:
	- `PERF_RESULT <title> <PASS|FAIL> MAPPER=<mode> ELAPSED_US=<micros> BUDGET_US=<micros> DIGEST=<hash>`
	- `PERF_GATE_SUMMARY PASS=<n> FAIL=<n> BUDGET_US=<micros> DIGEST=<hash>`
- Added focused tests in `Atari2600PerformanceGateTests` for deterministic digest stability and strict-budget failure path behavior.

Validation command used for promoted TIA work:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600AudioPhaseATests.*:Atari2600RenderPhaseATests.*:Atari2600TiaPhaseATests.*:Atari2600RiotPhaseATests.*:Atari2600CpuPhaseATests.*:Atari2600TimingSpikeHarnessTests.*:Atari2600MapperPhaseATests.*:Atari2600MapperPhaseBTests.*:Atari2600MapperPhaseCTests.* --gtest_brief=1
```

Result: 31 tests from 9 suites passed.

Updated validation command after compatibility matrix integration:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600CompatibilityMatrixTests.*:Atari2600MapperPhaseDTests.*:Atari2600AudioPhaseATests.*:Atari2600RenderPhaseATests.*:Atari2600TiaPhaseATests.*:Atari2600RiotPhaseATests.*:Atari2600CpuPhaseATests.*:Atari2600TimingSpikeHarnessTests.*:Atari2600MapperPhaseATests.*:Atari2600MapperPhaseBTests.*:Atari2600MapperPhaseCTests.* --gtest_brief=1
```

Result: 36 tests from 11 suites passed.

Updated validation command after performance gate integration:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600* --gtest_brief=1
```

Result: 40 tests from 13 suites passed.

Comprehensive regression validation:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_brief=1
dotnet test --no-build -c Release
```

Result: 1681 native tests passed and 331 managed tests passed.

## Deferred Future-Work Linkage

Status: Future Work only. Do not start these issues until explicitly scheduled.

Completed from this deferred set:

- TIA timing follow-through: [#721](https://github.com/TheAnsarya/Nexen/issues/721)
- TIA render follow-through: [#722](https://github.com/TheAnsarya/Nexen/issues/722)
- TIA audio follow-through: [#723](https://github.com/TheAnsarya/Nexen/issues/723)
- Compatibility harness expansion: [#725](https://github.com/TheAnsarya/Nexen/issues/725)
- Performance gate harness expansion: [#727](https://github.com/TheAnsarya/Nexen/issues/727)

- Parent future-work epic: [#717](https://github.com/TheAnsarya/Nexen/issues/717)

## Dependencies

- [Atari 2600 TIA Video and Timing](../research/platform-parity/atari-2600/tia-video-timing.md)
- [Atari 2600 Frame Execution Model](../research/platform-parity/atari-2600/frame-execution-model.md)
