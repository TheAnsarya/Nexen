# Game Boy Advance

## Overview

Nexen supports GBA emulation with save states, rewind, TAS editing, and debugger tooling for iterative testing and route development.

## Hardware Focus

- CPU: ARM7TDMI behavior and instruction flow
- Video: handheld graphics pipeline and frame timing
- Audio: GBA audio pipeline integration
- Cartridge: GBA ROM and memory region handling

## Using GBA Features in Nexen

- Open ROM: select a .gba file.
- Save state workflows: quick save, browser, and designated slots.
- Rewind usage: use Backspace with rewind enabled.
- TAS workflows: record, edit, branch, and replay inputs frame-by-frame.

## Accuracy and Compatibility

- Prioritizes deterministic behavior for movie/TAS use cases.
- See [GB/GBA Core](../../~docs/GB-GBA-CORE.md) for internals.

## Related Links

- [Systems Index](README.md)
- [Movie System Guide](../Movie-System.md)
- [Debugging Guide](../Debugging.md)
