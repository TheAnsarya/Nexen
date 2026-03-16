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

## Related Research

- [Genesis VDP Rendering and DMA](../research/platform-parity/genesis/vdp-rendering-dma.md)
- [Genesis Scheduler and Clocking](../research/platform-parity/genesis/scheduler-and-clocking.md)
