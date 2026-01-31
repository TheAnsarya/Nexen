#pragma once
#include "pch.h"
#include "Shared/BaseState.h"


/// <summary>
/// 6502 processor status flag bits (P register).
/// </summary>
namespace PSFlags {
enum PSFlags : uint8_t {
	Carry = 0x01,      ///< Carry flag (C)
	Zero = 0x02,       ///< Zero flag (Z)
	Interrupt = 0x04,  ///< Interrupt disable (I)
	Decimal = 0x08,    ///< Decimal mode (D, unused on NES)
	Break = 0x10,      ///< Break command (B)
	Reserved = 0x20,   ///< Always set (unused)
	Overflow = 0x40,   ///< Overflow flag (V)
	Negative = 0x80    ///< Negative flag (N)
};
} // namespace PSFlags


/// <summary>
/// 6502 addressing modes for instruction decoding.
/// </summary>
enum class NesAddrMode {
	None,    ///< Not used
	Acc,     ///< Accumulator
	Imp,     ///< Implied
	Imm,     ///< Immediate
	Rel,     ///< Relative (branch)
	Zero,    ///< Zero page
	Abs,     ///< Absolute
	ZeroX,   ///< Zero page,X
	ZeroY,   ///< Zero page,Y
	Ind,     ///< Indirect (JMP)
	IndX,    ///< (Indirect,X)
	IndY,    ///< (Indirect),Y
	IndYW,   ///< (Indirect),Y with wrap
	AbsX,    ///< Absolute,X
	AbsXW,   ///< Absolute,X with wrap
	AbsY,    ///< Absolute,Y
	AbsYW,   ///< Absolute,Y with wrap
	Other    ///< Special/illegal
};


/// <summary>
/// Sources of IRQ (interrupt requests) on the NES.
/// </summary>
enum class IRQSource {
	External = 1,      ///< External IRQ (mapper, controller, etc.)
	FrameCounter = 2,  ///< APU frame counter
	DMC = 4,           ///< DMC sample playback
	FdsDisk = 8,       ///< FDS disk system
	Epsm = 16          ///< EPSM expansion audio
};


/// <summary>
/// Memory operation type for tracing and debugging.
/// </summary>
enum class MemoryOperation {
	Read = 1,   ///< Read operation
	Write = 2,  ///< Write operation
	Any = 3     ///< Any access
};


/// <summary>
/// Complete 6502 CPU state for NES emulation.
/// </summary>
struct NesCpuState : BaseState {
	uint64_t CycleCount = 0; ///< Total CPU cycles executed
	uint16_t PC = 0;         ///< Program counter
	uint8_t SP = 0;          ///< Stack pointer
	uint8_t A = 0;           ///< Accumulator
	uint8_t X = 0;           ///< X index register
	uint8_t Y = 0;           ///< Y index register
	uint8_t PS = 0;          ///< Processor status (flags)
	uint8_t IrqFlag = 0;     ///< IRQ pending flag
	bool NmiFlag = false;    ///< NMI pending flag
};


/// <summary>
/// Types of PRG (program) memory in NES address space.
/// </summary>
enum class PrgMemoryType {
	PrgRom,     ///< Cartridge ROM
	SaveRam,    ///< Battery-backed RAM
	WorkRam,    ///< Internal work RAM
	MapperRam,  ///< Mapper-controlled RAM
};


/// <summary>
/// Types of CHR (character/graphics) memory in NES PPU address space.
/// </summary>
enum class ChrMemoryType {
	Default,      ///< Default (mapper-specific)
	ChrRom,       ///< CHR ROM (cartridge)
	ChrRam,       ///< CHR RAM (cartridge)
	NametableRam, ///< Nametable RAM (CIRAM)
	MapperRam,    ///< Mapper-controlled RAM
};


/// <summary>
/// Memory access permissions for address decoding.
/// </summary>
enum MemoryAccessType {
	Unspecified = -1, ///< Not specified
	NoAccess = 0x00,  ///< No access
	Read = 0x01,      ///< Read allowed
	Write = 0x02,     ///< Write allowed
	ReadWrite = 0x03  ///< Read and write allowed
};


/// <summary>
/// Nametable mirroring types for PPU address mapping.
/// </summary>
enum class MirroringType {
	Horizontal,   ///< Horizontal mirroring (vertical split)
	Vertical,     ///< Vertical mirroring (horizontal split)
	ScreenAOnly,  ///< Single screen A
	ScreenBOnly,  ///< Single screen B
	FourScreens   ///< Four-screen VRAM
};


/// <summary>
/// Value types for storing mapper state variables.
/// </summary>
enum class MapperStateValueType {
	None,      ///< No value
	String,    ///< String value
	Bool,      ///< Boolean value
	Number8,   ///< 8-bit integer
	Number16,  ///< 16-bit integer
	Number32   ///< 32-bit integer
};

/// <summary>
/// Entry for saving/restoring mapper state (address, name, value).
/// </summary>
struct MapperStateEntry {
	static constexpr int MaxLength = 40;

	int64_t RawValue = INT64_MIN; ///< Raw value (if numeric)
	MapperStateValueType Type = MapperStateValueType::Number8; ///< Value type
	uint8_t Address[MapperStateEntry::MaxLength] = {}; ///< Address string
	uint8_t Name[MapperStateEntry::MaxLength] = {};    ///< Name string
	uint8_t Value[MapperStateEntry::MaxLength] = {};   ///< Value string

	MapperStateEntry() {}

	MapperStateEntry(string address, string name) {
		memcpy(Address, address.c_str(), std::min<size_t>(MapperStateEntry::MaxLength - 1, address.size()));
		memcpy(Name, name.c_str(), std::min<size_t>(MapperStateEntry::MaxLength - 1, name.size()));
		Type = MapperStateValueType::None;
	}

	MapperStateEntry(string address, string name, string value, int64_t rawValue = INT64_MIN) : MapperStateEntry(address, name) {
		memcpy(Value, value.c_str(), std::min<size_t>(MapperStateEntry::MaxLength - 1, value.size()));
		RawValue = rawValue;
		Type = MapperStateValueType::String;
	}

	MapperStateEntry(string address, string name, bool value) : MapperStateEntry(address, name) {
		Value[0] = value;
		Type = MapperStateValueType::Bool;
	}

	MapperStateEntry(string address, string name, int64_t value, MapperStateValueType length) : MapperStateEntry(address, name) {
		for (int i = 0; i < 8; i++) {
			Value[i] = value & 0xFF;
			value >>= 8;
		}
		Type = length;
	}
};

/// <summary>
/// Cartridge memory configuration and mapper state.
/// </summary>
/// <remarks>
/// Stores the current memory mapping state including:
/// - PRG ROM/RAM page mappings (256 slots of 256 bytes each)
/// - CHR ROM/RAM page mappings (64 slots of 256 bytes each)
/// - Nametable mirroring configuration
/// - Custom mapper state entries
/// </remarks>
struct CartridgeState {
	/// <summary>Total PRG ROM size in bytes.</summary>
	uint32_t PrgRomSize = 0;

	/// <summary>Total CHR ROM size in bytes.</summary>
	uint32_t ChrRomSize = 0;

	/// <summary>Total CHR RAM size in bytes.</summary>
	uint32_t ChrRamSize = 0;

	/// <summary>Number of PRG pages mapped.</summary>
	uint32_t PrgPageCount = 0;

	/// <summary>Size of each PRG page in bytes.</summary>
	uint32_t PrgPageSize = 0;

	/// <summary>PRG memory offsets for each CPU address page.</summary>
	int32_t PrgMemoryOffset[0x100] = {};

	/// <summary>PRG memory type for each CPU address page.</summary>
	PrgMemoryType PrgType[0x100] = {};

	/// <summary>PRG memory access permissions for each CPU address page.</summary>
	MemoryAccessType PrgMemoryAccess[0x100] = {};

	/// <summary>Number of CHR pages mapped.</summary>
	uint32_t ChrPageCount = 0;

	/// <summary>Size of each CHR ROM page in bytes.</summary>
	uint32_t ChrPageSize = 0;

	/// <summary>Size of each CHR RAM page in bytes.</summary>
	uint32_t ChrRamPageSize = 0;

	/// <summary>CHR memory offsets for each PPU address page.</summary>
	int32_t ChrMemoryOffset[0x40] = {};

	/// <summary>CHR memory type for each PPU address page.</summary>
	ChrMemoryType ChrType[0x40] = {};

	/// <summary>CHR memory access permissions for each PPU address page.</summary>
	MemoryAccessType ChrMemoryAccess[0x40] = {};

	/// <summary>Size of each Work RAM page in bytes.</summary>
	uint32_t WorkRamPageSize = 0;

	/// <summary>Size of each Save RAM page in bytes.</summary>
	uint32_t SaveRamPageSize = 0;

	/// <summary>Current nametable mirroring mode.</summary>
	MirroringType Mirroring = {};

	/// <summary>True if cartridge has battery-backed save RAM.</summary>
	bool HasBattery = false;

	/// <summary>Number of custom mapper state entries.</summary>
	uint32_t CustomEntryCount = 0;

	/// <summary>Custom mapper state entries for debugging.</summary>
	MapperStateEntry CustomEntries[200] = {};
};

/// <summary>
/// PPU status register flags ($2002).
/// </summary>
struct PPUStatusFlags {
	/// <summary>More than 8 sprites on scanline (buggy on real hardware).</summary>
	bool SpriteOverflow;

	/// <summary>Sprite 0 hit flag (opaque sprite 0 pixel overlaps opaque BG).</summary>
	bool Sprite0Hit;

	/// <summary>Vertical blank flag (set at start of VBlank).</summary>
	bool VerticalBlank;
};

/// <summary>
/// PPU control register flags ($2000).
/// </summary>
struct PpuControlFlags {
	/// <summary>Background pattern table address (0x0000 or 0x1000).</summary>
	uint16_t BackgroundPatternAddr;

	/// <summary>Sprite pattern table address (0x0000 or 0x1000).</summary>
	uint16_t SpritePatternAddr;

	/// <summary>VRAM address increment (false=+1, true=+32).</summary>
	bool VerticalWrite;

	/// <summary>Sprite size (false=8x8, true=8x16).</summary>
	bool LargeSprites;

	/// <summary>Select PPU chip (always 0 on standard NES).</summary>
	bool SecondaryPpu;

	/// <summary>Generate NMI at start of VBlank.</summary>
	bool NmiOnVerticalBlank;
};

/// <summary>
/// PPU mask register flags ($2001).
/// </summary>
struct PpuMaskFlags {
	/// <summary>Produce grayscale output.</summary>
	bool Grayscale;

	/// <summary>Show background in leftmost 8 pixels.</summary>
	bool BackgroundMask;

	/// <summary>Show sprites in leftmost 8 pixels.</summary>
	bool SpriteMask;

	/// <summary>Enable background rendering.</summary>
	bool BackgroundEnabled;

	/// <summary>Enable sprite rendering.</summary>
	bool SpritesEnabled;

	/// <summary>Emphasize red (NTSC) / green (PAL).</summary>
	bool IntensifyRed;

	/// <summary>Emphasize green (NTSC) / red (PAL).</summary>
	bool IntensifyGreen;

	/// <summary>Emphasize blue.</summary>
	bool IntensifyBlue;
};

/// <summary>
/// Background tile rendering information.
/// </summary>
struct TileInfo {
	/// <summary>Pattern table address for tile.</summary>
	uint16_t TileAddr;

	/// <summary>Low bitplane byte.</summary>
	uint8_t LowByte;

	/// <summary>High bitplane byte.</summary>
	uint8_t HighByte;

	/// <summary>Palette offset (0, 4, 8, or 12).</summary>
	uint8_t PaletteOffset;
};

/// <summary>
/// Sprite rendering information from OAM evaluation.
/// </summary>
struct NesSpriteInfo {
	/// <summary>Flip sprite horizontally.</summary>
	bool HorizontalMirror;

	/// <summary>Render behind background.</summary>
	bool BackgroundPriority;

	/// <summary>X coordinate of sprite.</summary>
	uint8_t SpriteX;

	/// <summary>Low bitplane byte.</summary>
	uint8_t LowByte;

	/// <summary>High bitplane byte.</summary>
	uint8_t HighByte;

	/// <summary>Palette offset (16, 20, 24, or 28).</summary>
	uint8_t PaletteOffset;
};

/// <summary>
/// Complete NES PPU (Picture Processing Unit) state.
/// </summary>
/// <remarks>
/// The NES PPU handles all graphics rendering:
/// - 256x240 pixel output (NTSC) or 256x240 (PAL)
/// - 2 pattern tables (4KB each) for tiles
/// - 2 nametables for background layout
/// - 64 sprites via OAM (Object Attribute Memory)
/// - 32 bytes of palette RAM
/// </remarks>
struct NesPpuState : public BaseState {
	/// <summary>PPU status register flags.</summary>
	PPUStatusFlags StatusFlags;

	/// <summary>PPU mask register flags.</summary>
	PpuMaskFlags Mask;

	/// <summary>PPU control register flags.</summary>
	PpuControlFlags Control;

	/// <summary>Current scanline (-1 to 260/310).</summary>
	int32_t Scanline;

	/// <summary>Current cycle within scanline (0-340).</summary>
	uint32_t Cycle;

	/// <summary>Total frames rendered.</summary>
	uint32_t FrameCount;

	/// <summary>Scanline where NMI occurs (usually 241).</summary>
	uint32_t NmiScanline;

	/// <summary>Total scanlines per frame (262 NTSC, 312 PAL).</summary>
	uint32_t ScanlineCount;

	/// <summary>First scanline where OAM writes are safe.</summary>
	uint32_t SafeOamScanline;

	/// <summary>Current PPU bus address.</summary>
	uint16_t BusAddress;

	/// <summary>Internal read buffer for $2007 reads.</summary>
	uint8_t MemoryReadBuffer;

	/// <summary>Current VRAM address (15-bit).</summary>
	uint16_t VideoRamAddr;

	/// <summary>Temporary VRAM address (15-bit).</summary>
	uint16_t TmpVideoRamAddr;

	/// <summary>Fine X scroll (0-7).</summary>
	uint8_t ScrollX;

	/// <summary>Address latch toggle for $2005/$2006 writes.</summary>
	bool WriteToggle;

	/// <summary>OAM address register ($2003).</summary>
	uint8_t SpriteRamAddr;
};

/// <summary>
/// APU length counter state for Square, Triangle, and Noise channels.
/// </summary>
/// <remarks>
/// The length counter silences a channel after a specified number of clocks.
/// Counter values are loaded from a lookup table based on a 5-bit index.
/// </remarks>
struct ApuLengthCounterState {
	/// <summary>When true, counter is halted (doesn't decrement).</summary>
	bool Halt;

	/// <summary>Current counter value.</summary>
	uint8_t Counter;

	/// <summary>Value to reload when triggered.</summary>
	uint8_t ReloadValue;
};

/// <summary>
/// APU envelope generator state for Square and Noise channels.
/// </summary>
/// <remarks>
/// The envelope provides volume decay or constant volume output.
/// When looping is enabled, the envelope repeats instead of staying at 0.
/// </remarks>
struct ApuEnvelopeState {
	/// <summary>Start flag - begins envelope from max volume.</summary>
	bool StartFlag;

	/// <summary>Loop flag - restart envelope when it reaches 0.</summary>
	bool Loop;

	/// <summary>Constant volume mode vs. envelope decay.</summary>
	bool ConstantVolume;

	/// <summary>Divider period (controls decay rate).</summary>
	uint8_t Divider;

	/// <summary>Current divider counter.</summary>
	uint8_t Counter;

	/// <summary>Envelope volume or constant volume value (0-15).</summary>
	uint8_t Volume;
};

/// <summary>
/// APU Square wave channel state.
/// </summary>
/// <remarks>
/// Two square wave channels provide the main melodic voices.
/// Features include:
/// - Duty cycle selection (12.5%, 25%, 50%, 75%)
/// - Hardware sweep unit for pitch bending
/// - Length counter for note duration
/// - Envelope for volume control
/// </remarks>
struct ApuSquareState {
	/// <summary>Duty cycle (0-3: 12.5%, 25%, 50%, 75%).</summary>
	uint8_t Duty;

	/// <summary>Current position in duty cycle waveform.</summary>
	uint8_t DutyPosition;

	/// <summary>Timer period (11-bit).</summary>
	uint16_t Period;

	/// <summary>Current timer value.</summary>
	uint16_t Timer;

	/// <summary>Sweep unit enabled.</summary>
	bool SweepEnabled;

	/// <summary>Sweep direction (true = subtract).</summary>
	bool SweepNegate;

	/// <summary>Sweep divider period.</summary>
	uint8_t SweepPeriod;

	/// <summary>Sweep shift count.</summary>
	uint8_t SweepShift;

	/// <summary>Channel is producing output.</summary>
	bool Enabled;

	/// <summary>Current output volume (0-15).</summary>
	uint8_t OutputVolume;

	/// <summary>Current frequency in Hz.</summary>
	double Frequency;

	/// <summary>Length counter state.</summary>
	ApuLengthCounterState LengthCounter;

	/// <summary>Envelope generator state.</summary>
	ApuEnvelopeState Envelope;
};

/// <summary>
/// APU Triangle wave channel state.
/// </summary>
/// <remarks>
/// The triangle channel produces a 32-step triangle wave.
/// It has a linear counter in addition to the length counter
/// for more precise note durations.
/// </remarks>
struct ApuTriangleState {
	/// <summary>Timer period (11-bit).</summary>
	uint16_t Period;

	/// <summary>Current timer value.</summary>
	uint16_t Timer;

	/// <summary>Current position in 32-step sequence.</summary>
	uint8_t SequencePosition;

	/// <summary>Channel is producing output.</summary>
	bool Enabled;

	/// <summary>Current frequency in Hz.</summary>
	double Frequency;

	/// <summary>Current output volume (0-15).</summary>
	uint8_t OutputVolume;

	/// <summary>Linear counter value.</summary>
	uint8_t LinearCounter;

	/// <summary>Linear counter reload value.</summary>
	uint8_t LinearCounterReload;

	/// <summary>Linear counter reload flag.</summary>
	bool LinearReloadFlag;

	/// <summary>Length counter state.</summary>
	ApuLengthCounterState LengthCounter;
};

/// <summary>
/// APU Noise channel state.
/// </summary>
/// <remarks>
/// The noise channel produces pseudo-random noise using an LFSR.
/// Two modes available: long sequence (32767 steps) or short (93 steps).
/// </remarks>
struct ApuNoiseState {
	/// <summary>Timer period from lookup table.</summary>
	uint16_t Period;

	/// <summary>Current timer value.</summary>
	uint16_t Timer;

	/// <summary>15-bit shift register for noise generation.</summary>
	uint16_t ShiftRegister;

	/// <summary>Noise mode (false = long, true = short sequence).</summary>
	bool ModeFlag;

	/// <summary>Channel is producing output.</summary>
	bool Enabled;

	/// <summary>Current frequency in Hz.</summary>
	double Frequency;

	/// <summary>Current output volume (0-15).</summary>
	uint8_t OutputVolume;

	/// <summary>Length counter state.</summary>
	ApuLengthCounterState LengthCounter;

	/// <summary>Envelope generator state.</summary>
	ApuEnvelopeState Envelope;
};

/// <summary>
/// APU DMC (Delta Modulation Channel) state.
/// </summary>
/// <remarks>
/// The DMC plays 1-bit delta-modulated samples directly from ROM.
/// Features include:
/// - Direct sample playback from CPU address space
/// - Optional looping
/// - IRQ generation at sample end
/// </remarks>
struct ApuDmcState {
	/// <summary>Current sample playback rate in Hz.</summary>
	double SampleRate;

	/// <summary>Sample start address ($C000-$FFFF, 64-byte aligned).</summary>
	uint16_t SampleAddr;

	/// <summary>Next sample address to play.</summary>
	uint16_t NextSampleAddr;

	/// <summary>Sample length in bytes.</summary>
	uint16_t SampleLength;

	/// <summary>Loop sample when finished.</summary>
	bool Loop;

	/// <summary>Generate IRQ when sample finishes.</summary>
	bool IrqEnabled;

	/// <summary>Timer period from lookup table.</summary>
	uint16_t Period;

	/// <summary>Current timer value.</summary>
	uint16_t Timer;

	/// <summary>Bytes remaining in current sample.</summary>
	uint16_t BytesRemaining;

	/// <summary>Current output level (0-127).</summary>
	uint8_t OutputVolume;
};

/// <summary>
/// APU Frame Counter state.
/// </summary>
/// <remarks>
/// The frame counter clocks the length counters, envelopes,
/// and sweep units at regular intervals. Two modes:
/// - 4-step: Clocks at 7457, 14913, 22371, 29829 cycles
/// - 5-step: Adds an extra step, no IRQ
/// </remarks>
struct ApuFrameCounterState {
	/// <summary>Frame counter mode (false = 4-step, true = 5-step).</summary>
	bool FiveStepMode;

	/// <summary>Current position in sequence (0-4).</summary>
	uint8_t SequencePosition;

	/// <summary>Generate IRQ on step 4 (4-step mode only).</summary>
	bool IrqEnabled;
};

/// <summary>
/// Complete NES APU (Audio Processing Unit) state.
/// </summary>
struct ApuState {
	/// <summary>Square wave channel 1 state.</summary>
	ApuSquareState Square1;

	/// <summary>Square wave channel 2 state.</summary>
	ApuSquareState Square2;

	/// <summary>Triangle wave channel state.</summary>
	ApuTriangleState Triangle;

	/// <summary>Noise channel state.</summary>
	ApuNoiseState Noise;

	/// <summary>DMC (sample playback) channel state.</summary>
	ApuDmcState Dmc;

	/// <summary>Frame counter state.</summary>
	ApuFrameCounterState FrameCounter;
};

/// <summary>
/// NES game system/region variants.
/// </summary>
enum class GameSystem {
	/// <summary>NTSC NES (North America).</summary>
	NesNtsc,

	/// <summary>PAL NES (Europe/Australia).</summary>
	NesPal,

	/// <summary>Famicom (Japan).</summary>
	Famicom,

	/// <summary>Dendy (Russia/Eastern Europe clone).</summary>
	Dendy,

	/// <summary>VS. System arcade hardware.</summary>
	VsSystem,

	/// <summary>PlayChoice-10 arcade hardware.</summary>
	Playchoice,

	/// <summary>Famicom Disk System.</summary>
	FDS,

	/// <summary>Famicom Network System.</summary>
	FamicomNetworkSystem,

	/// <summary>Unknown system.</summary>
	Unknown,
};

/// <summary>
/// Bus conflict behavior setting for mappers.
/// </summary>
enum class BusConflictType {
	/// <summary>Use mapper-specific default.</summary>
	Default = 0,

	/// <summary>Force bus conflicts enabled.</summary>
	Yes,

	/// <summary>Force bus conflicts disabled.</summary>
	No
};

/// <summary>
/// ROM hash information for identification.
/// </summary>
struct HashInfo {
	/// <summary>CRC32 of entire ROM file.</summary>
	uint32_t Crc32 = 0;

	/// <summary>CRC32 of PRG ROM only.</summary>
	uint32_t PrgCrc32 = 0;

	/// <summary>CRC32 of PRG + CHR ROM.</summary>
	uint32_t PrgChrCrc32 = 0;
};

/// <summary>
/// VS. System protection types and configurations.
/// </summary>
enum class VsSystemType {
	/// <summary>Standard VS. System.</summary>
	Default = 0,

	/// <summary>RBI Baseball copy protection.</summary>
	RbiBaseballProtection = 1,

	/// <summary>TKO Boxing copy protection.</summary>
	TkoBoxingProtection = 2,

	/// <summary>Super Xevious copy protection.</summary>
	SuperXeviousProtection = 3,

	/// <summary>Ice Climber copy protection.</summary>
	IceClimberProtection = 4,

	/// <summary>VS. Dual System (two monitors).</summary>
	VsDualSystem = 5,

	/// <summary>Raid on Bungeling Bay copy protection.</summary>
	RaidOnBungelingBayProtection = 6,
};

/// <summary>
/// Game input device configurations from NES 2.0 header.
/// </summary>
enum class GameInputType {
	/// <summary>Unspecified input device.</summary>
	Unspecified = 0,

	/// <summary>Standard NES controllers.</summary>
	StandardControllers = 1,

	/// <summary>Four Score adapter (4-player NES).</summary>
	FourScore = 2,

	/// <summary>Famicom 4-player adapter.</summary>
	FourPlayerAdapter = 3,

	/// <summary>VS. System input.</summary>
	VsSystem = 4,

	/// <summary>VS. System with swapped controllers.</summary>
	VsSystemSwapped = 5,

	/// <summary>VS. System with swapped A/B buttons.</summary>
	VsSystemSwapAB = 6,

	/// <summary>VS. Zapper light gun.</summary>
	VsZapper = 7,

	/// <summary>NES Zapper light gun.</summary>
	Zapper = 8,

	/// <summary>Two NES Zappers.</summary>
	TwoZappers = 9,

	/// <summary>Bandai Hyper Shot.</summary>
	BandaiHypershot = 0x0A,

	/// <summary>Power Pad Side A.</summary>
	PowerPadSideA = 0x0B,

	/// <summary>Power Pad Side B.</summary>
	PowerPadSideB = 0x0C,

	/// <summary>Family Trainer Side A.</summary>
	FamilyTrainerSideA = 0x0D,

	/// <summary>Family Trainer Side B.</summary>
	FamilyTrainerSideB = 0x0E,

	/// <summary>Arkanoid Controller (NES).</summary>
	ArkanoidControllerNes = 0x0F,

	/// <summary>Arkanoid Controller (Famicom).</summary>
	ArkanoidControllerFamicom = 0x10,

	/// <summary>Two Arkanoid Controllers.</summary>
	DoubleArkanoidController = 0x11,

	/// <summary>Konami Hyper Shot.</summary>
	KonamiHyperShot = 0x12,

	/// <summary>Pachinko Controller.</summary>
	PachinkoController = 0x13,

	/// <summary>Exciting Boxing controller.</summary>
	ExcitingBoxing = 0x14,

	/// <summary>Jissen Mahjong Controller.</summary>
	JissenMahjong = 0x15,

	/// <summary>Party Tap.</summary>
	PartyTap = 0x16,

	/// <summary>Oeka Kids Tablet.</summary>
	OekaKidsTablet = 0x17,

	/// <summary>Barcode Battler.</summary>
	BarcodeBattler = 0x18,

	/// <summary>Miracle Piano (not supported).</summary>
	MiraclePiano = 0x19,

	/// <summary>Pokkun Moguraa (not supported).</summary>
	PokkunMoguraa = 0x1A,

	/// <summary>Top Rider (not supported).</summary>
	TopRider = 0x1B,

	/// <summary>Double Fisted (not supported).</summary>
	DoubleFisted = 0x1C,

	/// <summary>Famicom 3D System (not supported).</summary>
	Famicom3dSystem = 0x1D,

	/// <summary>Doremikko Keyboard (not supported).</summary>
	DoremikkoKeyboard = 0x1E,

	/// <summary>R.O.B. (not supported).</summary>
	ROB = 0x1F,

	/// <summary>Famicom Data Recorder.</summary>
	FamicomDataRecorder = 0x20,

	/// <summary>Turbo File.</summary>
	TurboFile = 0x21,

	/// <summary>Battle Box.</summary>
	BattleBox = 0x22,

	/// <summary>Family BASIC Keyboard.</summary>
	FamilyBasicKeyboard = 0x23,

	/// <summary>PEC-586 Keyboard (not supported).</summary>
	Pec586Keyboard = 0x24,

	/// <summary>Bit-79 Keyboard (not supported).</summary>
	Bit79Keyboard = 0x25,

	/// <summary>Subor Keyboard.</summary>
	SuborKeyboard = 0x26,

	/// <summary>Subor Keyboard + Mouse (variant 1).</summary>
	SuborKeyboardMouse1 = 0x27,

	/// <summary>Subor Keyboard + Mouse (variant 2).</summary>
	SuborKeyboardMouse2 = 0x28,

	/// <summary>SNES Mouse.</summary>
	SnesMouse = 0x29,

	/// <summary>Generic Multicart (not supported).</summary>
	GenericMulticart = 0x2A,

	/// <summary>SNES Controllers.</summary>
	SnesControllers = 0x2B,

	/// <summary>Racermate Bicycle (not supported).</summary>
	RacermateBicycle = 0x2C,

	/// <summary>U-Force (not supported).</summary>
	UForce = 0x2D,

	/// <summary>Last entry marker.</summary>
	LastEntry
};

/// <summary>
/// PPU model/revision variants for VS. System and emulation accuracy.
/// </summary>
enum class PpuModel {
	/// <summary>Standard NTSC PPU.</summary>
	Ppu2C02 = 0,

	/// <summary>RGB PPU (PlayChoice).</summary>
	Ppu2C03 = 1,

	/// <summary>VS. System PPU variant A.</summary>
	Ppu2C04A = 2,

	/// <summary>VS. System PPU variant B.</summary>
	Ppu2C04B = 3,

	/// <summary>VS. System PPU variant C.</summary>
	Ppu2C04C = 4,

	/// <summary>VS. System PPU variant D.</summary>
	Ppu2C04D = 5,

	/// <summary>VS. System PPU variant 2C05A.</summary>
	Ppu2C05A = 6,

	/// <summary>VS. System PPU variant 2C05B.</summary>
	Ppu2C05B = 7,

	/// <summary>VS. System PPU variant 2C05C.</summary>
	Ppu2C05C = 8,

	/// <summary>VS. System PPU variant 2C05D.</summary>
	Ppu2C05D = 9,

	/// <summary>VS. System PPU variant 2C05E.</summary>
	Ppu2C05E = 10
};

/// <summary>
/// Audio channel identifiers for mixing and visualization.
/// </summary>
enum class AudioChannel {
	/// <summary>APU Square channel 1.</summary>
	Square1 = 0,

	/// <summary>APU Square channel 2.</summary>
	Square2 = 1,

	/// <summary>APU Triangle channel.</summary>
	Triangle = 2,

	/// <summary>APU Noise channel.</summary>
	Noise = 3,

	/// <summary>APU DMC channel.</summary>
	DMC = 4,

	/// <summary>FDS expansion audio.</summary>
	FDS = 5,

	/// <summary>MMC5 expansion audio.</summary>
	MMC5 = 6,

	/// <summary>VRC6 expansion audio.</summary>
	VRC6 = 7,

	/// <summary>VRC7 expansion audio.</summary>
	VRC7 = 8,

	/// <summary>Namco 163 expansion audio.</summary>
	Namco163 = 9,

	/// <summary>Sunsoft 5B expansion audio.</summary>
	Sunsoft5B = 10
};

/// <summary>
/// Complete NES system state for save states and debugging.
/// </summary>
struct NesState {
	/// <summary>CPU state.</summary>
	NesCpuState Cpu;

	/// <summary>PPU state.</summary>
	NesPpuState Ppu;

	/// <summary>Cartridge/mapper state.</summary>
	CartridgeState Cartridge;

	/// <summary>APU state.</summary>
	ApuState Apu;

	/// <summary>Master clock rate in Hz.</summary>
	uint32_t ClockRate;
};
