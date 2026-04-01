# WonderSwan / WonderSwan Color

## Overview

Nexen includes WonderSwan and WonderSwan Color support with movie tools, TAS editor workflows, save states, rewind, and debugger tooling.

## Hardware Focus

- CPU: V30MZ behavior
- Video: handheld display and timing behavior
- Audio: WonderSwan audio pipeline behavior
- Cartridge: ROM layout and memory handling

## Using WS Features in Nexen

- Open ROM from the File menu.
- Use save states for checkpointed testing.
- Use rewind and frame stepping for route analysis.
- Use TAS editor to edit and branch timeline inputs.

## Accuracy and Compatibility

- Designed for deterministic playback and route iteration.
- See [SMS/PCE/WS Core](../../~docs/SMS-PCE-WS-CORE.md) for implementation details.

## Test Coverage Snapshot

Current WonderSwan native test coverage includes targeted suites for:

- CPU behavior
- Memory and IRQ behavior
- APU and HyperVoice state behavior
- PPU behavior
- Timer and DMA behavior
- EEPROM behavior
- Save state behavior
- Type and coverage-focused guard tests

Primary source files are under `Core.Tests/WS/` and include:

- `WsCpuTests.cpp`
- `WsMemoryTests.cpp`
- `WsApuTests.cpp`
- `WsPpuTests.cpp`
- `WsTimerDmaTests.cpp`
- `WsEepromTests.cpp`
- `WsStateTests.cpp`
- `WsCoverageTests.cpp`

Remaining WS work should prioritize hardware-faithful edge cases and controller/integration scenarios without regressing determinism.

## Related Links

- [Systems Index](README.md)
- [TAS Editor Manual](../TAS-Editor-Manual.md)
- [Save States Guide](../Save-States.md)
