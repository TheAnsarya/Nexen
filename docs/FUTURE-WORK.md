# Nexen Future Work Index

This page tracks medium-term and long-term work that extends Nexen beyond current platform coverage and core feature baselines.

## Active Platform-Parity Program

The current expansion effort is tracked under GitHub Epic [#673](https://github.com/TheAnsarya/Nexen/issues/673).

| Track | Scope | Primary Issue | Supporting Plan |
|---|---|---|---|
| Documentation and execution index | Publish roadmap docs, tutorials, and contributor execution flow | [#694](https://github.com/TheAnsarya/Nexen/issues/694) | [Q2 Program Plan](../~docs/plans/future-work-program-2026-q2.md) |
| Atari 2600 feasibility spike | Define architecture constraints, test harness, and validation strategy | [#695](https://github.com/TheAnsarya/Nexen/issues/695) | [Atari 2600 Feasibility Plan](../~docs/plans/atari-2600-feasibility-and-harness-plan.md) |
| Sega Genesis feasibility spike | Define M68000 and VDP strategy, risks, and phased implementation path | [#696](https://github.com/TheAnsarya/Nexen/issues/696) | [Genesis Architecture Plan](../~docs/plans/genesis-architecture-and-incremental-plan.md) |

Atari 2600 follow-up implementation issues:

- [#697](https://github.com/TheAnsarya/Nexen/issues/697): CPU/RIOT bring-up skeleton
- [#698](https://github.com/TheAnsarya/Nexen/issues/698): TIA timing spike and smoke-test harness
- [#699](https://github.com/TheAnsarya/Nexen/issues/699): Bankswitching order and regression matrix

Genesis follow-up implementation issues:

- [#700](https://github.com/TheAnsarya/Nexen/issues/700): M68000 integration boundary and bring-up spike
- [#701](https://github.com/TheAnsarya/Nexen/issues/701): VDP timing and DMA milestone plan with harness checkpoints
- [#702](https://github.com/TheAnsarya/Nexen/issues/702): Z80 and audio subsystem integration staging
- [#703](https://github.com/TheAnsarya/Nexen/issues/703): Benchmark and correctness gates per phase

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
