# Nexen Future Work Index

This page tracks medium-term and long-term work that extends Nexen beyond current platform coverage and core feature baselines.

## Active Platform-Parity Program

The current expansion effort is tracked under GitHub Epic [#673](https://github.com/TheAnsarya/Nexen/issues/673).

| Track | Scope | Primary Issue | Supporting Plan |
|---|---|---|---|
| Documentation and execution index | Publish roadmap docs, tutorials, and contributor execution flow | [#694](https://github.com/TheAnsarya/Nexen/issues/694) | [Q2 Program Plan](../~docs/plans/future-work-program-2026-q2.md) |
| Atari 2600 feasibility spike | Define architecture constraints, test harness, and validation strategy | [#695](https://github.com/TheAnsarya/Nexen/issues/695) | [Atari 2600 Feasibility Plan](../~docs/plans/atari-2600-feasibility-and-harness-plan.md) |
| Sega Genesis feasibility spike | Define M68000 and VDP strategy, risks, and phased implementation path | [#696](https://github.com/TheAnsarya/Nexen/issues/696) | [Genesis Architecture Plan](../~docs/plans/genesis-architecture-and-incremental-plan.md) |
| Scaffold follow-through backlog (deferred) | Track post-scaffold implementation backlog for Atari and Genesis without starting execution yet | [#717](https://github.com/TheAnsarya/Nexen/issues/717), [#718](https://github.com/TheAnsarya/Nexen/issues/718) | [Scaffold Future-Work Backlog](../~docs/plans/platform-parity-scaffold-future-work-backlog.md) |
| Atari 2600 deep research corpus | Publish source-linked subsystem tree for CPU/TIA/RIOT/mappers/timing | [#704](https://github.com/TheAnsarya/Nexen/issues/704) | [Atari 2600 Research Index](../~docs/research/platform-parity/atari-2600/README.md) |
| Genesis emulator comparative research | Evaluate emulator code architecture and identify best integration patterns | [#705](https://github.com/TheAnsarya/Nexen/issues/705) | [Genesis Research Index](../~docs/research/platform-parity/genesis/README.md) |
| Sonic 1 and Jurassic Park compatibility execution path | Build title-targeted milestone matrix and harness strategy | [#706](https://github.com/TheAnsarya/Nexen/issues/706) | [Compatibility Path](../~docs/research/platform-parity/compatibility/sonic1-jurassic-park.md) |
| Multi-session research and tutorial program | Maintain repeatable cadence for parity research and staged delivery | [#707](https://github.com/TheAnsarya/Nexen/issues/707) | [Q3 Program Plan](../~docs/plans/platform-parity-research-program-2026-q3.md) |

Atari 2600 follow-up implementation issues:

- [#697](https://github.com/TheAnsarya/Nexen/issues/697): CPU/RIOT bring-up skeleton ([Plan](../~docs/plans/atari-2600-cpu-riot-bringup-skeleton.md))
- [#698](https://github.com/TheAnsarya/Nexen/issues/698): TIA timing spike and smoke-test harness ([Plan](../~docs/plans/atari-2600-tia-timing-spike-harness.md))
- [#699](https://github.com/TheAnsarya/Nexen/issues/699): Bankswitching order and regression matrix ([Plan](../~docs/plans/atari-2600-mapper-order-and-regression-matrix.md))

Genesis follow-up implementation issues:

- [#700](https://github.com/TheAnsarya/Nexen/issues/700): M68000 integration boundary and bring-up spike ([Plan](../~docs/plans/genesis-m68000-boundary-and-bringup.md))
- [#701](https://github.com/TheAnsarya/Nexen/issues/701): VDP timing and DMA milestone plan with harness checkpoints ([Plan](../~docs/plans/genesis-vdp-dma-milestones-and-harness.md))
- [#702](https://github.com/TheAnsarya/Nexen/issues/702): Z80 and audio subsystem integration staging ([Plan](../~docs/plans/genesis-z80-audio-integration-staging.md))
- [#703](https://github.com/TheAnsarya/Nexen/issues/703): Benchmark and correctness gates per phase ([Plan](../~docs/plans/platform-parity-benchmark-and-correctness-gates.md))

Deferred scaffold completion sub-epics:

- [#717](https://github.com/TheAnsarya/Nexen/issues/717): Atari scaffold-to-production future-work program ([Plan](../~docs/plans/platform-parity-scaffold-future-work-backlog.md))
- [#718](https://github.com/TheAnsarya/Nexen/issues/718): Genesis scaffold-to-production future-work program ([Plan](../~docs/plans/platform-parity-scaffold-future-work-backlog.md))

Research and compatibility follow-up issues:

- [#704](https://github.com/TheAnsarya/Nexen/issues/704): Atari 2600 subsystem research corpus
- [#705](https://github.com/TheAnsarya/Nexen/issues/705): Genesis emulator architecture comparison
- [#706](https://github.com/TheAnsarya/Nexen/issues/706): Sonic 1 and Jurassic Park compatibility path
- [#707](https://github.com/TheAnsarya/Nexen/issues/707): Multi-session parity research cadence

## Near-Term Improvement Tracks

| Track | Description | Plan |
|---|---|---|
| Emulator performance | Hot-path and system-wide optimization with correctness gates | [Performance Improvement Plan](../~docs/plans/performance-improvement-plan.md) |
| Debugger throughput | Reduce debugger overhead during interactive sessions | [Debugger Performance Optimization](../~docs/plans/debugger-performance-optimization.md) |
| Save-state reliability | Continue architecture modernization and stability improvements | [Save-State System Overhaul](../~docs/plans/save-state-system-overhaul.md) |
| TAS workflow expansion | Continue TAS UI parity and movie workflow quality improvements | [TAS UI Comprehensive Plan](../~docs/plans/tas-ui-comprehensive-plan.md) |

## How To Use This Index

1. Start with [docs/TUTORIALS.md](TUTORIALS.md) for contributor workflow guides.
2. Use the linked GitHub issues to track execution status and discussion.
3. Use the linked planning documents for scope, milestones, and acceptance criteria.
