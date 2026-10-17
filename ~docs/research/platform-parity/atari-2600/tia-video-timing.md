# Atari 2600 TIA Video and Timing

## Why This Is Critical

TIA behavior is the main source of hard compatibility bugs on Atari 2600. Many games rely on precise line timing and delayed register effects.

## Key Behaviors To Model

- Register write side effects with delay semantics.
- WSYNC/RSYNC behavior and scanline control.
- HBlank and visible region transitions.
- Collision and object state updates at cycle-accurate boundaries.

## Research Findings

- Stella maps mirrored TIA space aggressively and runs explicit TIA update routines before register handling.
- Stella uses an internal delay queue and frame manager model for TIA lifecycle behavior.
- Gopher2600 documents and implements TIA around explicit color-clock stepping with delayed events.

## Reference Evidence

- Stella TIA core: [https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIA.cxx](https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIA.cxx)
- Stella TIA interface and internals: [https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIA.hxx](https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIA.hxx)
- Stella frame manager modules: [https://github.com/stella-emu/stella/tree/main/src/emucore/tia/frame-manager](https://github.com/stella-emu/stella/tree/main/src/emucore/tia/frame-manager)
- Gopher2600 TIA core loop: [https://github.com/jetsetilly/gopher2600/blob/main/hardware/tia/tia.go](https://github.com/jetsetilly/gopher2600/blob/main/hardware/tia/tia.go)
- Gopher2600 TIA design notes: [https://github.com/jetsetilly/gopher2600/blob/main/hardware/tia/doc.go](https://github.com/jetsetilly/gopher2600/blob/main/hardware/tia/doc.go)

## Proposed Nexen Strategy

1. Implement a minimal TIA timing skeleton with explicit per-color-clock stepping.
2. Add delayed register event model before adding rendering optimizations.
3. Build line-level trace assertions early to catch subtle drift.
