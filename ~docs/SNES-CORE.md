# SNES Emulation Core Documentation

The SNES (Super Nintendo Entertainment System) emulation core provides cycle-accurate emulation of the Super Famicom/SNES hardware, including all official coprocessors.

## Architecture Overview

```text
SNES Core Architecture
══════════════════════

┌────────────────────────────────────────────────────────────────┐
│						SnesConsole							 │
│   (Main coordinator - owns all components, runs main loop)	 │
└────────────────────────────────────────────────────────────────┘
			  │
	┌─────────┼─────────┬──────────────┬──────────────┐
	│		 │		 │			  │			  │
	▼		 ▼		 ▼			  ▼			  ▼
┌───────┐ ┌───────┐ ┌─────────┐ ┌───────────┐ ┌─────────────┐
│SnesCpu│ │SnesPpu│ │  SPC700 │ │  DSP	  │ │Coprocessors │
│	   │ │	   │ │		 │ │  (Audio)  │ │			 │
│ 65816 │ │ Video │ │ Sound   │ │  BRR	  │ │ SA-1, GSU,  │
│	   │ │	   │ │ CPU	 │ │  Samples  │ │ DSP, etc.   │
└───────┘ └───────┘ └─────────┘ └───────────┘ └─────────────┘
			  │
			  ▼
	┌─────────────────┐
	│  MemoryManager  │
	│  LoROM/HiROM	│
	│  Bank mapping   │
	└─────────────────┘
```

## Directory Structure

```text
Core/SNES/
├── Main Components
│   ├── SnesConsole.h/cpp	   - Console coordinator
│   ├── SnesCpu.h/cpp		   - 65816 CPU emulation
│   ├── SnesPpu.h/cpp		   - PPU (Picture Processing Unit)
│   ├── SnesMemoryManager.h/cpp - Memory mapping & bus
│   └── SnesDmaController.h/cpp - DMA/HDMA controller
│
├── Audio
│   ├── Spc.h/cpp			   - SPC700 sound CPU
│   ├── SpcTimer.h			  - SPC700 timers
│   └── DSP/					- S-DSP audio processor
│
├── Coprocessors/
│   ├── BaseCoprocessor.h	   - Base class
│   ├── SA1/					- SA-1 (65816 coprocessor)
│   ├── GSU/					- Super FX (GSU)
│   ├── DSP/					- DSP-1/2/3/4 (math)
│   ├── CX4/					- Cx4 (wireframe 3D)
│   ├── SDD1/				   - S-DD1 (decompression)
│   ├── SPC7110/				- SPC7110 (decompression)
│   ├── OBC1/				   - OBC-1 (sprite management)
│   ├── ST018/				  - ST018 (AI processor)
│   ├── BSX/					- BS-X Satellaview
│   ├── SGB/					- Super Game Boy
│   ├── MSU1/				   - MSU-1 (CD-quality audio)
│   └── SufamiTurbo/			- Sufami Turbo adapter
│
├── Input/
│   └── Input handlers		  - Controllers, multitap, etc.
│
├── Video Filters
│   ├── SnesDefaultVideoFilter  - Basic RGB output
│   └── SnesNtscFilter		  - NTSC composite simulation
│
├── Debugger/
│   └── SNES-specific debug	 - Breakpoints, trace, viewers
│
└── Support Files
	├── SnesCpuTypes.h		  - CPU state structures
	├── SnesPpuTypes.h		  - PPU state structures
	├── SpcTypes.h			  - SPC700 state structures
	├── CartTypes.h			 - Cartridge types
	└── SnesState.h			 - Combined system state
```

## Core Components

### SnesConsole (SnesConsole.h)

The main coordinator that owns all SNES components.

**Responsibilities:**

- Initialize and connect components
- Run the main frame loop with cycle-accurate timing
- Handle state serialization
- Manage coprocessor selection based on ROM header

### SnesCpu (SnesCpu.h)

Emulates the Ricoh 5A22, a 65816 CPU with DMA controller.

**Key Features:**

- Full 65816 instruction set (all 256 opcodes)
- 24-bit addressing (16MB address space)
- 8/16-bit accumulator and index registers (M/X flags)
- Emulation mode (6502 compatibility)
- Cycle-accurate execution with FastROM support

**CPU State:**

```cpp
struct SnesCpuState {
	uint16_t A;	 // Accumulator (16-bit)
	uint16_t X;	 // X Index (16-bit)
	uint16_t Y;	 // Y Index (16-bit)
	uint16_t SP;	// Stack Pointer (16-bit)
	uint16_t D;	 // Direct Page register
	uint16_t PC;	// Program Counter
	uint8_t K;	  // Program Bank (PBR)
	uint8_t DBR;	// Data Bank Register
	uint8_t PS;	 // Processor Status
	bool EmulationMode;  // 6502 emulation mode
};
```

**Addressing Modes (24 total):**

- Direct Page (dp), Direct Page Indexed (dp,X / dp,Y)
- Absolute, Absolute Indexed (a,X / a,Y)
- Long (al), Long Indexed (al,X)
- Stack Relative, Indirect modes
- Block Move (MVN/MVP)

### SnesPpu (SnesPpu.h)

Emulates the S-PPU (Picture Processing Unit) with two chips (PPU1/PPU2).

**Key Features:**

- Multiple resolutions: 256×224, 256×239, 512×224, 512×448
- 8 graphics modes (Mode 0-7)
- 4 background layers (BG1-BG4)
- 128 sprites (OAM), up to 32 per scanline
- Color math (add/subtract blending)
- HDMA effects (per-scanline register changes)
- Mode 7: Affine transformation (rotation/scaling)

**Graphics Modes:**

| Mode | BG1 | BG2 | BG3 | BG4 | Notes |
| ------ | ----- | ----- | ----- | ----- | ------- |
| 0 | 2bpp | 2bpp | 2bpp | 2bpp | 4 layers, 4 colors each |
| 1 | 4bpp | 4bpp | 2bpp | - | Most common mode |
| 2 | 4bpp | 4bpp | OPT | - | Offset-per-tile |
| 3 | 8bpp | 4bpp | - | - | 256 colors BG1 |
| 4 | 8bpp | 2bpp | OPT | - | 256 colors + OPT |
| 5 | 4bpp | 2bpp | - | - | Hi-res 512 width |
| 6 | 4bpp | - | OPT | - | Hi-res + OPT |
| 7 | 8bpp | EXTBG | - | - | Rotation/scaling |

**PPU Memory:**

- 64KB VRAM (Video RAM)
- 544 bytes OAM (sprite attributes)
- 512 bytes CGRAM (color palette)

### Spc (Spc.h) - SPC700 Sound CPU

Emulates the Sony SPC700 8-bit audio processor.

**Key Features:**

- 8-bit CPU at ~1.024 MHz
- 64KB dedicated audio RAM
- Independent from main CPU
- Communicates via 4 I/O ports

**SPC700 State:**

```cpp
struct SpcState {
	uint16_t PC;	// Program Counter
	uint8_t A;	  // Accumulator
	uint8_t X;	  // X Index
	uint8_t Y;	  // Y Index
	uint8_t SP;	 // Stack Pointer
	uint8_t PS;	 // Processor Status
};
```

### DSP (DSP/) - S-DSP Audio Processor

The S-DSP generates the final audio output using BRR-encoded samples.

**Key Features:**

- 8 voices (channels)
- 32kHz sample rate output
- BRR compression (16:9 ratio)
- ADSR envelopes
- Echo/reverb with 8-tap FIR filter
- Noise generation
- Pitch modulation

**Voice Registers (per channel):**

- VOL (L/R): Volume
- PITCH: Sample rate
- SRCN: Sample source number
- ADSR: Attack/Decay/Sustain/Release
- GAIN: Alternative envelope control
- ENVX/OUTX: Envelope/output readback

### DMA Controller (SnesDmaController.h)

Handles both regular DMA and HDMA (Horizontal DMA).

**DMA Features:**

- 8 DMA channels
- Block transfer modes
- Fixed/increment/decrement addressing
- CPU halt during DMA

**HDMA Features:**

- Per-scanline register writes
- Used for gradient effects, window changes
- Table-driven with repeat counts

## Memory Map

```text
SNES Memory Map (LoROM example)
═══════════════════════════════
Bank $00-$3F (System Area):
  $0000-$1FFF  Work RAM (WRAM) mirror
  $2000-$20FF  Unused
  $2100-$213F  PPU registers
  $2140-$2143  APU I/O ports
  $2180-$2183  WRAM access registers
  $4000-$41FF  Joypad registers
  $4200-$43FF  DMA, PPU2, misc registers
  $8000-$FFFF  ROM (LoROM: lower 32KB of bank)

Bank $40-$6F:
  Full 64KB ROM per bank (LoROM)

Bank $7E-$7F:
  $7E:0000-$FFFF  128KB Work RAM
  $7F:0000-$FFFF  Additional 128KB WRAM

Bank $80-$FF:
  Mirror of $00-$7F (FastROM at higher banks)
```

## Coprocessors

### SA-1 (SA1/)
A second 65816 CPU running at 10.74 MHz.

- Used in: Kirby Super Star, Super Mario RPG
- Features: Memory mapping, BCD math, bit manipulation

### Super FX / GSU (GSU/)
RISC processor for 3D graphics.

- Used in: Star Fox, Yoshi's Island, Doom
- Features: Plot/draw commands, cache, variable clock

### DSP-1/2/3/4 (DSP/)
Math coprocessors for 3D calculations.

- Used in: Pilotwings, Super Mario Kart
- Features: Trigonometry, matrix math, projection

### Cx4 (CX4/)
Wireframe 3D graphics processor.

- Used in: Mega Man X2, Mega Man X3
- Features: Line drawing, polygon rendering

### S-DD1 (SDD1/)
Data decompression coprocessor.

- Used in: Star Ocean, Street Fighter Alpha 2
- Features: Real-time decompression

### SPC7110 (SPC7110/)
Enhanced decompression and memory mapping.

- Used in: Far East of Eden Zero, Momotaro Dentetsu Happy

### Super Game Boy (SGB/)
Runs original Game Boy games with SNES enhancements.

- Features: Custom borders, color palettes, multiplayer

### MSU-1 (MSU1/)
Modern enhancement chip for CD-quality audio/video.

- Features: 4GB data access, streaming audio
- Used in: ROM hacks and homebrew

## Timing

**Main CPU (5A22):**

- Master clock: 21.477 MHz
- FastROM: 3.58 MHz (6 cycles/access)
- SlowROM: 2.68 MHz (8 cycles/access)
- Variable speed based on memory region

**SPC700:**

- ~1.024 MHz (1024000 Hz)
- Independent timing from main CPU

**PPU:**

- 262 scanlines (NTSC) / 312 (PAL)
- 340 dots per scanline
- ~60.098 Hz (NTSC) / ~50.007 Hz (PAL)

## Integration Points

### With Shared Infrastructure

- Uses `Serializer` for save states
- Uses `SimpleLock` for thread safety
- Debugger interface for step/trace

### State Serialization

```cpp
void SnesCpu::Serialize(Serializer& s) {
	SV(_state.A);
	SV(_state.X);
	SV(_state.Y);
	SV(_state.SP);
	SV(_state.D);
	SV(_state.PC);
	SV(_state.K);
	SV(_state.DBR);
	SV(_state.PS);
	SV(_state.EmulationMode);
	// ...
}
```

## Debugging Features

- CPU disassembly (65816 with bank awareness)
- SPC700 disassembly
- PPU viewer (tilemaps, OAM, CGRAM)
- VRAM viewer
- DMA/HDMA trace
- Breakpoints on all buses
- State history (rewind)

## Related Documentation

- [NES-CORE.md](NES-CORE.md) - NES emulation (simpler 6502)
- [ARCHITECTURE-OVERVIEW.md](ARCHITECTURE-OVERVIEW.md) - Overall structure
- [UTILITIES-LIBRARY.md](UTILITIES-LIBRARY.md) - Shared utilities
