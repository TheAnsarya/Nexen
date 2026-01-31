#pragma once
#include "pch.h"
#include "Shared/MemoryType.h"
#include "Shared/BaseState.h"


/// <summary>
/// Complete Game Boy CPU state (Sharp LR35902, Z80-like).
/// </summary>
struct GbCpuState : BaseState {
	uint64_t CycleCount;   ///< Total CPU cycles executed
	uint16_t PC;           ///< Program counter
	uint16_t SP;           ///< Stack pointer
	uint16_t HaltCounter;  ///< Cycles remaining in HALT

	uint8_t A;             ///< Accumulator
	uint8_t Flags;         ///< Flags register (Z, N, H, C)

	uint8_t B;
	uint8_t C;
	uint8_t D;
	uint8_t E;

	uint8_t H;
	uint8_t L;

	bool EiPending;        ///< EI instruction pending
	bool IME;              ///< Interrupt master enable
	bool HaltBug;          ///< HALT bug active
	bool Stopped;          ///< STOP instruction active
};


/// <summary>
/// Game Boy CPU flag bits (F register).
/// </summary>
namespace GbCpuFlags {
enum GbCpuFlags {
	Zero = 0x80,      ///< Zero flag (Z)
	AddSub = 0x40,    ///< Add/Subtract flag (N)
	HalfCarry = 0x20, ///< Half-carry flag (H)
	Carry = 0x10      ///< Carry flag (C)
};
} // namespace GbCpuFlags


/// <summary>
/// Game Boy interrupt sources (IF/IE bits).
/// </summary>
namespace GbIrqSource {
enum GbIrqSource {
	VerticalBlank = 0x01, ///< V-Blank interrupt
	LcdStat = 0x02,       ///< LCD STAT interrupt
	Timer = 0x04,         ///< Timer overflow
	Serial = 0x08,        ///< Serial transfer
	Joypad = 0x10         ///< Joypad input
};
} // namespace GbIrqSource


/// <summary>
/// Helper for 16-bit register access from two 8-bit halves.
/// </summary>
class Register16 {
	uint8_t* _low;
	uint8_t* _high;

public:
	Register16(uint8_t* high, uint8_t* low) {
		_high = high;
		_low = low;
	}

	uint16_t Read() {
		return (*_high << 8) | *_low;
	}

	void Write(uint16_t value) {
		*_high = (uint8_t)(value >> 8);
		*_low = (uint8_t)value;
	}

	void Inc() {
		Write(Read() + 1);
	}

	void Dec() {
		Write(Read() - 1);
	}

	operator uint16_t() { return Read(); }
};


/// <summary>
/// Game Boy PPU mode (STAT register bits 0-1).
/// </summary>
enum class PpuMode {
	HBlank,         ///< Mode 0: HBlank
	VBlank,         ///< Mode 1: VBlank
	OamEvaluation,  ///< Mode 2: OAM scan
	Drawing,        ///< Mode 3: Pixel transfer
	NoIrq,          ///< Not generating IRQ
};


/// <summary>
/// Types of OAM corruption (hardware bugs).
/// </summary>
enum class GbOamCorruptionType {
	Read,        ///< Corruption on OAM read
	Write,       ///< Corruption on OAM write
	ReadIncDec   ///< Corruption on OAM inc/dec
};


/// <summary>
/// LCD STAT interrupt sources (STAT register bits 3-6).
/// </summary>
namespace GbPpuStatusFlags {
enum GbPpuStatusFlags {
	CoincidenceIrq = 0x40, ///< LYC=LY coincidence
	OamIrq = 0x20,         ///< OAM interrupt
	VBlankIrq = 0x10,      ///< VBlank interrupt
	HBlankIrq = 0x08       ///< HBlank interrupt
};
} // namespace GbPpuStatusFlags


/// <summary>
/// Debug event color codes for PPU visualization.
/// </summary>
enum class EvtColor {
	HBlank = 0,
	VBlank = 1,
	OamEvaluation = 2,
	RenderingIdle = 3,
	RenderingBgLoad = 4,
	RenderingOamLoad = 5,
};


/// <summary>
/// Pixel type for PPU output (BG or OBJ).
/// </summary>
enum class GbPixelType : uint8_t {
	Background, ///< Background pixel
	Object      ///< Sprite/object pixel
};


/// <summary>
/// FIFO entry for pixel pipeline (color, attributes, index).
/// </summary>
struct GbFifoEntry {
	uint8_t Color;      ///< Palette color index
	uint8_t Attributes; ///< Attribute bits (priority, palette)
	uint8_t Index;      ///< Pixel index in tile
};

/// <summary>
/// Pixel FIFO for PPU pixel pipeline.
/// </summary>
/// <remarks>
/// The Game Boy PPU uses a FIFO to buffer pixels before output.
/// The FIFO holds up to 8 pixels and is filled by the pixel fetcher.
/// </remarks>
struct GbPpuFifo {
	/// <summary>Current read position in FIFO (0-7).</summary>
	uint8_t Position = 0;

	/// <summary>Number of pixels in FIFO (0-8).</summary>
	uint8_t Size = 0;

	/// <summary>FIFO content buffer (8 pixels).</summary>
	GbFifoEntry Content[8] = {};

	/// <summary>Clears the FIFO.</summary>
	void Reset() {
		Size = 0;
		Position = 0;
		memset(Content, 0, sizeof(Content));
	}

	/// <summary>Removes one pixel from the FIFO.</summary>
	void Pop() {
		Content[Position].Color = 0;
		Position = (Position + 1) & 0x07;
		Size--;
	}
};

/// <summary>
/// Pixel fetcher state for PPU background/sprite loading.
/// </summary>
struct GbPpuFetcher {
	/// <summary>VRAM address being fetched.</summary>
	uint16_t Addr = 0;

	/// <summary>Tile attributes (CGB: palette, flip, priority).</summary>
	uint8_t Attributes = 0;

	/// <summary>Current fetch step (0-7).</summary>
	uint8_t Step = 0;

	/// <summary>Low bitplane byte.</summary>
	uint8_t LowByte = 0;

	/// <summary>High bitplane byte.</summary>
	uint8_t HighByte = 0;
};

/// <summary>
/// Complete Game Boy PPU state.
/// </summary>
/// <remarks>
/// The Game Boy PPU renders 160x144 pixels with:
/// - Background layer (32x32 tilemap)
/// - Window overlay (optional second BG)
/// - 40 sprites (OAM, max 10 per line)
/// - 4-shade monochrome (DMG) or 32768 colors (CGB)
/// </remarks>
struct GbPpuState : public BaseState {
	/// <summary>Current scanline being processed (0-153).</summary>
	uint8_t Scanline;

	/// <summary>Current cycle within scanline.</summary>
	uint16_t Cycle;

	/// <summary>Idle cycles remaining.</summary>
	uint16_t IdleCycles;

	/// <summary>Current PPU mode (HBlank/VBlank/OAM/Drawing).</summary>
	PpuMode Mode;

	/// <summary>Mode for IRQ generation.</summary>
	PpuMode IrqMode;

	/// <summary>STAT interrupt line state.</summary>
	bool StatIrqFlag;

	/// <summary>Current LY value (internal).</summary>
	uint8_t Ly;

	/// <summary>LY value for LYC comparison.</summary>
	int16_t LyForCompare;

	/// <summary>LY compare value (LYC register).</summary>
	uint8_t LyCompare;

	/// <summary>LY == LYC coincidence flag.</summary>
	bool LyCoincidenceFlag;

	/// <summary>Background palette (BGP, DMG only).</summary>
	uint8_t BgPalette;

	/// <summary>Object palette 0 (OBP0, DMG only).</summary>
	uint8_t ObjPalette0;

	/// <summary>Object palette 1 (OBP1, DMG only).</summary>
	uint8_t ObjPalette1;

	/// <summary>Background X scroll (SCX).</summary>
	uint8_t ScrollX;

	/// <summary>Background Y scroll (SCY).</summary>
	uint8_t ScrollY;

	/// <summary>Window X position (WX).</summary>
	uint8_t WindowX;

	/// <summary>Window Y position (WY).</summary>
	uint8_t WindowY;

	/// <summary>LCD control register (LCDC).</summary>
	uint8_t Control;

	/// <summary>LCD enabled (LCDC bit 7).</summary>
	bool LcdEnabled;

	/// <summary>Window tilemap select (LCDC bit 6).</summary>
	bool WindowTilemapSelect;

	/// <summary>Window enabled (LCDC bit 5).</summary>
	bool WindowEnabled;

	/// <summary>BG/Window tile data select (LCDC bit 4).</summary>
	bool BgTileSelect;

	/// <summary>BG tilemap select (LCDC bit 3).</summary>
	bool BgTilemapSelect;

	/// <summary>8x16 sprite mode (LCDC bit 2).</summary>
	bool LargeSprites;

	/// <summary>Sprites enabled (LCDC bit 1).</summary>
	bool SpritesEnabled;

	/// <summary>BG/Window enabled (LCDC bit 0).</summary>
	bool BgEnabled;

	/// <summary>LCD status register (STAT).</summary>
	uint8_t Status;

	/// <summary>Total frames rendered.</summary>
	uint32_t FrameCount;

	/// <summary>CGB mode active.</summary>
	bool CgbEnabled;

	/// <summary>Current CGB VRAM bank (0-1).</summary>
	uint8_t CgbVramBank;

	/// <summary>CGB BG palette write position.</summary>
	uint8_t CgbBgPalPosition;

	/// <summary>CGB BG palette auto-increment.</summary>
	bool CgbBgPalAutoInc;

	/// <summary>CGB BG palettes (8 palettes × 4 colors).</summary>
	uint16_t CgbBgPalettes[4 * 8];

	/// <summary>CGB OBJ palette write position.</summary>
	uint8_t CgbObjPalPosition;

	/// <summary>CGB OBJ palette auto-increment.</summary>
	bool CgbObjPalAutoInc;

	/// <summary>CGB OBJ palettes (8 palettes × 4 colors).</summary>
	uint16_t CgbObjPalettes[4 * 8];
};

/// <summary>
/// Game Boy DMA controller state.
/// </summary>
/// <remarks>
/// Handles OAM DMA (DMG/CGB) and HDMA/GDMA (CGB only).
/// OAM DMA transfers 160 bytes to OAM.
/// HDMA transfers 16 bytes per HBlank.
/// </remarks>
struct GbDmaControllerState {
	/// <summary>OAM DMA source address high byte.</summary>
	uint8_t OamDmaSource;

	/// <summary>DMA start delay cycles.</summary>
	uint8_t DmaStartDelay;

	/// <summary>Internal destination counter.</summary>
	uint8_t InternalDest;

	/// <summary>DMA byte counter.</summary>
	uint8_t DmaCounter;

	/// <summary>DMA read buffer.</summary>
	uint8_t DmaReadBuffer;

	/// <summary>OAM DMA is in progress.</summary>
	bool OamDmaRunning;

	/// <summary>CGB HDMA source address.</summary>
	uint16_t CgbDmaSource;

	/// <summary>CGB HDMA destination address.</summary>
	uint16_t CgbDmaDest;

	/// <summary>CGB HDMA remaining length (blocks of 16).</summary>
	uint8_t CgbDmaLength;

	/// <summary>CGB HDMA is in progress.</summary>
	bool CgbHdmaRunning;

	/// <summary>CGB HDMA pending for next HBlank.</summary>
	bool CgbHdmaPending;

	/// <summary>CGB HDMA trigger flag.</summary>
	bool CgbHdmaTrigger;
};

/// <summary>
/// Game Boy timer state (DIV, TIMA, TMA, TAC).
/// </summary>
struct GbTimerState {
	/// <summary>16-bit divider (DIV is high byte at $FF04).</summary>
	uint16_t Divider;

	/// <summary>TIMA overflow pending, will reload from TMA next cycle.</summary>
	bool NeedReload;

	/// <summary>TIMA was just reloaded (affects TMA/TIMA writes).</summary>
	bool Reloaded;

	/// <summary>Timer counter (TIMA at $FF05).</summary>
	uint8_t Counter;

	/// <summary>Timer modulo/reload value (TMA at $FF06).</summary>
	uint8_t Modulo;

	/// <summary>Timer control register (TAC at $FF07).</summary>
	uint8_t Control;

	/// <summary>Timer enabled (TAC bit 2).</summary>
	bool TimerEnabled;

	/// <summary>Timer divider for frequency selection.</summary>
	uint16_t TimerDivider;
};

/// <summary>
/// Game Boy APU Square wave channel state.
/// </summary>
/// <remarks>
/// Two square channels with duty cycle, envelope, and sweep (channel 1 only).
/// </remarks>
struct GbSquareState {
	/// <summary>Frequency (11-bit).</summary>
	uint16_t Frequency;

	/// <summary>Current timer value.</summary>
	uint16_t Timer;

	/// <summary>Sweep timer.</summary>
	uint16_t SweepTimer;

	/// <summary>Sweep shadow frequency.</summary>
	uint16_t SweepFreq;

	/// <summary>Sweep period.</summary>
	uint16_t SweepPeriod;

	/// <summary>Sweep update delay.</summary>
	uint8_t SweepUpdateDelay;

	/// <summary>Sweep direction (true = subtract).</summary>
	bool SweepNegate;

	/// <summary>Sweep shift amount.</summary>
	uint8_t SweepShift;

	/// <summary>Sweep unit enabled.</summary>
	bool SweepEnabled;

	/// <summary>Sweep negate calculation done.</summary>
	bool SweepNegateCalcDone;

	/// <summary>Current envelope volume (0-15).</summary>
	uint8_t Volume;

	/// <summary>Initial envelope volume.</summary>
	uint8_t EnvVolume;

	/// <summary>Envelope direction (true = increase).</summary>
	bool EnvRaiseVolume;

	/// <summary>Envelope period.</summary>
	uint8_t EnvPeriod;

	/// <summary>Envelope timer.</summary>
	uint8_t EnvTimer;

	/// <summary>Envelope stopped (reached 0 or 15).</summary>
	bool EnvStopped;

	/// <summary>Duty cycle (0-3: 12.5%, 25%, 50%, 75%).</summary>
	uint8_t Duty;

	/// <summary>Length counter (0-64).</summary>
	uint8_t Length;

	/// <summary>Length counter enabled.</summary>
	bool LengthEnabled;

	/// <summary>Channel is producing output.</summary>
	bool Enabled;

	/// <summary>First step after enable.</summary>
	bool FirstStep;

	/// <summary>Current position in duty cycle.</summary>
	uint8_t DutyPos;

	/// <summary>Current output value.</summary>
	uint8_t Output;
};

/// <summary>
/// Game Boy APU Noise channel state.
/// </summary>
struct GbNoiseState {
	/// <summary>Current envelope volume (0-15).</summary>
	uint8_t Volume;

	/// <summary>Initial envelope volume.</summary>
	uint8_t EnvVolume;

	/// <summary>Envelope direction (true = increase).</summary>
	bool EnvRaiseVolume;

	/// <summary>Envelope period.</summary>
	uint8_t EnvPeriod;

	/// <summary>Envelope timer.</summary>
	uint8_t EnvTimer;

	/// <summary>Envelope stopped.</summary>
	bool EnvStopped;

	/// <summary>Length counter (0-64).</summary>
	uint8_t Length;

	/// <summary>Length counter enabled.</summary>
	bool LengthEnabled;

	/// <summary>15-bit LFSR for noise generation.</summary>
	uint16_t ShiftRegister;

	/// <summary>Period shift (clock divider exponent).</summary>
	uint8_t PeriodShift;

	/// <summary>Base divisor code (0-7).</summary>
	uint8_t Divisor;

	/// <summary>7-bit LFSR mode (true) vs 15-bit (false).</summary>
	bool ShortWidthMode;

	/// <summary>Channel is producing output.</summary>
	bool Enabled;

	/// <summary>Current timer value.</summary>
	uint32_t Timer;

	/// <summary>Current output value.</summary>
	uint8_t Output;
};

/// <summary>
/// Game Boy APU Wave channel state.
/// </summary>
/// <remarks>
/// The wave channel plays 4-bit samples from 16-byte wave RAM.
/// </remarks>
struct GbWaveState {
	/// <summary>DAC enabled (NR30 bit 7).</summary>
	bool DacEnabled;

	/// <summary>Current sample buffer value.</summary>
	uint8_t SampleBuffer;

	/// <summary>Wave pattern RAM (16 bytes, 32 samples).</summary>
	uint8_t Ram[0x10];

	/// <summary>Current position in wave RAM (0-31).</summary>
	uint8_t Position;

	/// <summary>Volume shift (0-3: 0%, 100%, 50%, 25%).</summary>
	uint8_t Volume;

	/// <summary>Frequency (11-bit).</summary>
	uint16_t Frequency;

	/// <summary>Length counter (0-256).</summary>
	uint16_t Length;

	/// <summary>Length counter enabled.</summary>
	bool LengthEnabled;

	/// <summary>Channel is producing output.</summary>
	bool Enabled;

	/// <summary>Current timer value.</summary>
	uint16_t Timer;

	/// <summary>Current output value.</summary>
	uint8_t Output;
};

/// <summary>
/// Game Boy APU global state.
/// </summary>
struct GbApuState {
	/// <summary>APU master enable (NR52 bit 7).</summary>
	bool ApuEnabled;

	/// <summary>Square 1 enabled on left output.</summary>
	uint8_t EnableLeftSq1;

	/// <summary>Square 2 enabled on left output.</summary>
	uint8_t EnableLeftSq2;

	/// <summary>Wave enabled on left output.</summary>
	uint8_t EnableLeftWave;

	/// <summary>Noise enabled on left output.</summary>
	uint8_t EnableLeftNoise;

	/// <summary>Square 1 enabled on right output.</summary>
	uint8_t EnableRightSq1;

	/// <summary>Square 2 enabled on right output.</summary>
	uint8_t EnableRightSq2;

	/// <summary>Wave enabled on right output.</summary>
	uint8_t EnableRightWave;

	/// <summary>Noise enabled on right output.</summary>
	uint8_t EnableRightNoise;

	/// <summary>Left master volume (0-7).</summary>
	uint8_t LeftVolume;

	/// <summary>Right master volume (0-7).</summary>
	uint8_t RightVolume;

	/// <summary>External audio left enabled (VIN).</summary>
	bool ExtAudioLeftEnabled;

	/// <summary>External audio right enabled (VIN).</summary>
	bool ExtAudioRightEnabled;

	/// <summary>Frame sequencer step (0-7).</summary>
	uint8_t FrameSequenceStep;
};

/// <summary>
/// Complete APU debug state with all channels.
/// </summary>
struct GbApuDebugState {
	/// <summary>Global APU state.</summary>
	GbApuState Common;

	/// <summary>Square channel 1 state.</summary>
	GbSquareState Square1;

	/// <summary>Square channel 2 state.</summary>
	GbSquareState Square2;

	/// <summary>Wave channel state.</summary>
	GbWaveState Wave;

	/// <summary>Noise channel state.</summary>
	GbNoiseState Noise;
};

/// <summary>
/// Memory register access type.
/// </summary>
enum class RegisterAccess {
	/// <summary>No access.</summary>
	None = 0,

	/// <summary>Read only.</summary>
	Read = 1,

	/// <summary>Write only.</summary>
	Write = 2,

	/// <summary>Read and write.</summary>
	ReadWrite = 3
};

/// <summary>
/// Game Boy memory types for address mapping.
/// </summary>
enum class GbMemoryType {
	/// <summary>No memory mapped.</summary>
	None = 0,

	/// <summary>Program ROM (cartridge).</summary>
	PrgRom = (int)MemoryType::GbPrgRom,

	/// <summary>Work RAM (internal).</summary>
	WorkRam = (int)MemoryType::GbWorkRam,

	/// <summary>Cartridge RAM (save RAM).</summary>
	CartRam = (int)MemoryType::GbCartRam,

	/// <summary>Boot ROM.</summary>
	BootRom = (int)MemoryType::GbBootRom,
};

/// <summary>
/// Game Boy memory manager state.
/// </summary>
struct GbMemoryManagerState {
	/// <summary>APU cycle count for audio timing.</summary>
	uint64_t ApuCycleCount;

	/// <summary>CGB Work RAM bank (1-7).</summary>
	uint8_t CgbWorkRamBank;

	/// <summary>CGB speed switch requested.</summary>
	bool CgbSwitchSpeedRequest;

	/// <summary>CGB running in double speed mode.</summary>
	bool CgbHighSpeed;

	/// <summary>CGB infrared register (RP).</summary>
	uint8_t CgbRegRpInfrared;

	/// <summary>CGB undocumented register $FF72.</summary>
	uint8_t CgbRegFF72;

	/// <summary>CGB undocumented register $FF73.</summary>
	uint8_t CgbRegFF73;

	/// <summary>CGB undocumented register $FF74.</summary>
	uint8_t CgbRegFF74;

	/// <summary>CGB undocumented register $FF75.</summary>
	uint8_t CgbRegFF75;

	/// <summary>Boot ROM disabled (set after boot).</summary>
	bool DisableBootRom;

	/// <summary>Interrupt request flags (IF).</summary>
	uint8_t IrqRequests;

	/// <summary>Interrupt enable flags (IE).</summary>
	uint8_t IrqEnabled;

	/// <summary>Serial transfer data (SB).</summary>
	uint8_t SerialData;

	/// <summary>Serial transfer control (SC).</summary>
	uint8_t SerialControl;

	/// <summary>Serial bit counter.</summary>
	uint8_t SerialBitCount;

	/// <summary>Read register flags for each I/O address.</summary>
	bool IsReadRegister[0x100];

	/// <summary>Write register flags for each I/O address.</summary>
	bool IsWriteRegister[0x100];

	/// <summary>Memory type for each page.</summary>
	GbMemoryType MemoryType[0x100];

	/// <summary>Memory offset for each page.</summary>
	uint32_t MemoryOffset[0x100];

	/// <summary>Memory access type for each page.</summary>
	RegisterAccess MemoryAccessType[0x100];
};

/// <summary>
/// Game Boy controller/input state.
/// </summary>
struct GbControlManagerState {
	/// <summary>Input select (P1 register bits 4-5).</summary>
	uint8_t InputSelect;
};

/// <summary>
/// Game Boy hardware type.
/// </summary>
enum class GbType {
	/// <summary>Original Game Boy (DMG).</summary>
	Gb = 0,

	/// <summary>Game Boy Color (CGB).</summary>
	Cgb = 1,
};

/// <summary>
/// Complete Game Boy system state for save states.
/// </summary>
struct GbState {
	/// <summary>Hardware type (DMG or CGB).</summary>
	GbType Type;

	/// <summary>CPU state.</summary>
	GbCpuState Cpu;

	/// <summary>PPU state.</summary>
	GbPpuState Ppu;

	/// <summary>APU state with all channels.</summary>
	GbApuDebugState Apu;

	/// <summary>Memory manager state.</summary>
	GbMemoryManagerState MemoryManager;

	/// <summary>Timer state.</summary>
	GbTimerState Timer;

	/// <summary>DMA controller state.</summary>
	GbDmaControllerState Dma;

	/// <summary>Controller manager state.</summary>
	GbControlManagerState ControlManager;

	/// <summary>Cartridge has battery-backed save RAM.</summary>
	bool HasBattery;
};
