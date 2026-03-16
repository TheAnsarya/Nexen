# SNES / Super Famicom

## Overview

Nexen includes full SNES emulation support with CPU, PPU, audio subsystem behavior, debugger tooling, TAS workflows, and movie support.

## Hardware Focus

- CPU: 65816 execution model
- Video: SNES PPU pipeline and rendering behavior
- Audio: SPC subsystem integration
- Cartridge: banked ROM layouts and coprocessor-aware behavior

## Using SNES Features in Nexen

- Load ROM: File menu and select a .sfc or .smc file.
- Save and load states: F1 and F4 workflows.
- Rewind and frame advance: Backspace and backtick in TAS workflows.
- TAS editor: edit per-frame inputs, branch timelines, and compare routes.

## Accuracy and Compatibility

- Designed for stable gameplay, debugging, and TAS production.
- See [SNES Core](../../~docs/SNES-CORE.md) for low-level architecture.

## Related Links

- [Systems Index](README.md)
- [TAS Editor Manual](../TAS-Editor-Manual.md)
- [Movie System Guide](../Movie-System.md)
