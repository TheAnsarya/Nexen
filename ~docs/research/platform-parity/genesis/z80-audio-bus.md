# Genesis Z80 Audio Bus and Arbitration

## Why This Matters

Z80 bus request/reset behavior is a major compatibility axis for audio and title stability.

## Research Findings

- Genesis Plus GX models Z80<->68k bus interactions directly in memory handlers.
- MAME documents and implements explicit bus-request and reset register behavior at canonical addresses.
- PicoDrive maintains explicit cycle conversion and synchronization points between 68k and Z80.

## Reference Evidence

- Genesis Plus GX Z80 memory and bus access: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/memz80.c](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/memz80.c)
- Genesis Plus GX 68k-side Z80 bus access hooks: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/mem68k.c](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/mem68k.c)
- MAME 68k bus request/reset handlers: [https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.cpp](https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.cpp)
- MAME Genesis state variables: [https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.h](https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.h)
- PicoDrive cycle conversion macros: [https://github.com/notaz/picodrive/blob/main/pico/pico_int.h](https://github.com/notaz/picodrive/blob/main/pico/pico_int.h)

## Nexen Guidance

1. Implement bus request and reset semantics before advanced audio polish.
2. Add explicit ownership assertions for 68k/Z80 windows.
3. Build dedicated regression tests for bus handoff edge cases.
