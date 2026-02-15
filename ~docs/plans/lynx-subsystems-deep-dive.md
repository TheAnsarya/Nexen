# Atari Lynx Subsystems Deep Dive

> Detailed implementation plans for every subsystem: save states, SRAM/EEPROM, movie/TAS, metadata/hashes, input mapping, dual-chip debugging, Mikey, Suzy, Pansy export.

---

## 1. Save State System

### Architecture
Nexen uses `ISerializable` + `Serializer` with `SV()` / `SVArray()` macros. Every component (CPU, Mikey, Suzy, memory, controllers, cart) must implement `Serialize(Serializer& s)`. The same method handles both save and load — `Serializer` tracks direction via `IsSaving()`.

### LynxConsole::Serialize Pattern
```cpp
void LynxConsole::Serialize(Serializer& s) {
	// Model validation (prevent cross-model state loads)
	LynxModel model = _model;
	SV(model);
	if (!s.IsSaving() && model != _model) {
		MessageManager::DisplayMessage("SaveStates",
			"Cannot load save state from different Lynx model.");
		s.SetErrorFlag();
		return;
	}

	// Stream all components
	SV(_cpu);
	SV(_mikey);
	SV(_suzy);
	SV(_memoryManager);
	SV(_controlManager);
	SV(_cart);

	// Raw memory arrays
	SVArray(_workRam, 0x10000);  // 64KB

	// EEPROM data (if cart has EEPROM)
	if (_eepromData && _eepromSize) {
		SVArray(_eepromData, _eepromSize);
	}
}
```

### Per-Component Serialize
Each component streams its internal registers:

**LynxCpu::Serialize:**
```cpp
void LynxCpu::Serialize(Serializer& s) {
	SV(_state.A); SV(_state.X); SV(_state.Y);
	SV(_state.SP); SV(_state.PC); SV(_state.PS);
	SV(_state.CycleCount);
	SV(_state.NmiFlag); SV(_state.IrqSource);
	SV(_state.StoppedForStp); SV(_state.StoppedForWai);
}
```

**LynxMikey::Serialize:**
```cpp
void LynxMikey::Serialize(Serializer& s) {
	// 8 timers
	for (int i = 0; i < 8; i++) {
		SVI(_timers[i].BackupValue);
		SVI(_timers[i].ControlA);
		SVI(_timers[i].Count);
		SVI(_timers[i].ControlB);
		// ... all timer fields
	}

	// Audio channels
	for (int i = 0; i < 4; i++) {
		SVI(_audioChannels[i]); // Each is ISerializable
	}

	// Display
	SV(_displayControl);
	SV(_displayAddress);
	SV(_currentScanline);
	SVArray(_palette, 16);    // 16 palette entries
	SVArray(_paletteRaw, 32); // Raw palette registers

	// Interrupt controller
	SV(_irqEnabled);
	SV(_irqPending);

	// UART
	SV(_uartData);
	SV(_uartControl);
}
```

**LynxSuzy::Serialize:**
```cpp
void LynxSuzy::Serialize(Serializer& s) {
	// Sprite engine state
	SV(_spriteControl);
	SV(_spriteInit);
	SV(_scbAddress);
	SV(_currentSCB);
	SV(_spriteBusy);

	// Math coprocessor
	SV(_mathA); SV(_mathB);
	SV(_mathC); SV(_mathD);
	SV(_mathE); SV(_mathF);
	SV(_mathG); SV(_mathH);
	SV(_mathJ); SV(_mathK);
	SV(_mathM); SV(_mathN);
	SV(_mathSign);
	SV(_mathAccumulate);

	// Collision depository
	SVArray(_collisionBuffer, 16);

	// Joystick state
	SV(_joystick);
	SV(_switches);
}
```

### Thumbnails
`GetPpuFrame()` returns the 160×102 frame buffer → SaveStateManager captures it automatically. No extra work needed.

### Versioning
Use `SaveStateManager::FileFormatVersion` (currently 4). If LynxState changes shape later, use `s.GetVersion()` for backward compat.

---

## 2. SRAM / EEPROM Save System

### Lynx Save Data Types
| Type | Chip | Size | Games |
|------|------|------|-------|
| No save | — | 0 | Most games |
| 93C46 EEPROM | 128 bytes | Many RPGs, tournament modes |
| 93C66 EEPROM | 512 bytes | Few games |
| 93C86 EEPROM | 2048 bytes | Rare |

### EEPROM Hardware Interface
- Accessed through Suzy registers at `$FD40-$FD43`
- Serial protocol: CS (chip select), CLK (clock), DI (data in), DO (data out)
- Three operations: READ, WRITE, ERASE (plus EWEN/EWDS for write enable/disable)
- Bit-banged by game software — emulator must simulate the serial protocol

### LynxEeprom Class Design
```cpp
class LynxEeprom : public ISerializable {
private:
	vector<uint8_t> _data;
	uint16_t _size;          // 128, 512, or 2048 bytes
	Emulator* _emu;

	// Serial protocol state
	uint8_t _state;          // Idle, ReceivingOpcode, ReceivingAddress, ReceivingData, SendingData
	uint16_t _opcode;
	uint16_t _address;
	uint16_t _dataBuffer;
	uint8_t _bitCount;
	bool _writeEnabled;
	bool _csActive;
	bool _clockState;

public:
	void Write(uint8_t value);  // CS, CLK, DI pins from Suzy register
	uint8_t Read();             // DO pin to Suzy register
	void SaveBattery();
	void LoadBattery();
	void Serialize(Serializer& s) override;
};
```

### Battery Manager Integration
```cpp
void LynxConsole::SaveBattery() {
	if (_eeprom) {
		_eeprom->SaveBattery();
	}
}

void LynxConsole::LoadBattery() {
	if (_eeprom) {
		_eeprom->LoadBattery();
	}
}

// In LynxEeprom:
void LynxEeprom::SaveBattery() {
	_emu->GetBatteryManager()->SaveBattery(".sav",
		std::span<const uint8_t>(_data.data(), _data.size()));
}

void LynxEeprom::LoadBattery() {
	_emu->GetBatteryManager()->LoadBattery(".sav",
		std::span<uint8_t>(_data.data(), _data.size()));
}
```

### EEPROM Detection
From LNX header byte at offset 58 (`audin` field) or from ROM database. Some games don't declare EEPROM in the header — need a fallback database.

---

## 3. Movie / TAS System

### Movie Format
Nexen movies are ZIP files (`.nexen-movie`) containing:
- `Input.txt` — Frame-by-frame input: `|UDLRAB12P\n` per frame
- `GameSettings.txt` — ROM hash, emulator version
- `SaveState.nexen-save` — Optional initial state
- `Battery.sav` — Optional battery data

### LynxController Design
```cpp
class LynxController : public BaseControlDevice {
public:
	enum Buttons {
		Up = 0, Down, Left, Right,  // D-pad
		A, B,                        // Face buttons
		Option1, Option2,            // Option buttons
		Pause,                       // Pause button
		ButtonCount
	};

	string GetKeyNames() override {
		return "UDLRaboOP";  // One char per button
	}

	void InternalSetStateFromInput() override {
		for (KeyMapping& keyMapping : _keyMappings) {
			SetPressedState(Buttons::A, keyMapping.A);
			SetPressedState(Buttons::B, keyMapping.B);
			SetPressedState(Buttons::Up, keyMapping.Up);
			SetPressedState(Buttons::Down, keyMapping.Down);
			SetPressedState(Buttons::Left, keyMapping.Left);
			SetPressedState(Buttons::Right, keyMapping.Right);
			SetPressedState(Buttons::Option1, keyMapping.L);     // Map to L shoulder
			SetPressedState(Buttons::Option2, keyMapping.R);     // Map to R shoulder
			SetPressedState(Buttons::Pause, keyMapping.Start);   // Map to Start
		}
	}

	uint8_t ReadRam(uint16_t addr) override {
		// Suzy reads joystick state from $FCB0
		uint8_t value = 0;
		if (IsPressed(Buttons::Right))   value |= 0x10;
		if (IsPressed(Buttons::Left))    value |= 0x20;
		if (IsPressed(Buttons::Down))    value |= 0x40;
		if (IsPressed(Buttons::Up))      value |= 0x80;
		if (IsPressed(Buttons::Option1)) value |= 0x01; // Note: active-low in hardware
		if (IsPressed(Buttons::Option2)) value |= 0x02;
		if (IsPressed(Buttons::A))       value |= 0x01; // bit position TBD per schematic
		if (IsPressed(Buttons::B))       value |= 0x02;
		// Pause goes to Mikey interrupt
		return value;
	}

	void InternalDrawController(InputHud& hud) override {
		hud.DrawOutline(35, 14);
		hud.DrawButton(5, 3, 3, 3, IsPressed(Buttons::Left));
		hud.DrawButton(11, 3, 3, 3, IsPressed(Buttons::Right));
		hud.DrawButton(8, 0, 3, 3, IsPressed(Buttons::Up));
		hud.DrawButton(8, 6, 3, 3, IsPressed(Buttons::Down));
		hud.DrawButton(25, 3, 3, 3, IsPressed(Buttons::A));
		hud.DrawButton(29, 3, 3, 3, IsPressed(Buttons::B));
		hud.DrawButton(15, 11, 4, 2, IsPressed(Buttons::Option1));
		hud.DrawButton(20, 11, 4, 2, IsPressed(Buttons::Option2));
		hud.DrawButton(0, 11, 4, 2, IsPressed(Buttons::Pause));
	}
};
```

### LynxControlManager Design
```cpp
class LynxControlManager : public BaseControlManager {
public:
	shared_ptr<BaseControlDevice> CreateControllerDevice(
		ControllerType type, uint8_t port) override {
		switch (type) {
			case ControllerType::LynxController:
				return make_shared<LynxController>(_emu, _console, port,
					_emu->GetSettings()->GetLynxConfig().Controller.Keys);
			default:
				return nullptr;
		}
	}

	void UpdateControlDevices() override {
		LynxConfig cfg = _emu->GetSettings()->GetLynxConfig();
		if (_emu->GetSettings()->IsEqual(_prevConfig, cfg) && !_controlDevices.empty()) return;
		auto lock = _deviceLock.AcquireSafe();
		ClearDevices();
		auto device = CreateControllerDevice(ControllerType::LynxController, 0);
		if (device) RegisterControlDevice(device);
		_prevConfig = cfg;
	}
};
```

### TAS-Specific Considerations
1. **Screen rotation** — Lynx games can be played in 3 orientations. TAS must record rotation state.
2. **Pause button** — Goes through Mikey interrupt, not Suzy joystick register. Must be recorded separately.
3. **ComLynx** — Multiplayer via serial. Not needed for initial TAS support.
4. **Frame timing** — Lynx runs at ~75.0 Hz (not 60). TAS tools must handle this correctly.
5. **Power-on state** — Lynx has a boot ROM sequence. Movies must either start from boot ROM or from a save state after boot.

---

## 4. Metadata / Hashes

### ROM Format: LNX
```
Offset  Size  Field
0       4     Magic: "LYNX"
4       2     Page size bank 0 (256-byte pages)
6       2     Page size bank 1
8       2     Version (1)
10      32    Cart name (null-padded)
42      16    Manufacturer name
44      1     Rotation (0=none, 1=left, 2=right)
45-63   19    Reserved / flags
```
Total header: 64 bytes. ROM data follows immediately.

### Hash Calculation
Override `GetHash()` to hash only the ROM payload (excluding 64-byte LNX header):
```cpp
string LynxConsole::GetHash(HashType type) {
	// Skip 64-byte LNX header for hashing
	if (_prgRomSize > 64) {
		vector<uint8_t> romData(_prgRom + 64, _prgRom + _prgRomSize);
		if (type == HashType::Sha1) return SHA1::GetHash(romData);
		if (type == HashType::Md5) return MD5::GetHash(romData);
	}
	return "";
}
```

### ROM Database
- No official Lynx game database exists in Mesen2/Nexen
- Create `LynxGameDatabase.h` with ~85 entries from No-Intro DAT
- Fields: CRC32, name, EEPROM type, rotation, player count
- Used for: auto-detecting rotation, EEPROM support, game-specific fixes

### CRC32 for ROM Identification
```cpp
uint32_t LynxConsole::GetCrc32() {
	if (_prgRomSize > 64) {
		return CRC32::GetCrc(_prgRom + 64, _prgRomSize - 64);
	}
	return CRC32::GetCrc(_prgRom, _prgRomSize);
}
```

---

## 5. Input Button Mapping

### Lynx Hardware Buttons
| Button | Hardware | Register | Bit |
|--------|----------|----------|-----|
| Up | D-pad | JOYSTICK ($FCB0) | Bit 7 |
| Down | D-pad | JOYSTICK ($FCB0) | Bit 6 |
| Left | D-pad | JOYSTICK ($FCB0) | Bit 5 |
| Right | D-pad | JOYSTICK ($FCB0) | Bit 4 |
| Option 1 | Button | JOYSTICK ($FCB0) | Bit 3 |
| Option 2 | Button | JOYSTICK ($FCB0) | Bit 2 |
| A (Inside) | Button | JOYSTICK ($FCB0) | Bit 1 |
| B (Outside) | Button | JOYSTICK ($FCB0) | Bit 0 |
| Pause | Button | Mikey IRQ | Bit in INTSET ($FD80) |

**Note:** All buttons are active-LOW (0 = pressed, 1 = released) — emulator inverts.

### Rotation Handling
When the screen is rotated, the D-pad physical orientation changes but the electrical connections don't. The game software handles this. However, the emulator must:
1. Read rotation from LNX header (or ROM database)
2. Present correct button layout in InputHud
3. Optionally auto-remap D-pad for rotated games in the UI

### UI Config (C#)
```csharp
public class LynxConfig : BaseConfig<LynxConfig> {
	[Reactive] public ControllerConfig Controller { get; set; } = new();
	[Reactive] public LynxRotation Rotation { get; set; } = LynxRotation.None;
	[Reactive] public bool AutoRotate { get; set; } = true;
	[Reactive] public bool BlendFrames { get; set; } = false;
}

public struct InteropLynxConfig {
	public InteropControllerConfig Controller;
	public LynxRotation Rotation;
	[MarshalAs(UnmanagedType.I1)] public bool AutoRotate;
	[MarshalAs(UnmanagedType.I1)] public bool BlendFrames;
}
```

---

## 6. Dual-Chip Debugging (Mikey + Suzy)

### Architecture
Unlike SNES (which has separate CPU types for SPC700, SA-1, etc.), the Lynx has a single CPU that accesses both Mikey and Suzy through memory-mapped I/O. There is NO separate CPU in either chip — they are peripherals.

**This means:** Only ONE `CpuType::Lynx` is needed. The debugger shows Mikey and Suzy state as register views, not as separate CPU debuggers.

### Debugger Register Views
The register viewer should have tabs/sections:
1. **CPU** — A, X, Y, SP, PC, P (flags), cycle count
2. **Mikey Timers** — All 8 timers: backup, count, controlA, controlB
3. **Mikey Audio** — 4 channels: volume, feedback, output, control
4. **Mikey Display** — Display control, address, current scanline
5. **Mikey Interrupts** — IRQ enable, IRQ pending, IRQ source
6. **Suzy Sprite** — SCBADR, PROCADR, MATHC/D/E/F, sprite control
7. **Suzy Math** — Multiply/divide registers, accumulator
8. **Suzy Collision** — 16-slot collision depository
9. **Memory Manager** — MAPCTL state, overlay state

### Event Manager (Timeline)
Track events on a per-scanline basis:
- CPU reads/writes to Mikey registers
- CPU reads/writes to Suzy registers
- Timer fire events (all 8)
- IRQ assertions
- Sprite engine start/stop
- DMA events
- Audio output samples

### Breakpoint Types
- Standard CPU breakpoints (address, value, read/write)
- Mikey register access breakpoints (e.g., break on palette write)
- Suzy register access breakpoints (e.g., break on sprite engine start)
- Timer expire breakpoints
- IRQ assertion breakpoints
- Scanline breakpoints

### Memory Views
| Memory Type | Address Range | Size | Purpose |
|-------------|---------------|------|---------|
| LynxWorkRam | $0000-$FFFF | 64KB | Main RAM |
| LynxPrgRom | Cart data | Variable | ROM |
| LynxMikeyRegisters | $FD00-$FDFF | 256B | Mikey I/O |
| LynxSuzyRegisters | $FC00-$FCFF | 256B | Suzy I/O |
| LynxBootRom | $FE00-$FFFF | 512B | Boot ROM |
| LynxVideoRam | From _displayAddress | Variable | Frame buffer |

### PPU Tools (Tile/Sprite Viewer)
Lynx doesn't have traditional tiles — it uses sprite-based rendering exclusively. PPU tools should show:
1. **Frame buffer** — Current display buffer (pointed to by DISPADR)
2. **Palette** — 16 colors with RGB values from Green/BlueRed registers
3. **Active sprites** — Parse SCB chain and show each sprite's properties
4. **Collision buffer** — Visualize the 16-slot collision depository

---

## 7. Mikey Implementation Details

### Timer System (Critical for Everything)
Mikey has 8 timers that form two chains:
```
Chain 1: Timer 0 → Timer 2 → Timer 4 (→ used by programmers)
Chain 2: Timer 1 → Timer 3 → Timer 5 → Timer 7
         Timer 6 ← Audio sample rate (standalone)
```

Timer assignments:
| Timer | Default Use | Linked From | Linked To |
|-------|-------------|-------------|-----------|
| 0 | Horizontal line | Clock source | Timer 2 |
| 1 | Audio Ch 0 reload | Clock source | Timer 3 |
| 2 | Vertical line (VBlank) | Timer 0 | Timer 4 |
| 3 | Audio Ch 1 reload | Timer 1 | Timer 5 |
| 4 | General purpose | Timer 2 | — |
| 5 | Audio Ch 2 reload | Timer 3 | Timer 7 |
| 6 | Audio sample rate | Clock source | — |
| 7 | Audio Ch 3 reload | Timer 5 | — |

**Each timer has:**
- `BACKUP` — Reload value
- `CTLA` — Control register A (clock source, enable, linking, reset-on-done, magic-tap-enable)
- `COUNT` — Current countdown value
- `CTLB` — Status (timer done, last clock, borrow-in, borrow-out)

**Timer Tick Algorithm:**
```cpp
void LynxMikey::TickTimer(int timerIndex) {
	LynxTimer& t = _timers[timerIndex];
	if (!(t.ControlA & 0x08)) return; // Timer disabled

	if (t.Count == 0) {
		// Timer expired
		t.ControlB |= 0x01; // Set timer-done flag

		// Reload from backup
		t.Count = t.BackupValue;

		// Fire IRQ if enabled
		if (t.ControlA & 0x80) {
			_irqPending |= (1 << timerIndex);
			UpdateIrqLine();
		}

		// Cascade to linked timer (if linking enabled)
		int linkedTimer = GetLinkedTimer(timerIndex);
		if (linkedTimer >= 0) {
			TickTimer(linkedTimer);
		}
	} else {
		t.Count--;
	}
}
```

### Display DMA
- Timer 0 fires at end of each horizontal line (~15.625 kHz)
- Timer 2 fires at end of each frame (~75 Hz, counts 105 Timer 0 fires: 102 visible + 3 VBlank)
- Display DMA reads frame buffer from RAM at address in DISPADR ($FD94) on each line
- Each line: 80 bytes (160 pixels × 4bpp = 80 bytes), read MSN first

### Audio System
- 4 channels, each with 12-bit LFSR (Linear Feedback Shift Register) waveform
- Timer 6 generates the sample rate (~15.9 kHz default)
- Each channel `OUTPUT` register provides signed 8-bit audio
- Integration mode: channels can feed into each other
- `ATTEN` registers provide L/R stereo panning

### Interrupt Controller
- 8 IRQ sources (one per timer)
- Two registers: `INTSET` (set/read pending), `INTRST` (clear pending)
- `MIKEYHREV` at $FD88 — read for Mikey revision, write to enable IRQs
- No NMI on Lynx! Only IRQ through the 65C02 IRQ line.
- Pause button generates Timer 0... actually, it sets a bit in the interrupt controller directly.

---

## 8. Suzy Implementation Details

### Sprite Engine
The sprite engine processes a linked list of Sprite Control Blocks (SCBs) in RAM:

**SCB Format (variable length, 8-18 bytes minimum):**
```
Byte 0: SPRCTL0 — Sprite type, depth, flip H/V
Byte 1: SPRCTL1 — Start drawing action, reload mode
Byte 2: COLLOFF — Collision depository number
Byte 3-4: SCBNEXT — Pointer to next SCB (little-endian)
Byte 5-6: SPRDLINE — Pointer to sprite data
Byte 7-8: HPOS — Horizontal position
Byte 9-10: VPOS — Vertical position
Byte 11-12: HSIZE — Horizontal stretch (8.8 fixed point)
Byte 13-14: VSIZE — Vertical stretch (8.8 fixed point)
Byte 15-16: HSIZOFF — H stretch accumulator
Byte 17-18: VSIZOFF — V stretch accumulator
```

**8 Sprite Types:**
| Type | Encoding | Description |
|------|----------|-------------|
| 0 | Background (no data) | Fill with SPRCOL |
| 1 | Normal | Packed pixel data |
| 2 | Boundary | Packed, respects HSIZE boundaries |
| 3 | Normal shadow | Like 1 but uses shadow palette |
| 4 | Boundary shadow | Like 2 with shadow |
| 5 | Non-collidable | Renders but no collision |
| 6 | XOR shadow | XOR blending |
| 7 | Shadow | Shadow rendering |

**Sprite Data Encoding:**
- Run-length encoded with literal and repeat packets
- 1/2/3/4 bits per pixel (set by SPRCTL0)
- Bit-packed with offset and repeat information
- Each scan line has a header byte (line length or 0 for end)

### Hardware Math
```
MATHC:MATHD × MATHE:MATHF → MATHG:MATHH:MATHJ:MATHK (32-bit result)
MATHG:MATHH:MATHJ:MATHK ÷ MATHE:MATHF → MATHC:MATHD remainder MATHG:MATHH (quotient)
```
- 16×16 → 32 multiply takes ~54 CPU cycles
- 32÷16 → 16r16 divide takes ~176 CPU cycles
- Optional signed mode and accumulate mode
- **Hardware bug:** Unsigned multiply of values ≥$8000 produces wrong results

### Collision Detection
- 16-slot collision depository (`$FC80-$FC8F`)
- During sprite rendering, if pixel overlaps existing collision number, both sprite's and existing collision slot get updated
- Games use this for hit detection without per-pixel CPU checks

### Joystick
- JOYSTICK register at `$FCB0` — directly connected to buttons
- Active-low: bit=0 means pressed
- Bits: [7:Up] [6:Down] [5:Left] [4:Right] [3:Opt1] [2:Opt2] [1:B_Inside] [0:A_Outside]

### SWITCHES register at `$FCB1`
- Bit 0: Cart 0 / Cart 1 (cartridge bank select)
- Bit 1: Unused
- Bit 2: Pause (active-low)
- Bit 4: Bus request grant

---

## 9. Pansy Export Integration

### What's Needed
1. Add `RomFormat.Lynx` to `PlatformIds` dictionary with a Pansy platform ID
2. Add `ConsoleType.Lynx → CpuType.Lynx` in `ConsoleTypeExtensions.GetMainCpuType()`
3. Register Lynx `MemoryType` entries in `MemoryTypeExtensions` for region naming
4. Everything else works automatically through the debugger infrastructure (CDL, labels, memory regions)

### Memory Regions for Pansy
```csharp
case ConsoleType.Lynx:
	regions.Add(new PansyMemoryRegion("RAM", 0x0000, 0xFBFF, MemoryType.LynxWorkRam));
	regions.Add(new PansyMemoryRegion("Suzy", 0xFC00, 0xFCFF, MemoryType.LynxSuzyRegisters));
	regions.Add(new PansyMemoryRegion("Mikey", 0xFD00, 0xFDFF, MemoryType.LynxMikeyRegisters));
	regions.Add(new PansyMemoryRegion("ROM", 0xFE00, 0xFFF7, MemoryType.LynxBootRom));
	regions.Add(new PansyMemoryRegion("Vectors", 0xFFF8, 0xFFFF, MemoryType.LynxBootRom));
	break;
```

---

## 10. Complete File Modification Map

### New Files (~20)
| File | Purpose |
|------|---------|
| Core/Lynx/LynxConsole.h/.cpp | Main console class |
| Core/Lynx/LynxCpu.h/.cpp | 65C02 CPU |
| Core/Lynx/LynxMikey.h/.cpp | Mikey chip (timers, audio, video, IRQ) |
| Core/Lynx/LynxSuzy.h/.cpp | Suzy chip (sprites, math, collision, input) |
| Core/Lynx/LynxMemoryManager.h/.cpp | Memory management + MAPCTL |
| Core/Lynx/LynxCart.h/.cpp | Cartridge + EEPROM |
| Core/Lynx/LynxControlManager.h/.cpp | Input management |
| Core/Lynx/LynxController.h/.cpp | Controller device |
| Core/Lynx/LynxDefaultVideoFilter.h/.cpp | Video filter |
| Core/Lynx/LynxTypes.h | All state structs and constants |
| Core/Lynx/Debugger/LynxDebugger.h/.cpp | Debugger implementation |
| Core/Lynx/Debugger/LynxDisUtils.h/.cpp | Disassembly |
| Core/Lynx/Debugger/LynxTraceLogger.h/.cpp | Trace logging |
| Core/Lynx/Debugger/LynxEventManager.h/.cpp | Event timeline |
| Core/Lynx/Debugger/LynxPpuTools.h/.cpp | Sprite/palette viewer |
| Core/Lynx/Debugger/DummyLynxCpu.h/.cpp | Predictive CPU for breakpoints |
| UI/Config/LynxConfig.cs | C# config class |
| UI/Debugger/RegisterViewer/LynxRegisterViewer.cs | Register tabs |
| UI/Debugger/StatusViews/LynxStatusViewModel.cs | CPU status |

### Existing Files to Modify (~33)
See lynx-code-plans.md and the architecture audit for the complete list.

---

## 11. Implementation Priority Order

### Phase 1: Foundation (Sessions 1-2)
1. Enums (ConsoleType, CpuType, MemoryType, RomFormat) — C++ and C#
2. LynxTypes.h — All state structs
3. LynxConsole skeleton — Stub IConsole
4. LynxMemoryManager — MAPCTL, RAM
5. LynxCpu — Opcodes, addressing modes
6. LynxCart — LNX header parsing, ROM loading
7. Build system — vcxproj entries

### Phase 2: Mikey Core (Sessions 3-4)
1. Timer system (all 8 timers with linking)
2. Interrupt controller
3. Display DMA — frame buffer reading
4. Palette — 16 entries from Green/BlueRed registers
5. Audio channels (basic output)
6. LynxDefaultVideoFilter

### Phase 3: Suzy Core (Sessions 5-6)
1. Sprite engine — SCB parsing, basic rendering
2. Hardware math — multiply and divide
3. Collision detection — 16-slot depository
4. Joystick input
5. EEPROM serial protocol

### Phase 4: Integration (Sessions 7-8)
1. UI registration — All C# switch statements
2. Config system — LynxConfig, settings
3. Input mapping — Controller UI
4. Save states — All Serialize() methods
5. Battery/EEPROM saves

### Phase 5: Debugger (Sessions 9-10)
1. LynxDebugger — Core IDebugger
2. LynxDisUtils — 65C02 disassembly
3. Breakpoints, watches, memory views
4. Register viewer tabs
5. Event manager timeline
6. PPU tools (sprite/palette viewer)

### Phase 6: Polish (Sessions 11-12)
1. Movie/TAS support
2. Pansy export
3. ROM database
4. Game compatibility testing
5. Performance optimization
