# Genesis Emulator Code Comparison

## Compared Projects

- Genesis Plus GX
- PicoDrive
- ares
- MAME

## Comparative Summary

| Project | Strengths | Trade-offs |
|---|---|---|
| Genesis Plus GX | Deep compatibility and detailed timing treatment; broad practical edge-case coverage | Large, tightly-coupled C codebase can be harder to incrementally transplant |
| PicoDrive | Clear performance-oriented scheduling and cycle conversion strategy | Fast-path complexity can increase reasoning burden during bring-up |
| ares | Clean component boundaries and reusable processor abstractions | Architectural style differs from existing Nexen code patterns, requiring adaptation |
| MAME | Hardware-faithful device models and bus arbitration behavior | Full framework patterns may be too heavyweight to copy directly |

## Key Evidence

- Genesis Plus GX core integration: [https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/shared.h](https://github.com/ekeeke/Genesis-Plus-GX/blob/main/core/shared.h)
- Genesis Plus GX VDP and sound paths: [https://github.com/ekeeke/Genesis-Plus-GX/tree/main/core](https://github.com/ekeeke/Genesis-Plus-GX/tree/main/core)
- PicoDrive scheduler and memory paths: [https://github.com/notaz/picodrive/tree/main/pico](https://github.com/notaz/picodrive/tree/main/pico)
- ares Mega Drive component tree: [https://github.com/ares-emulator/ares/tree/main/ares/md](https://github.com/ares-emulator/ares/tree/main/ares/md)
- MAME Genesis driver and state: [https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.cpp](https://github.com/mamedev/mame/blob/main/src/mame/sega/megadriv.cpp)

## Recommendation For Nexen

1. Use Genesis Plus GX and MAME as primary accuracy references.
2. Use PicoDrive for scheduler simplification and performance tuning ideas.
3. Use ares for clean interface boundaries and modular subsystem separation patterns.
