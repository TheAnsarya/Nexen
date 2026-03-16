# Platform Parity Research Program

This folder tracks source-cited research and execution planning for Epic [#673](https://github.com/TheAnsarya/Nexen/issues/673), with specific focus on issues [#704](https://github.com/TheAnsarya/Nexen/issues/704), [#705](https://github.com/TheAnsarya/Nexen/issues/705), [#706](https://github.com/TheAnsarya/Nexen/issues/706), and [#707](https://github.com/TheAnsarya/Nexen/issues/707).

## Program Structure

| Area | Description |
|---|---|
| [Source Index](source-index.md) | Curated external references and emulator code evidence |
| [Atari 2600 Research](atari-2600/README.md) | Subsystem-by-subsystem architecture and implementation breakdown |
| [Genesis Research](genesis/README.md) | Comparative emulator architecture analysis for Mega Drive support |
| [Compatibility Planning](compatibility/README.md) | Title-targeted strategy for Sonic 1 and Jurassic Park |

## Implementation Staging Links

Atari 2600 execution plans:

- [CPU/RIOT Bring-Up Skeleton](../../plans/atari-2600-cpu-riot-bringup-skeleton.md) (#697)
- [TIA Timing Spike and Harness](../../plans/atari-2600-tia-timing-spike-harness.md) (#698)
- [Mapper Order and Regression Matrix](../../plans/atari-2600-mapper-order-and-regression-matrix.md) (#699)

Genesis execution plans:

- [M68000 Boundary and Bring-Up](../../plans/genesis-m68000-boundary-and-bringup.md) (#700)
- [VDP Timing and DMA Milestones](../../plans/genesis-vdp-dma-milestones-and-harness.md) (#701)
- [Z80 and Audio Integration Staging](../../plans/genesis-z80-audio-integration-staging.md) (#702)
- [Benchmark and Correctness Gates](../../plans/platform-parity-benchmark-and-correctness-gates.md) (#703)

## Execution Notes

- Accuracy is non-negotiable.
- Research outputs must map to testable milestones.
- Every implementation phase must be linked to GitHub issues and harness evidence.

## Related Docs

- [Q2 2026 Future Work Program](../../plans/future-work-program-2026-q2.md)
- [Atari 2600 Feasibility Plan](../../plans/atari-2600-feasibility-and-harness-plan.md)
- [Genesis Architecture Plan](../../plans/genesis-architecture-and-incremental-plan.md)
