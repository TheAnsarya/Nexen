# Genesis VDP Rendering and DMA

## Scope

VDP timing, DMA windows, FIFO behavior, and interrupt timing are primary determinants of visual correctness.

## Research Findings

- Genesis Plus GX has extensive timing variables and DMA/FIFO handling paths in VDP control files.
- MAME VDP hookup highlights interrupt wiring and 68k halt interactions via device callbacks.
- Multiple projects emphasize HBlank/VBlank timing as a high-risk compatibility area.

## Reference Evidence

- Genesis Plus GX VDP interface variables: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/vdp_ctrl.h](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/vdp_ctrl.h)
- Genesis Plus GX VDP DMA/FIFO timing logic: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/vdp_ctrl.c](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/vdp_ctrl.c)
- MAME VDP integration in Genesis machine config: [https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.cpp](https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.cpp)
- ares CPU interrupt path to VDP: [https://github.com/ares-emulator/ares/blob/main/ares/md/cpu/cpu.cpp](https://github.com/ares-emulator/ares/blob/main/ares/md/cpu/cpu.cpp)

## Nexen Guidance

1. Start with correctness-first line and interrupt timing model.
2. Add DMA and FIFO validation probes to trace output.
3. Gate title compatibility milestones on VDP trace stability.
