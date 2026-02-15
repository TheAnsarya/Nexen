# Atari Lynx Implementation Prompts

> A series of prompts to run IN ORDER to implement Atari Lynx emulation in Nexen.
> Each prompt is self-contained and should be run as a separate chat session.
> All work must be done on the `features-atari-lynx` branch.
>
> **IMPORTANT:** Before running any prompt, ensure you are on the `features-atari-lynx` branch.
> After each prompt completes, commit the work and verify it builds.

---

## Pre-Flight

```
Before starting, verify: git branch --show-current
Expected: features-atari-lynx
If not: git checkout features-atari-lynx
```

---

## PROMPT 1: Core Enum Registration (C++ Side)

```
[Nexen] On the features-atari-lynx branch, add Atari Lynx to all C++ core enums. This is the FOUNDATION that everything else depends on. Make these exact changes:

1. Core/Shared/SettingTypes.h:
   - Add `Lynx = 7` to `ConsoleType` enum (after Ws = 6)
   - Add `LynxDebuggerEnabled = (1 << 13)` to `DebuggerFlags` enum
   - Add a `LynxConfig` struct with: ControllerConfig Controller, bool AutoRotate=true, bool BlendFrames=false

2. Core/Shared/CpuType.h:
   - Add `Lynx` to `CpuType` enum (after Ws)
   - Update `CpuTypeUtilities::GetCpuTypeCount()` to return `(int)CpuType::Lynx + 1`

3. Core/Shared/MemoryType.h:
   - Add these MemoryType entries (before `None`):
     LynxMemory, LynxPrgRom, LynxWorkRam, LynxBootRom,
     LynxMikeyRegisters, LynxSuzyRegisters, LynxVideoRam, LynxSaveRam

4. Core/Shared/RomInfo.h:
   - Add `Lynx` to `RomFormat` enum (after Ws)

5. Core/Shared/EmuSettings.h/.cpp:
   - Add `LynxConfig _lynx` member
   - Add `SetLynxConfig(LynxConfig&)` and `GetLynxConfig()` methods
   - Add `case ConsoleType::Lynx:` in all switch blocks in EmuSettings.cpp (Serialize, GetOverscan, etc.)

6. Core/Debugger/DebugUtilities.h:
   - Add `case CpuType::Lynx:` returning `MemoryType::LynxMemory` in GetCpuMemoryType()
   - Add `case CpuType::Lynx:` returning 4 in GetProgramCounterSize() (16-bit addresses)
   - Add all LynxMemory types in ToCpuType() returning CpuType::Lynx
   - Update GetLastCpuType() to return CpuType::Lynx
   - Update GetLastCpuMemoryType() to return MemoryType::LynxMemory
   - Add Lynx types to IsPpuMemory(), IsRom(), IsVolatileRam() as appropriate

Build and verify: no errors. Commit: "feat(lynx): Register Lynx in all C++ core enums - #303"
```

---

## PROMPT 2: C# UI Enum Mirrors

```
[Nexen] On features-atari-lynx, mirror all C++ enum changes to the C# UI side. The C++ enums were already added in the previous prompt. Now add matching C# entries:

1. UI/Interop/EmuApi.cs:
   - Add `Lynx = 7` to `ConsoleType` enum
   - Add `Lynx` to `RomFormat` enum (same position as C++)

2. UI/Interop/DebugApi.cs:
   - Add `Lynx` to `CpuType` enum (after Ws)
   - Add all Lynx MemoryType entries in EXACT same positions as C++ (LynxMemory, LynxPrgRom, LynxWorkRam, LynxBootRom, LynxMikeyRegisters, LynxSuzyRegisters, LynxVideoRam, LynxSaveRam)

3. UI/Interop/ConfigApi.cs:
   - Add `LynxDebuggerEnabled = 1 << 13` to `DebuggerFlags` enum
   - Add `[DllImport(DllPath)] public static extern void SetLynxConfig(InteropLynxConfig config);`

4. UI/Interop/ConsoleTypeExtensions.cs:
   - Add `ConsoleType.Lynx => CpuType.Lynx` in GetMainCpuType()
   - Add `ConsoleType.Lynx => false` in SupportsCheats()

5. UI/Interop/CpuTypeExtensions.cs:
   - Add `CpuType.Lynx =>` case to ALL 15+ switch expressions (ToMemoryType, GetVramMemoryType, GetSpriteRamMemoryType, GetPrgRomMemoryType, GetSystemRamType, GetAddressSize, GetByteCodeSize, GetDebuggerFlag, GetConsoleType, SupportsAssembler, SupportsFunctionList, SupportsCallStack, SupportsMemoryMappings, GetNopOpCode, CanAccessMemoryType)

6. Create UI/Config/LynxConfig.cs modeled on WsConfig.cs

7. UI/Config/Configuration.cs:
   - Add `[Reactive] public LynxConfig Lynx { get; set; } = new();`

8. UI/Utilities/GameDataManager.cs:
   - Add `ConsoleType.Lynx => "LYNX"` in folder name switch

9. InteropDLL/ConfigApiWrapper.cpp:
   - Add `SetLynxConfig` DLL export

Build both C++ and C# projects. Commit: "feat(lynx): Mirror Lynx enums to C# UI - #303"
```

---

## PROMPT 3: LynxTypes.h — State Structs and Constants

```
[Nexen] On features-atari-lynx, create Core/Lynx/LynxTypes.h with ALL the state structures and constants needed for the Lynx emulator. Reference the lynx-code-plans.md and lynx-subsystems-deep-dive.md files in ~docs/plans/ for the exact designs.

Create Core/Lynx/LynxTypes.h containing:

1. Enums: LynxModel (LynxI, LynxII), LynxRotation (None, Left, Right)

2. Timer state: struct LynxTimerState { uint8_t BackupValue, ControlA, Count, ControlB; uint64_t LastTick; bool TimerDone, Linked; }

3. Audio channel state: struct LynxAudioChannelState { uint8_t Volume, Feedback, Output, ShiftRegister, BackupValue, Control, Counter; bool Integrate; }

4. CPU state: struct LynxCpuState { uint16_t PC; uint8_t A, X, Y, SP, PS; uint64_t CycleCount; bool NmiFlag, IrqSource; bool StoppedForStp, StoppedForWai; }

5. Mikey state: struct LynxMikeyState { LynxTimerState Timers[8]; LynxAudioChannelState AudioChannels[4]; uint8_t IrqEnabled, IrqPending; uint16_t DisplayAddress; uint8_t DisplayControl; uint8_t Palette[16]; uint8_t PaletteGreen[16]; uint8_t PaletteBR[16]; uint8_t SerialControl; uint16_t CurrentScanline; }

6. Suzy state: struct LynxSuzyState { uint16_t SCBAddress; uint8_t SpriteControl0, SpriteControl1; bool SpriteBusy; uint16_t MathA, MathB; uint32_t MathResult; int16_t MathC, MathD; uint16_t MathE, MathF; uint8_t Joystick, Switches; uint8_t CollisionBuffer[16]; }

7. Memory manager state: struct LynxMemoryManagerState { uint8_t Mapctl; bool VectorSpaceVisible, MikeySpaceVisible, SuzySpaceVisible, RomSpaceVisible; }

8. Cart info: struct LynxCartInfo { char Name[33]; char Manufacturer[17]; uint32_t RomSize; uint16_t PageSizeBank0, PageSizeBank1; LynxRotation Rotation; bool HasEeprom; uint16_t EepromSize; }

9. Overall state: struct LynxState { LynxModel Model; LynxCpuState Cpu; LynxMikeyState Mikey; LynxSuzyState Suzy; LynxMemoryManagerState MemoryManager; LynxCartInfo CartInfo; }

10. Timing constants: constexpr values for master clock (16MHz), CPU divider (4), display width (160), height (102), FPS (~75.0), cycles per frame, etc.

Also add Core/Lynx/LynxTypes.h to Core.vcxproj. Commit: "feat(lynx): Add LynxTypes.h with all state structs - #303"
```

---

## PROMPT 4: LynxConsole Skeleton

```
[Nexen] On features-atari-lynx, create the LynxConsole class — the main console implementation. This should be a COMPILABLE SKELETON that stubs all IConsole methods. Reference Core/WS/WsConsole.h/.cpp as the pattern.

Create Core/Lynx/LynxConsole.h and Core/Lynx/LynxConsole.cpp:

1. Forward-declare all component classes (LynxCpu, LynxMikey, LynxSuzy, LynxMemoryManager, LynxCart, LynxControlManager)

2. Implement ALL IConsole pure virtuals as stubs:
   - GetSupportedExtensions() → return {".lnx", ".o"}
   - GetSupportedSignatures() → return {"LYNX"}
   - GetConsoleType() → ConsoleType::Lynx
   - GetCpuTypes() → {CpuType::Lynx}
   - GetRegion() → ConsoleRegion::Ntsc
   - GetMasterClockRate() → 16000000 (16 MHz crystal)
   - GetFps() → 75.0
   - GetPpuFrame() → return frame buffer info (160×102, ARGB)
   - GetRomFormat() → RomFormat::Lynx
   - RunFrame() → stub (empty loop)
   - Reset() → stub
   - LoadRom() → parse LNX header, allocate RAM, create components
   - SaveBattery() → delegate to EEPROM
   - Serialize() → SV all components
   - GetConsoleState() → fill LynxState struct
   - GetAbsoluteAddress() / GetRelativeAddress() → address translation stubs

3. Add `TryLoadRom<LynxConsole>()` call in Core/Shared/Emulator.cpp (with #include)

4. Add source files to Core.vcxproj

5. Make it COMPILE (stub implementations are fine, but no linker errors)

Build and verify. Commit: "feat(lynx): Add LynxConsole skeleton - #304"
```

---

## PROMPT 5: LynxMemoryManager — MAPCTL and Memory Map

```
[Nexen] On features-atari-lynx, create the LynxMemoryManager class. This handles the Lynx's unique MAPCTL-based memory overlay system. Reference the lynx-subsystems-deep-dive.md for details.

Create Core/Lynx/LynxMemoryManager.h and Core/Lynx/LynxMemoryManager.cpp:

1. Memory map: 64KB address space with overlays controlled by MAPCTL register at $FFF9:
   - Bit 0: Suzy space ($FC00-$FCFF) visible when 0
   - Bit 1: Mikey space ($FD00-$FDFF) visible when 0
   - Bit 2: ROM space ($FE00-$FFF7 + vectors $FFF8-$FFFF) visible when 0
   - Bit 3: Vector space ($FFFA-$FFFF from ROM) visible when 0
   - When overlays are disabled, RAM is visible at those addresses

2. Implement Read/Write methods:
   - Read(uint16_t addr) → check MAPCTL overlays, dispatch to Mikey/Suzy/ROM/RAM
   - Write(uint16_t addr, uint8_t value) → check overlays, dispatch appropriately
   - Handle MAPCTL write at $FFF9

3. Debug support:
   - GetAbsoluteAddress(AddressInfo& info) for debugger
   - GetRelativeAddress(AddressInfo& info) for debugger
   - RegisterHandlers for memory type mapping (MemoryType::LynxWorkRam, LynxPrgRom, etc.)

4. State: MAPCTL value, pointers to RAM/ROM arrays, pointers to Mikey/Suzy/BootRom

5. Implement ISerializable::Serialize() for save states

6. Add to Core.vcxproj

Build and verify. Commit: "feat(lynx): Add LynxMemoryManager with MAPCTL overlays - #305"
```

---

## PROMPT 6: LynxCpu — 65C02 Core (Opcodes + Addressing)

```
[Nexen] On features-atari-lynx, create the LynxCpu class — a full 65C02 (WDC) CPU core. This is NOT the same as the NES 6502 (NMOS) — the 65C02 fixes bugs and adds instructions. Do NOT reuse the NES CPU. Reference the Lynx technical report in ~docs/plans/.

Create Core/Lynx/LynxCpu.h and Core/Lynx/LynxCpu.cpp:

1. **Registers:** A, X, Y, SP, PC, P (status flags: NV-BDIZC)

2. **All 256 opcodes** — 65C02 instruction set:
   - Standard 6502: ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK, BVC, BVS, CLC, CLD, CLI, CLV, CMP, CPX, CPY, DEC, DEX, DEY, EOR, INC, INX, INY, JMP, JSR, LDA, LDX, LDY, LSR, NOP, ORA, PHA, PHP, PLA, PLP, ROL, ROR, RTI, RTS, SBC, SEC, SED, SEI, STA, STX, STY, TAX, TAY, TSX, TXA, TXS, TYA
   - 65C02 additions: BRA, PHX, PHY, PLX, PLY, STZ, TRB, TSB, WAI, STP
   - 65C02 additions: (zp) addressing mode (not just (zp,X) and (zp),Y)
   - 65C02 BCD fix: ADC/SBC in decimal mode set flags correctly (unlike NMOS 6502)
   - 65C02 bug fixes: JMP ($xxFF) crosses page correctly, RMW instructions don't write-back twice
   - All undefined opcodes are NOP (various sizes, not illegal ops like NMOS)

3. **13 addressing modes:**
   Immediate, ZeroPage, ZeroPage+X, ZeroPage+Y, Absolute, Absolute+X, Absolute+Y,
   (Indirect,X), (Indirect),Y, (Indirect), Relative, Absolute Indirect, (Absolute,X)

4. **Cycle-accurate timing** — each instruction has correct cycle count including page-crossing penalties

5. **IRQ handling** — Check IRQ line after each instruction, push PC and P, vector from $FFFE/$FFFF
   - No NMI on Lynx! The NMI vector exists but is unused.

6. **WAI and STP** — WAI halts until IRQ, STP halts until reset

7. Memory interface: Read(addr)/Write(addr,val) through LynxMemoryManager

8. Implement ISerializable::Serialize()

9. Add to Core.vcxproj

This is a large file (~2000+ lines). Build and verify. Commit: "feat(lynx): Add LynxCpu 65C02 core - #279, #280"
```

---

## PROMPT 7: LynxCart — ROM Loading and LNX Header Parsing

```
[Nexen] On features-atari-lynx, create the LynxCart class for cartridge emulation and LNX ROM format parsing.

Create Core/Lynx/LynxCart.h and Core/Lynx/LynxCart.cpp:

1. LNX Header Parsing:
   - Read 64-byte header: magic "LYNX", bank sizes, cart name, manufacturer, rotation
   - Validate magic bytes
   - Extract ROM data (everything after header)
   - Determine EEPROM type from header or ROM database

2. Bank switching:
   - Cart has two banks with independent page counters
   - CART0 and CART1 accent lines from Suzy select banks
   - Page counters in Suzy registers at $FCB2/$FCB3
   - Shift register for reading data (directly read; no complex mapper like NES)

3. Data access:
   - Suzy drives cart access during sprite DMA and CPU read
   - CARTDATA register at $FCB3 reads next byte from cart
   - CARTADDR at $FCA0-$FCA1 sets cart address

4. Also support headerless .o format (raw ROM, no metadata)

5. Implement ISerializable::Serialize()

6. Wire into LynxConsole::LoadRom() — actually load and init the cart

7. Add to Core.vcxproj

Build and verify. Commit: "feat(lynx): Add LynxCart with LNX header parsing - #306"
```

---

## PROMPT 8: Mikey Timers and Interrupts

```
[Nexen] On features-atari-lynx, implement the Mikey timer system and interrupt controller. This is the HEART of the Lynx — everything depends on timers.

Create Core/Lynx/LynxMikey.h and Core/Lynx/LynxMikey.cpp:

1. **8 Timers** with full linking chain:
   - Chain 1: Timer 0 (HTIMER) → Timer 2 (VTIMER) → Timer 4
   - Chain 2: Timer 1 → Timer 3 → Timer 5 → Timer 7
   - Timer 6 standalone (audio sample rate)
   - Each timer has: BACKUP, CTLA, COUNT, CTLB registers
   - Timers decrement based on clock source (1MHz, 500kHz, 250kHz, ... or linked)
   - On underflow: reload from BACKUP, fire IRQ if enabled, cascade to linked timer

2. **Timer Register Addresses** ($FD00-$FD1F):
   - Timer 0: $FD00 (BACKUP), $FD01 (CTLA), $FD02 (COUNT), $FD03 (CTLB)
   - Timer 1: $FD08-$FD0B
   - Timer 2: $FD10-$FD13
   - ... pattern: base = $FD00 + (timerIndex * 4), but timers 0-3 at $FD00-$FD0F, timers 4-7 at $FD10-$FD1F
   - Actually: Timer N at $FD00 + N*4 for N=0-3, $FD10 + (N-4)*4 for N=4-7

3. **Interrupt Controller:**
   - INTSET ($FD80): Read pending IRQs, Write to set IRQ bits (for software IRQs)
   - INTRST ($FD81): Write to clear IRQ bits
   - Each timer can generate an IRQ (bit N = timer N)
   - Timer 4 = serial/UART IRQ in some configs
   - IRQ line to CPU: asserted when (INTSET & enabled_mask) != 0

4. **Register Read/Write** dispatch:
   - ReadRegister(uint8_t addr) — dispatch based on address ($FD00-$FDFF)
   - WriteRegister(uint8_t addr, uint8_t value)

5. **Tick method** — called by CPU on each cycle to advance timers

6. Implement ISerializable::Serialize()
7. Add to Core.vcxproj
8. Wire into LynxMemoryManager for $FD00-$FDFF reads/writes

Build and verify. Commit: "feat(lynx): Add Mikey timer system and interrupt controller - #287, #288"
```

---

## PROMPT 9: Mikey Display and Palette

```
[Nexen] On features-atari-lynx, implement Mikey's display DMA and palette system.

In Core/Lynx/LynxMikey.h/.cpp, add:

1. **Display DMA:**
   - DISPADR ($FD94-$FD95): Base address of frame buffer in RAM
   - DISPCTL ($FD92): Display control (color mode, flip, DMA enable)
   - Timer 0 triggers horizontal line DMA
   - Timer 2 triggers VBlank
   - Each line: read 80 bytes (160 pixels × 4bpp) from RAM at DISPADR + (scanline * 80)
   - Pixel format: each byte = 2 pixels, high nibble first
   - 102 visible lines + 3 VBlank lines = 105 lines per frame

2. **Palette System:**
   - 16 entries, each with 12-bit color (4 bits each R, G, B)
   - GREEN registers ($FDA0-$FDAF): Green[i] at $FDA0+i
   - BLUERED registers ($FDB0-$FDBF): BlueRed[i] at $FDB0+i (high nibble=blue, low=red)
   - Convert to 32-bit ARGB for display: R = (red << 4) | red, G = (green << 4) | green, B = (blue << 4) | blue

3. **Frame Buffer Output:**
   - Maintain internal uint32_t _frameBuffer[160 * 102]
   - LynxConsole::GetPpuFrame() returns pointer to this buffer
   - Create LynxDefaultVideoFilter.h/.cpp for any post-processing

4. **Scanline tracking:**
   - _currentScanline incremented by Timer 0 fires
   - When scanline reaches 102, start VBlank period
   - When Timer 2 fires, reset to line 0

5. Add to Core.vcxproj

Build and verify. If possible, test with a simple ROM that writes to palette registers. Commit: "feat(lynx): Add Mikey display DMA and palette - #289, #295"
```

---

## PROMPT 10: Suzy Sprite Engine

```
[Nexen] On features-atari-lynx, implement Suzy's sprite engine. This is the most complex part of the Lynx hardware.

Create Core/Lynx/LynxSuzy.h and Core/Lynx/LynxSuzy.cpp:

1. **Sprite Control Blocks (SCB):**
   - SPRSYS ($FC92) write: Start sprite engine
   - SUZYBUSEN ($FC90) read: Check if Suzy is busy
   - SCBADR ($FC10-$FC11): Pointer to first SCB in RAM
   - Walk the SCB linked list: each SCB points to next via SCBNEXT field
   - Process each sprite: read SCB fields, decode sprite data, render to frame buffer

2. **SCB Processing:**
   - Read SPRCTL0, SPRCTL1 (sprite type, drawing mode)
   - Read SCBNEXT pointer → chain to next sprite
   - Read SPRDLINE → pointer to pixel data
   - Read HPOS, VPOS → screen position
   - Read HSIZE, VSIZE → scaling (8.8 fixed-point)
   - Depending on SPRCTL1 reload flags, some fields reuse previous values

3. **Sprite Data Decoding:**
   - Each scanline: read 1-byte line header (0 = end of sprite)
   - Decode run-length encoded pixel data within each line
   - Bit depth: 1, 2, 3, or 4 bits per pixel (from SPRCTL0)
   - Literal and repeat packets for each scan line

4. **Collision Detection:**
   - Collision depository: 16 slots at $FC80-$FC8F
   - Each sprite has a collision number (from COLLOFF)
   - When pixel drawn overlaps existing non-zero entry, record collision in both slots
   - DONT_COLLIDE flag suppresses collision for this sprite

5. **Bus Contention:**
   - While Suzy processes sprites, CPU is halted (bus stolen)
   - Track cycle count for Suzy operations to accurately stall CPU

6. **Hardware Math** (also in Suzy):
   - MATHC:MATHD × MATHE:MATHF → MATHG:MATHH:MATHJ:MATHK
   - MATHG:MATHH:MATHJ:MATHK ÷ MATHE:MATHF → MATHC:MATHD (quotient), MATHG:MATHH (remainder)
   - Signed/unsigned modes
   - Accumulate mode (add to existing result)

7. **Register Read/Write dispatch** for $FC00-$FCFF range:
   - ReadRegister(uint8_t addr)
   - WriteRegister(uint8_t addr, uint8_t value)

8. Implement ISerializable::Serialize()
9. Add to Core.vcxproj
10. Wire into LynxMemoryManager for $FC00-$FCFF

Build and verify. Commit: "feat(lynx): Add Suzy sprite engine - #296, #297, #298, #299"
```

---

## PROMPT 11: Input and Controller

```
[Nexen] On features-atari-lynx, implement the Lynx controller and input system.

1. **Add ControllerType::LynxController** to SettingTypes.h ControllerType enum

2. Create Core/Lynx/LynxController.h and Core/Lynx/LynxController.cpp:
   - 9 buttons: Up, Down, Left, Right, A, B, Option1, Option2, Pause
   - GetKeyNames() → "UDLRabOoP"
   - InternalSetStateFromInput() — map KeyMapping fields to buttons
   - ReadRam() — return joystick register value (active-low)
   - InternalDrawController() — draw Lynx controller shape in InputHud

3. Create Core/Lynx/LynxControlManager.h and Core/Lynx/LynxControlManager.cpp:
   - CreateControllerDevice() — create LynxController
   - UpdateControlDevices() — read LynxConfig, update device list

4. Wire joystick reading into LynxSuzy:
   - JOYSTICK ($FCB0) read returns controller state (active-low)
   - SWITCHES ($FCB1) bit 2 = Pause button

5. Wire Pause button into Mikey interrupt system

6. Add all files to Core.vcxproj

Build and verify. Commit: "feat(lynx): Add LynxController and input system - #311"
```

---

## PROMPT 12: EEPROM Save Data

```
[Nexen] On features-atari-lynx, implement EEPROM emulation for battery-backed save data.

Create Core/Lynx/LynxEeprom.h and Core/Lynx/LynxEeprom.cpp:

1. Support 93C46 (128 bytes), 93C66 (512 bytes), 93C86 (2048 bytes)

2. Serial protocol emulation:
   - CS (chip select), CLK (clock), DI (data in), DO (data out)
   - Operations: READ, WRITE, ERASE, ERAL (erase all), WRAL (write all), EWEN (write enable), EWDS (write disable)
   - Bit-banged by game software through Suzy registers

3. State machine: Idle → ReceivingOpcode → ReceivingAddress → ReadingData/WritingData

4. Battery save/load through BatteryManager (.sav extension)

5. Serialize() for save states

6. Wire into LynxSuzy — EEPROM pins accessible through Suzy I/O ($FD40-$FD43 or appropriate Suzy address)

7. Detect EEPROM type from LNX header or ROM database

8. Add to Core.vcxproj

Build and verify. Commit: "feat(lynx): Add EEPROM emulation - #308"
```

---

## PROMPT 13: Audio System

```
[Nexen] On features-atari-lynx, implement the Lynx audio system in Mikey.

In Core/Lynx/LynxMikey.h/.cpp (or create separate LynxApu.h/.cpp):

1. **4 Audio Channels**, each with:
   - 12-bit LFSR (Linear Feedback Shift Register) driven polynomial counter
   - 8-bit volume register
   - Feedback/tap select register (configures LFSR polynomial)
   - Output register (signed 8-bit sample)
   - Backup/reload value
   - Integration mode: output feeds into next channel

2. **Audio Registers** ($FD20-$FD3F):
   - Ch N volume: $FD20 + N*8
   - Ch N feedback: $FD21 + N*8
   - Ch N output: $FD22 + N*8
   - Ch N shift: $FD23 + N*8
   - Ch N backup: $FD24 + N*8
   - Ch N control: $FD25 + N*8

3. **ATTEN registers** ($FD40-$FD47):
   - Left/right stereo panning for each channel
   - ATTEN_A ($FD40), ATTEN_B ($FD41), ATTEN_C ($FD42), ATTEN_D ($FD43)

4. **STEREO register** ($FD50):
   - Global stereo control

5. **Sample rate:**
   - Timer 6 drives the audio sample rate
   - Default ~15.9 kHz (Timer 6 backup = 55 at 500kHz clock)
   - On each Timer 6 tick: advance all 4 channels, mix, output

6. **Mixing:** Sum all 4 channels, apply ATTEN panning, output stereo L/R

7. Output audio samples via Emulator's audio system

8. Add to Core.vcxproj

Build and verify. Commit: "feat(lynx): Add 4-channel audio system - #290, #291"
```

---

## PROMPT 14: Wire Everything Together — First Boot

```
[Nexen] On features-atari-lynx, wire all Lynx components together and attempt first boot. This is the integration prompt.

1. **LynxConsole::LoadRom()** — fully implement:
   - Parse LNX header via LynxCart
   - Allocate 64KB work RAM
   - Create all components: CPU, Mikey, Suzy, MemoryManager, Cart, ControlManager
   - Wire components: MemoryManager gets pointers to Mikey, Suzy, RAM, ROM, BootRom
   - CPU gets pointer to MemoryManager
   - Initialize MAPCTL to 0 (all overlays visible)
   - Set PC to reset vector at $FFFC-$FFFD (from boot ROM)

2. **LynxConsole::RunFrame()** — implement main loop:
   - Loop until VBlank (102 scanlines completed):
     - CPU executes next instruction
     - Mikey ticks timers based on CPU cycle count
     - Check Suzy sprite engine busy state
     - Handle interrupts
   - Copy Mikey frame buffer to output

3. **Boot ROM handling:**
   - If boot ROM present ($FE00-$FFFF, 512 bytes): execute from reset vector
   - If no boot ROM: HLE — skip boot sequence, set PC to cart entry point

4. **Fix all linker errors** — ensure all components link correctly

5. **Test:** Try loading a Lynx ROM (e.g., a simple homebrew). It doesn't need to be playable yet — just validate that:
   - ROM loads without crash
   - CPU starts executing
   - No assertion failures
   - Frame buffer produces SOMETHING (even garbage is progress)

Build and verify. Commit: "feat(lynx): Wire all components — first boot attempt - #304"
```

---

## PROMPT 15: Debugger Foundation

```
[Nexen] On features-atari-lynx, implement the Lynx debugger infrastructure.

1. Create Core/Lynx/Debugger/LynxDebugger.h and .cpp:
   - Implement all IDebugger pure virtuals
   - Create CodeDataLogger, TraceLogger, EventManager, BreakpointManager, CallstackManager
   - Implement Step(), Run(), GetProgramCounter(), SetProgramCounter()
   - GetSupportedFeatures(): RunToIrq=true, StepOver=true, StepOut=true, CallStack=true, ChangeProgramCounter=true, CpuCycleStep=true
   - CPU vectors: IRQ at $FFFE, RESET at $FFFC (no NMI used on Lynx)

2. Create Core/Lynx/Debugger/LynxDisUtils.h and .cpp:
   - Disassemble 65C02 instructions (reuse PCE's HuC6280 disassembly approach as reference, but 65C02 specific)
   - Handle all addressing modes: implied, immediate, zp, zp+x, zp+y, abs, abs+x, abs+y, (ind,x), (ind),y, (ind), relative, (abs,x)
   - Format: "LDA $1234,X" style

3. Create Core/Lynx/Debugger/LynxTraceLogger.h and .cpp:
   - Log CPU state per instruction: PC, opcode, A, X, Y, SP, P, cycles

4. Create Core/Lynx/Debugger/LynxEventManager.h and .cpp:
   - Track events: CPU reads/writes, timer fires, IRQs, sprite engine, DMA

5. Create Core/Lynx/Debugger/DummyLynxCpu.h and .cpp:
   - Predictive CPU for evaluating breakpoint conditions without side effects

6. Add `case CpuType::Lynx:` to all 14+ switch blocks in Core/Debugger/Debugger.cpp

7. Add `case CpuType::Lynx:` to all switch blocks in Core/Debugger/DisassemblyInfo.cpp

8. Add all files to Core.vcxproj

Build and verify. Commit: "feat(lynx): Add debugger infrastructure - #318, #319, #320, #321"
```

---

## PROMPT 16: UI Integration — Menus, Config, Views

```
[Nexen] On features-atari-lynx, integrate Lynx into the UI. Add Lynx cases to ALL remaining C# switch statements.

1. UI/ViewModels/MainMenuViewModel.cs:
   - Add ConsoleType.Lynx to all visibility/region/feature checks (~15 locations)
   - Add Lynx to file filter dialog extensions (.lnx, .o)

2. UI/Utilities/ShortcutHandler.cs:
   - Add `case ConsoleType.Lynx:` for layer toggles (sprites only, no BG layers)

3. UI/Config/ConsoleOverrideConfig.cs:
   - Add Lynx config overrides

4. UI/Debugger/ViewModels/RegisterViewerWindowViewModel.cs:
   - Add Lynx state fetching with tabs: CPU, Mikey Timers, Mikey Audio, Mikey Display, Suzy Sprite, Suzy Math

5. UI/Debugger/ViewModels/RefreshTimingViewModel.cs:
   - Add `ConsoleType.Lynx => 102` for default scanline count

6. UI/Debugger/ViewModels/MemoryMappingViewModel.cs:
   - Add Lynx memory map display

7. UI/Debugger/ViewModels/TraceLoggerViewModel.cs:
   - Add Lynx trace logger style

8. UI/Debugger/Labels/PansyExporter.cs:
   - Add `[RomFormat.Lynx] = 0x0C` to PlatformIds
   - Add Lynx memory regions in the switch statement

9. UI/Debugger/Utilities/DebugWorkspaceManager.cs:
   - Add Lynx symbol import case

10. UI/Interop/DebugApi.cs:
    - Add Lynx state validation in IsValidCpuState/IsValidPpuState

Build both C++ and C# projects. Commit: "feat(lynx): Full UI integration - #311, #312, #313, #314, #315"
```

---

## PROMPT 17: PPU Tools and Sprite Viewer

```
[Nexen] On features-atari-lynx, implement Lynx-specific PPU debugging tools.

1. Create Core/Lynx/Debugger/LynxPpuTools.h and .cpp:
   - GetPaletteInfo(): Return 16-color palette with RGB values
   - GetSpriteInfo(): Walk SCB chain, decode each sprite's properties (position, size, type, collision number, data pointer)
   - GetTileInfo(): Lynx has no tiles — return empty or show sprite data regions
   - GetSpritePreview(): Render individual sprite to preview buffer

2. Create UI/Debugger/RegisterViewer/LynxRegisterViewer.cs:
   - CPU tab: A, X, Y, SP, PC, flags (N, V, -, B, D, I, Z, C)
   - Mikey Timers tab: All 8 timers with backup, count, controlA, controlB
   - Mikey Audio tab: 4 channels with volume, feedback, output, control
   - Mikey Display tab: DISPCTL, DISPADR, current scanline, palette
   - Suzy tab: Sprite control, SCB address, math registers, collision buffer
   - Memory tab: MAPCTL state, overlay visibility flags

3. Wire LynxPpuTools into LynxDebugger::GetPpuTools()

4. Add files to Core.vcxproj

Build and verify. Commit: "feat(lynx): Add PPU tools, sprite viewer, register viewer - #322, #323, #324"
```

---

## PROMPT 18: Movie/TAS Support

```
[Nexen] On features-atari-lynx, verify and configure Movie/TAS recording and playback for Lynx.

1. Verify LynxController::GetTextState() and SetTextState() work correctly for movie serialization (inherited from BaseControlDevice, should work if GetKeyNames() returns correct string)

2. Test movie recording flow:
   - Record: MovieRecorder calls GetTextState() each frame → "|UDLRabOoP\n" format
   - Playback: MovieRecorder calls SetTextState() → sets button states

3. Verify save state integration:
   - Movie start from power-on: must serialize boot ROM state
   - Movie start from save state: must serialize complete state

4. Handle Lynx-specific TAS considerations:
   - 75 Hz frame rate (not 60) — verify TAS editor handles this
   - Screen rotation — record rotation state in movie metadata if needed
   - Pause button through Mikey IRQ — ensure it's captured in controller state

5. MovieConverter: If needed, add Lynx support to MovieConverter project for importing/exporting

6. Verify InputHud displays correctly with the LynxController::InternalDrawController() implementation

Build and verify. Commit: "feat(lynx): Verify and configure Movie/TAS support - #331, #332"
```

---

## PROMPT 19: ROM Database and Game Compatibility

```
[Nexen] On features-atari-lynx, create a ROM database for Lynx games and improve compatibility.

1. Create Core/Lynx/LynxGameDatabase.h:
   - Struct: LynxGameEntry { uint32_t crc32; const char* name; LynxRotation rotation; uint16_t eepromSize; uint8_t playerCount; }
   - Populate with ~85 entries from No-Intro DAT
   - At minimum include:
     - California Games (rotation=right, no eeprom)
     - Chip's Challenge (no rotation, eeprom 128B)
     - Todd's Adventures in Slime World (no rotation, no eeprom)
     - Blue Lightning (no rotation, no eeprom)
     - Gates of Zendocon (no rotation, no eeprom)
     - Rygar (no rotation, no eeprom)
     - Warbirds (rotation=left, no eeprom)

2. Implement LynxConsole::GetHash() — hash ROM data excluding LNX header

3. Auto-detect rotation from ROM database when LNX header doesn't specify

4. Auto-detect EEPROM type from ROM database

5. Apply game-specific fixes (if any known) based on CRC32 lookup

Build and verify. Commit: "feat(lynx): Add ROM database for game identification - #310"
```

---

## PROMPT 20: Final Polish and Testing

```
[Nexen] On features-atari-lynx, do final polish, fix remaining build warnings, and test game compatibility.

1. Fix all compiler warnings in Core/Lynx/ directory

2. Run all existing test suites — ensure no regressions in other consoles

3. Test with Lynx homebrew ROMs:
   - "Hello World" type programs
   - Simple games from the homebrew community
   - Verify: loads, displays, accepts input

4. Test save states: save and load in a game, verify state restored correctly

5. Test EEPROM: play a game with saves, verify data persists to .sav file

6. Test debugger: step through code, set breakpoints, view registers, disassemble

7. Performance check: ensure Lynx emulation doesn't slow down significantly vs other consoles

8. Update CHANGELOG.md with Lynx emulation entry

9. Update README.md:
   - Add Atari Lynx to supported systems table
   - Add Lynx features description
   - Add Lynx to download section if applicable

10. Close completed GitHub issues with commit references

Build and verify. Create a summary of test results. Commit: "feat(lynx): Final polish and compatibility fixes"
```

---

## PROMPT 21: Assembler and Expression Evaluator

```
[Nexen] On features-atari-lynx, implement the Lynx assembler and expression evaluator for the debugger.

1. Create Core/Lynx/Debugger/LynxAssembler.h and .cpp:
   - Implement IAssembler interface
   - Support all 65C02 instructions and addressing modes
   - Parse: "LDA #$FF", "STA $1234,X", "BRA label", etc.
   - Reference PCE's assembler as a template (similar CPU)

2. Add Lynx registers to Core/Debugger/ExpressionEvaluator.cpp:
   - In the CpuType::Lynx case, register: A, X, Y, SP, PC, PS (and individual flags N, V, D, I, Z, C)

3. Verify breakpoint expressions work: "A == $FF", "PC == $1234", etc.

Build and verify. Commit: "feat(lynx): Add assembler and expression evaluator - #326, #327"
```

---

## PROMPT 22: Hardware Bug Emulation

```
[Nexen] On features-atari-lynx, implement known Lynx hardware bugs for accuracy.

Reference the 13 documented bugs from the Lynx technical report in ~docs/plans/atari-lynx-technical-report.md:

1. **Unsigned multiply bug:** Values >= $8000 produce incorrect results — emulate the hardware behavior
2. **Timer linking race condition:** When timer reaches 0 and cascades, there's a 1-cycle window
3. **Sprite engine last-pixel bug:** Last pixel of a sprite line may not render
4. **UART timing issues:** Serial port has known timing quirks
5. Any other bugs documented in the technical report

Each bug should be:
- Documented with a comment explaining the hardware behavior
- Toggleable via a debug flag if possible (for testing)
- Verified against known test cases

Build and verify. Commit: "feat(lynx): Implement hardware bug emulation for accuracy - #284"
```

---

## PROMPT 23: Boot ROM and HLE

```
[Nexen] On features-atari-lynx, implement boot ROM support and HLE (High-Level Emulation) fallback.

1. **Real Boot ROM path:**
   - User provides 512-byte Lynx boot ROM via firmware settings
   - Stored as FirmwareType::LynxBootRom (add to FirmwareType enum)
   - Mapped to $FE00-$FFFF when ROM overlay is active
   - CPU starts at reset vector $FFFC-$FFFD

2. **HLE Boot ROM:** When no boot ROM is provided:
   - Skip the boot animation/logo check
   - Initialize hardware registers to post-boot state
   - Set MAPCTL to disable ROM overlay (RAM visible)
   - Copy cart entry point to RAM
   - Jump directly to game code
   - Initialize palette to reasonable defaults

3. **Firmware UI:**
   - Add Lynx Boot ROM to firmware configuration dialog
   - UI/Config/FirmwareConfig.cs — add LynxBootRom path
   - Validate file size (must be exactly 512 bytes)

4. Wire into LynxConsole::LoadRom()

Build and verify. Commit: "feat(lynx): Add boot ROM support and HLE fallback - #307"
```

---

## PROMPT 24: ComLynx (Serial/UART) — Future

```
[Nexen] On features-atari-lynx, add stub ComLynx (UART/serial) support. Full multiplayer is deferred, but the registers must be implemented to prevent games from hanging.

1. In LynxMikey, implement UART registers:
   - SERCTL ($FD8C): Serial control
   - SERDAT ($FD8D): Serial data
   - Timer 4 drives serial baud rate

2. Stub behavior:
   - Reads from SERDAT return 0 (no peer connected)
   - Writes to SERDAT are discarded
   - SERCTL status: always report "transmit buffer empty", "no data received"
   - This prevents games from hanging while waiting for serial input

3. Document that full ComLynx multiplayer is a future enhancement

Build and verify. Commit: "feat(lynx): Stub ComLynx serial registers - #294"
```

---

## Post-Implementation

After all prompts are complete:

```
1. Review all 24 prompts completed
2. Run full build: MSBuild Nexen.sln /p:Configuration=Release /p:Platform=x64
3. Run all tests
4. Test with 5+ Lynx games
5. Close all resolved GitHub issues
6. Create a pull request from features-atari-lynx to master
7. Update session logs
```

---

## Issue Cross-Reference

| Prompt | Issues Addressed |
|--------|-----------------|
| 1 | #303 (Register Lynx types) |
| 2 | #303 (C# mirrors) |
| 3 | #303 (State structs) |
| 4 | #304 (Console skeleton) |
| 5 | #305 (Memory manager) |
| 6 | #279, #280 (CPU opcodes + addressing) |
| 7 | #306 (Cart/ROM loading) |
| 8 | #287, #288 (Timers + interrupts) |
| 9 | #289, #295 (Display + palette) |
| 10 | #296, #297, #298, #299 (Sprite engine) |
| 11 | #311 (Input/controller) |
| 12 | #308 (EEPROM) |
| 13 | #290, #291 (Audio) |
| 14 | #304 (Integration) |
| 15 | #318-#321 (Debugger foundation) |
| 16 | #311-#315 (UI integration) |
| 17 | #322-#324 (PPU tools) |
| 18 | #331, #332 (Movie/TAS) |
| 19 | #310 (ROM database) |
| 20 | Final polish |
| 21 | #326, #327 (Assembler) |
| 22 | #284 (Hardware bugs) |
| 23 | #307 (Boot ROM) |
| 24 | #294 (ComLynx) |
