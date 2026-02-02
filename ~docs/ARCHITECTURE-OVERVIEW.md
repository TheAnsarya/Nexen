# Nexen Architecture Overview

Nexen is a multi-system emulator supporting NES, SNES, Game Boy, GBA, SMS, PC Engine, and WonderSwan. This document provides a high-level overview of the codebase architecture.

## Project Structure

```text
Nexen/
├── Core/				   # C++ emulation cores (this document)
│   ├── NES/				# Nintendo Entertainment System
│   ├── SNES/			   # Super Nintendo
│   ├── Gameboy/			# Game Boy / Game Boy Color
│   ├── GBA/				# Game Boy Advance
│   ├── SMS/				# Sega Master System / Game Gear
│   ├── PCE/				# PC Engine / TurboGrafx-16
│   ├── WS/				 # WonderSwan / WonderSwan Color
│   ├── Shared/			 # Cross-platform shared code
│   ├── Debugger/		   # Unified debugger framework
│   └── Netplay/			# Network play support
│
├── Utilities/			  # Shared utility library
│   ├── Serializer		  # State save/load
│   ├── File I/O			# VirtualFile, archives
│   ├── Graphics filters	# HQ2x, xBRZ, NTSC, etc.
│   └── Platform abstraction
│
├── Core.Tests/			 # C++ unit tests (Google Test)
├── Core.Benchmarks/		# Performance benchmarks (Google Benchmark)
│
├── InteropDLL/			 # C++/C# interop layer
│   └── EmuApiHandler	   # Expose C++ API to .NET
│
├── UI/					 # .NET UI application
│   ├── Views/			  # Avalonia XAML views
│   ├── ViewModels/		 # MVVM view models
│   └── Debugger/		   # Debugger windows
│
└── Linux/				  # Linux-specific code
```

## System Layers

```text
┌─────────────────────────────────────────────────────────────────┐
│						UI Layer (C#/.NET)						│
│  Avalonia cross-platform UI, debugger windows, settings, menus  │
└─────────────────────────────────────────────────────────────────┘
								│
								│ InteropDLL (P/Invoke)
								│
┌─────────────────────────────────────────────────────────────────┐
│					  Emulator (C++)							  │
│  Main emulation controller - manages consoles, runs frames	   │
└─────────────────────────────────────────────────────────────────┘
								│
		┌───────────┬───────────┼───────────┬───────────┐
		│		   │		   │		   │		   │
		▼		   ▼		   ▼		   ▼		   ▼
   ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐
   │   NES   │ │  SNES   │ │   GB	│ │   GBA   │ │  SMS/   │
   │ Console │ │ Console │ │ Console │ │ Console │ │PCE/WS   │
   └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘
		│		   │		   │		   │		   │
		└───────────┴───────────┼───────────┴───────────┘
								│
┌─────────────────────────────────────────────────────────────────┐
│					  Shared Infrastructure					   │
│  Serializer, Debugger, Settings, Video Output, Audio Mixing	  │
└─────────────────────────────────────────────────────────────────┘
```

## Core Component Pattern

Each emulated system follows a consistent pattern:

```cpp
class [System]Console : public IConsole, public ISerializable {
	// Owns all system components
	unique_ptr<[System]Cpu> _cpu;
	unique_ptr<[System]Ppu> _ppu;
	unique_ptr<[System]MemoryManager> _memoryManager;
	unique_ptr<BaseMapper> _mapper;  // Cartridge hardware
	
	// Main emulation loop
	void Run(uint64_t runUntilMasterClock);
	
	// Frame timing
	void RunFrame();
	
	// State management
	void Serialize(Serializer& s) override;
};
```

## Key Interfaces

### IConsole
Base interface for all emulated systems:

```cpp
class IConsole : public ISerializable {
	virtual void Run(uint64_t masterClock) = 0;
	virtual void Reset(bool softReset) = 0;
	virtual ConsoleType GetConsoleType() = 0;
	virtual double GetFps() = 0;
	// ... video, audio, input interfaces
};
```

### ISerializable
All stateful objects implement serialization:

```cpp
class ISerializable {
	virtual void Serialize(Serializer& s) = 0;
};
```

### BaseMapper
Cartridge hardware abstraction:

```cpp
class BaseMapper : public ISerializable {
	virtual uint8_t ReadRam(uint16_t addr) = 0;
	virtual void WriteRam(uint16_t addr, uint8_t value) = 0;
	virtual uint8_t ReadChr(uint16_t addr) = 0;
	// ... banking, mirroring, IRQ
};
```

## Memory Architecture

Each system has a memory manager that routes reads/writes:

```text
CPU Read/Write Request
		 │
		 ▼
┌─────────────────────┐
│   Memory Manager	│
│  ┌───────────────┐  │
│  │  Address Map  │  │
│  │  $0000-$07FF → RAM
│  │  $2000-$2007 → PPU
│  │  $4000-$401F → APU/IO
│  │  $4020-$FFFF → Mapper
│  └───────────────┘  │
└─────────────────────┘
```

## Timing Model

Nexen uses a master clock for precise timing:

```cpp
// Each system has a master clock relationship
// NES:  CPU runs at 1/3 master clock, PPU at master clock
// SNES: CPU varies, PPU at master clock

uint64_t _masterClock;  // Absolute timing reference

void Console::Run(uint64_t targetClock) {
	while (_masterClock < targetClock) {
		// Step CPU
		_cpu->Exec();
		
		// PPU runs proportionally
		_ppu->Run(_masterClock);
		
		// APU runs at its rate
		_apu->Run(_masterClock);
	}
}
```

## Debugger Architecture

The unified debugger supports all systems:

```text
┌─────────────────────────────────────────────┐
│			  Debugger Framework			 │
├─────────────────────────────────────────────┤
│ ┌─────────────┐  ┌─────────────────────────┐│
│ │ Breakpoints │  │   Disassembler		  ││
│ │ - Read	  │  │   - System-specific	 ││
│ │ - Write	 │  │   - Symbol support	  ││
│ │ - Execute   │  │   - Code labels		 ││
│ └─────────────┘  └─────────────────────────┘│
│ ┌─────────────┐  ┌─────────────────────────┐│
│ │ Watch	   │  │   Trace Logger		  ││
│ │ - Registers │  │   - Per-instruction log ││
│ │ - Memory	│  │   - Conditional logging ││
│ └─────────────┘  └─────────────────────────┘│
└─────────────────────────────────────────────┘
```

## State Serialization

Save states use a bidirectional serializer:

```cpp
void NesCpu::Serialize(Serializer& s) {
	// Same code for save and load!
	SV(_state.PC);
	SV(_state.SP);
	SV(_state.A);
	SV(_state.X);
	SV(_state.Y);
	SV(_state.PS);
	
	// Arrays
	SVArray(_ram, sizeof(_ram));
	
	// Nested objects
	SV(_apu);  // Calls _apu->Serialize(s)
}
```

## Video Pipeline

```text
PPU Frame Buffer ──► Video Filter ──► Display Output
	 (raw pixels)	   (HQ2x/etc)	   (SDL/UI)
```

**Filters:**

- Raw/None
- NTSC composite simulation
- HQ2x/3x/4x
- xBRZ
- Scale2x/3x
- Various CRT shaders

## Audio Pipeline

```text
┌─────────┐   ┌─────────┐   ┌─────────┐
│Channel 1│   │Channel 2│   │Channel N│
└────┬────┘   └────┬────┘   └────┬────┘
	 │			 │			 │
	 └──────┬──────┴──────┬──────┘
			│			 │
			▼			 ▼
	 ┌─────────────────────────┐
	 │	 Sound Mixer		 │
	 │  - Resampling		   │
	 │  - Volume/Balance	   │
	 │  - Low-pass filter	  │
	 └───────────┬─────────────┘
				 │
				 ▼
	 ┌─────────────────────────┐
	 │	Audio Output		 │
	 │  - SDL Audio / WASAPI   │
	 └─────────────────────────┘
```

## Supported Systems

| System | CPU | Resolution | Audio |
| -------- | ----- | ------------ | ------- |
| **NES** | 6502 (2A03) | 256×240 | 5 channels |
| **SNES** | 65816 | 256-512×224-478 | 8 channels (SPC700) |
| **Game Boy** | LR35902 | 160×144 | 4 channels |
| **GBA** | ARM7TDMI | 240×160 | 6 channels |
| **SMS** | Z80 | 256×192 | 3 channels (PSG) |
| **PCE** | HuC6280 | 256-512×224-242 | 6 channels |
| **WonderSwan** | V30MZ | 224×144 | 4 channels |

## Build System

**Windows:**

- Visual Studio solution (Nexen.sln)
- MSBuild for C++ projects
- .NET SDK for UI

**Linux:**

- Makefile-based build
- GCC or Clang
- .NET SDK for UI

**Dependencies:**

- vcpkg for C++ dependencies (Google Test, Google Benchmark)
- NuGet for .NET dependencies

## Testing

- **Unit Tests:** `Core.Tests/` using Google Test (420+ tests)
- **Benchmarks:** `Core.Benchmarks/` using Google Benchmark (220+ benchmarks)
- **Integration Tests:** RecordedRomTest for emulation accuracy

## Key Source Files

| File | Purpose |
| ------ | --------- |
| `Shared/Emulator.cpp` | Main emulator controller |
| `Shared/BaseConsole.h` | Console base class |
| `Utilities/Serializer.cpp` | State serialization |
| `InteropDLL/EmuApiHandler.cpp` | C++/C# bridge |
| `[System]/[System]Console.cpp` | Per-system main entry |

## Related Documentation

- [NES-CORE.md](NES-CORE.md) - NES-specific documentation
- [UTILITIES-LIBRARY.md](UTILITIES-LIBRARY.md) - Utility library
- [CODE-DOCUMENTATION-STYLE.md](CODE-DOCUMENTATION-STYLE.md) - Documentation standards
- [CPP-DEVELOPMENT-GUIDE.md](CPP-DEVELOPMENT-GUIDE.md) - C++ coding standards
