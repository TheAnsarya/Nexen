# NES Emulation Core Documentation

The NES (Nintendo Entertainment System) emulation core provides cycle-accurate emulation of the original NES/Famicom hardware.

## Architecture Overview

```text
NES Core Architecture
═════════════════════

┌──────────────────────────────────────────────────────────────┐
│						NesConsole							│
│  (Main coordinator - owns all components, runs main loop)	│
└──────────────────────────────────────────────────────────────┘
			  │
	┌─────────┼─────────┬─────────────┬─────────────┐
	│		 │		 │			 │			 │
	▼		 ▼		 ▼			 ▼			 ▼
┌───────┐ ┌───────┐ ┌─────────┐ ┌─────────┐ ┌───────────────┐
│NesCpu │ │NesPpu │ │NesApu   │ │Mapper   │ │MemoryManager  │
│	   │ │	   │ │		 │ │(ROM)	│ │			   │
│ 2A03  │ │ 2C02  │ │ 2A03	│ │Various  │ │ Bus routing   │
│ (6502)│ │(Video)│ │ (Audio) │ │ types   │ │ & mirroring   │
└───────┘ └───────┘ └─────────┘ └─────────┘ └───────────────┘
```

## Directory Structure

```text
Core/NES/
├── Main Components
│   ├── NesConsole.h/cpp		- Console coordinator
│   ├── NesCpu.h/cpp			- 6502 CPU emulation
│   ├── NesPpu.h/cpp			- 2C02 PPU (Picture Processing Unit)
│   ├── NesMemoryManager.h/cpp  - Memory bus & I/O
│   └── NesSoundMixer.h/cpp	 - Audio mixing
│
├── APU (Audio Processing Unit)
│   └── APU/					- 2A03 audio channels
│	   ├── SquareChannel	   - Square wave (2 channels)
│	   ├── TriangleChannel	 - Triangle wave
│	   ├── NoiseChannel		- Noise generator
│	   ├── DeltaModChannel	 - DPCM sample playback
│	   └── ApuFrameCounter	 - Frame sequencer
│
├── Mappers (Cartridge Hardware)
│   ├── BaseMapper.h/cpp		- Mapper base class
│   ├── MapperFactory.h/cpp	 - Mapper instantiation
│   └── Mappers/				- Mapper implementations
│	   ├── MMC1, MMC3, MMC5	- Nintendo mappers
│	   ├── VRC2/4/6/7		  - Konami mappers
│	   ├── Sunsoft FME-7	   - Sunsoft mapper
│	   └── 200+ other mappers  - Full compatibility
│
├── Input
│   └── Input/				  - Controller handling
│	   ├── Standard controllers
│	   ├── Zapper light gun
│	   ├── Power Pad
│	   └── Various peripherals
│
├── Video Filters
│   ├── NesDefaultVideoFilter   - Basic RGB output
│   ├── NesNtscFilter		   - NTSC composite simulation
│   └── BisqwitNtscFilter	   - Advanced NTSC filter
│
├── HD Packs
│   └── HdPacks/				- HD graphics replacement
│
├── Debugger Integration
│   └── Debugger/			   - NES-specific debug features
│
├── ROM Loading
│   └── Loaders/				- iNES, NES 2.0, UNIF, FDS
│
└── Support Files
	├── NesTypes.h			  - NES-specific types & enums
	├── NesConstants.h		  - Hardware constants
	├── NesHeader.h/cpp		 - ROM header parsing
	└── GameDatabase.h/cpp	  - Game-specific fixes
```

## Core Components

### NesConsole (NesConsole.h)

The main coordinator that owns all NES components and manages the emulation loop.

**Responsibilities:**

- Initialize and connect components
- Run the main frame loop
- Handle state serialization
- Manage debugging features

**Key Methods:**

```cpp
void Run(uint64_t runUntilMasterClock);  // Main emulation loop
void Reset(bool softReset);			   // Reset console
void PowerCycle();						// Full power cycle
```

### NesCpu (NesCpu.h)

Emulates the Ricoh 2A03 (NTSC) / 2A07 (PAL) CPU, a modified MOS 6502.

**Key Differences from 6502:**

- No decimal mode (BCD operations do nothing)
- Integrated audio APU
- DMA controller for OAM transfers

**Features:**

- All 56 official opcodes
- Unofficial/undocumented opcodes
- Cycle-accurate execution
- Interrupt handling (NMI, IRQ, RESET)

**CPU State:**

```cpp
struct NesCpuState {
	uint16_t PC;		// Program Counter
	uint8_t SP;		 // Stack Pointer
	uint8_t A;		  // Accumulator
	uint8_t X;		  // X Index Register
	uint8_t Y;		  // Y Index Register
	uint8_t PS;		 // Processor Status (flags)
	uint8_t IRQFlag;	// IRQ pending flags
	bool NMIFlag;	   // NMI pending
	uint64_t CycleCount;// Total CPU cycles
};
```

**Addressing Modes:**

- Immediate, Zero Page, Zero Page X/Y
- Absolute, Absolute X/Y
- Indirect, Indexed Indirect (X), Indirect Indexed (Y)

### NesPpu (NesPpu.h)

Emulates the 2C02 Picture Processing Unit.

**Key Features:**

- 256×240 pixel output (262 scanlines NTSC, 312 PAL)
- 8×8 pixel tiles, 64 sprites max (8 per scanline)
- 2KB internal VRAM + cartridge CHR ROM/RAM
- 256-byte OAM (Object Attribute Memory)

**PPU Registers ($2000-$2007):**

| Address | Name | Description |
| --------- | ------ | ------------- |
| $2000 | PPUCTRL | Control register |
| $2001 | PPUMASK | Mask register |
| $2002 | PPUSTATUS | Status register |
| $2003 | OAMADDR | OAM address |
| $2004 | OAMDATA | OAM data |
| $2005 | PPUSCROLL | Scroll position |
| $2006 | PPUADDR | VRAM address |
| $2007 | PPUDATA | VRAM data |

**PPU State:**

```cpp
struct NesPpuState {
	uint16_t Cycle;		 // Current dot (0-340)
	uint16_t Scanline;	  // Current scanline (-1 to 260)
	uint16_t VideoRamAddr;  // VRAM address (v)
	uint16_t TmpVideoRamAddr; // Temp VRAM address (t)
	uint8_t ScrollX;		// Fine X scroll (0-7)
	uint8_t WriteToggle;	// First/second write toggle
	// ... plus many internal latches
};
```

### APU (APU/)

The Audio Processing Unit provides 5 audio channels:

**Channels:**

1. **Square 1/2** - Square wave with duty cycle, sweep, envelope
2. **Triangle** - Triangle wave with linear counter
3. **Noise** - Pseudo-random noise generator
4. **DMC** - Delta Modulation Channel (sample playback)

**Frame Counter:**

- Clocks envelope and sweep units
- 4-step or 5-step mode
- Generates IRQ (4-step mode only)

### Memory Map

```text
CPU Memory Map ($0000-$FFFF)
════════════════════════════
$0000-$07FF  Internal RAM (2KB, mirrored 4x)
$0800-$1FFF  RAM mirrors
$2000-$2007  PPU registers
$2008-$3FFF  PPU register mirrors
$4000-$4017  APU and I/O registers
$4018-$401F  APU test registers (disabled)
$4020-$FFFF  Cartridge space (PRG-ROM/RAM, mapper)

PPU Memory Map ($0000-$3FFF)
════════════════════════════
$0000-$0FFF  Pattern Table 0 (CHR-ROM/RAM)
$1000-$1FFF  Pattern Table 1 (CHR-ROM/RAM)
$2000-$23FF  Nametable 0
$2400-$27FF  Nametable 1
$2800-$2BFF  Nametable 2 (usually mirror of 0/1)
$2C00-$2FFF  Nametable 3 (usually mirror of 0/1)
$3000-$3EFF  Nametable mirrors
$3F00-$3F1F  Palette RAM
$3F20-$3FFF  Palette mirrors
```

### Mappers (Mappers/)

Cartridge hardware abstraction. Mappers handle:

- PRG-ROM/RAM banking
- CHR-ROM/RAM banking
- Nametable mirroring control
- IRQ generation
- Expansion audio

**Common Mappers:**

| # | Name | Games |
| --- | ------ | ------- |
| 0 | NROM | SMB, Donkey Kong |
| 1 | MMC1 | Zelda, Metroid |
| 2 | UxROM | Mega Man, Contra |
| 4 | MMC3 | SMB3, TMNT |
| 5 | MMC5 | Castlevania III |
| 7 | AxROM | Battletoads |

### GameDatabase (GameDatabase.h)

Contains game-specific fixes and information:

- CRC-based game identification
- System type detection (NTSC/PAL/Dendy)
- Board type identification
- Fixes for problematic ROMs

## Timing

**NTSC (2A03):**

- CPU: 1.789773 MHz
- PPU: 5.369318 MHz (3x CPU)
- Frame: 60.0988 Hz (262 scanlines × 341 dots)

**PAL (2A07):**

- CPU: 1.662607 MHz
- PPU: 5.320342 MHz (3.2x CPU)
- Frame: 50.0070 Hz (312 scanlines × 341 dots)

## Integration Points

### With Shared Infrastructure

- Uses `Serializer` for save states
- Uses `SimpleLock` for thread safety
- Uses debugger interface for step/trace

### With Emulator

- Reports timing via master clock
- Provides video/audio buffers
- Handles input polling

## Debugging Features

Located in `Debugger/`:

- CPU disassembly with symbol support
- PPU viewer (nametables, CHR, sprites)
- Memory viewer/editor
- Breakpoints (read/write/exec)
- Trace logging
- State history (rewind)

## HD Packs

Located in `HdPacks/`:

- Replace original graphics with high-resolution art
- Condition-based rules for context-aware replacement
- Background music replacement
- Custom palettes

## Related Documentation

- [UTILITIES-LIBRARY.md](UTILITIES-LIBRARY.md) - Shared utilities
- [CODE-DOCUMENTATION-STYLE.md](CODE-DOCUMENTATION-STYLE.md) - Documentation standards
- [NES Wiki](https://www.nesdev.org/wiki/) - External NES technical reference
