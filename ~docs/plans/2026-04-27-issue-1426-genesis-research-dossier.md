# Issue #1426 - Genesis Research Dossier

## Scope

Actionable research baseline for Sega Genesis implementation work in Nexen, with source references, comparative emulator module map, and validation corpus planning.

## Authoritative Hardware References

### Core System and Bus

- Sega Genesis Software Manual (official programming guide)
- Sega Mega Drive/Genesis Hardware Manual (memory map, buses, interrupts)
- Motorola M68000 Programmer's Reference Manual
- Zilog Z80 CPU User Manual
- Sega 315-5313 VDP documentation and register behavior references (community-corroborated)

### Video

- VDP register-level docs and timing notes (DMA modes, H/V counter behavior, CRAM/VRAM/VSRAM access windows)
- Nemesis/Exodus documentation threads for VDP timing edge cases and DMA contention behavior

### Audio

- Yamaha YM2612 programming and timing reference
- TI SN76489 PSG reference and implementation notes
- Genesis bus arbitration notes for Z80<->68k audio ownership and reset windows

### Input and I/O

- 3-button and 6-button pad protocol documentation (TH line transition timing)
- Controller/multitap protocol notes and edge timing behavior

## Comparative Emulator Module Map

## ares

- Genesis system orchestration and scheduler
- 68000 core integration boundaries
- VDP pipeline and DMA sequencing model
- Z80 and FM/PSG audio ownership split

## BlastEm

- Cycle-aware timing model for 68k/Z80/VDP interactions
- Practical VDP and DMA edge-case handling in compatibility-focused paths
- Mature handling of controller protocol timing

## MAME

- Device-model decomposition for Genesis components
- Explicit memory-map and bus-device boundaries
- Useful for cross-checking hardware-facing abstractions

## Genesis Plus GX

- High-compatibility console behavior handling
- Well-documented user-visible edge behavior and mapper/peripheral support
- Good cross-check source for practical game compatibility behavior

## Mednafen

- Deterministic emulation discipline and timing-oriented design patterns
- Useful reference for state serialization expectations and replay determinism

## Proposed Nexen Module Mapping

- `Core/Genesis/` as the primary integration boundary for:
  - M68000 CPU integration layer
  - Z80 coprocessor/audio control boundary
  - VDP timing + DMA + interrupt service boundary
  - Input I/O state machines (3-button/6-button polling)
- Validation seams:
  - Transcript/digest determinism paths in runtime handshakes
  - Save-state parity checkpoints for mixed-width I/O sequences
  - Bus arbitration and reset sequencing probes

## Validation Corpus

### CPU and Timing

- 68000 instruction and interrupt behavior ROM suites
- Z80 ownership/reset handoff timing checks
- H/V interrupt cadence validation patterns

### VDP and DMA

- DMA fill/copy/VRAM stress ROMs
- HBlank/VBlank boundary behavior ROMs
- CRAM/VSRAM access timing edge tests

### Audio

- YM2612 register/timing conformance ROMs
- PSG tonal/timing test ROMs
- Mixed Z80+FM contention stress cases

### Input

- 3-button and 6-button handshake protocol test ROMs
- TH transition cadence and glitch resistance checks

### Compatibility Smoke Set

- Sonic the Hedgehog (timing/input baseline)
- Gunstar Heroes (VDP and DMA stress)
- Streets of Rage 2 (audio/input cadence)
- Phantasy Star IV (save-state replay sanity)

## Follow-Up Implementation Issues

- [#1427](https://github.com/TheAnsarya/Nexen/issues/1427) - Genesis core implementation track
- [#1428](https://github.com/TheAnsarya/Nexen/issues/1428) - Genesis UX/debugger track
- [#1460](https://github.com/TheAnsarya/Nexen/issues/1460) - Genesis baseline conformance
- [#1463](https://github.com/TheAnsarya/Nexen/issues/1463) - Sega CD integration track
- [#1465](https://github.com/TheAnsarya/Nexen/issues/1465) - Genesis-family conformance track
- [#1466](https://github.com/TheAnsarya/Nexen/issues/1466) - Genesis-family debugger track

## Execution Notes

This dossier is intentionally implementation-oriented: each reference category maps to concrete subsystem slices and measurable test gates.
