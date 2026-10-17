# Atari 2600 Emulator Code Survey

## Scope

Compare architecture signals from mature implementations to identify safe adoption patterns for Nexen.

## Stella (Primary Reference)

Observed characteristics:

- Monolithic but highly detailed TIA implementation with explicit delayed event and frame management concepts.
- Tight coupling between TIA updates and CPU/system cycle state.
- Mature RIOT model with extensive debugger-facing introspection.

### Source Code Analysis (2026-03-20)

Detailed analysis of Stella source files:

#### M6502 CPU (`src/emucore/M6502.hxx`)

- Boolean status flags: individual `bool` fields for N, V, B, D, I, notZ, C (rather than packed byte)
- System cycles per CPU cycle = 1 (Stella tracks system cycles, TIA runs 3 color clocks per system cycle)
- Ghost reads: CPU performs dummy reads on indexed addressing page crosses
- Execution model: `execute(cycles)` loop with cycle-accurate peek/poke
- Debugger support: breakpoints, timers, conditional traps on PC/address
- Halt support: stops execution for WSYNC (TIA tells CPU to halt until end of scanline)

#### TIA (`src/emucore/tia/TIA.hxx`)

- 228 color clocks per scanline = 76 CPU cycles × 3 clocks
- 160 visible pixels, 68 hblank clocks
- `H_PIXEL = 160`, `H_CYCLES = 76`, `CYCLE_CLOCKS = 3`, `H_CLOCKS = 228`, `H_BLANK_CLOCKS = 68`
- Frame buffer: 160×320 buffer, 320×240 viewable
- 7 graphical objects: Background, Playfield, Player0, Player1, Missile0, Missile1, Ball
- Each object is a separate class with its own state and rendering logic
- `DelayQueue` (16 slots): delayed writes to TIA registers (registers don't take effect immediately)
- Priority encoder: determines which object's pixel is visible based on priority mode
- HState enum: blank, frame — tracks horizontal blanking state
- HMOVE handling: extends hblank by 8 pixels on HMOVE strobe, with per-object counters
- Collision detection: 15-bit bitfield covering all object pair combinations
- Frame manager: handles VSYNC/VBLANK timing, auto-detects frame height
- Phosphor mode: phosphor persistence (blends current frame with previous)
- Scanline jitter: models real hardware timing inconsistencies
- Fixed debug colors: assigns distinct colors to each TIA object for debugging

#### TIA Constants (`src/emucore/tia/TIAConstants.hxx`)

- Write registers: VSYNC=0x00 through CXCLR=0x2c (45 total)
- Read registers: CXM0P=0x00 through INPT5=0x0d (14 total)
- Collision bits: 15 combinations (M0-P1, M0-P0, M1-P0, M1-P1, P0-PF, etc.)
- Color indices: indexed by object type (BK=0, PF=1, P0=2, P1=3, M0=4, M1=5, BL=6, HBLANK=7)
- Frame dimensions: `frameBufferHeight = 320`, viewable area 320×240

#### M6532 RIOT (`src/emucore/M6532.hxx`)

- 128 bytes RAM at $80-$ff
- Timer with configurable divider: ÷1, ÷8, ÷64, ÷1024
- DDR for ports A and B: direction register controls input/output per bit
- Interrupt flags: `TimerBit = 0x80`, `PA7Bit = 0x40`
- PA7 edge detection: latch on positive or negative edge (configurable)
- Timer interrupt: fires on underflow, reads clear interrupt

### Key Differences: Stella vs. Nexen

| Aspect | Stella | Nexen |
|--------|--------|-------|
| CPU flags | Boolean fields | Packed byte |
| CPU opcodes | Full 6502 ISA + illegals | ~20 opcodes |
| TIA architecture | Separate object classes | Monolithic state struct |
| TIA DelayQueue | 16-slot delayed writes | Not implemented |
| Collision detection | 15-bit comprehensive | 8 collision registers |
| HMOVE | Full per-object counter | Basic apply on strobe |
| Frame manager | Auto-detect frame height | Fixed 262 scanlines |
| Phosphor mode | Yes | No |
| Controller support | Full (joystick, paddle, keypad) | Stubbed |
| Audio output | Connected to SDL | Generated but not connected |
| Mappers | 40+ | 8 + fallback |

References:

- [https://github.com/stella-emu/stella/blob/main/src/emucore/M6502.hxx](https://github.com/stella-emu/stella/blob/main/src/emucore/M6502.hxx)
- [https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIA.hxx](https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIA.hxx)
- [https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIA.cxx](https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIA.cxx)
- [https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIAConstants.hxx](https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIAConstants.hxx)
- [https://github.com/stella-emu/stella/blob/main/src/emucore/M6532.hxx](https://github.com/stella-emu/stella/blob/main/src/emucore/M6532.hxx)

## Gopher2600

Observed characteristics:

- Clear package-level separation (TIA, RIOT, memory bus layers).
- Explicit step model and strong inline design commentary.
- Mapper implementations with practical compatibility notes.

References:

- [https://github.com/jetsetilly/gopher2600/blob/main/hardware/tia/tia.go](https://github.com/jetsetilly/gopher2600/blob/main/hardware/tia/tia.go)
- [https://github.com/jetsetilly/gopher2600/blob/main/hardware/riot/riot.go](https://github.com/jetsetilly/gopher2600/blob/main/hardware/riot/riot.go)
- [https://github.com/jetsetilly/gopher2600/blob/main/hardware/step.go](https://github.com/jetsetilly/gopher2600/blob/main/hardware/step.go)
- [https://github.com/jetsetilly/gopher2600/tree/main/hardware/memory/cartridge](https://github.com/jetsetilly/gopher2600/tree/main/hardware/memory/cartridge)

## ares

Observed characteristics:

- Componentized CPU/thread structure compatible with multi-system architecture goals.
- Useful as a minimal reference for integration boundaries.

References:

- [https://github.com/ares-emulator/ares/blob/main/ares/a26/cpu/cpu.cpp](https://github.com/ares-emulator/ares/blob/main/ares/a26/cpu/cpu.cpp)
- [https://github.com/ares-emulator/ares/blob/main/ares/a26/cpu/cpu.hpp](https://github.com/ares-emulator/ares/blob/main/ares/a26/cpu/cpu.hpp)

## Recommendations For Nexen

1. Adopt explicit cycle-step contracts similar to Gopher2600 for clarity and testability.
2. Borrow detailed TIA edge-case validation ideas from Stella.
3. Keep component boundaries close to existing Nexen architecture, using ares as a structural reference.
