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

## Run Command Templates

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600TiaTiming* --gtest_brief=1
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600SmokeHarness* --gtest_brief=1
```

## Expected Outcomes

- All baseline checkpoints return `PASS`.
- Digest remains stable across repeated runs on unchanged code.
- Failing checkpoint reports include cycle/scanline context.

## Dependencies

- [Atari 2600 TIA Video and Timing](../research/platform-parity/atari-2600/tia-video-timing.md)
- [Atari 2600 Frame Execution Model](../research/platform-parity/atari-2600/frame-execution-model.md)
