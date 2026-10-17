# Sonic 1 and Jurassic Park Compatibility Path

Issue linkage: [#706](https://github.com/TheAnsarya/Nexen/issues/706)

## Scope

Define the minimum emulator correctness milestones required to make both target titles reliable on a future Genesis core in Nexen:

- Sonic the Hedgehog (Genesis)
- Jurassic Park (Genesis)

## Why These Games

- Sonic 1 stresses timing-sensitive VDP behavior, sprite processing, and interrupt scheduling.
- Jurassic Park stresses CPU/VDP/audio interactions and gameplay-critical timing consistency.

## Milestone Gates

| Gate | Required Capability | Evidence |
|---|---|---|
| G1 | Stable M68000 fetch/decode/interrupt baseline | CPU instruction and interrupt regression suite |
| G2 | Correct Z80 bus request/reset arbitration | Deterministic bus handoff tests and no deadlocks |
| G3 | VDP line timing, HBlank/VBlank IRQ, and DMA behavior | Per-scanline trace comparison and frame hash checks |
| G4 | YM2612 and SN76489 scheduling consistency | Audio timing trace checks and deterministic playback windows |
| G5 | Input and region timing correctness | Controller polling and NTSC/PAL behavior validation |
| G6 | End-to-end gameplay stability for both titles | Scripted boot-to-gameplay smoke tests and replay checks |

## Proposed Execution Sequence

1. Complete architecture/harness prerequisites from issues #700 to #703.
2. Build title-specific smoke scripts for Sonic 1 (boot, title screen, level start, ring collection, pause/resume) and Jurassic Park (boot, title flow, first gameplay section, repeated input interactions).
3. Add trace capture points for IRQ acknowledge timing, VDP DMA windows, and Z80 bus ownership transitions.
4. Compare behavior against one fast reference core and one accuracy-first reference core.
5. Promote only when both titles pass deterministic replay criteria across repeated runs.

## Risk Matrix

| Risk | Impact | Mitigation |
|---|---|---|
| Incorrect CPU/VDP scheduling coupling | Hard-to-reproduce gameplay bugs | Per-line scheduler assertions and trace checkpoints |
| Z80 bus arbitration drift | Audio stalls or hangs | Dedicated bus ownership tests before title runs |
| DMA timing inaccuracies | Visual corruption and state divergence | DMA stress ROMs plus title-specific render checkpoints |
| Audio chip timing drift | Music and SFX instability | Fixed-point timing model and trace-based verification |

## Deliverables

- Compatibility test harness scripts for both titles.
- Deterministic replay artifacts and pass/fail matrix.
- Regression suite entries tied to each gate.
- Issue updates documenting gate completion evidence.

## Related Documents

- [Genesis Architecture Overview](../genesis/architecture-overview.md)
- [Genesis Scheduler and Clocking](../genesis/scheduler-and-clocking.md)
- [Genesis Emulator Comparison](../genesis/emulator-code-comparison.md)
