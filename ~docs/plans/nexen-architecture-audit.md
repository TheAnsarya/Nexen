# Nexen Console Architecture Audit — Adding a New System

> **Purpose:** Complete checklist of all integration points for adding a new console (Atari Lynx) to Nexen  
> **Based on:** WonderSwan reference implementation, existing multi-console architecture  
> **Created:** 2026-02-15  

---

## 1. Enum Registration Points

These enums need a new `Lynx` (or `Lynx*`) value:

### C++ Enums

| Enum | File | New Value | Notes |
|------|------|-----------|-------|
| `ConsoleType` | `Core/Shared/BaseState.h` | `Lynx = 7` | After WonderSwan (6) |
| `CpuType` | `Core/Shared/BaseState.h` | `Lynx` | After CpuType::Ws |
| `MemoryType` | `Core/Shared/MemoryType.h` | `LynxMemory`, `LynxPrgRom`, `LynxWorkRam`, `LynxBootRom`, `LynxCartRom`, `LynxSaveRam` | ~6 entries |
| `RomFormat` | `Core/Shared/RomFormat.h` | `Lynx` | ROM detection |
| `DebuggerFlags` | `Core/Debugger/DebugTypes.h` | `LynxDebuggerEnabled = 1 << 13` | After WS bit |
| `SnesMemoryType` variants | Used as generic MemoryType | Add Lynx variants | Memory viewer |

### C# Enum Mirrors

| Enum | File | New Value |
|------|------|-----------|
| `ConsoleType` | `UI/Interop/ConsoleType.cs` | `Lynx = 7` |
| `CpuType` | `UI/Interop/CpuType.cs` | `Lynx` |
| `MemoryType` | `UI/Interop/MemoryType.cs` | All Lynx memory types |
| `RomFormat` | `UI/Interop/RomFormat.cs` | `Lynx` |

### Sentinel Updates

| Function | File | Change |
|----------|------|--------|
| `CpuTypeUtilities::GetCpuTypeCount()` | `Core/Shared/CpuTypeUtilities.h` | Update count |
| `DebugUtilities::GetLastCpuType()` | `Core/Shared/DebugUtilities.h` | Update sentinel |

---

## 2. Interface Implementation

### IConsole (20+ pure virtual methods)

The main console class must implement:

```cpp
class LynxConsole : public IConsole {
public:
	// Core
	void Reset() override;
	LoadRomResult LoadRom(VirtualFile romFile) override;
	void RunFrame() override;
	void ProcessEndOfFrame() override;

	// Subsystem access
	BaseControlManager* GetControlManager() override;
	ConsoleType GetConsoleType() override;
	vector<CpuType> GetCpuTypes() override;
	DipSwitchInfo GetDipSwitchInfo() override;

	// State
	void Serialize(Serializer& s) override;
	SaveStateCompatInfo ValidateSaveStateCompatibility(Serializer& s) override;

	// Region/timing
	TimingInfo GetTimingInfo() override;
	BaseVideoFilter* GetVideoFilter(bool get498EmptyFilter) override;
	double GetFps() override;

	// Audio/video
	uint32_t GetMasterClockRate() override;
	AudioTrackInfo GetAudioTrackInfo() override;

	// Debug
	void GetConsoleSpecificStatus(StatusSummary& summary) override;
	AddressInfo GetAbsoluteAddress(AddressInfo& relAddr) override;
	AddressInfo GetRelativeAddress(AddressInfo& absAddr, CpuType cpuType) override;
};
```

### IDebugger (10+ pure virtual methods)

```cpp
class LynxDebugger : public IDebugger {
public:
	void Step(int32_t stepCount, StepType type) override;
	void Reset() override;

	uint64_t GetProgramCounter(bool adjusted) override;

	BreakpointManager* GetBreakpointManager() override;
	ITraceLogger* GetTraceLogger() override;
	IAssembler* GetAssembler() override;
	CallstackManager* GetCallstackManager() override;
	BaseEventManager* GetEventManager() override;

	void ProcessInputOverride(DebugControllerState& input) override;
	void SetProgramCounter(uint64_t addr) override;
};
```

---

## 3. WonderSwan Reference — File Structure (~35 files)

The WonderSwan console is the closest reference for complexity level:

```text
Core/WS/
├── WsConsole.h/.cpp         # Main console (IConsole)
├── WsCpu.h/.cpp             # V30MZ CPU
├── WsMemoryManager.h/.cpp   # Memory mapping
├── WsControlManager.h/.cpp  # Input handling
├── WsController.h           # Button definitions
├── WsEeprom.h/.cpp          # Save data
├── WsPpu.h/.cpp             # Display processor
├── WsTimer.h/.cpp           # System timer
├── WsSerial.h/.cpp          # Serial port
├── WsDmaController.h/.cpp   # DMA
├── WsTypes.h                # State structs
├── WsDefaultVideoFilter.h/.cpp  # Video output
├── APU/
│   ├── WsApu.h/.cpp         # Audio unit
│   └── WsApuCh*.h/.cpp      # Per-channel
├── Carts/
│   └── WsCart.h/.cpp         # Cartridge
└── Debugger/
    ├── WsDebugger.h/.cpp     # IDebugger
    ├── WsTraceLogger.h/.cpp  # Trace log
    ├── WsDisUtils.h/.cpp     # Disassembler
    ├── WsAssembler.h/.cpp    # Assembler
    ├── WsEventManager.h/.cpp # Event timeline
    └── WsPpuTools.h/.cpp     # Display tools
```

---

## 4. Files Requiring Modification (~40 files)

### Core Registration & Routing

| # | File | Change |
|---|------|--------|
| 1 | `Core/Shared/Emulator.cpp` | Add `TryLoadRom<LynxConsole>()` in ROM format detection |
| 2 | `Core/Shared/BaseState.h` | Add ConsoleType::Lynx, CpuType::Lynx |
| 3 | `Core/Shared/MemoryType.h` | Add LynxMemory, LynxWorkRam, etc. |
| 4 | `Core/Shared/MemoryType.Extensions.cpp` | Add memory type metadata (name, size, etc.) |
| 5 | `Core/Shared/CpuTypeUtilities.h` | Update GetCpuTypeCount() |
| 6 | `Core/Shared/RomFormat.h` | Add RomFormat::Lynx |
| 7 | `Core/Shared/FirmwareType.h` | Add FirmwareType::LynxBootRom |

### Debugger Core

| # | File | Change |
|---|------|--------|
| 8 | `Core/Debugger/Debugger.cpp` | Factory switch for LynxDebugger creation |
| 9 | `Core/Debugger/DebugTypes.h` | Add LynxDebuggerEnabled flag |
| 10 | `Core/Debugger/DebugUtilities.h` | Add switch cases for Lynx CpuType |
| 11 | `Core/Debugger/MemoryDumper.h` | Add Lynx memory type mapping |
| 12 | `Core/Debugger/DisassemblyInfo.h` | Add Lynx disassembly support |
| 13 | `Core/Debugger/LabelManager.h` | Add Lynx label support |
| 14 | `Core/Debugger/ScriptManager.cpp` | Add Lynx script handlers |
| 15 | `Core/Debugger/CodeDataLogger.h` | Add Lynx CDL support |
| 16 | `Core/Debugger/ExpressionEvaluator.h` | Add Lynx token support |

### Interop DLL

| # | File | Change |
|---|------|--------|
| 17 | `InteropDLL/DebugApiWrapper.cpp` | Add Lynx state getters |
| 18 | `InteropDLL/EmuApiWrapper.cpp` | Add Lynx config getters/setters |
| 19 | `InteropDLL/ConsoleSpecificInfo.cpp` | Add Lynx info switches |

### UI — C# Files

| # | File | Change |
|---|------|--------|
| 20 | `UI/Interop/ConsoleType.cs` | Add Lynx enum value |
| 21 | `UI/Interop/CpuType.cs` | Add Lynx enum value |
| 22 | `UI/Interop/MemoryType.cs` | Add Lynx memory types |
| 23 | `UI/Interop/RomFormat.cs` | Add Lynx ROM format |
| 24 | `UI/Interop/EmuApi.cs` | Add Lynx interop methods |
| 25 | `UI/Interop/DebugApi.cs` | Add Lynx debug interop |
| 26 | `UI/Config/Configuration.cs` | Add LynxConfig property |
| 27 | `UI/Utilities/FolderHelper.cs` | Add .lnx, .lyx extensions |
| 28 | `UI/Utilities/FileDialogHelper.cs` | Add Lynx file filters |
| 29 | `UI/Views/MainMenuView.axaml` | Add Lynx menu items |
| 30 | `UI/ViewModels/MainMenuViewModel.cs` | Add Lynx handlers |
| 31 | `UI/ViewModels/ShortcutHandler.cs` | Add Lynx shortcuts |
| 32 | `UI/Debugger/DebugWindowManager.cs` | Add Lynx debugger window |
| 33 | `UI/Debugger/ViewModels/DebuggerViewModel.cs` | Add Lynx support |
| 34 | `UI/Debugger/ViewModels/MemoryMappingViewModel.cs` | Add Lynx memory map |

### Build System

| # | File | Change |
|---|------|--------|
| 35 | `Core/Core.vcxproj` | Add all new Lynx source files |
| 36 | `Core/Core.vcxproj.filters` | Add Lynx filter group |
| 37 | `makefile` | Add Lynx source files to build |

### MovieConverter / TAS

| # | File | Change |
|---|------|--------|
| 38 | `MovieConverter/MovieConverter.cs` | Add Lynx input format |
| 39 | `UI/TAS/PianoRollControl.cs` | Add Lynx button layout |
| 40 | `UI/TAS/TasLuaApi.cs` | Add Lynx input API |

### Pansy Export

| # | File | Change |
|---|------|--------|
| 41 | `UI/Pansy/PansyExporter.cs` | Add Lynx platform support |
| 42 | `UI/Pansy/PansyPlatform.cs` | Add Lynx platform definition |

---

## 5. Lynx-Specific Architectural Considerations

### Memory Overlay (MAPCTL)

The Lynx has a unique memory overlay system controlled by the MAPCTL register ($FFF9):

- Bit 0: FE00-FFFF — Vector space (ROM or RAM)
- Bit 1: Mikey I/O page ($FD00-$FDFF)
- Bit 2: Suzy I/O page ($FC00-$FCFF)
- Bit 3: ROM at $FE00-$FFFF
- Bit 6-7: Sequential cart disable

This requires careful memory manager implementation — the same address can map to different things.

### Bus Contention

The Lynx has only one data bus shared by 4 bus masters:

1. CPU (lowest priority)
2. Refresh
3. Mikey (video DMA)
4. Suzy (sprite engine)

When Suzy is doing sprites, the CPU is halted. This affects timing.

### No NMI

Unlike the NES/SNES/GB, the Lynx does NOT use NMI for frame timing. Instead:

- Timer interrupts (IRQ) drive everything
- VBL is signaled by Timer 0 or Timer 2 depending on configuration
- HBL is signaled by Timer 0

### Screen Rotation

Some games are designed to be played with the Lynx rotated 90° or 180°:

- Hardware supports screen flipping via DISPCTL register
- Emulator needs to handle button remapping for rotated modes
- ROM database includes rotation hints per game

### Save Data

Not all games have save data. Those that do use EEPROM:

- 93C46 (128×8 or 64×16) — most common
- 93C66 (256×16) — some games
- 93C86 (1024×16) — rare

EEPROM type is determined per-game (needs ROM database or auto-detection).
