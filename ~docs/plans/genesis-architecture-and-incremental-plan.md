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

## 2026-04-25 Strategic Expansion Track

Parent epics:

- [#1428](https://github.com/TheAnsarya/Nexen/issues/1428)
- [#1467](https://github.com/TheAnsarya/Nexen/issues/1467)

Strategic child issues:

- [#1468](https://github.com/TheAnsarya/Nexen/issues/1468) - 32X architecture/timing/validation planning.
- [#1469](https://github.com/TheAnsarya/Nexen/issues/1469) - Sega CD subsystem integration planning.
- [#1470](https://github.com/TheAnsarya/Nexen/issues/1470) - Power Base Converter path via SMS prerequisites.
- [#1471](https://github.com/TheAnsarya/Nexen/issues/1471) - Genesis-family controller and peripheral matrix.
- [#1472](https://github.com/TheAnsarya/Nexen/issues/1472) - UI/TAS/Cheat/Debugger full integration backlog.
- [#1473](https://github.com/TheAnsarya/Nexen/issues/1473) - Living roadmap/problem-log maintenance.

### Multi-Track Milestones

| Track | Goal | Phase 0 Output | Exit Gate |
|---|---|---|---|
| Genesis Core Hardening | Stabilize baseline and deterministic behavior | Incremental benchmark/test guard coverage and compatibility checkpoints | Genesis C++ suite and benchmark suites pass without new regressions |
| 32X | Define host/base + add-on integration contract | SH2/bus/VDP composition architecture plan and test strategy | Issue-scoped validation matrix approved before implementation |
| Sega CD | Define CD subsystem coupling model | Sub-CPU/CD block/memory overlay timing plan | Deterministic save-state and synchronization test strategy approved |
| Power Base Converter | Sequence SMS prerequisites and adapter behavior | SMS compatibility gate list + adapter risk register | SMS prerequisites closed and adapter scope issue tree ready |
| Controller/Peripheral Matrix | Reach full input hardware coverage | Matrix with implemented/missing/blocked states | Matrix-backed tests added for each promoted slice |
| UI/TAS/Cheat/Debugger | Reach first-class Genesis-family tooling parity | Prioritized backlog linked to implementation slices | Feature-area tests and docs updated per delivered slice |

### Living Future-Work Backlog

#### Core and Hardware

- Build cycle-documented bus arbitration traces for 68k/Z80/VDP interactions in edge DMA windows.
- Add deterministic 32X integration harness design (dual-SH2 scheduling checkpoints + frame-composition checkpoints).
- Add Sega CD integration harness design (sub-CPU cadence, command/response lanes, PCM/CDDA synchronization gates).
- Define Power Base Converter adapter boundaries and hardware capability matrix tied to SMS readiness gates.

#### Controller and Peripheral Matrix

- Validate 3-button and 6-button behavior across native port paths and hub/multitap paths.
- Add matrix coverage for multitap families and mode-switch/TH edge sequencing cases.
- Expand peripheral matrix entries and mark each as implemented, planned, or blocked by hardware-model gaps.

#### UX and Tooling

- Expand Genesis-family configuration discoverability and status visibility in UI.
- Add TAS editor affordances for Genesis-family controller variants and hardware-specific mappings.
- Expand cheat UX + validation flow for Genesis-family systems.
- Continue debugger/Event Viewer parity slices with Genesis-family system-specific state views.

#### Validation and Operations

- Keep benchmark guard assumptions paired with C++ tests for every new benchmark behavior requirement.
- Maintain targeted + full-matrix validation commands in session logs for every slice.
- Track flaky or environment-sensitive validation behavior separately from deterministic regressions.

### Living Problem Log

| Date | Area | Problem | Status | Tracking |
|---|---|---|---|---|
| 2026-04-25 | Controller Routing | `ControllerHub` switch did not include `ControllerType::GenesisController`, causing incomplete type coverage and GCC switch warnings in CI artifacts. | Fixed in active slice; regression tests added. | [#1471](https://github.com/TheAnsarya/Nexen/issues/1471) |
| 2026-04-25 | Controller Identity | `GenesisController` instances were constructed with `ControllerType::None`, making `HasControllerType(GenesisController)` checks fail. | Fixed in active slice; regression tests added. | [#1471](https://github.com/TheAnsarya/Nexen/issues/1471) |
| 2026-04-25 | Strategic Planning | Full-stack Genesis roadmap for 32X/Sega CD/Power Base/controllers/UX existed only as scattered notes. | Epic and child planning issues created; this document promoted to living roadmap. | [#1467](https://github.com/TheAnsarya/Nexen/issues/1467), [#1473](https://github.com/TheAnsarya/Nexen/issues/1473) |

### Maintenance Rules for This Document

1. Add newly discovered Genesis-family problems to the problem log table on discovery.
2. Link every promoted backlog item to a concrete GitHub issue before implementation.
3. Preserve closed/completed entries for traceability; do not rewrite history.
4. Update milestone status as slices ship so this file remains the canonical roadmap view.
