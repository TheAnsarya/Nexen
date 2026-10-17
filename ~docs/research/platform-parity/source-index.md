# Platform Parity Source Index

This index provides primary references used in the Atari 2600 and Genesis research tracks.

## Atari 2600 Hardware References

- Atari 2600 overview: [https://en.wikipedia.org/wiki/Atari_2600](https://en.wikipedia.org/wiki/Atari_2600)
- MOS 6507 overview: [https://en.wikipedia.org/wiki/MOS_Technology_6507](https://en.wikipedia.org/wiki/MOS_Technology_6507)
- TIA overview: [https://en.wikipedia.org/wiki/Television_Interface_Adapter](https://en.wikipedia.org/wiki/Television_Interface_Adapter)
- MOS 6532 RIOT overview: [https://en.wikipedia.org/wiki/MOS_Technology_6532](https://en.wikipedia.org/wiki/MOS_Technology_6532)

## Atari 2600 Emulator Codebases

- Stella repository: [https://github.com/stella-emu/stella](https://github.com/stella-emu/stella)
- Gopher2600 repository: [https://github.com/JetSetIlly/Gopher2600](https://github.com/JetSetIlly/Gopher2600)
- ares Atari 2600 CPU entry: [https://github.com/ares-emulator/ares/blob/main/ares/a26/cpu/cpu.cpp](https://github.com/ares-emulator/ares/blob/main/ares/a26/cpu/cpu.cpp)
- ares Atari 2600 CPU interface: [https://github.com/ares-emulator/ares/blob/main/ares/a26/cpu/cpu.hpp](https://github.com/ares-emulator/ares/blob/main/ares/a26/cpu/cpu.hpp)

## Genesis Hardware and Ecosystem References

- Sega Genesis overview: [https://en.wikipedia.org/wiki/Sega_Genesis](https://en.wikipedia.org/wiki/Sega_Genesis)
- Sega Mega Drive technical references (community): [https://segaretro.org/Sega_Mega_Drive/Technical_specifications](https://segaretro.org/Sega_Mega_Drive/Technical_specifications)
- Genesis hardware and emulation research forum: [https://gendev.spritesmind.net/forum/](https://gendev.spritesmind.net/forum/)

## Genesis Emulator Codebases

- Genesis Plus GX repository: [https://github.com/ekeeke/Genesis-Plus-GX](https://github.com/ekeeke/Genesis-Plus-GX)
- PicoDrive repository: [https://github.com/notaz/picodrive](https://github.com/notaz/picodrive)
- ares repository: [https://github.com/ares-emulator/ares](https://github.com/ares-emulator/ares)
- MAME repository: [https://github.com/mamedev/mame](https://github.com/mamedev/mame)

## Key Genesis Code Pointers

### Genesis Plus GX

- Core wiring include graph: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/shared.h](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/shared.h)
- VDP interface and timing globals: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/vdp_ctrl.h](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/vdp_ctrl.h)
- VDP DMA/FIFO timing internals: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/vdp_ctrl.c](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/vdp_ctrl.c)
- 68k memory map setup: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/genesis.c](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/genesis.c)
- Z80 memory and 68k bus interaction: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/memz80.c](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/memz80.c)
- Sound scheduler and YM core plumbing: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/sound/sound.c](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/sound/sound.c)

### PicoDrive

- Core frame sync and scheduling path: [https://github.com/notaz/picodrive/blob/main/pico/pico_cmn.c](https://github.com/notaz/picodrive/blob/main/pico/pico_cmn.c)
- 68k abstraction and alternate cores: [https://github.com/notaz/picodrive/blob/main/pico/sek.c](https://github.com/notaz/picodrive/blob/main/pico/sek.c)
- Shared timing conversion macros: [https://github.com/notaz/picodrive/blob/main/pico/pico_int.h](https://github.com/notaz/picodrive/blob/main/pico/pico_int.h)
- Main memory and YM access paths: [https://github.com/notaz/picodrive/blob/main/pico/memory.c](https://github.com/notaz/picodrive/blob/main/pico/memory.c)
- YM2612 implementation details: [https://github.com/notaz/picodrive/blob/main/pico/sound/ym2612.c](https://github.com/notaz/picodrive/blob/main/pico/sound/ym2612.c)

### ares

- Mega Drive aggregate architecture includes: [https://github.com/ares-emulator/ares/blob/main/ares/md/md.hpp](https://github.com/ares-emulator/ares/blob/main/ares/md/md.hpp)
- Main 68000 CPU component: [https://github.com/ares-emulator/ares/blob/main/ares/md/cpu/cpu.cpp](https://github.com/ares-emulator/ares/blob/main/ares/md/cpu/cpu.cpp)
- Main CPU interface contract: [https://github.com/ares-emulator/ares/blob/main/ares/md/cpu/cpu.hpp](https://github.com/ares-emulator/ares/blob/main/ares/md/cpu/cpu.hpp)
- APU (Z80 domain) component: [https://github.com/ares-emulator/ares/blob/main/ares/md/apu/apu.cpp](https://github.com/ares-emulator/ares/blob/main/ares/md/apu/apu.cpp)
- System-level sync policy: [https://github.com/ares-emulator/ares/blob/main/ares/md/system/system.cpp](https://github.com/ares-emulator/ares/blob/main/ares/md/system/system.cpp)

### MAME

- Genesis driver implementation: [https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.cpp](https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.cpp)
- Genesis state declarations: [https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.h](https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.h)

## Notes On Source Availability

- Some Sega Retro pages are intermittently blocked by anti-bot protections in automated fetch contexts.
- When a direct page fetch fails, prefer repository mirrors or alternate technical references and document the fallback.
