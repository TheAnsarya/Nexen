# Genesis Scheduler and Clocking

## Objective

Define a deterministic synchronization model across M68000, Z80, VDP, and audio chips.

## Research Findings

- Genesis Plus GX records explicit timing values and line-oriented cycle variables for VDP and CPU domains.
- PicoDrive uses conversion macros between 68k and Z80 cycles and explicit sync calls in frame flow.
- ares sets platform-specific synchronization cadence at system load level.

## Reference Evidence

- Genesis Plus GX timing state variables: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/vdp_ctrl.c](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/vdp_ctrl.c)
- Genesis Plus GX line/frame setup: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/genesis.c](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/genesis.c)
- PicoDrive sync loop: [https://github.com/notaz/picodrive/blob/main/pico/pico_cmn.c](https://github.com/notaz/picodrive/blob/main/pico/pico_cmn.c)
- PicoDrive cycle ratio macros: [https://github.com/notaz/picodrive/blob/main/pico/pico_int.h](https://github.com/notaz/picodrive/blob/main/pico/pico_int.h)
- ares system sync policy: [https://github.com/ares-emulator/ares/blob/main/ares/md/system/system.cpp](https://github.com/ares-emulator/ares/blob/main/ares/md/system/system.cpp)

## Nexen Guidance

1. Introduce a single timing authority per frame.
2. Synchronize secondary domains at deterministic checkpoints.
3. Emit synchronization trace stamps for regression harnesses.
