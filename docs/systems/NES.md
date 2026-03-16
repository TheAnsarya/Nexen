# NES / Famicom

## Overview

Nexen provides full NES/Famicom emulation with cycle-accurate CPU and PPU behavior, mapper support, save states, rewind, movie recording, and TAS editor integration.

## Hardware Focus

- CPU: 6502-compatible core
- Video: PPU timing and sprite/background composition
- Audio: APU channels and mixer behavior
- Cartridge: Mapper-driven memory layout and bank switching

## Using NES Features in Nexen

- Load ROM: File menu and select a .nes file.
- Save states: Use F1, Shift+F1, F4, Shift+F4.
- Rewind: Hold or tap Backspace depending on rewind settings.
- TAS workflow: Use movie recording and TAS editor for frame-level input edits.

## Accuracy and Compatibility

- Target: gameplay-correct and debugger-friendly behavior.
- Use [NES Core](../../~docs/NES-CORE.md) for implementation details.

## Related Links

- [Systems Index](README.md)
- [Feature Guides](../README.md)
- [Movie Format](../NEXEN_MOVIE_FORMAT.md)
