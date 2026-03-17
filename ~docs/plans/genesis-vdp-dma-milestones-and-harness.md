# Genesis VDP Timing and DMA Milestone Plan with Harness Checkpoints

Issue linkage: [#701](https://github.com/TheAnsarya/Nexen/issues/701)

Parent issue: [#696](https://github.com/TheAnsarya/Nexen/issues/696)

## Goal

Define phased VDP timing and DMA milestones with deterministic checkpoint outputs suitable for continuous regression validation.

## Scope

- VDP timing boundary phases (startup, scanline progression, frame completion).
- DMA behavior milestones (basic copy, fill, and contention-sensitive paths).
- Harness checkpoint definitions with stable output contracts.

## Milestone Matrix

| Milestone | Description | Checkpoint Group |
|---|---|---|
| M1 | VDP register bootstrap and frame counter stability | VDP-BOOT-* |
| M2 | Scanline and visible-area timing consistency | VDP-SCAN-* |
| M3 | DMA transfer correctness for baseline copy/fill | VDP-DMA-* |
| M4 | CPU/VDP contention-sensitive sequencing guardrails | VDP-CONT-* |

## Harness Checkpoints

| Checkpoint ID | Assertion | Output |
|---|---|---|
| VDP-BOOT-01 | Initial register snapshot is deterministic | `PASS VDP-BOOT-01` |
| VDP-SCAN-01 | Scanline counter and frame cadence remain stable | `PASS VDP-SCAN-01` |
| VDP-DMA-01 | Baseline DMA copy writes expected destination words | `PASS VDP-DMA-01` |
| VDP-CONT-01 | Contention scenario preserves ordering invariants | `PASS VDP-CONT-01` |

## Output Contract

- `CHECKPOINT <id> <PASS|FAIL> <context>`
- `VDP_SUMMARY PASS=<n> FAIL=<n> DIGEST=<hash>`

## Smoke ROM Set

| Target ID | Focus | Expected Outcome |
|---|---|---|
| GEN-SMOKE-BOOT-01 | Basic startup and VDP register initialization | Stable startup digest and no checkpoint failures |
| GEN-SMOKE-TIMING-01 | Scanline/frame cadence stability | Matching checkpoint digest across repeated runs |
| GEN-SMOKE-DMA-01 | DMA copy/fill baseline behavior | No data mismatch in checkpointed memory windows |
| GEN-SMOKE-COMPAT-SONIC1 | Compatibility-aligned validation with Sonic 1 path | No regressions in milestone checkpoints tied to compatibility plan |
| GEN-SMOKE-COMPAT-JP | Compatibility-aligned validation with Jurassic Park path | No regressions in milestone checkpoints tied to compatibility plan |

## Validation Command Templates

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=GenesisVdpTiming* --gtest_brief=1
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=GenesisVdpDma* --gtest_brief=1
```

## Exit Criteria

- Milestones are dependency-ordered and documented.
- Harness checkpoint contract is explicit and reviewable.
- Deterministic digest strategy is defined for repeated-run validation.
- Smoke ROM targets and expected pass outcomes are documented.

## Scaffold Checkpoint Evidence

- Added deterministic VDP window decode tracking to `GenesisPlatformBusStub` for addresses `0xc00000-0xc0001f`.
- Added DMA latch checkpoint behavior for control-port writes (`0xc00004-0xc00007` with bit `0x80`).
- Added focused regression suite: `GenesisVdpDmaScaffoldTests`.

Focused command:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=GenesisM68kBoundaryScaffoldTests.*:GenesisVdpDmaScaffoldTests.* --gtest_brief=1
```

Observed result snapshot:

- `[==========] 5 tests from 2 test suites ran.`

## Timing Scaffold Evidence

- Added deterministic scanline/frame checkpoint counters to `GenesisM68kBoundaryScaffold`.
- Timing progression now advances from CPU cycle stepping with explicit scanline and frame rollover behavior.
- Added focused regression suite: `GenesisVdpTimingScaffoldTests`.

Focused command:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=GenesisM68kBoundaryScaffoldTests.*:GenesisVdpDmaScaffoldTests.*:GenesisVdpTimingScaffoldTests.* --gtest_brief=1
```

Observed result snapshot:

- `[==========] 8 tests from 3 test suites ran.`

## Execution Evidence: Promoted VDP Register/Status Follow-Through

Status: Completed from deferred backlog (2026-03-17)

### Issue [#730](https://github.com/TheAnsarya/Nexen/issues/730)

- Added deterministic VDP register-file and status model state to the Genesis scaffold bus path.
- Implemented control-port command decoding for register writes and deterministic status side effects.
- Implemented status-port read behavior with sticky pending-bit clear-on-read semantics.
- Preserved and expanded control/data-port deterministic read/write behavior for scaffold checkpoints.
- Added focused tests in `GenesisVdpRegisterStatusTests` and updated `GenesisVdpDmaScaffoldTests` for control/data-port semantics.

Validation commands:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Genesis* --gtest_brief=1
.\bin\win-x64\Release\Core.Tests.exe --gtest_brief=1
dotnet test --no-build -c Release
```

Result: 15 Genesis tests passed; 1688 native tests passed; 331 managed tests passed.

## Deferred Future-Work Linkage

Status: Future Work only. Do not start these issues until explicitly scheduled.

- Parent future-work epic: [#718](https://github.com/TheAnsarya/Nexen/issues/718)
- VDP register semantics follow-through: [#730](https://github.com/TheAnsarya/Nexen/issues/730)
- VDP render pipeline follow-through: [#731](https://github.com/TheAnsarya/Nexen/issues/731)
- VDP DMA and contention follow-through: [#732](https://github.com/TheAnsarya/Nexen/issues/732)
- H/V interrupt scheduling follow-through: [#733](https://github.com/TheAnsarya/Nexen/issues/733)
- Sonic/Jurassic compatibility harness execution: [#737](https://github.com/TheAnsarya/Nexen/issues/737)

## Related Research

- [Genesis VDP Rendering and DMA](../research/platform-parity/genesis/vdp-rendering-dma.md)
- [Genesis Scheduler and Clocking](../research/platform-parity/genesis/scheduler-and-clocking.md)
