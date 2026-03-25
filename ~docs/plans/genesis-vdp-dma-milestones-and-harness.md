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

### Issue [#731](https://github.com/TheAnsarya/Nexen/issues/731)

- Added deterministic scaffold render-composition inputs for plane A, plane B, window, sprite, priorities, and scroll offsets.
- Added priority-aware deterministic render composition (`ComposeRenderPixel`) for background/window/sprite pipeline checkpointing.
- Added deterministic line rendering and digest capture (`RenderScaffoldLine`, `GetRenderLineDigest`) to support repeatable checkpoint assertions.
- Added focused `GenesisVdpRenderPipelineTests` for:
	- sprite-priority overlay behavior,
	- window-priority override behavior,
	- deterministic digest stability on repeated runs,
	- digest variation under scroll changes.

Updated validation result after render-pipeline scaffold integration:

- Focused Genesis tests: 19 tests from 6 suites passed.
- Full native regression: 1692 tests from 131 suites passed.
- Managed regression: 331 tests passed.

### Issue [#732](https://github.com/TheAnsarya/Nexen/issues/732)

- Added deterministic DMA mode scaffold state (`None`, `Copy`, `Fill`) with transfer accounting and active-cycle tracking.
- Added deterministic CPU/VDP contention penalty model consumed from `GenesisM68kBoundaryScaffold::StepFrameScaffold`.
- Added transfer start and contention consumption APIs (`BeginDmaTransfer`, `ConsumeDmaContention`) for deterministic gate coverage.
- Added focused `GenesisVdpDmaContentionTests` to validate:
	- DMA mode latch behavior from control-register commands,
	- deterministic contention penalty effect on executable CPU cycles,
	- deterministic repeatability of contention counters across identical runs.

Updated validation result after DMA contention scaffold integration:

- Focused Genesis tests: 22 tests from 7 suites passed.
- Full native regression: 1695 tests from 132 suites passed.
- Managed regression: 331 tests passed.

### Issue [#733](https://github.com/TheAnsarya/Nexen/issues/733)

- Added deterministic horizontal/vertical interrupt scheduling controls in `GenesisM68kBoundaryScaffold`.
- Added scanline/frame event ordering log output (`HINT`/`VINT`) with deterministic counters.
- Added configurable interrupt schedule controls (`ConfigureInterruptSchedule`) and event-log reset hook (`ClearTimingEvents`).
- Added focused `GenesisInterruptSchedulingTests` to validate:
	- horizontal interrupt cadence at configured scanline intervals,
	- vertical interrupt trigger on frame rollover,
	- deterministic event ordering when H/V interrupts co-occur.

Updated validation result after H/V interrupt scheduling integration:

- Focused Genesis tests: 25 tests from 8 suites passed.
- Full native regression: 1698 tests from 133 suites passed.
- Managed regression: 331 tests passed.

### Issue [#737](https://github.com/TheAnsarya/Nexen/issues/737)

- Added executable Genesis compatibility matrix harness (`GenesisSmokeHarness::RunCompatibilityMatrix`) with title-targeted checkpoints.
- Added machine-readable output lines for compatibility diagnostics:
	- `GEN_COMPAT_CHECK <id> <PASS|FAIL> <context>`
	- `GEN_COMPAT_RESULT <title> <PASS|FAIL> PASS=<n> FAIL=<n> DIGEST=<hash>`
	- `GEN_COMPAT_MATRIX_SUMMARY PASS=<n> FAIL=<n> DIGEST=<hash>`
- Added Sonic/Jurassic-style title checkpoint routing and deterministic digest verification gates.
- Added focused `GenesisCompatibilityHarnessTests` for deterministic digest stability and failure-path diagnostics on invalid entries.

Updated validation result after compatibility harness integration:

- Focused Genesis tests: 36 tests from 12 suites passed.
- Full native regression: 1709 tests from 137 suites passed.
- Managed regression: 331 tests passed.

### Issue [#738](https://github.com/TheAnsarya/Nexen/issues/738)

- Added explicit Genesis scaffold save-state snapshots for bus, CPU, and scaffold timing/scheduler state.
- Added deterministic state restore API coverage across:
	- memory and mapped window counters,
	- VDP register/latch/render state,
	- DMA/Z80 arbitration state,
	- YM2612/SN76489/mixed-audio counters and digests,
	- CPU execution and interrupt-sequence state,
	- frame/scanline scheduler state and timing events.
- Added focused `GenesisSaveStateDeterminismTests` to validate:
	- exact save-state round-trip identity after destructive mutation,
	- deterministic replay-equivalence after restore for render/audio digest and timing state.

Updated validation result after save-state determinism integration:

- Focused Genesis tests: 38 tests from 13 suites passed.
- Full native regression: 1711 tests from 138 suites passed.
- Managed regression: 331 tests passed.

### Issue [#739](https://github.com/TheAnsarya/Nexen/issues/739)

- Added deterministic Genesis performance gate harness (`GenesisSmokeHarness::RunPerformanceGate`) with machine-readable output lines:
	- `GEN_PERF_RESULT <title> <PASS|FAIL> CLASS=<class> ELAPSED_US=<n> BUDGET_US=<n> DIGEST=<hash>`
	- `GEN_PERF_GATE_SUMMARY PASS=<n> FAIL=<n> BUDGET_US=<n> DIGEST=<hash>`
- Added deterministic performance-gate digest composition independent of wall-clock timing jitter.
- Added restore/replay deterministic checkpoint gating inside performance execution path to keep correctness first.
- Added focused `GenesisPerformanceGateTests` to validate:
	- deterministic performance-gate digest stability across repeated runs,
	- strict-budget failure path behavior.

Updated validation result after performance gate integration:

- Focused Genesis tests: 40 tests from 14 suites passed.
- Full native regression: 1713 tests from 139 suites passed.
- Managed regression: 331 tests passed.

## Deferred Future-Work Linkage

Status: Future Work by default. Issues [#738](https://github.com/TheAnsarya/Nexen/issues/738) and [#739](https://github.com/TheAnsarya/Nexen/issues/739) have been promoted and completed in this execution sprint.

- Parent future-work epic: [#718](https://github.com/TheAnsarya/Nexen/issues/718)
- VDP register semantics follow-through: [#730](https://github.com/TheAnsarya/Nexen/issues/730)
- VDP render pipeline follow-through: [#731](https://github.com/TheAnsarya/Nexen/issues/731)
- VDP DMA and contention follow-through: [#732](https://github.com/TheAnsarya/Nexen/issues/732)
- H/V interrupt scheduling follow-through: [#733](https://github.com/TheAnsarya/Nexen/issues/733)
- Sonic/Jurassic compatibility harness execution: [#737](https://github.com/TheAnsarya/Nexen/issues/737)
- Save-state serialization and replay determinism gates: [#738](https://github.com/TheAnsarya/Nexen/issues/738) (completed)
- Performance gate execution after correctness stabilization: [#739](https://github.com/TheAnsarya/Nexen/issues/739) (completed)

## Related Research

- [Genesis VDP Rendering and DMA](../research/platform-parity/genesis/vdp-rendering-dma.md)
- [Genesis Scheduler and Clocking](../research/platform-parity/genesis/scheduler-and-clocking.md)
