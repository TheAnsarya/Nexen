#pragma once
#include "pch.h"
#include "Shared/BaseState.h"


/// <summary>
/// PC Engine interrupt sources (IRQ lines).
/// </summary>
enum class PceIrqSource {
	Irq2 = 1,      ///< IRQ2 (external)
	Irq1 = 2,      ///< IRQ1 (external)
	TimerIrq = 4   ///< Timer IRQ
};


/// <summary>
/// PC Engine CPU status flags (HuC6280, 6502-like).
/// </summary>
namespace PceCpuFlags {
enum PceCpuFlags : uint8_t {
	Carry = 0x01,      ///< Carry flag (C)
	Zero = 0x02,       ///< Zero flag (Z)
	Interrupt = 0x04,  ///< IRQ disable (I)
	Decimal = 0x08,    ///< Decimal mode (D)
	Break = 0x10,      ///< Break (B)
	Memory = 0x20,     ///< Memory/Accumulator width (M, unused)
	Overflow = 0x40,   ///< Overflow flag (V)
	Negative = 0x80    ///< Negative flag (N)
};
} // namespace PceCpuFlags


/// <summary>
/// Complete PC Engine CPU state (HuC6280, 6502-like).
/// </summary>
struct PceCpuState : public BaseState {
	uint64_t CycleCount = 0; ///< Total CPU cycles executed
	uint16_t PC = 0;         ///< Program counter
	uint8_t SP = 0;          ///< Stack pointer
	uint8_t A = 0;           ///< Accumulator
	uint8_t X = 0;           ///< X index register
	uint8_t Y = 0;           ///< Y index register
	uint8_t PS = 0;          ///< Processor status (flags)
};


/// <summary>
/// PC Engine CPU addressing modes (HuC6280).
/// </summary>
enum class PceAddrMode {
	None,       ///< No addressing mode
	Acc,        ///< Accumulator
	Imp,        ///< Implied
	Imm,        ///< Immediate
	Rel,        ///< Relative
	Zero,       ///< Zero page
	Abs,        ///< Absolute
	ZeroX,      ///< Zero page,X
	ZeroY,      ///< Zero page,Y
	Ind,        ///< Indirect
	IndX,       ///< (Zero,X)
	IndY,       ///< (Zero),Y
	AbsX,       ///< Absolute,X
	AbsY,       ///< Absolute,Y
	ZInd,       ///< Zero page indirect
	ZeroRel,    ///< Zero page relative
	Block,      ///< Block transfer
	ImZero,     ///< Immediate/Zero page
	ImZeroX,    ///< Immediate/Zero page,X
	ImAbs,      ///< Immediate/Absolute
	ImAbsX,     ///< Immediate/Absolute,X
	AbsXInd     ///< Absolute,X indirect
};

/// <summary>
/// PC Engine VDC register latches.
/// Some registers are latched at specific times during rendering.
/// </summary>
struct PceVdcHvLatches {
	// R07 - BXR
	uint16_t BgScrollX;           ///< Background horizontal scroll

	// R08 - BYR
	uint16_t BgScrollY;           ///< Background vertical scroll

	// R09 - MWR - Memory Width
	uint8_t ColumnCount;          ///< BAT width (32/64/128 tiles)
	uint8_t RowCount;             ///< BAT height (32/64)
	uint8_t SpriteAccessMode;     ///< Sprite pattern access mode
	uint8_t VramAccessMode;       ///< VRAM access mode
	bool CgMode;                  ///< Character generator mode

	// R0A - HSR
	uint8_t HorizDisplayStart;    ///< Horizontal display start position
	uint8_t HorizSyncWidth;       ///< Horizontal sync width

	// R0B - HDR
	uint8_t HorizDisplayWidth;    ///< Horizontal display width
	uint8_t HorizDisplayEnd;      ///< Horizontal display end position

	// R0C - VPR
	uint8_t VertDisplayStart;     ///< Vertical display start position
	uint8_t VertSyncWidth;        ///< Vertical sync width

	// R0D - VDW
	uint16_t VertDisplayWidth;    ///< Vertical display width

	// R0E - VCR
	uint8_t VertEndPosVcr;        ///< Vertical display end position
};

/// <summary>
/// PC Engine Video Display Controller (VDC) state.
/// HuC6270 chip responsible for background and sprite rendering.
/// </summary>
struct PceVdcState {
	uint32_t FrameCount;          ///< Total frames rendered

	uint16_t HClock;              ///< Horizontal clock counter
	uint16_t Scanline;            ///< Current scanline
	uint16_t RcrCounter;          ///< Raster compare counter

	uint8_t CurrentReg;           ///< Currently selected register

	// R00 - MAWR
	uint16_t MemAddrWrite;        ///< VRAM write address

	// R01 - MARR
	uint16_t MemAddrRead;         ///< VRAM read address
	uint16_t ReadBuffer;          ///< Read-ahead buffer

	// R02 - VWR
	uint16_t VramData;            ///< VRAM write data register

	// R05 - CR - Control
	bool EnableCollisionIrq;      ///< Sprite 0 collision IRQ enable
	bool EnableOverflowIrq;       ///< Sprite overflow IRQ enable
	bool EnableScanlineIrq;       ///< Raster compare IRQ enable
	bool EnableVerticalBlankIrq;  ///< VBlank IRQ enable
	bool OutputVerticalSync;      ///< Output vertical sync signal
	bool OutputHorizontalSync;    ///< Output horizontal sync signal
	bool SpritesEnabled;          ///< Sprite layer enabled
	bool BackgroundEnabled;       ///< Background layer enabled
	uint8_t VramAddrIncrement;    ///< Address increment (1/32/64/128)

	// R06 - RCR
	uint16_t RasterCompareRegister;  ///< Scanline for raster IRQ

	bool BgScrollYUpdatePending;  ///< Y scroll update pending for next line

	PceVdcHvLatches HvLatch;      ///< Latched H/V timing values
	PceVdcHvLatches HvReg;        ///< Register H/V timing values

	// R0F - DCR - DMA Control
	bool VramSatbIrqEnabled;      ///< SATB DMA complete IRQ enable
	bool VramVramIrqEnabled;      ///< VRAM-VRAM DMA complete IRQ enable
	bool DecrementSrc;            ///< Decrement source address
	bool DecrementDst;            ///< Decrement destination address
	bool RepeatSatbTransfer;      ///< Auto-repeat SATB transfer each frame

	// R10 - SOUR
	uint16_t BlockSrc;            ///< VRAM-VRAM DMA source address

	// R11 - DESR
	uint16_t BlockDst;            ///< VRAM-VRAM DMA destination address

	// R12 - LENR
	uint16_t BlockLen;            ///< VRAM-VRAM DMA length

	// R13 - DVSSR
	uint16_t SatbBlockSrc;        ///< SATB DMA source address
	bool SatbTransferPending;     ///< SATB transfer requested
	bool SatbTransferRunning;     ///< SATB transfer in progress

	uint16_t SatbTransferNextWordCounter;  ///< Words remaining in transfer
	uint8_t SatbTransferOffset;            ///< Current offset in SATB

	// Status flags
	bool VerticalBlank;           ///< Currently in VBlank
	bool VramTransferDone;        ///< VRAM-VRAM DMA complete
	bool SatbTransferDone;        ///< SATB DMA complete
	bool ScanlineDetected;        ///< Raster compare match occurred
	bool SpriteOverflow;          ///< More than 16 sprites on scanline
	bool Sprite0Hit;              ///< Sprite 0 collision detected

	bool BurstModeEnabled;        ///< Burst mode for faster VRAM access
	bool NextSpritesEnabled;      ///< Sprites enabled next frame
	bool NextBackgroundEnabled;   ///< Background enabled next frame
};

/// <summary>
/// PC Engine Video Color Encoder (VCE) state.
/// HuC6260 chip responsible for color palette and timing.
/// </summary>
struct PceVceState {
	uint16_t ScanlineCount;       ///< Total scanlines per frame
	uint16_t PalAddr;             ///< Current palette address
	uint8_t ClockDivider;         ///< Master clock divider (5/7/10 MHz modes)
	bool Grayscale;               ///< Grayscale output mode
};

/// <summary>
/// PC Engine memory manager state.
/// Handles MMU (MPR registers), IRQ, and CPU speed control.
/// </summary>
struct PceMemoryManagerState {
	uint64_t CycleCount;          ///< Total memory access cycles
	uint8_t Mpr[8];               ///< Memory Paging Registers (8x8KB banks)
	uint8_t ActiveIrqs;           ///< Currently active IRQ flags
	uint8_t DisabledIrqs;         ///< IRQ disable mask
	bool FastCpuSpeed;            ///< High-speed mode (7.16 MHz vs 1.79 MHz)
	uint8_t MprReadBuffer;        ///< MPR read buffer
	uint8_t IoBuffer;             ///< I/O port buffer
};

/// <summary>
/// PC Engine controller state.
/// </summary>
struct PceControlManagerState {
};

/// <summary>
/// PC Engine timer state.
/// 7-bit countdown timer with configurable rate.
/// </summary>
struct PceTimerState {
	uint8_t ReloadValue;          ///< Timer reload value (7-bit)
	uint8_t Counter;              ///< Current counter value
	uint16_t Scaler;              ///< Prescaler counter
	bool Enabled;                 ///< Timer running
};

/// <summary>
/// PC Engine PSG global state.
/// Controls channel selection and master volume.
/// </summary>
struct PcePsgState {
	uint8_t ChannelSelect;        ///< Currently selected channel (0-5)
	uint8_t LeftVolume;           ///< Master left volume
	uint8_t RightVolume;          ///< Master right volume
	uint8_t LfoFrequency;         ///< LFO frequency for channel 1
	uint8_t LfoControl;           ///< LFO control register
};

/// <summary>
/// PC Engine PSG channel state.
/// 6 channels total: 4 waveform, 2 waveform/noise.
/// </summary>
struct PcePsgChannelState {
	uint16_t Frequency;           ///< Frequency register (12-bit)
	uint8_t Amplitude;            ///< Channel volume (5-bit)
	bool Enabled;                 ///< Channel output enabled
	uint8_t LeftVolume;           ///< Left channel volume
	uint8_t RightVolume;          ///< Right channel volume
	uint8_t WaveData[0x20];       ///< 32-byte waveform table

	bool DdaEnabled;              ///< Direct DAC mode enabled
	uint8_t DdaOutputValue;       ///< Direct DAC output sample

	uint8_t WaveAddr;             ///< Current waveform position
	uint32_t Timer;               ///< Period countdown timer
	int8_t CurrentOutput;         ///< Current output sample

	// Channel 5 & 6 only (noise capable)
	uint32_t NoiseLfsr;           ///< Noise LFSR state
	uint32_t NoiseTimer;          ///< Noise frequency counter
	bool NoiseEnabled;            ///< Noise mode enabled
	int8_t NoiseOutput;           ///< Noise output sample
	uint8_t NoiseFrequency;       ///< Noise frequency divider
};

/// <summary>
/// VPC (Video Priority Controller) priority mode.
/// Determines layer ordering between dual VDCs (SuperGrafx).
/// </summary>
enum class PceVpcPriorityMode {
	Default = 0,                  ///< Normal priority (VDC1 over VDC2)
	Vdc2SpritesAboveVdc1Bg = 1,   ///< VDC2 sprites above VDC1 background
	Vdc1SpritesBelowVdc2Bg = 2,   ///< VDC1 sprites below VDC2 background
};

/// <summary>
/// VPC pixel window selection.
/// </summary>
enum class PceVpcPixelWindow {
	NoWindow,   ///< Outside all windows
	Window1,    ///< Inside window 1 only
	Window2,    ///< Inside window 2 only
	Both        ///< Inside both windows
};

/// <summary>
/// VPC window priority configuration.
/// </summary>
struct PceVpcPriorityConfig {
	PceVpcPriorityMode PriorityMode;  ///< Layer priority mode
	bool Vdc1Enabled;                  ///< VDC1 output enabled in this window
	bool Vdc2Enabled;                  ///< VDC2 output enabled in this window
};

/// <summary>
/// VPC (Video Priority Controller) state.
/// SuperGrafx-only chip for compositing two VDCs.
/// </summary>
struct PceVpcState {
	PceVpcPriorityConfig WindowCfg[(int)PceVpcPixelWindow::Both + 1]; ///< Priority per window region
	uint8_t Priority1;            ///< Priority register 1
	uint8_t Priority2;            ///< Priority register 2
	uint16_t Window1;             ///< Window 1 horizontal position
	uint16_t Window2;             ///< Window 2 horizontal position
	bool StToVdc2Mode;            ///< ST (sprite?) to VDC2 mode
	bool HasIrqVdc1;              ///< VDC1 has pending IRQ
	bool HasIrqVdc2;              ///< VDC2 has pending IRQ
};

/// <summary>
/// Complete video state combining VDC, VCE, and VPC.
/// </summary>
struct PceVideoState : BaseState {
	PceVdcState Vdc;              ///< Primary VDC state
	PceVceState Vce;              ///< Video Color Encoder state
	PceVpcState Vpc;              ///< Video Priority Controller (SuperGrafx)
	PceVdcState Vdc2;             ///< Secondary VDC (SuperGrafx only)
};

/// <summary>
/// Arcade Card address offset trigger condition.
/// </summary>
enum class PceArcadePortOffsetTrigger {
	None = 0,               ///< No automatic offset
	AddOnLowWrite = 1,      ///< Add offset on low byte write
	AddOnHighWrite = 2,     ///< Add offset on high byte write
	AddOnReg0AWrite = 3,    ///< Add offset on register 0A write
};

/// <summary>
/// Arcade Card memory port configuration.
/// Provides 2MB RAM with auto-increment addressing.
/// </summary>
struct PceArcadeCardPortConfig {
	uint32_t BaseAddress;         ///< 24-bit base address in RAM
	uint16_t Offset;              ///< 16-bit address offset
	uint16_t IncValue;            ///< Auto-increment value

	uint8_t Control;              ///< Control register
	bool AutoIncrement;           ///< Auto-increment after access
	bool AddOffset;               ///< Add offset to base address
	bool SignedIncrement;         ///< Signed increment (unused?)
	bool SignedOffset;            ///< Signed offset
	bool AddIncrementToBase;      ///< Add increment to base (vs offset)
	PceArcadePortOffsetTrigger AddOffsetTrigger;  ///< When to apply offset
};

/// <summary>
/// Arcade Card state with 4 memory ports and ALU.
/// Expansion card for CD-ROM² providing 2MB RAM.
/// </summary>
struct PceArcadeCardState {
	PceArcadeCardPortConfig Port[4];  ///< Four memory access ports
	uint32_t ValueReg;                 ///< 32-bit value register for ALU
	uint8_t ShiftReg;                  ///< Shift amount register
	uint8_t RotateReg;                 ///< Rotate amount register
};

/// <summary>
/// CD-ROM interrupt sources.
/// </summary>
enum class PceCdRomIrqSource {
	Adpcm = 0x04,       ///< ADPCM playback event
	Stop = 0x08,        ///< CD audio stopped
	SubCode = 0x10,     ///< Subcode data ready
	StatusMsgIn = 0x20, ///< SCSI status/message ready
	DataIn = 0x40       ///< SCSI data transfer ready
};

/// <summary>
/// CD-ROM interface state (CD-ROM²/Super CD-ROM²).
/// </summary>
struct PceCdRomState {
	uint16_t AudioSampleLatch = 0;  ///< Latched CD audio sample
	uint8_t ActiveIrqs = 0;         ///< Currently active IRQ flags
	uint8_t EnabledIrqs = 0;        ///< Enabled IRQ mask
	bool ReadRightChannel = false;  ///< Read right (vs left) audio channel
	bool BramLocked = false;        ///< Backup RAM write-protected
	uint8_t ResetRegValue = 0;      ///< Reset register value
};

/// <summary>
/// ADPCM (Adaptive Differential PCM) state.
/// Provides hardware ADPCM decoding for CD-ROM².
/// </summary>
struct PceAdpcmState {
	bool Nibble;                  ///< High/low nibble select
	uint16_t ReadAddress;         ///< RAM read address
	uint16_t WriteAddress;        ///< RAM write address

	uint16_t AddressPort;         ///< Address port register

	uint8_t DmaControl;           ///< DMA control register
	uint8_t Control;              ///< ADPCM control register
	uint8_t PlaybackRate;         ///< Sample rate divider

	uint32_t AdpcmLength;         ///< Remaining samples to play
	bool EndReached;              ///< End of sample reached
	bool HalfReached;             ///< Half of buffer played

	bool Playing;                 ///< Playback in progress
	bool PlayRequest;             ///< Playback start requested

	uint8_t ReadBuffer;           ///< Read data buffer
	uint8_t ReadClockCounter;     ///< Read timing counter

	uint8_t WriteBuffer;          ///< Write data buffer
	uint8_t WriteClockCounter;    ///< Write timing counter
};

/// <summary>
/// CD audio playback end behavior.
/// </summary>
enum class CdPlayEndBehavior {
	Stop,   ///< Stop playback
	Loop,   ///< Loop back to start
	Irq     ///< Stop and trigger IRQ
};

/// <summary>
/// CD audio player status.
/// </summary>
enum class CdAudioStatus : uint8_t {
	Playing = 0,   ///< Audio playing
	Inactive = 1,  ///< No track loaded
	Paused = 2,    ///< Playback paused
	Stopped = 3    ///< Playback stopped
};

/// <summary>
/// CD audio player state.
/// </summary>
struct PceCdAudioPlayerState {
	CdAudioStatus Status;         ///< Current playback status

	uint32_t StartSector;         ///< Start sector of track
	uint32_t EndSector;           ///< End sector of track
	CdPlayEndBehavior EndBehavior; ///< Behavior at end of track

	uint32_t CurrentSector;       ///< Current reading sector
	uint32_t CurrentSample;       ///< Current sample within sector

	int16_t LeftSample;           ///< Current left audio sample
	int16_t RightSample;          ///< Current right audio sample
};

/// <summary>
/// SCSI bus phase for CD-ROM communication.
/// </summary>
enum class ScsiPhase {
	BusFree,     ///< Bus idle
	Command,     ///< Receiving command
	DataIn,      ///< Sending data to host
	DataOut,     ///< Receiving data from host (unused)
	MessageIn,   ///< Sending message to host
	MessageOut,  ///< Receiving message from host (unused)
	Status,      ///< Sending status byte
	Busy,        ///< Processing command
};

/// <summary>
/// SCSI bus state for CD-ROM drive communication.
/// </summary>
struct PceScsiBusState {
	bool Signals[9];              ///< SCSI bus signal lines
	ScsiPhase Phase;              ///< Current bus phase

	bool MessageDone;             ///< Message transfer complete
	uint8_t DataPort;             ///< Output data port
	uint8_t ReadDataPort;         ///< Input data port

	uint32_t Sector;              ///< Current sector number
	uint8_t SectorsToRead;        ///< Sectors remaining to read
};

/// <summary>
/// Audio fader target channel.
/// </summary>
enum class PceAudioFaderTarget {
	Adpcm,     ///< ADPCM audio
	CdAudio,   ///< CD audio
};

/// <summary>
/// Audio fader state for volume control.
/// </summary>
struct PceAudioFaderState {
	uint64_t StartClock;          ///< Fade start time
	PceAudioFaderTarget Target;   ///< Channel being faded
	bool FastFade;                ///< Fast fade rate
	bool Enabled;                 ///< Fader active
	uint8_t RegValue;             ///< Fader register value
};

/// <summary>
/// Complete PC Engine emulation state.
/// </summary>
struct PceState {
	PceCpuState Cpu;                   ///< HuC6280 CPU state
	PceVideoState Video;               ///< Video (VDC/VCE/VPC) state
	PceMemoryManagerState MemoryManager; ///< Memory manager state
	PceTimerState Timer;               ///< Timer state
	PcePsgState Psg;                   ///< PSG global state
	PcePsgChannelState PsgChannels[6]; ///< PSG channel states

	// CD-ROM² components
	PceCdRomState CdRom;               ///< CD-ROM interface state
	PceCdAudioPlayerState CdPlayer;    ///< CD audio player state
	PceAdpcmState Adpcm;               ///< ADPCM decoder state
	PceAudioFaderState AudioFader;     ///< Audio fader state
	PceScsiBusState ScsiDrive;         ///< SCSI drive state
	PceArcadeCardState ArcadeCard;     ///< Arcade Card state

	bool IsSuperGrafx;                 ///< SuperGrafx mode (dual VDC)
	bool HasArcadeCard;                ///< Arcade Card present
	bool HasCdRom;                     ///< CD-ROM² present
};
