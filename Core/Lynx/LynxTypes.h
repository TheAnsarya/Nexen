#pragma once
#include "pch.h"
#include "Shared/BaseState.h"
#include "Shared/SettingTypes.h"

// ============================================================================
// Constants
// ============================================================================

class LynxConstants {
public:
	/// <summary>Master crystal oscillator frequency (16 MHz)</summary>
	static constexpr uint32_t MasterClockRate = 16000000;

	/// <summary>CPU runs at master clock / 4 = 4 MHz</summary>
	static constexpr uint32_t CpuDivider = 4;

	/// <summary>Effective CPU clock rate (4 MHz)</summary>
	static constexpr uint32_t CpuClockRate = MasterClockRate / CpuDivider;

	/// <summary>Display width in pixels</summary>
	static constexpr uint32_t ScreenWidth = 160;

	/// <summary>Display height in pixels (visible scanlines)</summary>
	static constexpr uint32_t ScreenHeight = 102;

	/// <summary>Total pixel count for frame buffer</summary>
	static constexpr uint32_t PixelCount = ScreenWidth * ScreenHeight;

	/// <summary>Total scanlines per frame (102 visible + 3 VBlank)</summary>
	static constexpr uint32_t ScanlineCount = 105;

	/// <summary>Bytes per scanline (160 pixels × 4bpp = 80 bytes)</summary>
	static constexpr uint32_t BytesPerScanline = 80;

	/// <summary>Approximate frames per second (~75.0 Hz)</summary>
	static constexpr double Fps = 75.0;

	/// <summary>CPU cycles per scanline</summary>
	static constexpr uint32_t CpuCyclesPerScanline = CpuClockRate / (uint32_t)(Fps * ScanlineCount);

	/// <summary>CPU cycles per frame</summary>
	static constexpr uint32_t CpuCyclesPerFrame = CpuCyclesPerScanline * ScanlineCount;

	/// <summary>Work RAM size (64 KB)</summary>
	static constexpr uint32_t WorkRamSize = 0x10000;

	/// <summary>Boot ROM size (512 bytes)</summary>
	static constexpr uint32_t BootRomSize = 0x200;

	/// <summary>Mikey register space base address</summary>
	static constexpr uint16_t MikeyBase = 0xfd00;

	/// <summary>Mikey register space end address</summary>
	static constexpr uint16_t MikeyEnd = 0xfdff;

	/// <summary>Suzy register space base address</summary>
	static constexpr uint16_t SuzyBase = 0xfc00;

	/// <summary>Suzy register space end address</summary>
	static constexpr uint16_t SuzyEnd = 0xfcff;

	/// <summary>Boot ROM base address when mapped</summary>
	static constexpr uint16_t BootRomBase = 0xfe00;

	/// <summary>Number of Mikey timers</summary>
	static constexpr uint32_t TimerCount = 8;

	/// <summary>Number of audio channels</summary>
	static constexpr uint32_t AudioChannelCount = 4;

	/// <summary>Number of palette entries</summary>
	static constexpr uint32_t PaletteSize = 16;

	/// <summary>Collision depository size</summary>
	static constexpr uint32_t CollisionBufferSize = 16;
};

// ============================================================================
// Enums
// ============================================================================

/// <summary>Lynx hardware model</summary>
enum class LynxModel : uint8_t {
	LynxI,
	LynxII
};

/// <summary>Screen rotation as declared in LNX header</summary>
enum class LynxRotation : uint8_t {
	None = 0,
	Left = 1,
	Right = 2
};

/// <summary>65C02 CPU stop state</summary>
enum class LynxCpuStopState : uint8_t {
	Running = 0,
	Stopped = 1,     // STP instruction
	WaitingForIrq = 2 // WAI instruction
};

/// <summary>EEPROM chip type</summary>
enum class LynxEepromType : uint8_t {
	None = 0,
	Eeprom93c46 = 1, // 128 bytes (1024 bits, 64 × 16-bit words)
	Eeprom93c66 = 2, // 512 bytes (4096 bits, 256 × 16-bit words)
	Eeprom93c86 = 3  // 2048 bytes (16384 bits, 1024 × 16-bit words)
};

/// <summary>EEPROM serial protocol state</summary>
enum class LynxEepromState : uint8_t {
	Idle,
	ReceivingOpcode,
	ReceivingAddress,
	ReceivingData,
	SendingData
};

/// <summary>Sprite rendering type (from SPRCTL0 bits 2-0)</summary>
enum class LynxSpriteType : uint8_t {
	Background = 0,
	Normal = 1,
	Boundary = 2,
	NormalShadow = 3,
	BoundaryShadow = 4,
	NonCollidable = 5,
	XorShadow = 6,
	Shadow = 7
};

/// <summary>Sprite bits per pixel (from SPRCTL0 bits 7-6)</summary>
enum class LynxSpriteBpp : uint8_t {
	Bpp1 = 0,
	Bpp2 = 1,
	Bpp3 = 2,
	Bpp4 = 3
};

/// <summary>IRQ sources — one per timer (bit positions)</summary>
namespace LynxIrqSource {
	enum LynxIrqSource : uint8_t {
		Timer0 = 0x01,
		Timer1 = 0x02,
		Timer2 = 0x04,
		Timer3 = 0x08,
		Timer4 = 0x10,
		Timer5 = 0x20,
		Timer6 = 0x40,
		Timer7 = 0x80
	};
}

// ============================================================================
// 65C02 CPU State
// ============================================================================

/// <summary>65C02 processor status flags</summary>
namespace LynxCpuFlags {
	enum LynxCpuFlags : uint8_t {
		Carry = 0x01,
		Zero = 0x02,
		IrqDisable = 0x04,
		Decimal = 0x08,
		Break = 0x10,
		Reserved = 0x20,
		Overflow = 0x40,
		Negative = 0x80
	};
}

/// <summary>65C02 CPU register state</summary>
struct LynxCpuState : BaseState {
	uint64_t CycleCount;
	uint16_t PC;
	uint8_t SP;
	uint8_t A;
	uint8_t X;
	uint8_t Y;
	uint8_t PS;
	uint8_t IRQFlag;
	bool NmiFlag;
	LynxCpuStopState StopState;
};

// ============================================================================
// Timer State
// ============================================================================

/// <summary>State for a single Mikey timer</summary>
struct LynxTimerState {
	/// <summary>Reload value (written to BACKUP register)</summary>
	uint8_t BackupValue;

	/// <summary>Control register A (clock source, enable, linking, reset-on-done, magic-tap)</summary>
	uint8_t ControlA;

	/// <summary>Current countdown value</summary>
	uint8_t Count;

	/// <summary>Control register B / status (timer-done, last-clock, borrow-in, borrow-out)</summary>
	uint8_t ControlB;

	/// <summary>Cycle count at last timer tick (for sub-cycle accuracy)</summary>
	uint64_t LastTick;

	/// <summary>Whether this timer has fired (done flag)</summary>
	bool TimerDone;

	/// <summary>Whether this timer is linked to another (cascaded)</summary>
	bool Linked;
};

// ============================================================================
// Audio State
// ============================================================================

/// <summary>State for a single Lynx audio channel (LFSR-based)</summary>
struct LynxAudioChannelState {
	/// <summary>Channel output volume (4-bit)</summary>
	uint8_t Volume;

	/// <summary>Feedback tap select for LFSR</summary>
	uint8_t FeedbackEnable;

	/// <summary>Current audio output value (signed 8-bit)</summary>
	int8_t Output;

	/// <summary>12-bit linear feedback shift register</summary>
	uint16_t ShiftRegister;

	/// <summary>Timer reload value for this channel's frequency</summary>
	uint8_t BackupValue;

	/// <summary>Channel control register</summary>
	uint8_t Control;

	/// <summary>Current timer countdown value</summary>
	uint8_t Counter;

	/// <summary>Left channel attenuation (4-bit)</summary>
	uint8_t LeftAtten;

	/// <summary>Right channel attenuation (4-bit)</summary>
	uint8_t RightAtten;

	/// <summary>Integration mode — channel output feeds into next channel</summary>
	bool Integrate;

	/// <summary>Whether this channel is enabled</summary>
	bool Enabled;
};

/// <summary>Combined audio state for all 4 channels</summary>
struct LynxApuState {
	LynxAudioChannelState Channels[LynxConstants::AudioChannelCount];

	/// <summary>Master volume / attenuation control</summary>
	uint8_t MasterVolume;

	/// <summary>Stereo output enable</summary>
	bool StereoEnabled;
};

// ============================================================================
// Mikey (Display / Timer / Audio / IRQ) State
// ============================================================================

/// <summary>Mikey chip state — timers, audio, display DMA, interrupts, UART</summary>
struct LynxMikeyState {
	// --- Timers ---
	LynxTimerState Timers[LynxConstants::TimerCount];

	// --- Audio ---
	LynxApuState Apu;

	// --- Interrupt controller ---
	/// <summary>IRQ enable mask (one bit per timer)</summary>
	uint8_t IrqEnabled;

	/// <summary>IRQ pending flags (one bit per timer)</summary>
	uint8_t IrqPending;

	// --- Display ---
	/// <summary>Display buffer start address in RAM</summary>
	uint16_t DisplayAddress;

	/// <summary>Display control register</summary>
	uint8_t DisplayControl;

	/// <summary>Current scanline being rendered (0-104)</summary>
	uint16_t CurrentScanline;

	/// <summary>Processed palette — 16 entries of ARGB32</summary>
	uint32_t Palette[LynxConstants::PaletteSize];

	/// <summary>Raw green palette register values (16 entries)</summary>
	uint8_t PaletteGreen[LynxConstants::PaletteSize];

	/// <summary>Raw blue/red palette register values (16 entries, packed: [7:4]=blue, [3:0]=red)</summary>
	uint8_t PaletteBR[LynxConstants::PaletteSize];

	// --- UART ---
	/// <summary>Serial data register</summary>
	uint8_t SerialData;

	/// <summary>Serial control register</summary>
	uint8_t SerialControl;

	// --- Misc ---
	/// <summary>Mikey hardware revision register ($FD88)</summary>
	uint8_t HardwareRevision;
};

// ============================================================================
// PPU State (Display)
// ============================================================================

/// <summary>
/// PPU state for the debugger. Lynx doesn't have a traditional PPU —
/// display is driven by Mikey's Timer 0/2 DMA reading the frame buffer.
/// This struct provides the standard debugger interface.
/// </summary>
struct LynxPpuState : BaseState {
	uint32_t FrameCount;
	uint16_t Cycle;
	uint16_t Scanline;

	/// <summary>Display buffer start address</summary>
	uint16_t DisplayAddress;

	/// <summary>Display control register</summary>
	uint8_t DisplayControl;

	/// <summary>Whether the LCD is enabled</summary>
	bool LcdEnabled;
};

// ============================================================================
// Suzy (Sprite Engine / Math / Collision / Input) State
// ============================================================================

/// <summary>Suzy chip state — sprite engine, hardware math, collision, joystick</summary>
struct LynxSuzyState {
	// --- Sprite engine ---
	/// <summary>Address of current Sprite Control Block</summary>
	uint16_t SCBAddress;

	/// <summary>Sprite control register 0 (type, bpp, flip)</summary>
	uint8_t SpriteControl0;

	/// <summary>Sprite control register 1 (reload, draw action)</summary>
	uint8_t SpriteControl1;

	/// <summary>Sprite initialization register</summary>
	uint8_t SpriteInit;

	/// <summary>Whether the sprite engine is currently processing</summary>
	bool SpriteBusy;

	/// <summary>Sprite engine enable</summary>
	bool SpriteEnabled;

	// --- Hardware math ---
	/// <summary>Math operand A (16-bit: MATHA high, MATHB low — confusing Atari naming)</summary>
	uint16_t MathA;

	/// <summary>Math operand B</summary>
	uint16_t MathB;

	/// <summary>Math operand C</summary>
	int16_t MathC;

	/// <summary>Math operand D</summary>
	int16_t MathD;

	/// <summary>Math operand E</summary>
	uint16_t MathE;

	/// <summary>Math operand F</summary>
	uint16_t MathF;

	/// <summary>Math result G (high word of multiply result)</summary>
	uint16_t MathG;

	/// <summary>Math result H</summary>
	uint16_t MathH;

	/// <summary>Math result J</summary>
	uint16_t MathJ;

	/// <summary>Math result K (low word of multiply result)</summary>
	uint16_t MathK;

	/// <summary>Math result M (accumulator high)</summary>
	uint16_t MathM;

	/// <summary>Math result N (accumulator low)</summary>
	uint16_t MathN;

	/// <summary>Signed math mode enabled</summary>
	bool MathSign;

	/// <summary>Accumulate mode — add result to MNOP</summary>
	bool MathAccumulate;

	/// <summary>Whether a math operation is in progress</summary>
	bool MathInProgress;

	// --- Collision ---
	/// <summary>16-slot collision depository</summary>
	uint8_t CollisionBuffer[LynxConstants::CollisionBufferSize];

	// --- Input ---
	/// <summary>Joystick register ($FCB0) — D-pad + face buttons</summary>
	uint8_t Joystick;

	/// <summary>Switches register ($FCB1) — cart bank, pause, bus grant</summary>
	uint8_t Switches;
};

// ============================================================================
// Memory Manager State
// ============================================================================

/// <summary>Memory manager state — MAPCTL overlay control</summary>
struct LynxMemoryManagerState {
	/// <summary>MAPCTL register value at $FFF9</summary>
	uint8_t Mapctl;

	/// <summary>Vector space ($FFFA-$FFFF) overlay visible (MAPCTL bit 3)</summary>
	bool VectorSpaceVisible;

	/// <summary>Mikey space ($FD00-$FDFF) visible (MAPCTL bit 2)</summary>
	bool MikeySpaceVisible;

	/// <summary>Suzy space ($FC00-$FCFF) visible (MAPCTL bit 1)</summary>
	bool SuzySpaceVisible;

	/// <summary>ROM space ($FE00-$FFF7) visible (MAPCTL bit 0)</summary>
	bool RomSpaceVisible;

	/// <summary>Whether the boot ROM sequence is active</summary>
	bool BootRomActive;
};

// ============================================================================
// Cart State
// ============================================================================

/// <summary>Cartridge info parsed from LNX header</summary>
struct LynxCartInfo {
	/// <summary>Cart name from LNX header (null-terminated, max 32 chars)</summary>
	char Name[33];

	/// <summary>Manufacturer name from LNX header (null-terminated, max 16 chars)</summary>
	char Manufacturer[17];

	/// <summary>Total ROM size in bytes (excluding LNX header)</summary>
	uint32_t RomSize;

	/// <summary>Page size for bank 0 (in 256-byte pages)</summary>
	uint16_t PageSizeBank0;

	/// <summary>Page size for bank 1 (in 256-byte pages)</summary>
	uint16_t PageSizeBank1;

	/// <summary>Screen rotation hint</summary>
	LynxRotation Rotation;

	/// <summary>Whether this cart has EEPROM save data</summary>
	bool HasEeprom;

	/// <summary>EEPROM chip type</summary>
	LynxEepromType EepromType;

	/// <summary>LNX header version</summary>
	uint16_t Version;
};

/// <summary>Cart runtime state (banking)</summary>
struct LynxCartState {
	/// <summary>Cart info from LNX header</summary>
	LynxCartInfo Info;

	/// <summary>Currently selected ROM bank / page counter</summary>
	uint16_t CurrentBank;

	/// <summary>Cart shift register for bank switching</summary>
	uint8_t ShiftRegister;

	/// <summary>Cart address counter</summary>
	uint32_t AddressCounter;
};

// ============================================================================
// EEPROM State
// ============================================================================

/// <summary>EEPROM serial protocol state</summary>
struct LynxEepromSerialState {
	/// <summary>EEPROM chip type</summary>
	LynxEepromType Type;

	/// <summary>Current protocol state machine state</summary>
	LynxEepromState State;

	/// <summary>Opcode being received</summary>
	uint16_t Opcode;

	/// <summary>Address for current operation</summary>
	uint16_t Address;

	/// <summary>Data buffer for read/write operations</summary>
	uint16_t DataBuffer;

	/// <summary>Bit counter for serial I/O</summary>
	uint8_t BitCount;

	/// <summary>Write enable latch (EWEN/EWDS)</summary>
	bool WriteEnabled;

	/// <summary>Chip select active</summary>
	bool CsActive;

	/// <summary>Clock line state</summary>
	bool ClockState;

	/// <summary>Data out pin state</summary>
	bool DataOut;
};

// ============================================================================
// Control Manager State
// ============================================================================

/// <summary>Control manager state (minimal — Lynx has a single fixed controller)</summary>
struct LynxControlManagerState {
	/// <summary>Last read joystick value</summary>
	uint8_t Joystick;

	/// <summary>Last read switches value</summary>
	uint8_t Switches;
};

// ============================================================================
// Top-Level Console State
// ============================================================================

/// <summary>Complete Lynx emulation state for save states and debugger</summary>
struct LynxState {
	LynxModel Model;
	LynxCpuState Cpu;
	LynxPpuState Ppu;
	LynxMikeyState Mikey;
	LynxSuzyState Suzy;
	LynxApuState Apu;
	LynxMemoryManagerState MemoryManager;
	LynxControlManagerState ControlManager;
	LynxCartState Cart;
	LynxEepromSerialState Eeprom;
};
