# Atari 2600 Memory Map and Mirroring

## Problem Statement

Atari 2600 relies heavily on mirrored address spaces. Incorrect decode logic causes widespread false negatives in compatibility testing.

## Required Behaviors

- Correct mirroring of TIA registers.
- Correct RIOT RAM/I/O address decode behavior.
- Correct cartridge mapping windows and mapper handoff points.

## Research Findings

- Stella explicitly installs mirrored TIA access across expected lower address space ranges.
- Gopher2600 models TIA and RIOT chip memory spaces with dedicated bus-facing memory abstractions.

## Reference Evidence

- Stella mirrored TIA install path: [https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIA.cxx](https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIA.cxx)
- Gopher2600 TIA chip memory mapping: [https://github.com/jetsetilly/gopher2600/blob/main/hardware/memory/vcs/tia.go](https://github.com/jetsetilly/gopher2600/blob/main/hardware/memory/vcs/tia.go)
- Gopher2600 RIOT chip memory mapping: [https://github.com/jetsetilly/gopher2600/blob/main/hardware/memory/vcs/riot.go](https://github.com/jetsetilly/gopher2600/blob/main/hardware/memory/vcs/riot.go)

## Nexen Acceptance Criteria

- Address mirror tests pass for TIA and RIOT.
- No incorrect register aliasing under random access fuzzing.
- Mapper entry points remain deterministic under mirrored writes.
