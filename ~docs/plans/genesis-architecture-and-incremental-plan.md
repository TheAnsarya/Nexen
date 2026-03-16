# Sega Genesis Architecture and Incremental Plan

Parent issues: [#673](https://github.com/TheAnsarya/Nexen/issues/673), [#696](https://github.com/TheAnsarya/Nexen/issues/696)

## Objectives

1. Define integration boundaries for Genesis emulation subsystems in Nexen.
2. Build a phased roadmap that prioritizes correctness and measurable progress.
3. Establish benchmark and correctness gates for each delivery phase.

## Architectural Scope

### CPU Layer

- Primary CPU: Motorola 68000.
- Secondary CPU: Z80 for audio/control responsibilities.
- Requirement: explicit bus ownership and arbitration boundaries between CPUs and VDP.

### Video Layer

- Core component: Genesis VDP with scanline timing and DMA behavior.
- Minimum milestone coverage: timing model, scroll plane baseline, sprite baseline, DMA flow.

### Audio Layer

- YM2612 FM synthesis path.
- SN76489 PSG path.
- Synchronization requirement: deterministic timing alignment with CPU and VDP progression.

### Memory and Bus Arbitration

- Define shared memory access policy between 68000, Z80, and VDP.
- Ensure arbitration behavior is testable and replay-deterministic.

## Phase Plan

| Phase | Primary Goal | Entry Criteria | Exit Criteria |
|---|---|---|---|
| P1 | M68000 boundary and bring-up scaffold | Architecture boundary draft approved | Buildable scaffold and deterministic smoke stepping |
| P2 | VDP timing and DMA milestone harness | P1 complete with stable core loop | Harness checkpoints passing on baseline timing ROMs |
| P3 | Z80 and audio subsystem staging | P2 timing loop stable | Deterministic audio/timing regression checks pass |
| P4 | Performance and correctness gates integration | P3 subsystem path connected | Benchmark and correctness gates enforced per phase |

## Benchmark and Correctness Gates

### Baseline Rules

1. Correctness gates are mandatory before performance tuning.
2. Each phase requires reproducible pass/fail evidence.
3. Performance measurements must include before and after comparisons.

### Phase Gate Matrix

| Phase | Correctness Gate | Benchmark Gate | Evidence |
|---|---|---|---|
| P1 | Core smoke stepping remains deterministic | Build and startup overhead baseline captured | Smoke-step log + build/test output |
| P2 | Scanline and DMA checkpoints match expected values | Timing harness runtime baseline captured | Checkpoint report + timing benchmark |
| P3 | Audio synchronization and Z80 arbitration checks pass | Audio and mixed-frame overhead baseline captured | Regression report + subsystem benchmarks |
| P4 | Full phase-gate suite passes without regressions | No unexplained regressions versus previous phase | Consolidated gate report |

### Validation Command Targets

- Build: `Build Nexen Release x64` task in VS Code.
- C++ tests: `bin/win-x64/Release/Core.Tests.exe --gtest_brief=1`.
- Genesis-focused test and benchmark commands: to be introduced through issue [#703](https://github.com/TheAnsarya/Nexen/issues/703).

## Risk Register

| Risk | Impact | Mitigation |
|---|---|---|
| Incomplete M68000 boundary definition | High | Freeze interface contract before deep implementation |
| VDP timing drift under DMA | High | Add deterministic cycle checkpoints early |
| Z80 arbitration mismatch | High | Add bus handoff assertions and replay tests |
| FM synthesis correctness drift | Medium-High | Add audio regression baselines before optimization |
| Phase scope creep | Medium | Require issue-scoped deliverables per phase |

## Issue Decomposition

| Issue | Purpose | Dependency |
|---|---|---|
| [#700](https://github.com/TheAnsarya/Nexen/issues/700) | M68000 integration boundary and bring-up spike | None |
| [#701](https://github.com/TheAnsarya/Nexen/issues/701) | VDP timing and DMA milestones with harness checkpoints | Starts after #700 boundaries are stable |
| [#702](https://github.com/TheAnsarya/Nexen/issues/702) | Z80 and audio subsystem integration staging | Starts after #701 timing framework exists |
| [#703](https://github.com/TheAnsarya/Nexen/issues/703) | Benchmark and correctness gates per phase | Spans all phases, finalized after #702 |

## Completion Criteria for Parent Issue #696

- Architecture and risk plan is documented in this file.
- Phased issue tree exists and is linked.
- Benchmark and correctness gates are defined for each phase.
