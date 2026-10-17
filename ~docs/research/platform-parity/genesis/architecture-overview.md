# Genesis Architecture Overview

## Core Subsystems

- Main CPU: Motorola 68000 domain.
- Secondary CPU: Z80 domain for audio and bus-accessed operations.
- Video: VDP state machine with timing-sensitive DMA/FIFO behavior.
- Audio: YM2612 FM + SN76489 PSG scheduling.

## Research Signals

- Genesis Plus GX exposes a mature C-style architecture with explicit memory maps and timing variables.
- PicoDrive emphasizes performance-aware scheduling with explicit cycle conversion utilities.
- ares uses a componentized, thread-oriented architecture that aligns well with clean subsystem boundaries.
- MAME keeps hardware-faithful device and bus models, useful for arbitration behavior references.

## Reference Evidence

- Genesis Plus GX shared core includes: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/shared.h](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/shared.h)
- PicoDrive common frame scheduling: [https://github.com/notaz/picodrive/blob/main/pico/pico_cmn.c](https://github.com/notaz/picodrive/blob/main/pico/pico_cmn.c)
- ares Mega Drive aggregate architecture: [https://github.com/ares-emulator/ares/blob/main/ares/md/md.hpp](https://github.com/ares-emulator/ares/blob/main/ares/md/md.hpp)
- MAME Genesis driver: [https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.cpp](https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.cpp)

## Nexen Architectural Direction

1. Keep subsystem APIs explicit and testable.
2. Separate correctness-first timing model from optimization layers.
3. Add deterministic trace capture at CPU, VDP, and bus arbitration boundaries.
