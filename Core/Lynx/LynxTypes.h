#pragma once
#include "pch.h"
#include "Shared/BaseState.h"
#include "Shared/SettingTypes.h"

// ============================================================================
// CPU Addressing Modes and Flags
// ============================================================================

/// <summary>
/// 65C02 addressing modes (WDC variant).
/// Includes the (zp) indirect mode not present on NMOS 6502.
/// </summary>
enum class LynxAddrMode : uint8_t {
	None,      // No operand
	Acc,       // Accumulator (implicit A)
	Imp,       // Implied
	Imm,       // #nn
	Rel,       // Relative (branches)
	Zpg,       // $nn (zero page)
	ZpgX,      // $nn,X
	ZpgY,      // $nn,Y
	Abs,       // $nnnn
	AbsX,      // $nnnn,X
	AbsXW,     // $nnnn,X (always write, no page-cross optimization)
	AbsY,      // $nnnn,Y
	AbsYW,     // $nnnn,Y (always write)
	Ind,       // ($nnnn) — JMP indirect
	IndX,      // ($nn,X)
	IndY,      // ($nn),Y
	IndYW,     // ($nn),Y (always write)
	ZpgInd,    // ($nn) — 65C02 zero page indirect (no index)
	AbsIndX,   // ($nnnn,X) — 65C02 JMP (abs,X)
};

/// <summary>
/// 65C02 processor status flags (same bit layout as 6502).
/// </summary>
namespace LynxPSFlags {
	enum LynxPSFlags : uint8_t {
		Carry     = 0x01,
		Zero      = 0x02,
		Interrupt = 0x04,
		Decimal   = 0x08,
		Break     = 0x10,
		Reserved  = 0x20,
		Overflow  = 0x40,
		Negative  = 0x80
	};
}

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

/// <summary>
/// EEPROM chip type — values match the BLL/LNX header standard (byte offset 60).
///
/// The LNX header encodes the EEPROM type at byte 60. Values 1-5 specify the
/// Microwire serial EEPROM chip (93Cxx family). Additional flags:
///   - Bit 6 (0x40): SD card storage (flash cart feature, ignored by emulator)
///   - Bit 7 (0x80): 8-bit word organization instead of 16-bit
/// </summary>
enum class LynxEepromType : uint8_t {
	None       = 0, // No EEPROM
	Eeprom93c46 = 1, // 128 bytes  (64 × 16-bit words, 6 address bits)
	Eeprom93c56 = 2, // 256 bytes  (128 × 16-bit words, 7 address bits)
	Eeprom93c66 = 3, // 512 bytes  (256 × 16-bit words, 8 address bits)
	Eeprom93c76 = 4, // 1024 bytes (512 × 16-bit words, 9 address bits)
	Eeprom93c86 = 5  // 2048 bytes (1024 × 16-bit words, 10 address bits)
};

/// <summary>EEPROM serial protocol state</summary>
enum class LynxEepromState : uint8_t {
	Idle,
	ReceivingOpcode,
	ReceivingAddress,
	ReceivingData,
	SendingData
};

/// <summary>Sprite rendering type (from SPRCTL0 bits 2-0).
/// Names and values match Handy's susie.h enum.</summary>
enum class LynxSpriteType : uint8_t {
	BackgroundShadow = 0,      // Draws all pixels (incl pen 0), collision buffer write only
	BackgroundNonCollide = 1,  // Draws all pixels (incl pen 0), no collision
	BoundaryShadow = 2,        // Skip pen 0/0x0E/0x0F, collision
	Boundary = 3,              // Skip pen 0/0x0F, collision (skip 0x0E for collision)
	Normal = 4,                // Skip pen 0, collision
	NonCollidable = 5,         // Skip pen 0, no collision
	XorShadow = 6,             // Skip pen 0, XOR with existing, collision (skip 0x0E for collision)
	Shadow = 7                 // Skip pen 0, collision (skip 0x0E for collision)
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

	/// <summary>Timer done flag — blocks counting until cleared (HW Bug 13.6)</summary>
	bool TimerDone;

	/// <summary>Last master clock cycle this channel's timer was updated</summary>
	uint64_t LastTick;
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

	// --- UART / ComLynx ---
	// See ~docs/plans/lynx-uart-comlynx-hardware-reference.md for full details.

	/// <summary>Serial control register — raw value last written to SERCTL ($FD8C).
	/// Write bit layout (§2): B7=TXINTEN, B6=RXINTEN, B5=reserved, B4=PAREN,
	/// B3=RESETERR, B2=TXOPEN, B1=TXBRK, B0=PAREVEN.
	/// Read returns different status bits (§3).</summary>
	uint8_t SerialControl;

	/// <summary>TX countdown — Timer 4 ticks remaining until transmission completes (§6).
	/// Starts at 11 (UART_TX_TIME_PERIOD) on write to SERDAT.
	/// Sentinel 0x80000000 (UartTxInactive) = transmitter idle (§6.3).</summary>
	uint32_t UartTxCountdown;

	/// <summary>RX countdown — Timer 4 ticks remaining until next byte delivery (§7).
	/// Starts at 11 when first byte enqueued, then 55 (11+44) between subsequent
	/// bytes. Sentinel 0x80000000 (UartRxInactive) = receiver idle (§7.3, §7.4).</summary>
	uint32_t UartRxCountdown;

	/// <summary>Transmit data register (§4). Bits [7:0] = data byte. Bit 8 = parity/9th bit.
	/// When parity disabled and PAREVEN=1, bit 8 is set (mark bit mode).</summary>
	uint16_t UartTxData;

	/// <summary>Received data word (§4). Bits [7:0] = data byte. Bit 8 = parity.
	/// Bit 15 = break flag (UART_BREAK_CODE = 0x8000). Delivered from RX queue
	/// when countdown reaches 0 (§7.3).</summary>
	uint16_t UartRxData;

	/// <summary>RX data available to read via SERDAT (§3 bit 6 RXRDY, §4).
	/// Set when a byte is delivered from the RX queue. Cleared on SERDAT read.</summary>
	bool UartRxReady;

	/// <summary>TX interrupt enable (§2 bit 7). Level-sensitive — IRQ fires continuously
	/// while TX is idle and this bit is set (hardware bug §9.1).</summary>
	bool UartTxIrqEnable;

	/// <summary>RX interrupt enable (§2 bit 6). Level-sensitive — IRQ fires continuously
	/// while RX data is ready and this bit is set (hardware bug §9.1).</summary>
	bool UartRxIrqEnable;

	/// <summary>Parity generation/checking enabled (§2 bit 4).
	/// When set, UART calculates parity (unimplemented in Handy/Mednafen — §9).
	/// When clear, PAREVEN bit value is sent as 9th bit directly.</summary>
	bool UartParityEnable;

	/// <summary>Even parity select or 9th bit value (§2 bit 0).
	/// When PAREN=1: even(1) vs odd(0) parity.
	/// When PAREN=0: directly sent as the 9th bit of the frame.</summary>
	bool UartParityEven;

	/// <summary>Continuously send break signal (§2 bit 1, §6.2).
	/// While set, transmitter auto-retransmits UART_BREAK_CODE every 11 ticks.
	/// Break loopback is front-inserted into the RX queue (§7.2).</summary>
	bool UartSendBreak;

	/// <summary>RX overrun error (§3 bit 3, §7.3).
	/// Set when a new byte is delivered from the queue while UartRxReady is still true.
	/// Cleared by writing RESETERR (bit 3) to SERCTL.</summary>
	bool UartRxOverrunError;

	/// <summary>RX framing error (§3 bit 2).
	/// Never actually generated by emulation (perfect bit timing) but cleared by
	/// RESETERR. Present for register compatibility (§9 — known emulation gap).</summary>
	bool UartRxFramingError;

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

	// --- Hardware math (matching Handy/hardware byte-level registers) ---
	// The Lynx math hardware uses 4 register groups operated on as byte-level
	// CPU registers but processed internally as 32/16-bit values:
	//
	// ABCD (0xFC52-0x55): Multiply operands
	//   CD = multiplicand (C=high, D=low) at 0x52-0x53
	//   AB = multiplier (A=high, B=low) at 0x54-0x55
	//   Writing A triggers multiply. Writing D clears C. Writing B clears A.
	//
	// EFGH (0xFC60-0x63): Multiply result / divide dividend
	//   H(LSB)=0x60, G=0x61, F=0x62, E(MSB)=0x63
	//   Writing H clears G. Writing F clears E.
	//   Writing E triggers divide.
	//
	// NP (0xFC56-0x57): Divide divisor
	//   P(low)=0x56, N(high)=0x57. Writing P clears N.
	//
	// JKLM (0xFC6C-0x6F): Accumulator / divide remainder
	//   M(LSB)=0x6C, L=0x6D, K=0x6E, J(MSB)=0x6F
	//   Writing M clears L and clears MathOverflow.
	//   Writing K clears J.

	/// <summary>ABCD register group — multiply operands (32-bit).
	/// High word = AB (multiplier), Low word = CD (multiplicand).
	/// Initialized to 0xFFFFFFFF per Handy Reset (stun runner bug).</summary>
	uint32_t MathABCD;

	/// <summary>EFGH register group — multiply result / divide dividend (32-bit).
	/// E=MSB(byte 3), F(byte 2), G(byte 1), H=LSB(byte 0).
	/// Initialized to 0xFFFFFFFF per Handy Reset.</summary>
	uint32_t MathEFGH;

	/// <summary>JKLM register group — accumulator / divide remainder (32-bit).
	/// J=MSB(byte 3), K(byte 2), L(byte 1), M=LSB(byte 0).
	/// Initialized to 0xFFFFFFFF per Handy Reset.</summary>
	uint32_t MathJKLM;

	/// <summary>NP register group — divide divisor (16-bit).
	/// N=high byte, P=low byte.
	/// Initialized to 0xFFFF per Handy Reset.</summary>
	uint16_t MathNP;

	/// <summary>Sign of AB operand, tracked at register write time.
	/// +1 = positive, -1 = negative. HW Bug 13.8: $8000 treated as positive,
	/// $0000 treated as negative.</summary>
	int32_t MathAB_sign;

	/// <summary>Sign of CD operand, tracked at register write time.</summary>
	int32_t MathCD_sign;

	/// <summary>Sign of EFGH result, computed during multiply.</summary>
	int32_t MathEFGH_sign;

	/// <summary>Signed math mode enabled</summary>
	bool MathSign;

	/// <summary>Accumulate mode — add result to MNOP</summary>
	bool MathAccumulate;

	/// <summary>Whether a math operation is in progress</summary>
	bool MathInProgress;

	/// <summary>Math overflow/carry flag — set when multiply or accumulate overflows 32 bits.
	/// HW Bug 13.10: This flag can be lost if a second multiply overwrites the
	/// overflow status before the CPU reads SPRSYS.</summary>
	bool MathOverflow;

	/// <summary>Last carry bit from multiply — SPRSYS read bit 5 (0x20)</summary>
	bool LastCarry;

	/// <summary>Unsafe access detected — CPU tried to access Suzy during sprite
	/// processing. SPRSYS read bit 2 (0x04). Cleared by writing bit 2 to SPRSYS.</summary>
	bool UnsafeAccess;

	/// <summary>Sprite-to-sprite collision occurred (sticky flag).
	/// Internal tracking only — not directly exposed via SPRSYS register.
	/// Set when collision buffer entries are updated during sprite rendering.</summary>
	bool SpriteToSpriteCollision;

	/// <summary>Stop-on-current flag — SPRSYS write/read bit 1 (0x02).
	/// When set, requests the sprite engine stop after the current sprite.</summary>
	bool StopOnCurrent;

	/// <summary>VStretch enable — SPRSYS write/read bit 4 (0x10). When set, vsize
	/// is applied as a stretch factor instead of absolute size.</summary>
	bool VStretch;

	/// <summary>LeftHand enable — SPRSYS write/read bit 3 (0x08). Flips the
	/// coordinate system for left-handed Lynx orientation.</summary>
	bool LeftHand;

	// --- Collision ---
	/// <summary>16-slot collision depository</summary>
	uint8_t CollisionBuffer[LynxConstants::CollisionBufferSize];

	// --- Sprite rendering registers ---
	/// <summary>Horizontal screen offset (FC04-FC05). Scroll offset for sprite rendering.
	/// Most games use 0 but scrolling games modify this.</summary>
	int16_t HOffset;

	/// <summary>Vertical screen offset (FC06-FC07). Scroll offset for sprite rendering.</summary>
	int16_t VOffset;

	/// <summary>Video buffer base address in RAM (FC08-FC09).
	/// The sprite engine writes rendered pixels to this framebuffer.</summary>
	uint16_t VideoBase;

	/// <summary>Collision buffer base address in RAM (FC0A-FC0B).
	/// Collision data is written relative to this address.</summary>
	uint16_t CollisionBase;

	/// <summary>Collision depository offset (FC24-FC25).
	/// Collision results are written at SCBAddr + CollOffset in RAM.</summary>
	uint16_t CollOffset;

	/// <summary>Horizontal size offset / accumulator init (FC28-FC29).
	/// Initial value for the horizontal size accumulator. Reset default 0x007F.</summary>
	uint16_t HSizeOff;

	/// <summary>Vertical size offset / accumulator init (FC2A-FC2B).
	/// Initial value for the vertical size accumulator. Reset default 0x007F.</summary>
	uint16_t VSizeOff;

	/// <summary>EVERON flag from SPRGO bit 2.
	/// When set, the hardware tracks whether any sprite pixel was ever on-screen
	/// and writes the result into the collision depository high bit.</summary>
	bool EverOn;

	/// <summary>NoCollide mode — SPRSYS write bit 5.
	/// When set, all collision detection is globally disabled.</summary>
	bool NoCollide;

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

	/// <summary>Mikey space ($FD00-$FDFF) visible (MAPCTL bit 1)</summary>
	bool MikeySpaceVisible;

	/// <summary>Suzy space ($FC00-$FCFF) visible (MAPCTL bit 0)</summary>
	bool SuzySpaceVisible;

	/// <summary>ROM space ($FE00-$FFF7) visible (MAPCTL bit 2)</summary>
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
