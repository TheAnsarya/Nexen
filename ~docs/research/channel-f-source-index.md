# Fairchild Channel F Source Index

## Scope

This document tracks external technical references for the Channel F program and links them to Nexen implementation areas.

## Emulator Source References

| Source | URL | Primary Use |
|---|---|---|
| MAME Channel F driver | https://github.com/mamedev/mame/blob/master/src/mame/fairchild/channelf.cpp | Platform wiring, input/panel mapping, BIOS/cart handling patterns |
| MAME F8 CPU | https://github.com/mamedev/mame/blob/master/src/devices/cpu/f8/f8.cpp | Opcode semantics, register model, execution flow, device bus behavior |
| FreeChaF (libretro) | https://github.com/libretro/FreeChaF | Lightweight implementation reference, BIOS expectations, control mapping |

## Hardware and Documentation References

| Source | URL | Primary Use |
|---|---|---|
| Fairchild Channel F overview | https://en.wikipedia.org/wiki/Fairchild_Channel_F | Console models, controls, baseline specs |
| Fairchild F8 overview | https://en.wikipedia.org/wiki/Fairchild_F8 | CPU family structure, ISA context |
| Fairchild F8 brochure (bitsavers) | http://bitsavers.org/components/fairchild/f8/Fairchild_Semiconductor_-_F8_Microprocessor_brochure_-_1975.pdf | Instruction and bus behavior grounding |
| F3851 PSU documentation | https://console5.com/techwiki/images/5/50/Fairchild_F3851.pdf | Program storage unit behavior and integration |
| Channel F patent | https://patents.google.com/patent/US4095791 | Historical hardware architecture details |

## BIOS and ROM Notes

| Item | Value |
|---|---|
| Channel F BIOS PSU1 | `sl31253.bin` |
| Channel F BIOS PSU2 | `sl31254.bin` |
| Channel F II BIOS PSU1 | `sl90025.bin` |

Known BIOS hashes should be validated in Nexen metadata tables during bring-up.

## Mapping to Issue Tree

| Area | Issues |
|---|---|
| Core CPU + bus | #972, #973, #974, #975 |
| Video/audio/input | #977, #978, #979, #980 |
| Save states + accuracy/perf harness | #981, #982, #983 |
| UI/debugger/Lua/Pansy | #997-#1008 |
| TAS/movie/QA/docs/CI | #1009-#1024 |

## Research Quality Rules

- Prefer primary technical docs and source code over tertiary summaries.
- Treat Wikipedia as context, not ground truth for cycle-accurate implementation.
- Capture disagreements between references as explicit TODOs in issue comments.
- Do not copy external emulator code verbatim; re-implement behavior in Nexen style.
