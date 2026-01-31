#pragma once
#include "pch.h"
#include "Shared/MemoryType.h"
#include "Shared/BaseState.h"
#include "Shared/ArmEnums.h"
#include "Utilities/Serializer.h"

/// <summary>
/// ARM7TDMI CPU operating modes.
/// Each mode has its own banked registers and privilege level.
/// </summary>
enum class GbaCpuMode : uint8_t {
	User = 0b10000,       ///< Normal execution, no privileged access
	Fiq = 0b10001,        ///< Fast interrupt (banked R8-R14)
	Irq = 0b10010,        ///< Standard interrupt (banked R13-R14)
	Supervisor = 0b10011, ///< SWI handler mode
	Abort = 0b10111,      ///< Memory fault handler
	Undefined = 0b11011,  ///< Undefined instruction handler
	System = 0b11111,     ///< Privileged mode using User registers
};

/// <summary>
/// ARM exception vector addresses in BIOS.
/// CPU jumps to these addresses when exceptions occur.
/// </summary>
enum class GbaCpuVector : uint32_t {
	Undefined = 0x04,      ///< Undefined instruction
	SoftwareIrq = 0x08,    ///< SWI instruction
	AbortPrefetch = 0x0C,  ///< Prefetch abort (bad instruction fetch)
	AbortData = 0x10,      ///< Data abort (bad data access)
	Irq = 0x18,            ///< Hardware interrupt
	Fiq = 0x1C,            ///< Fast interrupt
};

typedef uint8_t GbaAccessModeVal;

/// <summary>
/// Memory access mode flags for bus timing and behavior.
/// </summary>
namespace GbaAccessMode {
enum Mode {
	Sequential = (1 << 0),  ///< Sequential access (faster)
	Word = (1 << 1),        ///< 32-bit access
	HalfWord = (1 << 2),    ///< 16-bit access
	Byte = (1 << 3),        ///< 8-bit access
	Signed = (1 << 4),      ///< Sign-extend result
	NoRotate = (1 << 5),    ///< Don't rotate misaligned reads
	Prefetch = (1 << 6),    ///< Instruction prefetch
	Dma = (1 << 7)          ///< DMA transfer access
};
} // namespace GbaAccessMode

/// <summary>
/// ARM CPU status flags (CPSR bits).
/// </summary>
struct GbaCpuFlags {
	GbaCpuMode Mode;    ///< Current CPU mode
	bool Thumb;         ///< Thumb state (16-bit instructions)
	bool FiqDisable;    ///< FIQ masked
	bool IrqDisable;    ///< IRQ masked

	bool Overflow;      ///< Arithmetic overflow (V)
	bool Carry;         ///< Carry/borrow (C)
	bool Zero;          ///< Zero result (Z)
	bool Negative;      ///< Negative/sign (N)

	/// <summary>Packs flags into 32-bit CPSR format.</summary>
	uint32_t ToInt32() {
		return (
		    (Negative << 31) |
		    (Zero << 30) |
		    (Carry << 29) |
		    (Overflow << 28) |

		    (IrqDisable << 7) |
		    (FiqDisable << 6) |
		    (Thumb << 5) |
		    (uint8_t)Mode);
	}
};

/// <summary>
/// Single instruction in the CPU pipeline.
/// </summary>
struct GbaInstructionData {
	uint32_t Address;   ///< Instruction address
	uint32_t OpCode;    ///< Instruction opcode
};

/// <summary>
/// ARM7 3-stage pipeline state (Fetch, Decode, Execute).
/// </summary>
struct GbaCpuPipeline {
	GbaInstructionData Fetch;     ///< Instruction being fetched
	GbaInstructionData Decode;    ///< Instruction being decoded
	GbaInstructionData Execute;   ///< Instruction being executed
	bool ReloadRequested;         ///< Pipeline flush pending
	GbaAccessModeVal Mode;        ///< Current access mode
};

/// <summary>
/// Complete ARM7TDMI CPU state including banked registers.
/// </summary>
struct GbaCpuState : BaseState {
	GbaCpuPipeline Pipeline;
	uint32_t R[16];       ///< General purpose registers (R0-R15, R15=PC)
	GbaCpuFlags CPSR;     ///< Current Program Status Register
	bool Stopped;         ///< CPU halted (STOP instruction)
	bool Frozen;          ///< CPU frozen (debugging)

	// Banked registers for each mode
	uint32_t UserRegs[7];       ///< User/System R8-R14
	uint32_t FiqRegs[7];        ///< FIQ R8-R14 (fully banked)
	uint32_t IrqRegs[2];        ///< IRQ R13-R14

	uint32_t SupervisorRegs[2]; ///< Supervisor R13-R14
	uint32_t AbortRegs[2];      ///< Abort R13-R14
	uint32_t UndefinedRegs[2];  ///< Undefined R13-R14

	// Saved PSR for each exception mode
	GbaCpuFlags FiqSpsr;        ///< FIQ Saved Program Status Register
	GbaCpuFlags IrqSpsr;        ///< IRQ SPSR
	GbaCpuFlags SupervisorSpsr; ///< Supervisor SPSR
	GbaCpuFlags AbortSpsr;      ///< Abort SPSR
	GbaCpuFlags UndefinedSpsr;  ///< Undefined SPSR

	uint64_t CycleCount;  ///< Total CPU cycles executed
};

/// <summary>Stereo 3D mode for backgrounds (used by some homebrew).</summary>
enum class GbaBgStereoMode : uint8_t {
	Disabled,     ///< Normal display
	EvenColumns,  ///< Display on even columns only
	OddColumns,   ///< Display on odd columns only
	Both          ///< Display on all columns
};

/// <summary>
/// Background layer configuration state.
/// </summary>
struct GbaBgConfig {
	uint16_t Control;      ///< BGCNT register value
	uint16_t TilemapAddr;  ///< Screen base block address
	uint16_t TilesetAddr;  ///< Character base block address
	uint16_t ScrollX;      ///< Horizontal scroll offset
	uint16_t ScrollY;      ///< Vertical scroll offset
	uint8_t ScreenSize;    ///< Screen size (0-3)
	bool DoubleWidth;      ///< 512 pixel width
	bool DoubleHeight;     ///< 512 pixel height
	uint8_t Priority;      ///< Display priority (0=highest)
	bool Mosaic;           ///< Mosaic effect enabled
	bool WrapAround;       ///< Wrap at edges (affine only)
	bool Bpp8Mode;         ///< 8bpp tiles (256 colors)
	bool Enabled;          ///< Layer enabled
	uint8_t EnableTimer;   ///< Frames until enable takes effect
	uint8_t DisableTimer;  ///< Frames until disable takes effect
	GbaBgStereoMode StereoMode;  ///< Stereo 3D mode
};

/// <summary>
/// Affine transformation parameters for rotation/scaling BGs.
/// </summary>
struct GbaTransformConfig {
	uint32_t OriginX;        ///< Reference point X (fixed-point)
	uint32_t OriginY;        ///< Reference point Y

	int32_t LatchOriginX;    ///< Latched origin X (per frame)
	int32_t LatchOriginY;    ///< Latched origin Y

	int16_t Matrix[4];       ///< 2x2 transform matrix (PA, PB, PC, PD)

	bool PendingUpdateX;     ///< X origin write pending
	bool PendingUpdateY;     ///< Y origin write pending
	bool NeedInit;           ///< Needs initialization
};

/// <summary>
/// Window boundary configuration.
/// </summary>
struct GbaWindowConfig {
	uint8_t LeftX;     ///< Left edge (inclusive)
	uint8_t RightX;    ///< Right edge (exclusive)
	uint8_t TopY;      ///< Top edge (inclusive)
	uint8_t BottomY;   ///< Bottom edge (exclusive)
};

/// <summary>
/// PPU color blending effect type.
/// </summary>
enum class GbaPpuBlendEffect : uint8_t {
	None,                ///< No blending
	AlphaBlend,          ///< Semi-transparency (EVA*A + EVB*B)
	IncreaseBrightness,  ///< Fade to white
	DecreaseBrightness   ///< Fade to black
};

/// <summary>
/// Sprite (OBJ) rendering mode.
/// </summary>
enum class GbaPpuObjMode : uint8_t {
	Normal,        ///< Standard sprite
	Blending,      ///< Semi-transparent (always first target)
	Window,        ///< Defines OBJ window region
	Stereoscopic   ///< Prohibited in GBA (treated as double-size)
};

/// <summary>
/// PPU memory access type flags for bus conflict detection.
/// </summary>
namespace GbaPpuMemAccess {
enum GbaPpuMemAccess : uint8_t {
	None = 0,
	Palette = 1,   ///< Accessing palette RAM
	Vram = 2,      ///< Accessing VRAM (BG)
	Oam = 4,       ///< Accessing OAM
	VramObj = 8    ///< Accessing VRAM (OBJ)
};
} // namespace GbaPpuMemAccess

/// <summary>
/// Complete PPU register state.
/// </summary>
struct GbaPpuState : BaseState {
	uint32_t FrameCount;   ///< Frames rendered
	uint16_t Cycle;        ///< Current cycle within scanline (0-1231)
	uint16_t Scanline;     ///< Current scanline (0-227)

	// DISPCNT register ($4000000)
	uint8_t Control;
	uint8_t BgMode;                    ///< Background mode (0-5)
	bool DisplayFrameSelect;           ///< Frame buffer select (modes 4/5)
	bool AllowHblankOamAccess;         ///< OAM accessible during HBlank
	bool ObjVramMappingOneDimension;   ///< 1D sprite tile mapping
	bool ForcedBlank;                  ///< Display disabled (white)
	uint8_t ForcedBlankDisableTimer;   ///< Frames until blank ends
	bool StereoscopicEnabled;          ///< Stereo 3D mode enabled

	uint8_t Control2;
	uint8_t ObjEnableTimer;            ///< Frames until OBJ enable
	bool ObjLayerEnabled;              ///< Sprite layer enabled
	bool Window0Enabled;               ///< Window 0 active
	bool Window1Enabled;               ///< Window 1 active
	bool ObjWindowEnabled;             ///< Sprite window active

	// DISPSTAT register ($4000004)
	uint8_t DispStat;
	bool VblankIrqEnabled;             ///< VBlank triggers IRQ
	bool HblankIrqEnabled;             ///< HBlank triggers IRQ
	bool ScanlineIrqEnabled;           ///< V-count match triggers IRQ
	uint8_t Lyc;                       ///< Scanline compare value

	// Blending registers
	uint8_t BlendMainControl;
	bool BlendMain[6];                 ///< First target layers
	uint8_t BlendSubControl;
	bool BlendSub[6];                  ///< Second target layers
	GbaPpuBlendEffect BlendEffect;     ///< Current blend mode
	uint8_t BlendMainCoefficient;      ///< EVA (0-16)
	uint8_t BlendSubCoefficient;       ///< EVB (0-16)
	uint8_t Brightness;                ///< EVY for brightness (0-16)

	GbaBgConfig BgLayers[4];           ///< BG0-BG3 configuration
	GbaTransformConfig Transform[2];   ///< BG2/BG3 affine transforms
	GbaWindowConfig Window[2];         ///< Window 0/1 bounds

	// Mosaic settings
	uint8_t BgMosaicSizeX;             ///< BG mosaic horizontal size
	uint8_t BgMosaicSizeY;             ///< BG mosaic vertical size
	uint8_t ObjMosaicSizeX;            ///< OBJ mosaic horizontal size
	uint8_t ObjMosaicSizeY;            ///< OBJ mosaic vertical size

	// Window layer visibility
	uint8_t Window0Control;
	uint8_t Window1Control;
	uint8_t ObjWindowControl;
	uint8_t OutWindowControl;
	bool WindowActiveLayers[5][6];     ///< [window][layer] visibility
};

/// <summary>
/// Memory manager state including interrupts and wait states.
/// </summary>
struct GbaMemoryManagerState {
	uint16_t IE;         ///< Interrupt Enable ($4000200)
	uint16_t IF;         ///< Interrupt Flags ($4000202)
	uint16_t NewIE;      ///< Pending IE write
	uint16_t NewIF;      ///< Pending IF write

	uint16_t WaitControl;              ///< WAITCNT ($4000204)
	uint8_t PrgWaitStates0[2] = {5, 3}; ///< ROM wait states bank 0 [N, S]
	uint8_t PrgWaitStates1[2] = {5, 5}; ///< ROM wait states bank 1
	uint8_t PrgWaitStates2[2] = {5, 9}; ///< ROM wait states bank 2
	uint8_t SramWaitStates = 5;         ///< SRAM wait states
	bool PrefetchEnabled;               ///< Prefetch buffer enabled
	uint8_t IME;         ///< Interrupt Master Enable ($4000208)
	uint8_t NewIME;      ///< Pending IME write
	uint8_t IrqUpdateCounter;  ///< Cycles until IRQ check
	uint8_t IrqLine;           ///< Current IRQ line state
	uint8_t IrqPending;        ///< IRQ pending
	bool BusLocked;            ///< Bus locked by DMA
	bool StopMode;             ///< CPU in STOP mode
	bool PostBootFlag;         ///< Boot ROM completed

	// Open bus values for different regions
	uint8_t BootRomOpenBus[4];
	uint8_t InternalOpenBus[4];
	uint8_t IwramOpenBus[4];
};

/// <summary>
/// ROM prefetch buffer state for faster sequential reads.
/// </summary>
struct GbaRomPrefetchState {
	uint32_t ReadAddr;      ///< Address being read
	uint32_t PrefetchAddr;  ///< Next address to prefetch
	uint8_t ClockCounter;
	bool WasFilled;
	bool Started;
	bool Sequential;
	bool HitBoundary;
};

/// <summary>
/// Individual timer channel state.
/// GBA has 4 hardware timers (TM0CNT-TM3CNT) that can cascade.
/// </summary>
struct GbaTimerState {
	uint16_t ReloadValue;       ///< Value loaded on overflow/start
	uint16_t NewReloadValue;    ///< Pending reload value
	uint16_t PrescaleMask;      ///< Prescaler mask (0, 63, 255, 1023)
	uint16_t Timer;             ///< Current counter value
	uint8_t Control;            ///< TMxCNT_H raw value

	uint8_t EnableDelay;        ///< Cycles until enable takes effect
	bool WritePending;          ///< Reload value write pending
	bool Mode;                  ///< Count-up mode (cascade from previous timer)
	bool IrqEnabled;            ///< Timer overflow triggers IRQ
	bool Enabled;               ///< Timer running
	bool ProcessTimer;          ///< Timer needs processing this cycle
};

/// <summary>
/// All four GBA timer channels.
/// </summary>
struct GbaTimersState {
	GbaTimerState Timer[4];     ///< Timer 0-3 states
};

/// <summary>
/// DMA trigger condition.
/// Determines when DMA transfer starts.
/// </summary>
enum class GbaDmaTrigger : uint8_t {
	Immediate = 0,  ///< Start immediately when enabled
	VBlank = 1,     ///< Start at VBlank
	HBlank = 2,     ///< Start at HBlank (each scanline)
	Special = 3     ///< Channel-specific (sound FIFO, video capture)
};

/// <summary>
/// DMA address update mode after each transfer unit.
/// </summary>
enum class GbaDmaAddrMode : uint8_t {
	Increment,       ///< Increment address after each unit
	Decrement,       ///< Decrement address after each unit
	Fixed,           ///< Address stays constant
	IncrementReload  ///< Increment, but reload at repeat (dest only)
};

/// <summary>
/// Individual DMA channel state.
/// GBA has 4 DMA channels with different capabilities:
/// - Ch0: Highest priority, no audio
/// - Ch1/2: Audio FIFO support
/// - Ch3: Video capture, general purpose
/// </summary>
struct GbaDmaChannel {
	uint64_t StartClock;       ///< Master clock when transfer started
	uint32_t ReadValue;        ///< Last value read (for open bus)

	uint32_t Destination;      ///< Destination address register
	uint32_t Source;           ///< Source address register
	uint16_t Length;           ///< Transfer length register

	uint32_t DestLatch;        ///< Latched destination address
	uint32_t SrcLatch;         ///< Latched source address
	uint16_t LenLatch;         ///< Latched transfer length

	uint16_t Control;          ///< DMAxCNT_H raw value

	GbaDmaAddrMode DestMode;   ///< Destination address control
	GbaDmaAddrMode SrcMode;    ///< Source address control

	bool Repeat;               ///< Repeat transfer on each trigger
	bool WordTransfer;         ///< True=32-bit, False=16-bit units
	bool DrqMode;              ///< Game Pak DRQ mode (Ch3 only)

	GbaDmaTrigger Trigger;     ///< When to start transfer
	bool IrqEnabled;           ///< IRQ on transfer complete
	bool Enabled;              ///< Channel enabled
	bool Active;               ///< Transfer currently in progress

	bool Pending;              ///< Transfer pending (waiting for trigger)
};

/// <summary>
/// All four GBA DMA channels.
/// </summary>
struct GbaDmaControllerState {
	GbaDmaChannel Ch[4];       ///< DMA channels 0-3
};

/// <summary>
/// Square wave channel state for GBA APU (channels 1 and 2).
/// Based on Game Boy sound hardware with minor differences.
/// </summary>
struct GbaSquareState {
	uint16_t Frequency;         ///< Frequency register (11-bit)
	uint16_t Timer;             ///< Period counter

	uint16_t SweepTimer;        ///< Sweep unit timer
	uint16_t SweepFreq;         ///< Shadow frequency for sweep
	uint16_t SweepPeriod;       ///< Sweep period (0-7)
	uint8_t SweepUpdateDelay;   ///< Delay before sweep update
	bool SweepNegate;           ///< True=decrease frequency
	uint8_t SweepShift;         ///< Frequency shift amount (0-7)

	bool SweepEnabled;          ///< Sweep unit active
	bool SweepNegateCalcDone;   ///< Negate calculation performed

	uint8_t Volume;             ///< Current envelope volume (0-15)
	uint8_t EnvVolume;          ///< Envelope starting volume
	bool EnvRaiseVolume;        ///< True=increase, False=decrease
	uint8_t EnvPeriod;          ///< Envelope period (0-7)
	uint8_t EnvTimer;           ///< Envelope timer counter
	bool EnvStopped;            ///< Envelope finished

	uint8_t Duty;               ///< Duty cycle (0-3: 12.5%, 25%, 50%, 75%)

	uint8_t Length;             ///< Length counter (0-63)
	bool LengthEnabled;         ///< Stop when length expires

	bool Enabled;               ///< Channel enabled (producing output)
	uint8_t DutyPos;            ///< Current position in duty cycle
	uint8_t Output;             ///< Current output sample (0-15)
};

/// <summary>
/// Noise channel state for GBA APU (channel 4).
/// Uses LFSR for pseudo-random noise generation.
/// </summary>
struct GbaNoiseState {
	uint8_t Volume;             ///< Current envelope volume (0-15)
	uint8_t EnvVolume;          ///< Envelope starting volume
	bool EnvRaiseVolume;        ///< True=increase, False=decrease
	uint8_t EnvPeriod;          ///< Envelope period (0-7)
	uint8_t EnvTimer;           ///< Envelope timer counter
	bool EnvStopped;            ///< Envelope finished

	uint8_t Length;             ///< Length counter (0-63)
	bool LengthEnabled;         ///< Stop when length expires

	uint16_t ShiftRegister;     ///< 15-bit LFSR state

	uint8_t PeriodShift;        ///< Clock divider shift (0-13)
	uint8_t Divisor;            ///< Base divisor (0-7)
	bool ShortWidthMode;        ///< True=7-bit LFSR, False=15-bit

	bool Enabled;               ///< Channel enabled (producing output)
	uint32_t Timer;             ///< Period counter
	uint8_t Output;             ///< Current output sample (0-15)
};

/// <summary>
/// Wave channel state for GBA APU (channel 3).
/// Plays samples from 32-byte wave RAM with 2 banks.
/// </summary>
struct GbaWaveState {
	bool DacEnabled;            ///< DAC power (NR30 bit 7)
	bool DoubleLength;          ///< Two 32-byte banks (GBA feature)
	uint8_t SelectedBank;       ///< Current playback bank (0 or 1)

	uint8_t SampleBuffer;       ///< Current sample byte buffer
	uint8_t Ram[0x20];          ///< Wave pattern RAM (32 bytes, 64 4-bit samples)
	uint8_t Position;           ///< Current sample position (0-63)

	uint8_t Volume;             ///< Volume code (0=mute, 1=100%, 2=50%, 3=25%)
	bool OverrideVolume;        ///< Force 75% volume (GBA feature)
	uint16_t Frequency;         ///< Frequency register (11-bit)

	uint16_t Length;            ///< Length counter (0-255)
	bool LengthEnabled;         ///< Stop when length expires

	bool Enabled;               ///< Channel enabled (producing output)
	uint16_t Timer;             ///< Period counter
	uint8_t Output;             ///< Current output sample (0-15)
};

/// <summary>
/// GBA APU state including GB-compatible channels and direct sound.
/// GBA extends Game Boy APU with two DMA sound channels (A and B).
/// </summary>
struct GbaApuState {
	int8_t DmaSampleA;          ///< Current DMA sound A sample
	int8_t DmaSampleB;          ///< Current DMA sound B sample

	uint8_t VolumeControl;      ///< SOUNDCNT_H ($4000082) low byte
	uint8_t GbVolume;           ///< GB channel master volume (0-2)
	uint8_t VolumeA;            ///< DMA sound A volume (0=50%, 1=100%)
	uint8_t VolumeB;            ///< DMA sound B volume (0=50%, 1=100%)

	uint8_t DmaSoundControl;    ///< SOUNDCNT_H high byte
	bool EnableRightA;          ///< Sound A to right speaker
	bool EnableLeftA;           ///< Sound A to left speaker
	uint8_t TimerA;             ///< Timer for sound A (0 or 1)
	bool EnableRightB;          ///< Sound B to right speaker
	bool EnableLeftB;           ///< Sound B to left speaker
	uint8_t TimerB;             ///< Timer for sound B (0 or 1)

	uint8_t EnabledGb;          ///< SOUNDCNT_L ($4000080) high byte
	uint8_t EnableLeftSq1;      ///< Square 1 to left
	uint8_t EnableLeftSq2;      ///< Square 2 to left
	uint8_t EnableLeftWave;     ///< Wave to left
	uint8_t EnableLeftNoise;    ///< Noise to left

	uint8_t EnableRightSq1;     ///< Square 1 to right
	uint8_t EnableRightSq2;     ///< Square 2 to right
	uint8_t EnableRightWave;    ///< Wave to right
	uint8_t EnableRightNoise;   ///< Noise to right

	uint8_t LeftVolume;         ///< Left master volume (0-7)
	uint8_t RightVolume;        ///< Right master volume (0-7)

	uint8_t FrameSequenceStep;  ///< Frame sequencer position (0-7)

	bool ApuEnabled;            ///< Master APU enable (SOUNDCNT_X bit 7)

	uint16_t Bias;              ///< SOUNDBIAS ($4000088) value
	uint8_t SamplingRate;       ///< Output sampling rate (0-3)
};

/// <summary>
/// Serial communication port state.
/// Supports multiplayer link, normal, UART, and JOY Bus modes.
/// </summary>
struct GbaSerialState {
	uint64_t StartMasterClock;   ///< Transfer start time
	uint64_t EndMasterClock;     ///< Transfer end time
	uint64_t IrqMasterClock;     ///< IRQ trigger time

	uint16_t Data[4];            ///< SIOMULTI0-3 ($4000120-$4000127)

	uint16_t Control;            ///< SIOCNT ($4000128)
	bool InternalShiftClock;     ///< Use internal clock (master mode)
	bool InternalShiftClockSpeed2MHz;  ///< 2MHz clock (vs 256KHz)
	bool Active;                 ///< Transfer in progress
	bool TransferWord;           ///< 32-bit transfer mode
	bool IrqEnabled;             ///< IRQ on transfer complete

	uint16_t SendData;           ///< SIODATA8/SIOMLT_SEND ($400012A)
	uint16_t Mode;               ///< RCNT ($4000134) mode select
	uint16_t JoyControl;         ///< JOYCNT ($4000140)
	uint32_t JoyReceive;         ///< JOY_RECV ($4000150)
	uint32_t JoySend;            ///< JOY_TRANS ($4000154)
	uint8_t JoyStatus;           ///< JOYSTAT ($4000158)
};

/// <summary>
/// Controller input state.
/// </summary>
struct GbaControlManagerState {
	uint16_t KeyControl;         ///< KEYCNT ($4000132) interrupt control
	uint16_t ActiveKeys;         ///< Currently pressed buttons
};

/// <summary>
/// APU debug state combining all channels.
/// </summary>
struct GbaApuDebugState {
	GbaApuState Common;          ///< Global APU registers
	GbaSquareState Square1;      ///< Square wave channel 1
	GbaSquareState Square2;      ///< Square wave channel 2
	GbaWaveState Wave;           ///< Wave channel
	GbaNoiseState Noise;         ///< Noise channel
};

/// <summary>
/// GPIO state for cartridge peripherals (RTC, solar sensor, etc).
/// </summary>
struct GbaGpioState {
	uint8_t Data;                ///< GPIO data register
	uint8_t WritablePins;        ///< GPIO direction (1=output)
	bool ReadWrite;              ///< Read/write enable
};

/// <summary>
/// Cartridge state including GPIO peripherals.
/// </summary>
struct GbaCartState {
	bool HasGpio;                ///< Cartridge has GPIO
	GbaGpioState Gpio;           ///< GPIO state
};

/// <summary>
/// Complete GBA emulation state for save states.
/// </summary>
struct GbaState {
	GbaCpuState Cpu;                     ///< ARM7TDMI CPU state
	GbaPpuState Ppu;                     ///< PPU (graphics) state
	GbaApuDebugState Apu;                ///< APU (audio) state
	GbaMemoryManagerState MemoryManager; ///< Memory and IRQ state
	GbaDmaControllerState Dma;           ///< DMA controller state
	GbaTimersState Timer;                ///< Hardware timers
	GbaRomPrefetchState Prefetch;        ///< ROM prefetch buffer
	GbaControlManagerState ControlManager; ///< Controller input
	GbaCartState Cart;                   ///< Cartridge peripherals
};

/// <summary>
/// Thumb instruction categories for disassembly.
/// 16-bit Thumb instruction set has distinct encoding groups.
/// </summary>
enum class GbaThumbOpCategory {
	MoveShiftedRegister,   ///< LSL, LSR, ASR by immediate
	AddSubtract,           ///< ADD/SUB with register/immediate
	MoveCmpAddSub,         ///< MOV/CMP/ADD/SUB with 8-bit immediate
	AluOperation,          ///< Data processing (AND, EOR, etc)
	HiRegBranchExch,       ///< High register ops, BX, BLX
	PcRelLoad,             ///< LDR Rd, [PC, #imm]
	LoadStoreRegOffset,    ///< LDR/STR with register offset
	LoadStoreSignExtended, ///< LDRSB, LDRSH, etc.
	LoadStoreImmOffset,    ///< LDR/STR with immediate offset
	LoadStoreHalfWord,     ///< LDRH/STRH
	SpRelLoadStore,        ///< LDR/STR relative to SP
	LoadAddress,           ///< ADR (load PC/SP relative address)
	AddOffsetToSp,         ///< ADD SP, #imm / SUB SP, #imm
	PushPopReg,            ///< PUSH/POP register list
	MultipleLoadStore,     ///< LDMIA/STMIA
	ConditionalBranch,     ///< B{cond} with 8-bit offset
	SoftwareInterrupt,     ///< SWI
	UnconditionalBranch,   ///< B with 11-bit offset
	LongBranchLink,        ///< BL (two-instruction sequence)

	InvalidOp,             ///< Invalid/undefined instruction
};

/// <summary>
/// IRQ source flags for interrupt handling.
/// </summary>
enum class GbaIrqSource {
	None = 0,                   ///< No interrupt
	LcdVblank = 1 << 0,         ///< VBlank start (scanline 160)
	LcdHblank = 1 << 1,         ///< HBlank start (each scanline)
	LcdScanlineMatch = 1 << 2,  ///< V-counter match (DISPSTAT LYC)

	Timer0 = 1 << 3,            ///< Timer 0 overflow
	Timer1 = 1 << 4,            ///< Timer 1 overflow
	Timer2 = 1 << 5,            ///< Timer 2 overflow
	Timer3 = 1 << 6,            ///< Timer 3 overflow

	Serial = 1 << 7,            ///< Serial communication complete

	DmaChannel0 = 1 << 8,       ///< DMA 0 complete
	DmaChannel1 = 1 << 9,       ///< DMA 1 complete
	DmaChannel2 = 1 << 10,      ///< DMA 2 complete
	DmaChannel3 = 1 << 11,      ///< DMA 3 complete

	Keypad = 1 << 12,           ///< Key combination interrupt
	External = 1 << 13          ///< Game Pak IRQ (rare)
};

/// <summary>
/// GBA display constants.
/// </summary>
class GbaConstants {
public:
	static constexpr uint32_t ScreenWidth = 240;   ///< Horizontal resolution
	static constexpr uint32_t ScreenHeight = 160;  ///< Vertical resolution
	static constexpr uint32_t PixelCount = GbaConstants::ScreenWidth * GbaConstants::ScreenHeight;

	static constexpr uint32_t ScanlineCount = 228; ///< Total scanlines per frame (160 visible + 68 vblank)
};