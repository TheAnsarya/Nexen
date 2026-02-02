# SMS, PC Engine, and WonderSwan Core Documentation

This document covers the Sega Master System, PC Engine/TurboGrafx-16, and WonderSwan emulation cores.

---

## Sega Master System / Game Gear

### Architecture Overview

```text
SMS Architecture
════════════════

┌──────────────────────────────────────────────────────┐
│					 SmsConsole					   │
│   (SMS, Game Gear, SG-1000 mode selection)		   │
└──────────────────────────────────────────────────────┘
			  │
	┌─────────┼─────────┬──────────────┐
	│		 │		 │			  │
	▼		 ▼		 ▼			  ▼
┌───────┐ ┌───────┐ ┌─────────┐ ┌───────────┐
│SmsCpu │ │SmsVdp │ │  SmsPsg │ │  Memory   │
│	   │ │	   │ │  + FM   │ │  Manager  │
│ Z80   │ │TMS9918│ │  Audio  │ │  + Cart   │
│ 3.58M │ │variant│ │		 │ │		   │
└───────┘ └───────┘ └─────────┘ └───────────┘
```

### Directory Structure

```text
Core/SMS/
├── Main Components
│   ├── SmsConsole.h/cpp		- Console coordinator
│   ├── SmsCpu.h/cpp			- Z80 CPU
│   ├── SmsVdp.h/cpp			- Video Display Processor
│   ├── SmsMemoryManager.h/cpp  - Memory mapping
│   └── SmsBiosMapper.h/cpp	 - BIOS ROM handling
│
├── Audio
│   ├── SmsPsg.h/cpp			- SN76489 PSG (4 channels)
│   └── SmsFmAudio.h/cpp		- YM2413 FM (optional)
│
├── Cartridges
│   └── Carts/				  - Mapper implementations
│
├── Input/
│   └── Controllers, Light Phaser
│
└── Support Files
	└── SmsTypes.h			  - State structures
```

### Core Components

#### SmsCpu (SmsCpu.h)

Emulates the Zilog Z80 CPU.

**Key Features:**

- 8-bit CPU at 3.579545 MHz (NTSC)
- 16-bit address bus (64KB)
- Extensive instruction set (700+ opcodes)
- IX/IY index registers
- Alternate register set (shadow registers)

**Z80 Registers:**

```cpp
struct SmsCpuState {
	// Main registers
	uint16_t AF, BC, DE, HL;
	// Alternate registers
	uint16_t AF2, BC2, DE2, HL2;
	// Index registers
	uint16_t IX, IY;
	// Special registers
	uint16_t SP;	// Stack Pointer
	uint16_t PC;	// Program Counter
	uint8_t I;	  // Interrupt Vector
	uint8_t R;	  // Memory Refresh
	bool IFF1, IFF2; // Interrupt flip-flops
};
```

#### SmsVdp (SmsVdp.h)

Video Display Processor based on TMS9918 with enhancements.

**Display Specifications:**

- SMS: 256×192 or 256×224 pixels
- Game Gear: 160×144 (centered window)
- 32 colors on screen from 64 (SMS) or 4096 (GG)
- 64 sprites, 8 per scanline

**VDP Modes:**

- Mode 4: Standard SMS mode (tile-based)
- Legacy modes: TMS9918 compatibility (SG-1000)

**Memory:**

- 16KB VRAM
- 32-byte palette RAM (CRAM)

#### SmsPsg (SmsPsg.h)

SN76489 Programmable Sound Generator.

**Channels:**

1. **Tone 1** - Square wave
2. **Tone 2** - Square wave
3. **Tone 3** - Square wave (or noise clock)
4. **Noise** - White or periodic noise

#### SmsFmAudio (SmsFmAudio.h)

Optional YM2413 FM audio (Japanese SMS, some games).

**Features:**

- 9 FM channels (or 6 FM + 5 rhythm)
- 15 preset instruments + 1 custom
- Used in: Phantasy Star, Ys, Space Harrier

### Memory Map

```text
SMS Memory Map
══════════════
$0000-$03FF  ROM (BIOS) or Cartridge
$0400-$3FFF  Cartridge ROM (page 0)
$4000-$7FFF  Cartridge ROM (page 1)
$8000-$BFFF  Cartridge ROM (page 2) / RAM
$C000-$DFFF  System RAM (8KB)
$E000-$FFFF  RAM mirror
```

---

## PC Engine / TurboGrafx-16

### Architecture Overview

```text
PC Engine Architecture
══════════════════════

┌──────────────────────────────────────────────────────┐
│					 PceConsole					   │
│   (HuCard, CD-ROM², Super CD-ROM² support)		   │
└──────────────────────────────────────────────────────┘
			  │
	┌─────────┼─────────┬──────────────┬──────────────┐
	│		 │		 │			  │			  │
	▼		 ▼		 ▼			  ▼			  ▼
┌───────┐ ┌───────┐ ┌─────────┐ ┌───────────┐ ┌───────┐
│PceCpu │ │PceVdc │ │  PcePsg │ │  Memory   │ │CD-ROM │
│	   │ │ +Vce  │ │		 │ │  Manager  │ │	   │
│HuC6280│ │ +Vpc  │ │  6 chan │ │  + Cart   │ │ADPCM  │
│ 7.16M │ │ Video │ │		 │ │		   │ │	   │
└───────┘ └───────┘ └─────────┘ └───────────┘ └───────┘
```

### Directory Structure

```text
Core/PCE/
├── Main Components
│   ├── PceConsole.h/cpp		- Console coordinator
│   ├── PceCpu.h/cpp			- HuC6280 CPU
│   ├── PceVdc.h/cpp			- Video Display Controller
│   ├── PceVce.h/cpp			- Video Color Encoder
│   ├── PceVpc.h/cpp			- Video Priority Controller
│   ├── PceMemoryManager.h/cpp  - Memory mapping
│   └── PceTimer.h/cpp		  - Timer subsystem
│
├── Audio
│   ├── PcePsg.h/cpp			- 6-channel PSG
│   └── PcePsgChannel.h/cpp	 - Individual channel
│
├── CD-ROM
│   └── CdRom/				  - CD-ROM² support
│	   ├── ADPCM audio
│	   ├── CD-DA playback
│	   └── System Card BIOS
│
├── Input/
│   └── Controllers, multitap
│
└── Support Files
	├── PceTypes.h			  - State structures
	└── PceConstants.h		  - Hardware constants
```

### Core Components

#### PceCpu (PceCpu.h)

Emulates the Hudson HuC6280, an enhanced 65C02.

**Key Features:**

- 8-bit CPU at 7.16 MHz (or 1.79 MHz)
- 65C02 base with extensions
- 8KB zero-page (MPR-based banking)
- Block transfer instructions
- Timer with IRQ
- Built-in PSG controller

**CPU Enhancements over 65C02:**

- Memory Mapping Registers (MPR0-7)
- Block transfer: TAI, TIA, TIN, TDD, TII
- Swap instructions: SXY, SAX, SAY
- Test/set/reset memory bits

**Memory Mapping:**

```cpp
// 8 Memory Mapping Registers divide 64KB into 8KB pages
// Each MPR selects one of 256 physical 8KB pages
struct PceMprState {
	uint8_t MPR[8];  // Each maps to physical page 0-255
};
```

#### PceVdc (PceVdc.h)

Video Display Controller (HuC6270).

**Display Specifications:**

- Resolution: 256-512 × 224-242 pixels
- 512 colors on screen from 512 total
- 2 scrolling background layers (with VPC)
- 64 sprites, 16 per scanline

**Video Memory:**

- 64KB VRAM (HuCard) or 128KB (SuperGrafx)
- Tile and sprite data

#### PceVce (PceVce.h)

Video Color Encoder (HuC6260).

- Manages color palette
- 512 colors (9-bit RGB)
- Outputs composite/S-Video/RGB

#### PceVpc (PceVpc.h)

Video Priority Controller (SuperGrafx only).

- Combines output of two VDCs
- Priority and window control

#### PcePsg (PcePsg.h)

6-channel wavetable PSG.

**Channels:**

- 6 identical channels
- 32-sample 5-bit waveforms
- Volume and noise control
- Channels 5-6: LFO modulation option

### Memory Map

```text
PC Engine Memory Map (via MPR)
══════════════════════════════
Physical pages:
$00-$7F  HuCard ROM (up to 1MB)
$80-$F7  Unused (some mappers use)
$F8	  RAM (8KB)
$F9-$FB  Unused
$FC	  Unused
$FD	  Unused
$FE	  Unused
$FF	  Hardware I/O page

I/O Page ($FF):
$0000-$03FF  VDC
$0400-$07FF  VCE
$0800-$0BFF  PSG
$0C00-$0FFF  Timer
$1000-$13FF  I/O port
$1400-$17FF  IRQ control
```

---

## WonderSwan / WonderSwan Color

### Architecture Overview

```text
WonderSwan Architecture
═══════════════════════

┌──────────────────────────────────────────────────────┐
│					 WsConsole						│
│   (WonderSwan, WonderSwan Color, SwanCrystal)		│
└──────────────────────────────────────────────────────┘
			  │
	┌─────────┼─────────┬──────────────┐
	│		 │		 │			  │
	▼		 ▼		 ▼			  ▼
┌───────┐ ┌───────┐ ┌─────────┐ ┌───────────┐
│ WsCpu │ │ WsPpu │ │  WsApu  │ │  Memory   │
│	   │ │	   │ │		 │ │  Manager  │
│ V30MZ │ │ Video │ │  4 chan │ │  + Cart   │
│ 3.07M │ │224×144│ │ + Voice │ │		   │
└───────┘ └───────┘ └─────────┘ └───────────┘
```

### Directory Structure

```text
Core/WS/
├── Main Components
│   ├── WsConsole.h/cpp		 - Console coordinator
│   ├── WsCpu.h/cpp			 - NEC V30MZ CPU
│   ├── WsCpuPrefetch.h/cpp	 - Instruction prefetch
│   ├── WsPpu.h/cpp			 - PPU (224×144)
│   ├── WsMemoryManager.h/cpp   - Memory mapping
│   ├── WsDmaController.h/cpp   - DMA controller
│   └── WsTimer.h/cpp		   - Timers
│
├── Audio
│   └── APU/					- 4-channel + voice
│
├── Cartridges
│   └── Carts/				  - ROM handling
│
├── Peripherals
│   ├── WsEeprom.h/cpp		  - Save data
│   └── WsSerial.h/cpp		  - Link cable
│
├── Input/
│   └── WsController.h
│
└── Support Files
	└── WsTypes.h			   - State structures
```

### Core Components

#### WsCpu (WsCpu.h)

Emulates the NEC V30MZ, an 80186-compatible CPU.

**Key Features:**

- 16-bit CPU at 3.072 MHz
- x86-compatible (80186 subset)
- 1MB address space (20-bit)
- Segmented memory model

**CPU Registers:**

```cpp
struct WsCpuState {
	// General registers
	uint16_t AX, BX, CX, DX;
	// Index registers
	uint16_t SI, DI, BP, SP;
	// Segment registers
	uint16_t CS, DS, ES, SS;
	// Program counter
	uint16_t IP;
	// Flags
	uint16_t Flags;
};
```

**Addressing:**

- Physical = (Segment << 4) + Offset
- Up to 1MB addressable (20-bit)

#### WsPpu (WsPpu.h)

Custom PPU for handheld display.

**Display Specifications:**

- Resolution: 224×144 pixels
- WS: 8 shades of gray
- WSC: 241 colors on screen from 4096
- 2 background layers + sprites
- 128 sprites, 32 per scanline

**Unique Features:**

- Vertical or horizontal orientation
- Hardware sprite windows
- Color mapped mode (WSC)

**Memory:**

- 16KB VRAM (WS) / 64KB (WSC)
- 512 sprites in OAM

#### WsApu (APU/)

4-channel audio + voice synthesis.

**Channels:**

1. **Wave 1** - Wavetable
2. **Wave 2** - Wavetable (or voice input)
3. **Wave 3** - Wavetable (or sweep)
4. **Wave 4** - Wavetable (or noise)

**Features:**

- Headphone stereo output
- Voice input/output (microphone)
- Per-channel volume

### Memory Map

```text
WonderSwan Memory Map
═════════════════════
$00000-$0FFFF  Internal RAM (64KB, WSC)
$10000-$1FFFF  SRAM (if present)
$20000-$FFFFF  Cartridge ROM (segments)

I/O Ports ($00-$FF):
$00-$3F  Display control
$40-$5F  Sound control
$60-$7F  System control
$80-$9F  Timers, interrupts
$A0-$BF  Serial, cartridge
$C0-$FF  Reserved
```

---

## Comparison Table

| System | CPU | Clock | Resolution | Colors | Audio |
| -------- | ----- | ------- | ------------ | -------- | ------- |
| **SMS** | Z80 | 3.58 MHz | 256×192 | 32/64 | 4ch PSG |
| **Game Gear** | Z80 | 3.58 MHz | 160×144 | 32/4096 | 4ch PSG |
| **PC Engine** | HuC6280 | 7.16 MHz | 256-512×224 | 512 | 6ch PSG |
| **WonderSwan** | V30MZ | 3.07 MHz | 224×144 | 8 gray | 4ch + voice |
| **WSC** | V30MZ | 3.07 MHz | 224×144 | 241/4096 | 4ch + voice |

## Related Documentation

- [NES-CORE.md](NES-CORE.md) - NES emulation
- [SNES-CORE.md](SNES-CORE.md) - SNES emulation
- [GB-GBA-CORE.md](GB-GBA-CORE.md) - Game Boy family
- [ARCHITECTURE-OVERVIEW.md](ARCHITECTURE-OVERVIEW.md) - Overall structure
