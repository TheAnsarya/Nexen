# Atari Lynx Emulation — Master Implementation Plan

> **Project:** Nexen Atari Lynx Emulation  
> **Status:** Planning Phase (Session 1 of N)  
> **Created:** 2026-02-15  
> **Epic:** #270 (to be created)  
> **Estimated Effort:** 8–12 sessions across multiple weeks  

---

## 1. Executive Summary

This document is the master plan for adding **first-class Atari Lynx emulation** to Nexen. The Lynx is an ideal system to differentiate Nexen from its Mesen2 predecessor:

- **No independent emulator exists** — every Lynx emulator traces to K. Wilkins' 1996 Handy codebase
- **Relatively simple hardware** — 65C02 CPU, two custom chips (Mikey + Suzy), 64KB RAM, 160×102 display
- **Well-documented** — Epyx developer kit manual covers all hardware behavior
- **Good complexity/value ratio** — complex enough to prove system expertise (two custom chips, sprite engine, math coprocessor), simple enough to implement in reasonable time

### Strategic Goals

1. **Accuracy First** — Cycle-accurate CPU, accurate timer/display timing, hardware bug emulation
2. **Full Debugging** — Mikey/Suzy register viewers, sprite inspector, timer visualization, trace logging
3. **TAS Support** — Movie recording/playback, greenzone, Lua scripting
4. **Pansy Integration** — Metadata export for Lynx ROMs
5. **First Independent Implementation** — Not a Handy fork; clean-room implementation from hardware documentation

---

## 2. Architecture Overview

### 2.1 Atari Lynx Hardware Architecture

```text
                    ┌──────────────────┐
                    │   16 MHz Crystal │
                    └────────┬─────────┘
                             │
              ┌──────────────┴──────────────┐
              │         MIKEY CHIP           │
              │  ┌───────────────────────┐   │
              │  │   65C02 CPU @ 4 MHz   │   │
              │  └───────────────────────┘   │
              │  ┌─────────┐ ┌───────────┐   │
              │  │ 8 Timers│ │  4-ch APU │   │
              │  └─────────┘ └───────────┘   │
              │  ┌─────────┐ ┌───────────┐   │
              │  │ Video   │ │  UART     │   │
              │  │ DMA     │ │ (ComLynx) │   │
              │  └─────────┘ └───────────┘   │
              │  ┌───────────────────────┐   │
              │  │ Interrupt Controller  │   │
              │  └───────────────────────┘   │
              │  ┌───────────────────────┐   │
              │  │   Color Palette (16)  │   │
              │  └───────────────────────┘   │
              └──────────────┬──────────────┘
                             │ 8-bit data bus
              ┌──────────────┼──────────────┐
              │              │              │
    ┌─────────┴──────┐  ┌───┴────┐  ┌──────┴────────┐
    │  SUZY CHIP     │  │ 64 KB  │  │  Cartridge    │
    │  ┌───────────┐ │  │  RAM   │  │  (128KB-1MB)  │
    │  │  Sprite   │ │  └────────┘  └───────────────┘
    │  │  Engine   │ │
    │  └───────────┘ │
    │  ┌───────────┐ │
    │  │ Math      │ │
    │  │ Coprocessor│ │
    │  └───────────┘ │
    │  ┌───────────┐ │
    │  │ Collision │ │
    │  │ Detection │ │
    │  └───────────┘ │
    │  ┌───────────┐ │
    │  │ Joystick  │ │
    │  │ / Buttons │ │
    │  └───────────┘ │
    └────────────────┘
```

### 2.2 Key Specifications

| Component | Detail |
|-----------|--------|
| CPU | 65C02 @ 4 MHz (16 MHz ÷ 4), ~3.6 MHz effective |
| RAM | 64 KB |
| Display | 160×102, 4bpp (16 colors from 4096 palette) |
| Audio | 4 channels, 8-bit DAC, 12-bit LFSR waveforms |
| Input | D-pad + A/B + Opt1/Opt2 + Pause (9 buttons) |
| Carts | 128KB–1MB, shift-register addressing |
| Boot ROM | 512 bytes ($FE00–$FFFF) inside Mikey |
| Revisions | Lynx I (65SC02, mono) / Lynx II (65C02, stereo) |

### 2.3 Nexen Integration Points

Based on the architecture audit (see `~docs/plans/nexen-architecture-audit.md`):

- **~35 new C++ files** in `Core/Lynx/`
- **~15 new C# files** in `UI/` (config, debugger views, state structs)
- **~40 existing files** need modification (enum additions, switch cases)
- **New enum values**: `ConsoleType::Lynx`, `CpuType::Lynx`, `MemoryType::Lynx*`, `RomFormat::Lynx`

---

## 3. Implementation Phases

### Phase 1: Foundation & CPU (Sessions 1–2)
**Goal:** Boot ROM executes, CPU passes instruction tests

| Task | Description | Files |
|------|-------------|-------|
| 1.1 | Register Lynx in all enums (C++ + C#) | ~12 files |
| 1.2 | Create `LynxConsole` skeleton (IConsole) | `Core/Lynx/LynxConsole.h/.cpp` |
| 1.3 | Implement 65C02 CPU core | `Core/Lynx/LynxCpu.h/.cpp` |
| 1.4 | Create memory manager with MAPCTL | `Core/Lynx/LynxMemoryManager.h/.cpp` |
| 1.5 | Implement boot ROM loading | `Core/Lynx/LynxTypes.h`, console |
| 1.6 | Add ROM loading (.lnx, .lyx, .o) | `Core/Lynx/LynxCart.h/.cpp` |
| 1.7 | Wire into Emulator.cpp for ROM detection | `Core/Shared/Emulator.cpp` |
| 1.8 | Basic serialization (save states) | `Serialize()` in all components |
| 1.9 | CPU instruction tests | `Core.Tests/` |

**Milestone:** Load a Lynx ROM, CPU starts executing, can step through instructions in debugger.

### Phase 2: Mikey — Timers, Interrupts, Display (Sessions 3–4)
**Goal:** Screen displays something, timers fire correctly

| Task | Description | Files |
|------|-------------|-------|
| 2.1 | Implement 8 timer system with linking | `Core/Lynx/LynxTimers.h/.cpp` |
| 2.2 | Implement interrupt controller | Part of Mikey |
| 2.3 | Implement display DMA (frame buffer → video out) | `Core/Lynx/LynxMikey.h/.cpp` |
| 2.4 | Implement color palette registers (16 from 4096) | Part of Mikey |
| 2.5 | Implement DISPCTL, DISPADR, PBKUP | Part of Mikey |
| 2.6 | Create video filter | `Core/Lynx/LynxDefaultVideoFilter.h/.cpp` |
| 2.7 | Implement screen rotation support | DISPCTL bit handling |
| 2.8 | Timer accuracy tests | `Core.Tests/` |

**Milestone:** Games boot and show title screens, HBL/VBL interrupts firing.

### Phase 3: Suzy — Sprites, Math, Collision (Sessions 5–6)
**Goal:** Games are playable, sprites render correctly

| Task | Description | Files |
|------|-------------|-------|
| 3.1 | Implement sprite engine (SCB chain) | `Core/Lynx/LynxSuzy.h/.cpp` |
| 3.2 | All 8 sprite types | Part of Suzy |
| 3.3 | Sprite collision detection | Part of Suzy |
| 3.4 | 16×16 hardware multiply | Part of Suzy |
| 3.5 | 32÷16 hardware divide | Part of Suzy |
| 3.6 | Joystick/button input register | Part of Suzy |
| 3.7 | Cart shift-register addressing | `Core/Lynx/LynxCart.h/.cpp` |
| 3.8 | EEPROM save data | `Core/Lynx/LynxEeprom.h/.cpp` |
| 3.9 | Sprite rendering tests | `Core.Tests/` |

**Milestone:** Multiple games fully playable with graphics and input working.

### Phase 4: Audio — Mikey Sound Engine (Session 7)
**Goal:** Game audio works correctly

| Task | Description | Files |
|------|-------------|-------|
| 4.1 | 4 audio channels with LFSR | `Core/Lynx/APU/LynxApu.h/.cpp` |
| 4.2 | Audio timer integration | Timers + APU |
| 4.3 | Stereo support (Lynx II) | APU stereo registers |
| 4.4 | Attenuation registers | APU |
| 4.5 | Audio output filtering | APU |
| 4.6 | ComLynx UART (basic) | `Core/Lynx/LynxSerial.h/.cpp` |

**Milestone:** Games have correct audio output.

### Phase 5: UI Integration (Session 8)
**Goal:** Full UI support for Lynx in Nexen

| Task | Description | Files |
|------|-------------|-------|
| 5.1 | LynxConfig + ConfigView | `UI/Config/LynxConfig.cs`, AXAML |
| 5.2 | Controller/input configuration | Config + ControlManager |
| 5.3 | File association (.lnx, .lyx) | FolderHelper, FileDialogHelper |
| 5.4 | Menu integration | MainMenuViewModel |
| 5.5 | Shortcut handling | ShortcutHandler |
| 5.6 | Game data management | GameDataManager |
| 5.7 | Console override config | ConsoleOverrideConfig |
| 5.8 | Firmware (boot ROM) management | FirmwareType |

**Milestone:** Lynx games can be opened, configured, and played from the UI.

### Phase 6: Debugger (Sessions 9–10)
**Goal:** Full debugging support including Mikey/Suzy register inspection

| Task | Description | Files |
|------|-------------|-------|
| 6.1 | LynxDebugger (IDebugger) | `Core/Lynx/Debugger/LynxDebugger.h/.cpp` |
| 6.2 | 65C02 disassembler | `Core/Lynx/Debugger/LynxDisUtils.h/.cpp` |
| 6.3 | Trace logger | `Core/Lynx/Debugger/LynxTraceLogger.h/.cpp` |
| 6.4 | Assembler (reuse Base6502) | `Core/Lynx/Debugger/LynxAssembler.h` |
| 6.5 | Event manager (timer/DMA/IRQ events) | `Core/Lynx/Debugger/LynxEventManager.h/.cpp` |
| 6.6 | PPU/sprite tools | `Core/Lynx/Debugger/LynxPpuTools.h/.cpp` |
| 6.7 | Dummy CPU for breakpoint prediction | `Core/Lynx/Debugger/DummyLynxCpu.h/.cpp` |
| 6.8 | Expression evaluator tokens | `Core/Debugger/ExpressionEvaluator.Lynx.cpp` |
| 6.9 | CPU status view (UI) | `LynxStatusView.axaml` + VM |
| 6.10 | **Mikey register viewer** | `LynxRegisterViewer.cs` |
| 6.11 | **Suzy register viewer** | Part of register viewer |
| 6.12 | Memory mapping display | MemoryMappingViewModel |
| 6.13 | C# state interop structs | `LynxState.cs` |

**Milestone:** Full debugging with stepping, breakpoints, register inspection, sprite viewer, timer visualization.

### Phase 7: Movie/TAS Support (Session 11)
**Goal:** Full TAS editor support for Lynx

| Task | Description | Files |
|------|-------------|-------|
| 7.1 | Lynx input recording in movie format | MovieConverter |
| 7.2 | Greenzone support for Lynx | GreenzoneManager |
| 7.3 | Piano roll display with Lynx buttons | PianoRollControl |
| 7.4 | Lua API for Lynx input | TasLuaApi |
| 7.5 | Controller input encoding | ControllerInput |
| 7.6 | TAS tests | Tests/ |

**Milestone:** Record, play back, and edit TAS movies for Lynx games.

### Phase 8: Pansy & Polish (Session 12)
**Goal:** Complete integration, documentation, testing

| Task | Description | Files |
|------|-------------|-------|
| 8.1 | Pansy metadata export for Lynx | PansyExporter |
| 8.2 | Debug workspace / label support | DebugWorkspaceManager |
| 8.3 | Comprehensive test suite | All test projects |
| 8.4 | Performance benchmarks | Benchmarks/ |
| 8.5 | Accuracy testing against Handy | Comparison tool |
| 8.6 | Hardware revision differences (Lynx I vs II) | CPU + config |
| 8.7 | User documentation | ~docs/ |
| 8.8 | Developer documentation | ~docs/ |

**Milestone:** Production-ready Lynx emulation with full feature parity.

---

## 4. Epic Structure

### Master Epic: Atari Lynx Emulation (#270)
**The master epic containing all Lynx work.**

### Epic: Lynx CPU — 65C02 Core (#271)
Sub-issues:

- #279 [270.1] 65C02 opcode implementation (all 256 opcodes)
- #280 [270.2] Addressing modes (16 modes)
- #281 [270.3] Cycle-accurate timing
- #282 [270.4] BCD mode
- #283 [270.5] Interrupt handling (IRQ/BRK)
- #284 [270.6] 65SC02 vs 65C02 variant support
- #285 [270.7] CPU instruction test suite
- #286 [270.8] DummyCpu for breakpoint prediction

### Epic: Lynx Mikey Chip (#272)
Sub-issues:

- #287 [270.9] Timer system (8 timers, linking chains)
- #288 [270.10] Interrupt controller (INTRST/INTSET)
- #289 [270.11] Video DMA (display buffer → LCD)
- #295 [270.12] Color palette registers (16 from 4096)
- #290 [270.13] Audio engine (4 channels, LFSR)
- #291 [270.14] Stereo support (Lynx II)
- #292 [270.15] UART / ComLynx serial
- #293 [270.16] System control registers (MAPCTL etc.)
- #294 [270.17] Mikey register viewer (debugger UI)

### Epic: Lynx Suzy Chip (#273)
Sub-issues:

- #296 [270.18] Sprite engine (SCB chain walking)
- #297 [270.19] 8 sprite types (normal, boundary, shadow, XOR, etc.)
- #298 [270.20] Sprite collision detection
- #299 [270.21] Hardware math (16×16 multiply, 32÷16 divide)
- #300 [270.22] Joystick/button input registers
- #301 [270.23] Cart shift-register address generator
- #302 [270.24] Suzy register viewer (debugger UI)

### Epic: Lynx Nexen Integration (#274)
Sub-issues:

- #303 [270.25] Enum registration (ConsoleType, CpuType, MemoryType, etc.)
- #304 [270.26] Console skeleton (IConsole implementation)
- #305 [270.27] Memory manager with MAPCTL overlay
- #306 [270.28] Cartridge loading (.lnx, .lyx, .o, BS93)
- #307 [270.29] Boot ROM support
- #308 [270.30] Save state serialization
- #309 [270.31] Video filter
- #310 [270.32] Build system integration (vcxproj, makefile)

### Epic: Lynx UI & Configuration (#275)
Sub-issues:

- #311 [270.33] LynxConfig + ConfigView (AXAML)
- #312 [270.34] Controller/input configuration
- #313 [270.35] File associations and ROM loading
- #314 [270.36] Menu integration
- #315 [270.37] Firmware (boot ROM) management
- #316 [270.38] Screen rotation UI option
- #317 [270.39] EEPROM save support

### Epic: Lynx Debugger (#276)
Sub-issues:

- #318 [270.40] LynxDebugger (IDebugger implementation)
- #319 [270.41] 65C02 disassembler
- #320 [270.42] Trace logger
- #321 [270.43] Assembler
- #322 [270.44] Event manager
- #323 [270.45] Sprite/tile viewer tools
- #324 [270.46] CPU status view (AXAML)
- #325 [270.47] Mikey register viewer with timer visualization
- #326 [270.48] Suzy register viewer with sprite state
- #327 [270.49] Memory mapping display
- #328 [270.50] Expression evaluator tokens
- #329 [270.51] C# state interop structs

### Epic: Lynx Movie/TAS Support (#277)
Sub-issues:

- #331 [270.52] Lynx input format in MovieConverter
- #332 [270.53] Piano roll with Lynx button layout
- #333 [270.54] Greenzone save state integration
- #334 [270.55] Lua scripting API support
- #335 [270.56] TAS tests and benchmarks

### Epic: Lynx Pansy & Documentation (#278)
Sub-issues:

- #336 [270.57] Pansy metadata export for Lynx platform
- #337 [270.58] Debug workspace / label support
- #338 [270.59] Comprehensive test suite
- #339 [270.60] Performance benchmarks
- #340 [270.61] Accuracy comparison methodology
- #341 [270.62] Lynx emulation documentation

---

## 5. Technical Decisions

### 5.1 CPU: Fresh 65C02 vs Reuse NES 6502

**Decision: Write a fresh 65C02 core.**

Rationale:

- The NES uses an NMOS 6502 with known bugs that the 65C02 specifically FIXES
- Maintaining backward compatibility with NES 6502 while adding 65C02 features would be messy
- The 65C02 has ~25 new instructions + new addressing modes + different interrupt handling
- Clean separation allows independent optimization and testing
- However, we CAN reuse `Base6502Assembler` patterns and test infrastructure

### 5.2 Mikey/Suzy: Separate vs Combined

**Decision: Separate classes for Mikey and Suzy.**

Rationale:

- They are physically separate chips with different address ranges
- Different complexity (Mikey is more complex with timers+audio+video)
- Separate classes enable separate register viewer tabs in debugger
- Matches Handy architecture for reference

### 5.3 Display Model: Per-Scanline vs Frame Buffer

**Decision: Frame buffer model (matches hardware).**

Rationale:

- The Lynx display hardware reads a pre-rendered frame buffer via DMA
- Games render sprites to a frame buffer, then swap buffers
- This is fundamentally different from NES/SNES scanline rendering
- The "PPU" for Lynx is really just Suzy's sprite engine writing to RAM
- Video DMA (Mikey) just reads completed frames

### 5.4 Accuracy Target

**Decision: Cycle-accurate with documented hardware bugs.**

Rationale:

- Section 13 of the technical report lists 13 known hardware bugs
- Several are significant (CPU sleep bug, UART level sensitivity, math sign extension)
- We will emulate ALL documented bugs for accuracy
- Configuration option to choose Lynx I (65SC02) vs Lynx II (65C02) behavior

### 5.5 Boot ROM Strategy

**Decision: Support both real boot ROM and HLE (High-Level Emulation).**

Rationale:

- Boot ROM is 512 bytes, copyrighted
- Many users won't have it
- HLE can skip the boot sequence (like Handy does)
- Real boot ROM support for maximum accuracy
- Firmware management UI for providing boot ROM file

---

## 6. Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| CPU accuracy | High | Extensive test suite, compare against real hardware traces |
| Timer timing | High | Per-tick timer simulation, don't use event scheduling shortcuts |
| Sprite edge cases | Medium | Test with all known Lynx commercial games |
| No test ROM suite | High | Create our own test ROMs, use cycle_check programs |
| Math coprocessor | Medium | Bit-exact implementation from Epyx spec |
| ComLynx multiplayer | Low | Implement basic UART, defer networked multiplayer |
| Performance | Low | 4 MHz CPU is trivial for modern hardware |

---

## 7. Reference Materials

### Primary Sources (for implementation)

1. **Epyx Developer Kit Manual** (monlynx.de) — Authoritative hardware reference
2. **~docs/plans/atari-lynx-technical-report.md** — Our synthesized technical report
3. **~docs/plans/lynx-emulation-research.md** — Emulator landscape analysis

### Secondary Sources (for verification)

1. **Handy source code** (zlib license) — Reference implementation for behavior verification
2. **Beetle-Lynx ROM database** — Game rotation and compatibility data
3. **42Bastian's cycle_check** — Real-hardware timing verification programs

### Nexen Architecture

1. **nexen-architecture-audit.md** — Complete registration/integration checklist
2. **Core/WS/** — Reference console implementation (most similar in complexity)

---

## 8. Success Criteria

1. **CPU**: All 256 opcodes implemented, cycle-accurate timing verified
2. **Display**: 160×102 output with correct colors and rotation
3. **Audio**: 4-channel audio output matching Handy reference
4. **Compatibility**: 80%+ of commercial Lynx library boots and plays
5. **Debugger**: Full stepping, breakpoints, register viewers for Mikey AND Suzy
6. **TAS**: Movie recording, playback, editing, greenzone, Lua API
7. **Pansy**: Metadata export with correct platform identification
8. **Performance**: <5% CPU usage on modern hardware at full speed
9. **Tests**: 95%+ code coverage for CPU, timer, and sprite engine
10. **Documentation**: Complete developer and user docs

---

## 9. Session Plan

| Session | Phase | Focus | Expected Output |
|---------|-------|-------|-----------------|
| **1** | Planning | Research, architecture, issue creation | This document, all GitHub issues |
| **2** | Phase 1 | Enum registration, console skeleton, 65C02 CPU | CPU executes instructions |
| **3** | Phase 1–2 | Memory manager, cart loading, timer skeleton | ROMs load, basic execution |
| **4** | Phase 2 | Mikey timers, interrupts, display DMA | Screen output visible |
| **5** | Phase 3 | Suzy sprite engine (basic rendering) | Sprites appear on screen |
| **6** | Phase 3 | Suzy math, collision, cart addressing | Games become playable |
| **7** | Phase 4 | Audio engine, stereo, UART | Games have sound |
| **8** | Phase 5 | UI integration, config, file handling | Full UI support |
| **9** | Phase 6 | Debugger core (stepping, breakpoints, disasm) | Basic debugging works |
| **10** | Phase 6 | Debugger UI (register viewers, sprite tools) | Full debug experience |
| **11** | Phase 7 | Movie/TAS support | TAS editing for Lynx |
| **12** | Phase 8 | Pansy, testing, documentation, polish | Production ready |

---

## 10. File Inventory — New Files to Create

### Core/Lynx/ (~35 files)

```text
Core/Lynx/
├── LynxConsole.h            # IConsole implementation - main console class
├── LynxConsole.cpp
├── LynxCpu.h                # 65C02 CPU core
├── LynxCpu.cpp
├── LynxCpu.Instructions.h   # Instruction decode tables
├── LynxMikey.h              # Mikey chip (timers, audio controller, video DMA, UART, IRQ)
├── LynxMikey.cpp
├── LynxSuzy.h               # Suzy chip (sprites, math, collision, input)
├── LynxSuzy.cpp
├── LynxMemoryManager.h      # Memory map with MAPCTL overlay
├── LynxMemoryManager.cpp
├── LynxControlManager.h     # BaseControlManager - input handling
├── LynxControlManager.cpp
├── LynxController.h         # Button definitions for Lynx pad
├── LynxCart.h               # Cartridge with shift-register addressing
├── LynxCart.cpp
├── LynxEeprom.h             # 93C46/66/86 EEPROM for save data
├── LynxEeprom.cpp
├── LynxSerial.h             # ComLynx UART
├── LynxSerial.cpp
├── LynxDefaultVideoFilter.h # Video output filter
├── LynxDefaultVideoFilter.cpp
├── LynxTypes.h              # All state structs and constants
├── APU/
│   ├── LynxApu.h            # Audio processing unit (4 channels)
│   ├── LynxApu.cpp
│   ├── LynxApuChannel.h     # Single audio channel (LFSR, integration mode)
│   └── LynxApuChannel.cpp
└── Debugger/
    ├── LynxDebugger.h        # IDebugger implementation
    ├── LynxDebugger.cpp
    ├── LynxTraceLogger.h     # Trace log formatting
    ├── LynxTraceLogger.cpp
    ├── LynxDisUtils.h        # 65C02 disassembly
    ├── LynxDisUtils.cpp
    ├── LynxAssembler.h       # 65C02 assembler (reuse base patterns)
    ├── LynxEventManager.h    # Event timeline (timer/DMA/IRQ events)
    ├── LynxEventManager.cpp
    ├── LynxPpuTools.h        # Sprite viewer, palette viewer
    ├── LynxPpuTools.cpp
    ├── DummyLynxCpu.h        # Prediction CPU for breakpoints
    └── DummyLynxCpu.cpp
```

### UI Files (~15 files)

```text
UI/Config/
├── LynxConfig.cs               # Lynx-specific settings

UI/Views/
├── LynxConfigView.axaml        # Configuration UI
├── LynxConfigView.axaml.cs

UI/ViewModels/
├── LynxConfigViewModel.cs      # Config view model

UI/Interop/ConsoleState/
├── LynxState.cs                # C# state struct mirrors

UI/Debugger/StatusViews/
├── LynxStatusView.axaml        # CPU register display
├── LynxStatusView.axaml.cs
├── LynxStatusViewModel.cs

UI/Debugger/RegisterViewer/
├── LynxRegisterViewer.cs       # Mikey + Suzy register tabs

Core/Debugger/
├── ExpressionEvaluator.Lynx.cpp  # Expression evaluator for Lynx regs
```

### Existing Files to Modify (~40 files)

See `~docs/plans/nexen-architecture-audit.md` for the complete list.

---

## 11. Dependencies and Prerequisites

### Before Starting Phase 1

- [ ] All research documents complete (this session)
- [ ] GitHub epics and issues created
- [ ] Code-plans for CPU and console skeleton written

### Before Starting Phase 2

- [ ] CPU core passes basic instruction tests
- [ ] Memory manager handles MAPCTL correctly
- [ ] Cart loading works for .lnx files

### Before Starting Phase 3

- [ ] Timer system fires HBL/VBL interrupts correctly
- [ ] Display DMA reads frame buffer and produces output
- [ ] At least 3 games reach their title screen

### Before Starting Phase 5 (UI)

- [ ] Core emulation runs games end-to-end
- [ ] Save states work (serialization)
- [ ] Input captured correctly

### Before Starting Phase 6 (Debugger)

- [ ] Core emulation stable
- [ ] All state structs defined (LynxTypes.h)
- [ ] C# state interop structs match C++

---

## Appendix A: Lynx Game Compatibility Targets

### Tier 1 — Must Work (Phase 3 milestone)

- California Games (rotation: left)
- Chip's Challenge
- Gates of Zendocon
- Todd's Adventures in Slime World
- Blue Lightning

### Tier 2 — Should Work (Phase 4 milestone)

- Batman Returns
- Ninja Gaiden III
- Rampart
- Shadow of the Beast
- Warbirds

### Tier 3 — Stretch Goals

- Lemmings
- Rygar
- Scrapyard Dog
- Zarlor Mercenary
- All homebrew titles

---

## Appendix B: Lynx Register Quick Reference

### Mikey I/O ($FD00–$FDFF)

| Range | Function |
|-------|----------|
| $FD00–$FD1F | 8 Timers (4 bytes each) |
| $FD20–$FD3F | 4 Audio channels (8 bytes each) |
| $FD40–$FD49 | Attenuation + stereo |
| $FD50–$FD5F | Unused |
| $FD80–$FD81 | Interrupt control (INTRST/INTSET) |
| $FD84–$FD8B | Palette (Green0-F, Blue/Red0-F) |
| $FD90–$FD97 | Serial / UART |
| $FD92–$FD95 | Display control (DISPCTL, PBKUP, DISPADR) |
| $FD9C–$FD9D | Misc control (IODIR, IODAT) |
| $FDFE–$FDFF | Audio/video off (CPUSLEEP) |

### Suzy I/O ($FC00–$FCFF)

| Range | Function |
|-------|----------|
| $FC00–$FC01 | TMPADR (temp address for sprites) |
| $FC02–$FC03 | TILTACUM |
| $FC04–$FC05 | HOFF (horizontal offset) |
| $FC06–$FC07 | VOFF (vertical offset) |
| $FC08–$FC09 | VIDBAS (video base address) |
| $FC0A–$FC0B | COLLBAS (collision base address) |
| $FC0C–$FC0D | VIDADR (video address) |
| $FC0E–$FC0F | COLLADR (collision address) |
| $FC10–$FC11 | SCBNEXT (next SCB pointer) |
| $FC12–$FC13 | SPRDLINE (sprite data line) |
| $FC14–$FC15 | HPOSSTRT (horizontal position start) |
| $FC16–$FC17 | VPOSSTRT (vertical position start) |
| $FC18–$FC19 | SPRHSIZ (horizontal sprite size) |
| $FC1A–$FC1B | SPRVSIZ (vertical sprite size) |
| $FC1C–$FC1D | STRETCH (sprite stretch) |
| $FC1E–$FC1F | TILT (sprite tilt) |
| $FC20–$FC27 | SPRDOFF / SPRVPOS / COLLOFF / VSIZACUM |
| $FC28–$FC2F | HSIZOFF / VSIZOFF / SCBADR / PROCADR |
| $FC52–$FC53 | MATHD/C (math result) |
| $FC54–$FC55 | MATHB/A (math operands) |
| $FC60–$FC6B | MATHJ-E (extended math) |
| $FC80–$FC83 | SPRCTL0/1, SPRCOLL, SPRINIT |
| $FC88–$FC8B | SUZYBUSEN, SPRGO, SPRSYS |
| $FC90 | JOYSTICK |
| $FC91 | SWITCHES |
| $FC92 | RCART0/1 (cart address) |
| $FCB0–$FCB3 | LEDS/PPORTSTAT/PPORTDATA |
