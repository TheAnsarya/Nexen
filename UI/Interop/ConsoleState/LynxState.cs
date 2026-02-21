using System;
using System.Runtime.InteropServices;

namespace Nexen.Interop;

/// <summary>Atari Lynx hardware model.</summary>
public enum LynxModel : byte {
	/// <summary>Original Lynx (1989) — larger form factor.</summary>
	LynxI = 0,
	/// <summary>Lynx II (1991) — smaller, improved battery life.</summary>
	LynxII = 1
}

/// <summary>Screen rotation for Lynx games.</summary>
/// <remarks>
/// The Lynx physically supports 180° flip via hardware rotation.
/// Games declare their preferred rotation in the LNX header.
/// </remarks>
public enum LynxRotation : byte {
	/// <summary>Landscape (default, 160×102).</summary>
	None = 0,
	/// <summary>Portrait, rotated left (102×160).</summary>
	Left = 1,
	/// <summary>Portrait, rotated right (102×160).</summary>
	Right = 2
}

/// <summary>65C02 CPU halt state.</summary>
public enum LynxCpuStopState : byte {
	/// <summary>CPU actively executing instructions.</summary>
	Running = 0,
	/// <summary>CPU halted via STP instruction (requires reset).</summary>
	Stopped = 1,
	/// <summary>CPU halted via WAI instruction (resumes on IRQ).</summary>
	WaitingForIrq = 2
}

/// <summary>Microwire serial EEPROM chip type (93Cxx family).</summary>
/// <remarks>
/// Values match the BLL/LNX header standard (byte offset 60).
/// Address bits and word count vary by chip.
/// </remarks>
public enum LynxEepromType : byte {
	/// <summary>No EEPROM present.</summary>
	None = 0,
	/// <summary>93C46: 128 bytes (64 × 16-bit words, 6 address bits).</summary>
	Eeprom93c46 = 1,
	/// <summary>93C56: 256 bytes (128 × 16-bit words, 7 address bits).</summary>
	Eeprom93c56 = 2,
	/// <summary>93C66: 512 bytes (256 × 16-bit words, 8 address bits).</summary>
	Eeprom93c66 = 3,
	/// <summary>93C76: 1024 bytes (512 × 16-bit words, 9 address bits).</summary>
	Eeprom93c76 = 4,
	/// <summary>93C86: 2048 bytes (1024 × 16-bit words, 10 address bits).</summary>
	Eeprom93c86 = 5
}

/// <summary>EEPROM serial protocol state machine phase.</summary>
public enum LynxEepromState : byte {
	/// <summary>Awaiting CS assertion and start bit.</summary>
	Idle = 0,
	/// <summary>Receiving 2-bit opcode after start bit.</summary>
	ReceivingOpcode = 1,
	/// <summary>Receiving address bits (6/8/10 depending on chip).</summary>
	ReceivingAddress = 2,
	/// <summary>Receiving 16-bit data word (WRITE/WRAL).</summary>
	ReceivingData = 3,
	/// <summary>Sending 16-bit data word (READ).</summary>
	SendingData = 4
}

/// <summary>65C02 CPU register state for debugger interop.</summary>
/// <remarks>Matches memory layout of LynxCpuState in Core/Lynx/LynxTypes.h.</remarks>
public struct LynxCpuState : BaseState {
	/// <summary>Total CPU cycles executed since power-on.</summary>
	public UInt64 CycleCount;
	/// <summary>Program Counter (16-bit).</summary>
	public UInt16 PC;
	/// <summary>Stack Pointer (8-bit, page $01).</summary>
	public byte SP;
	/// <summary>Accumulator register.</summary>
	public byte A;
	/// <summary>X index register.</summary>
	public byte X;
	/// <summary>Y index register.</summary>
	public byte Y;
	/// <summary>Processor Status flags (N/V/-/B/D/I/Z/C).</summary>
	public byte PS;
	/// <summary>IRQ pending flags (hardware signals).</summary>
	public byte IRQFlag;
	/// <summary>NMI pending (not used on Lynx, no NMI line).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool NmiFlag;
	/// <summary>CPU halt state (Running, Stopped, WaitingForIrq).</summary>
	public LynxCpuStopState StopState;
}

/// <summary>Mikey timer state for a single timer (0-7).</summary>
/// <remarks>Matches memory layout of LynxTimerState in Core/Lynx/LynxTypes.h.</remarks>
public struct LynxTimerState {
	/// <summary>Reload value (written via BACKUP register).</summary>
	public byte BackupValue;
	/// <summary>Control A: clock source, enable, linking, reset-on-done.</summary>
	public byte ControlA;
	/// <summary>Current countdown value.</summary>
	public byte Count;
	/// <summary>Control B / status: timer-done, last-clock, borrow flags.</summary>
	public byte ControlB;
	/// <summary>Master clock cycle at last timer tick.</summary>
	public UInt64 LastTick;
	/// <summary>Timer done flag (fired).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool TimerDone;
	/// <summary>Linked (cascaded) to another timer.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool Linked;
}

/// <summary>Audio channel state (LFSR-based synthesis).</summary>
/// <remarks>Matches memory layout of LynxAudioChannelState in Core/Lynx/LynxTypes.h.</remarks>
public struct LynxAudioChannelState {
	/// <summary>Output volume (signed 8-bit — Lynx hardware uses signed magnitude).</summary>
	public sbyte Volume;
	/// <summary>Feedback tap enable mask for LFSR.</summary>
	public byte FeedbackEnable;
	/// <summary>Current output sample (signed 8-bit).</summary>
	public sbyte Output;
	/// <summary>12-bit linear feedback shift register.</summary>
	public UInt16 ShiftRegister;
	/// <summary>Timer reload value (frequency control).</summary>
	public byte BackupValue;
	/// <summary>Channel control register.</summary>
	public byte Control;
	/// <summary>Current timer countdown value.</summary>
	public byte Counter;
	/// <summary>Per-channel stereo attenuation (ATTEN_x). Upper nibble = left, lower = right.</summary>
	public byte Attenuation;
	/// <summary>Integration mode: output feeds into next channel.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool Integrate;
	/// <summary>Channel enabled.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool Enabled;
	/// <summary>Timer done flag — blocks counting until cleared (HW Bug 13.6).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool TimerDone;
	/// <summary>Last master clock cycle this channel's timer was updated.</summary>
	public UInt64 LastTick;
}

/// <summary>Combined APU state for all 4 audio channels.</summary>
/// <remarks>Matches memory layout of LynxApuState in Core/Lynx/LynxTypes.h.</remarks>
public struct LynxApuState {
	/// <summary>State for all 4 audio channels.</summary>
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
	public LynxAudioChannelState[] Channels;
	/// <summary>MSTEREO ($FD50) — per-channel stereo enable bitmask.</summary>
	public byte Stereo;
	/// <summary>MPAN ($FD48) — per-channel panning enable.</summary>
	public byte Panning;
}

/// <summary>
/// Mikey chip state for interop with the C++ Core.
/// Matches the memory layout of LynxMikeyState in Core/Lynx/LynxTypes.h.
/// Includes timer, display, palette, and UART/ComLynx state.
/// </summary>
public struct LynxMikeyState : BaseState {
	/// <summary>Timer states for all 8 Mikey timers (0-7). Timer 4 drives the UART baud rate.</summary>
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
	public LynxTimerState[] Timers;

	public LynxApuState Apu;

	public byte IrqEnabled;
	public byte IrqPending;

	public UInt16 DisplayAddress;
	public byte DisplayControl;
	public UInt16 CurrentScanline;

	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
	public UInt32[] Palette;

	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
	public byte[] PaletteGreen;

	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
	public byte[] PaletteBR;

	/// <summary>Raw SERCTL write value (§2: B7=TXINTEN, B6=RXINTEN, B4=PAREN, B3=RESETERR, B2=TXOPEN, B1=TXBRK, B0=PAREVEN).</summary>
	public byte SerialControl;

	/// <summary>TX countdown — Timer 4 ticks remaining until TX complete. 0x80000000 = idle sentinel (§6.3).</summary>
	public UInt32 UartTxCountdown;

	/// <summary>RX countdown — Timer 4 ticks remaining until next byte delivery. 0x80000000 = idle (§7.4).</summary>
	public UInt32 UartRxCountdown;

	/// <summary>Transmit data register. Bits [7:0] = data, bit 8 = parity/9th bit (§4).</summary>
	public UInt16 UartTxData;

	/// <summary>Received data word. Bits [7:0] = data, bit 8 = parity, bit 15 = break flag (§4, §7.4).</summary>
	public UInt16 UartRxData;

	/// <summary>RX data available to read via SERDAT. Cleared on read (§4).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool UartRxReady;

	/// <summary>TX interrupt enable — level-sensitive, fires continuously while TX idle (§2, §9.1).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool UartTxIrqEnable;

	/// <summary>RX interrupt enable — level-sensitive, fires continuously while RX ready (§2, §9.1).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool UartRxIrqEnable;

	/// <summary>Parity generation/checking enabled. When false, PAREVEN is sent as 9th bit (§2).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool UartParityEnable;

	/// <summary>Even parity select, or 9th bit value when parity disabled (§2).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool UartParityEven;

	/// <summary>Continuously send break signal. Auto-retransmits every 11 ticks (§6.2).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool UartSendBreak;

	/// <summary>RX overrun error — new byte delivered before previous was read (§3, §7.3).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool UartRxOverrunError;

	/// <summary>RX framing error. Not generated by emulation but cleared by RESETERR (§3, §9).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool UartRxFramingError;

	public byte HardwareRevision;
}

/// <summary>Lynx PPU (display) state for debugger interop.</summary>
/// <remarks>The Lynx has no discrete PPU; display is managed by Mikey.</remarks>
public struct LynxPpuState : BaseState {
	/// <summary>Total frames rendered since power-on.</summary>
	public UInt32 FrameCount;
	/// <summary>Current horizontal position within scanline.</summary>
	public UInt16 Cycle;
	/// <summary>Current scanline (0-104, 102 visible + 3 VBlank).</summary>
	public UInt16 Scanline;
	/// <summary>Display buffer start address in RAM.</summary>
	public UInt16 DisplayAddress;
	/// <summary>Display control register.</summary>
	public byte DisplayControl;
	/// <summary>LCD output enabled.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool LcdEnabled;
}

/// <summary>Suzy coprocessor state for debugger interop.</summary>
/// <remarks>
/// Suzy handles sprite rendering, collision detection, and 16×16 math.
/// Matches memory layout of LynxSuzyState in Core/Lynx/LynxTypes.h.
/// </remarks>
public struct LynxSuzyState : BaseState {
	/// <summary>Sprite Control Block address in RAM.</summary>
	public UInt16 SCBAddress;
	/// <summary>SPRCTL0: sprite type, BPP, H/V flip.</summary>
	public byte SpriteControl0;
	/// <summary>SPRCTL1: reload flags, sizing mode, literal.</summary>
	public byte SpriteControl1;
	/// <summary>SPRCOLL (FC82): collision number (bits 3:0), don't collide (bit 5).</summary>
	public byte SpriteCollision;
	/// <summary>SPRINIT (FC83): initialization flags.</summary>
	public byte SpriteInit;
	/// <summary>Sprite engine currently processing.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool SpriteBusy;
	/// <summary>Sprite engine enabled (halts CPU during rendering).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool SpriteEnabled;

	/// <summary>Math register group ABCD (0xFC52-55): D=byte0, C=byte1, B=byte2, A=byte3.
	/// AB=multiplier, CD=multiplicand. Writing A triggers multiply.</summary>
	public UInt32 MathABCD;
	/// <summary>Math register group EFGH (0xFC60-63): H=byte0, G=byte1, F=byte2, E=byte3.
	/// Multiply result / divide dividend. Writing E triggers divide.</summary>
	public UInt32 MathEFGH;
	/// <summary>Math register group JKLM (0xFC6C-6F): M=byte0, L=byte1, K=byte2, J=byte3.
	/// Accumulator (multiply-add) / remainder (divide).</summary>
	public UInt32 MathJKLM;
	/// <summary>Math register group NP (0xFC56-57): P=byte0, N=byte1.
	/// Divisor for divide operations.</summary>
	public UInt16 MathNP;
	/// <summary>Sign tracking for AB group (1 = positive, -1 = negative).</summary>
	public Int32 MathAB_sign;
	/// <summary>Sign tracking for CD group (1 = positive, -1 = negative).</summary>
	public Int32 MathCD_sign;
	/// <summary>Sign tracking for EFGH group (1 = positive, -1 = negative).</summary>
	public Int32 MathEFGH_sign;
	/// <summary>Signed math operation in progress.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool MathSign;
	/// <summary>Accumulate mode (add to existing result).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool MathAccumulate;
	/// <summary>Math operation currently executing.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool MathInProgress;
	/// <summary>Math overflow occurred.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool MathOverflow;

	/// <summary>Last carry bit from multiply — SPRSYS read bit 5 (0x20).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool LastCarry;
	/// <summary>Unsafe access detected — CPU tried to access Suzy during sprite processing. SPRSYS read bit 2 (0x04).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool UnsafeAccess;
	/// <summary>Sprite-to-sprite collision occurred (sticky flag). Internal tracking, not directly exposed via SPRSYS.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool SpriteToSpriteCollision;
	/// <summary>Stop-on-current — SPRSYS write/read bit 1 (0x02). Requests sprite engine stop after current sprite.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool StopOnCurrent;
	/// <summary>VStretch enable — SPRSYS write/read bit 4 (0x10). When set, vsize is applied as stretch factor.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool VStretch;
	/// <summary>LeftHand enable — SPRSYS write/read bit 3 (0x08). Flips coordinate system for left-handed orientation.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool LeftHand;

	/// <summary>TMPADR (FC00-FC01): sprite engine temporary address.</summary>
	public UInt16 TempAddress;
	/// <summary>TILTACUM (FC02-FC03): tilt accumulator for sprite stretching/rotation.</summary>
	public UInt16 TiltAccum;
	/// <summary>VIDADR (FC0C-FC0D): current video buffer DMA address (read-only).</summary>
	public UInt16 VideoAddress;
	/// <summary>COLLADR (FC0E-FC0F): current collision buffer DMA address (read-only).</summary>
	public UInt16 CollisionAddress;

	/// <summary>Horizontal screen offset (FC04-FC05). Scroll offset for sprite rendering.</summary>
	public Int16 HOffset;
	/// <summary>Vertical screen offset (FC06-FC07). Scroll offset for sprite rendering.</summary>
	public Int16 VOffset;
	/// <summary>Video buffer base address in RAM (FC08-FC09).</summary>
	public UInt16 VideoBase;
	/// <summary>Collision buffer base address in RAM (FC0A-FC0B).</summary>
	public UInt16 CollisionBase;
	/// <summary>Collision depository offset (FC24-FC25).</summary>
	public UInt16 CollOffset;
	/// <summary>Horizontal size offset / accumulator init (FC28-FC29).</summary>
	public UInt16 HSizeOff;
	/// <summary>Vertical size offset / accumulator init (FC2A-FC2B).</summary>
	public UInt16 VSizeOff;
	/// <summary>EVERON flag from SPRGO bit 2. Tracks if any sprite pixel was on-screen.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool EverOn;
	/// <summary>NoCollide mode — all collision detection globally disabled.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool NoCollide;

	/// <summary>Joystick register (active-low: A/B/Opt1/Opt2/Up/Down/Left/Right).</summary>
	public byte Joystick;
	/// <summary>Switches register (Pause, cart control bits).</summary>
	public byte Switches;
}

/// <summary>Memory manager state (MAPCTL-based overlays).</summary>
/// <remarks>Matches memory layout of LynxMemoryManagerState in Core/Lynx/LynxTypes.h.</remarks>
public struct LynxMemoryManagerState : BaseState {
	/// <summary>MAPCTL register raw value.</summary>
	public byte Mapctl;
	/// <summary>Vector space ($FFFA-$FFFF) mapped to ROM.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool VectorSpaceVisible;
	/// <summary>Mikey registers ($FD00-$FDFF) visible.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool MikeySpaceVisible;
	/// <summary>Suzy registers ($FC00-$FCFF) visible.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool SuzySpaceVisible;
	/// <summary>ROM space ($FE00-$FFF7) visible.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool RomSpaceVisible;
	/// <summary>Boot ROM currently active (after reset).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool BootRomActive;
}

/// <summary>Complete Lynx console state snapshot for debugger interop.</summary>
/// <remarks>Aggregates all subsystem states for save states and debugging.</remarks>
public struct LynxState : BaseState {
	/// <summary>Hardware model (Lynx I or II).</summary>
	public LynxModel Model;
	/// <summary>65C02 CPU state.</summary>
	public LynxCpuState Cpu;
	/// <summary>PPU/display state.</summary>
	public LynxPpuState Ppu;
	/// <summary>Mikey chip state (timers, display, UART).</summary>
	public LynxMikeyState Mikey;
	/// <summary>Suzy chip state (sprites, math, collision).</summary>
	public LynxSuzyState Suzy;
	// NOTE: APU state lives inside Mikey.Apu — not duplicated at top level.
	// This matches hardware reality where audio channels are part of Mikey.
	/// <summary>Memory manager state.</summary>
	public LynxMemoryManagerState MemoryManager;
	/// <summary>Controller/input state.</summary>
	public LynxControlManagerState ControlManager;
	/// <summary>Cartridge state.</summary>
	public LynxCartState Cart;
	/// <summary>EEPROM state.</summary>
	public LynxEepromSerialState Eeprom;
}

/// <summary>Controller manager state.</summary>
public struct LynxControlManagerState : BaseState {
	/// <summary>Joystick register value (active-low).</summary>
	public byte Joystick;
	/// <summary>Switches register value.</summary>
	public byte Switches;
}

/// <summary>Cartridge information from LNX header.</summary>
public struct LynxCartInfo {
	/// <summary>Game title (max 32 chars + null).</summary>
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 33)] public string Name;
	/// <summary>Manufacturer name (max 16 chars + null).</summary>
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 17)] public string Manufacturer;
	/// <summary>Total ROM size in bytes.</summary>
	public UInt32 RomSize;
	/// <summary>Page size for bank 0 (in bytes).</summary>
	public UInt16 PageSizeBank0;
	/// <summary>Page size for bank 1 (in bytes).</summary>
	public UInt16 PageSizeBank1;
	/// <summary>Declared screen rotation.</summary>
	public LynxRotation Rotation;
	/// <summary>Cart has EEPROM for save data.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool HasEeprom;
	/// <summary>EEPROM chip type.</summary>
	public LynxEepromType EepromType;
	/// <summary>LNX format version.</summary>
	public UInt16 Version;
}

/// <summary>Cartridge state (bank switching, page counters).</summary>
/// <remarks>Matches memory layout of LynxCartState in Core/Lynx/LynxTypes.h.</remarks>
public struct LynxCartState : BaseState {
	/// <summary>Cartridge header info.</summary>
	public LynxCartInfo Info;
	/// <summary>Currently selected bank.</summary>
	public UInt16 CurrentBank;
	/// <summary>Shift register for bank selection.</summary>
	public byte ShiftRegister;
	/// <summary>Address counter for sequential access.</summary>
	public UInt32 AddressCounter;
}

/// <summary>EEPROM serial protocol state.</summary>
/// <remarks>Matches memory layout of LynxEepromSerialState in Core/Lynx/LynxTypes.h.</remarks>
public struct LynxEepromSerialState : BaseState {
	/// <summary>EEPROM chip type.</summary>
	public LynxEepromType Type;
	/// <summary>Protocol state machine phase.</summary>
	public LynxEepromState State;
	/// <summary>Opcode being received (2 bits + extended).</summary>
	public UInt16 Opcode;
	/// <summary>Address being received.</summary>
	public UInt16 Address;
	/// <summary>Data word buffer (read/write).</summary>
	public UInt16 DataBuffer;
	/// <summary>Bits received in current phase.</summary>
	public byte BitCount;
	/// <summary>Write operations enabled (EWEN command issued).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool WriteEnabled;
	/// <summary>Chip select active.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool CsActive;
	/// <summary>Current clock line state.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool ClockState;
	/// <summary>Data out pin state.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool DataOut;
}
