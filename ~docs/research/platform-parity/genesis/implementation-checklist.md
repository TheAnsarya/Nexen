# Genesis Implementation Checklist

Issue linkage: [#705](https://github.com/TheAnsarya/Nexen/issues/705)

## Session Checklist

| Step | Goal | Exit Criteria |
|---|---|---|
| G1 | M68000 bring-up | Instruction flow, exceptions, and interrupt baseline validated |
| G2 | Memory map and bus windows | Stable ROM/RAM/IO map with trace-verified accesses |
| G3 | Z80 arbitration | Bus request/reset semantics validated by focused tests |
| G4 | VDP timing and DMA | HBlank/VBlank, FIFO, and DMA windows pass trace checks |
| G5 | Audio synchronization | YM2612/SN76489 timing stable under deterministic replay |
| G6 | Target title harness | Sonic 1 and Jurassic Park milestone scripts pass |

## Cross-Cutting Quality Gates

- All milestones are issue-tracked and test-backed.
- No performance optimization is accepted without correctness evidence.
- Trace diffs are required for timing-sensitive bug fixes.

## Related Issues

- [#700](https://github.com/TheAnsarya/Nexen/issues/700)
- [#701](https://github.com/TheAnsarya/Nexen/issues/701)
- [#702](https://github.com/TheAnsarya/Nexen/issues/702)
- [#703](https://github.com/TheAnsarya/Nexen/issues/703)
- [#706](https://github.com/TheAnsarya/Nexen/issues/706)
