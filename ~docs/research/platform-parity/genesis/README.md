# Sega Genesis and Mega Drive Research Breakdown

Issue linkage: [#705](https://github.com/TheAnsarya/Nexen/issues/705)

This section breaks Genesis emulation research into focused subsystem studies with implementation-ready guidance.

## Subsystem Documents

| Document | Scope |
|---|---|
| [Architecture Overview](architecture-overview.md) | Component map and subsystem boundaries |
| [M68000 CPU Integration](cpu-m68000.md) | Main CPU contracts, interrupts, and memory interface boundaries |
| [Z80 Audio Bus and Arbitration](z80-audio-bus.md) | 68k/Z80 bus request/reset behavior and mapped windows |
| [VDP Rendering and DMA](vdp-rendering-dma.md) | HBlank/VBlank, DMA, FIFO, and timing-sensitive rendering |
| [YM2612 and SN76489 Audio](ym2612-sn76489-audio.md) | Audio scheduling, chip interaction, and timing fidelity |
| [Scheduler and Clocking](scheduler-and-clocking.md) | Cross-subsystem cycle model and synchronization strategy |
| [Emulator Code Comparison](emulator-code-comparison.md) | Comparative architecture findings across major emulators |
| [Implementation Checklist](implementation-checklist.md) | Session gates and acceptance criteria |

## Related Documents

- [Compatibility Plan](../compatibility/sonic1-jurassic-park.md)
- [Global Source Index](../source-index.md)
- [Genesis Incremental Plan](../../../plans/genesis-architecture-and-incremental-plan.md)
