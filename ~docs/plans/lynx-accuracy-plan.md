# Lynx Emulation Accuracy Comparison Methodology

## Overview

This document defines the methodology for verifying and measuring the accuracy of Nexen's Atari Lynx emulation core. The goal is to achieve **80%+ commercial game compatibility** and document emulation accuracy relative to real hardware and reference emulators.

## Reference Sources

### Primary References
1. **Real hardware** — Atari Lynx I/II behavior (gold standard)
2. **Handy** — Reference open-source Lynx emulator by K. Wilkins (most mature)
3. **Mednafen** — Highly accurate multi-system emulator with Lynx support

### Test ROM Sources
1. **42Bastian's test ROMs** — cycle_check, timer_test, sprite_test (timing verification)
2. **Retro homebrew scene** — Various hardware test ROMs
3. **Custom test ROMs** — Purpose-built for edge case verification

## Test Categories

### 1. CPU Accuracy (65C02)

| Test Area | Method | Pass Criteria |
|-----------|--------|---------------|
| All documented opcodes | Exhaustive logic tests | All 256 opcode slots verified |
| ADC/SBC binary mode | 256×256×2 exhaustive | Matches reference for all inputs |
| ADC/SBC decimal (BCD) mode | 65C02-specific Z/N flag behavior | CMOS behavior (not NMOS) |
| Addressing modes | Per-mode test coverage | All 13 modes correct |
| IRQ/BRK behavior | Break flag, D-flag clearing | 65C02 semantics |
| WAI/STP | Stop state transitions | Correct halt behavior |
| Cycle counts | Per-instruction timing | Match 65C02 datasheet |
| Page-crossing penalties | Branch/indexed modes | Extra cycle on page cross |

**Current coverage:** 94 CPU logic tests implemented (exhaustive ADC/SBC/shift/rotate/compare/BIT/TSB/TRB).

### 2. Timer Accuracy

| Test Area | Method | Pass Criteria |
|-----------|--------|---------------|
| Timer countdown | cycle_check ROM | Correct tick rates |
| Cascading chains | T0→T2→T4, T1→T3→T5→T7 | Borrow propagation correct |
| Prescaler values | All 8 clock sources | Divide ratios match spec |
| IRQ generation | Timer-based IRQ firing | Correct timing to CPU cycle |
| Timer reload | Backup value behavior | Proper reload on borrow |
| Timer linking | Linked vs prescaler mode | Switch behavior correct |

### 3. Video Accuracy

| Test Area | Method | Pass Criteria |
|-----------|--------|---------------|
| Palette colors | Visual comparison | 12-bit color mapping correct |
| Scanline timing | HSync/VSync timing | 105 scanlines per frame |
| Framebuffer addressing | DISPADR register | Correct memory read pattern |
| Display dimensions | 160×102 output | No off-by-one errors |
| VBlank timing | IRQ-based frame sync | Games sync correctly |

### 4. Sprite Accuracy (Suzy)

| Test Area | Method | Pass Criteria |
|-----------|--------|---------------|
| SCB chain walking | Multi-sprite scenes | All sprites rendered |
| 1/2/3/4 BPP decoding | Mixed-BPP sprite test | Correct pixel output |
| Sprite flipping | H/V mirror flags | Correct orientation |
| Collision detection | Collision depository | Correct collision pairs |
| Pen index remapping | Palette remap table | Colors mapped correctly |
| Sprite types | All 7 types | Background/normal/XOR etc. |
| Stretch/tilt | Scaling sprites | Correct geometry |
| Reload depth | SPRCTL1 bits 5:4 (ReloadDepth 0-3) | Field persistence correct |
| Bus contention | CPU-Suzy timing | Frame timing unaffected |

### 5. Audio Accuracy

| Test Area | Method | Pass Criteria |
|-----------|--------|---------------|
| LFSR output | Feedback polynomial | Waveform matches reference |
| Volume control | Per-channel volume | 7-bit signed level |
| Stereo attenuation | ATTEN_A-D registers | L/R balance correct |
| Timer-driven frequency | Channel frequency range | Audible range coverage |
| DAC mode (ch3) | PCM playback | Clean sample output |
| Integration mode | Accumulation behavior | Matches reference |

### 6. Memory Mapping

| Test Area | Method | Pass Criteria |
|-----------|--------|---------------|
| MAPCTL overlays | All bit combinations | Correct region switching |
| Suzy enable/disable | $FC00-$FCFF access | RAM/register switching |
| Mikey enable/disable | $FD00-$FDFF access | RAM/register switching |
| Boot ROM | $FE00-$FFF7 | ROM/RAM switching |
| Vector space | $FFFA-$FFFF | Vector/RAM switching |

### 7. Cartridge / EEPROM

| Test Area | Method | Pass Criteria |
|-----------|--------|---------------|
| Bank switching | Dual-bank games | Correct bank selection |
| Page counter | Sequential read | Proper advancement |
| Page sizes | 256/512/1024/2048 | All sizes handled |
| EEPROM R/W | 93C46/56/66/76/86 | Data persists correctly |
| EEPROM protocol | All Microwire commands | READ/WRITE/ERASE/EWEN/EWDS |

## Commercial Game Compatibility

### Testing Methodology

1. **Boot test** — Game reaches title screen
2. **Playability test** — First stage/level completable
3. **Extended test** — 10+ minutes of gameplay without issues
4. **Audio test** — Sound effects and music present and correct
5. **EEPROM test** — Save/load functions work (where applicable)

### Compatibility Tiers

| Tier | Definition | Target |
|------|-----------|--------|
| **Perfect** | Indistinguishable from hardware | Best effort |
| **Playable** | Fully completable, minor visual/audio glitches | 80%+ of library |
| **Boots** | Reaches title screen but has issues | 95%+ of library |
| **Broken** | Fails to boot or immediately crashes | <5% of library |

### Known Commercial Games to Test (Priority)

| Game | EEPROM | Rotation | Notes |
|------|--------|----------|-------|
| California Games | No | None | Popular, multi-event |
| Chip's Challenge | No | None | Puzzle, good logic test |
| Todd's Adventures in Slime World | 93C46 | Left | EEPROM + rotation |
| Blue Lightning | No | None | Scaling sprites |
| Electrocop | No | None | 3D perspective |
| Gates of Zendocon | No | None | Side-scroller |
| Rampage | No | None | Destructible environments |
| Rygar | No | None | Action platformer |
| Shanghai | No | None | Tile matching |
| Warbirds | No | None | Sprite-heavy |
| Lemmings | No | None | Complex AI/logic |
| Batman Returns | No | None | Beat-em-up |
| Stun Runner | No | None | 3D racing |

## Intentional Deviations

Document any cases where Nexen intentionally deviates from hardware:

| Area | Deviation | Reason |
|------|-----------|--------|
| WAI/STP recovery | Always halt (no Suzy bus check) | Simplification, no known game impact |
| ComLynx UART | Stub only (no serial data transfer) | Networking not implemented yet |
| Boot ROM | Skip if not present | Convenience for ROM loading |

## Testing Workflow

### For Each Test ROM

```
1. Run test ROM in Nexen
2. Run same ROM in Handy/Mednafen
3. Compare output (visual, audio, timing)
4. Document differences in compatibility table
5. Create GitHub issue for each failure
6. Prioritize fixes by commercial game impact
```

### For Regression Testing

```
1. Maintain list of known-working games
2. After each core change, verify subset still works
3. Automated: capture screenshots at key frames, compare hashes
4. Track any regressions as high-priority issues
```

## Metrics

| Metric | Current | Target |
|--------|---------|--------|
| CPU logic test coverage | 94 tests (exhaustive) | Maintained |
| Commercial games booting | TBD | 95%+ |
| Commercial games playable | TBD | 80%+ |
| Timer accuracy | TBD | Pass cycle_check |
| Sprite rendering | TBD | Visual parity |
| Audio accuracy | TBD | Audible correctness |

## Tools

- **Nexen debugger** — Breakpoints, trace, memory view for investigating failures
- **Event viewer** — IRQ/DMA timeline for timing analysis
- **Sprite viewer** — SCB chain inspection for rendering issues
- **Handy source code** — Reference implementation for behavior comparison
- **BizHawk Lynx core** — Additional reference (also based on Handy)
