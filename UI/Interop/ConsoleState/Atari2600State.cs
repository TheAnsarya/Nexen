using System;
using System.Runtime.InteropServices;

namespace Nexen.Interop;

/// <summary>Atari 2600 6507 CPU register state for debugger interop.</summary>
/// <remarks>Matches memory layout of Atari2600CpuState in Core/Atari2600/Atari2600Types.h.</remarks>
public struct Atari2600CpuState : BaseState {
	/// <summary>Total CPU cycles executed since power-on.</summary>
	public UInt64 CycleCount;
	/// <summary>Program Counter (only low 13 bits used on 6507 bus).</summary>
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
	/// <summary>IRQ pending flag (6507 has no IRQ pin, but BRK sets this).</summary>
	public byte IRQFlag;
	/// <summary>NMI pending flag (6507 has no NMI pin — always false).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool NmiFlag;
}

/// <summary>RIOT (6532) chip state for debugger interop.</summary>
/// <remarks>
/// The RIOT handles I/O ports (joystick, switches) and interval timer.
/// Matches memory layout of Atari2600RiotState in Core/Atari2600/Atari2600Console.h.
/// </remarks>
public struct Atari2600RiotState : BaseState {
	/// <summary>Port A data register ($0280) — joystick directions.</summary>
	public byte PortA;
	/// <summary>Port B data register ($0282) — console switches.</summary>
	public byte PortB;
	/// <summary>Port A data direction ($0281).</summary>
	public byte PortADirection;
	/// <summary>Port B data direction ($0283).</summary>
	public byte PortBDirection;
	/// <summary>Port A input latch.</summary>
	public byte PortAInput;
	/// <summary>Port B input latch.</summary>
	public byte PortBInput;
	/// <summary>Timer current value.</summary>
	public UInt16 Timer;
	/// <summary>Timer divider (1, 8, 64, or 1024).</summary>
	public UInt16 TimerDivider;
	/// <summary>Timer divider countdown counter.</summary>
	public UInt16 TimerDividerCounter;
	/// <summary>Timer has underflowed.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool TimerUnderflow;
	/// <summary>Interrupt flag set on timer underflow.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool InterruptFlag;
	/// <summary>Edge-detect interrupt counter.</summary>
	public UInt32 InterruptEdgeCount;
	/// <summary>CPU cycles at last timer update.</summary>
	public UInt64 CpuCycles;
}

/// <summary>TIA (Television Interface Adapter) state for debugger interop.</summary>
/// <remarks>
/// The TIA handles all Atari 2600 graphics and audio.
/// Unlike tile-based systems, the 2600 renders player/missile/ball objects
/// and a playfield register per scanline.
/// Matches memory layout of Atari2600TiaState in Core/Atari2600/Atari2600Console.h.
/// </remarks>
public struct Atari2600TiaState : BaseState {
	/// <summary>Total frames rendered since power-on.</summary>
	public UInt32 FrameCount;
	/// <summary>Current scanline (0-261 for NTSC).</summary>
	public UInt32 Scanline;
	/// <summary>Current color clock within scanline (0-227).</summary>
	public UInt32 ColorClock;

	/// <summary>WSYNC hold — CPU halted until end of scanline.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool WsyncHold;
	/// <summary>Count of WSYNC strobes this frame.</summary>
	public UInt32 WsyncCount;
	/// <summary>HMOVE pending — will be applied at next HBlank.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool HmovePending;
	/// <summary>HMOVE delayed to next scanline (late strobe).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool HmoveDelayToNextScanline;
	/// <summary>Number of HMOVE strobes.</summary>
	public UInt32 HmoveStrobeCount;
	/// <summary>Number of HMOVE applies.</summary>
	public UInt32 HmoveApplyCount;

	// Color registers
	/// <summary>COLUBK ($09) — background color.</summary>
	public byte ColorBackground;
	/// <summary>COLUPF ($08) — playfield color.</summary>
	public byte ColorPlayfield;
	/// <summary>COLUP0 ($06) — player 0 color.</summary>
	public byte ColorPlayer0;
	/// <summary>COLUP1 ($07) — player 1 color.</summary>
	public byte ColorPlayer1;

	// Playfield registers
	/// <summary>PF0 ($0D) — playfield register 0 (bits 4-7 used).</summary>
	public byte Playfield0;
	/// <summary>PF1 ($0E) — playfield register 1.</summary>
	public byte Playfield1;
	/// <summary>PF2 ($0F) — playfield register 2.</summary>
	public byte Playfield2;
	/// <summary>CTRLPF bit 0 — playfield reflected on right half.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool PlayfieldReflect;
	/// <summary>CTRLPF bit 1 — score mode (left=P0 color, right=P1 color).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool PlayfieldScoreMode;
	/// <summary>CTRLPF bit 2 — playfield priority (drawn over players).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool PlayfieldPriority;

	/// <summary>CTRLPF bits 4-5 — ball size (1, 2, 4, or 8 clocks).</summary>
	public byte BallSize;

	// Player/missile sizing
	/// <summary>NUSIZ0 ($04) — player 0 number/size.</summary>
	public byte Nusiz0;
	/// <summary>NUSIZ1 ($05) — player 1 number/size.</summary>
	public byte Nusiz1;

	// Player graphics
	/// <summary>GRP0 ($1B) — player 0 graphics register.</summary>
	public byte Player0Graphics;
	/// <summary>GRP1 ($1C) — player 1 graphics register.</summary>
	public byte Player1Graphics;
	/// <summary>REFP0 bit 3 — player 0 horizontal reflect.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool Player0Reflect;
	/// <summary>REFP1 bit 3 — player 1 horizontal reflect.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool Player1Reflect;

	// Missile/ball enables and resets
	/// <summary>RESMP0 — missile 0 locked to player 0 position.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool Missile0ResetToPlayer;
	/// <summary>RESMP1 — missile 1 locked to player 1 position.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool Missile1ResetToPlayer;
	/// <summary>ENAM0 ($1D) — missile 0 enabled.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool Missile0Enabled;
	/// <summary>ENAM1 ($1E) — missile 1 enabled.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool Missile1Enabled;
	/// <summary>ENABL ($1F) — ball enabled.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool BallEnabled;

	// Vertical delay
	/// <summary>VDELP0 — player 0 vertical delay.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool VdelPlayer0;
	/// <summary>VDELP1 — player 1 vertical delay.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool VdelPlayer1;
	/// <summary>VDELBL — ball vertical delay.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool VdelBall;

	// Delayed graphics
	/// <summary>Delayed player 0 graphics (used with VDELP0).</summary>
	public byte DelayedPlayer0Graphics;
	/// <summary>Delayed player 1 graphics (used with VDELP1).</summary>
	public byte DelayedPlayer1Graphics;
	/// <summary>Delayed ball enable state (used with VDELBL).</summary>
	[MarshalAs(UnmanagedType.I1)] public bool DelayedBallEnabled;

	// Object positions (horizontal)
	/// <summary>Player 0 horizontal position (color clocks from left).</summary>
	public byte Player0X;
	/// <summary>Player 1 horizontal position.</summary>
	public byte Player1X;
	/// <summary>Missile 0 horizontal position.</summary>
	public byte Missile0X;
	/// <summary>Missile 1 horizontal position.</summary>
	public byte Missile1X;
	/// <summary>Ball horizontal position.</summary>
	public byte BallX;

	// Horizontal motion registers
	/// <summary>HMP0 — player 0 horizontal motion (-8 to +7).</summary>
	public sbyte MotionPlayer0;
	/// <summary>HMP1 — player 1 horizontal motion.</summary>
	public sbyte MotionPlayer1;
	/// <summary>HMM0 — missile 0 horizontal motion.</summary>
	public sbyte MotionMissile0;
	/// <summary>HMM1 — missile 1 horizontal motion.</summary>
	public sbyte MotionMissile1;
	/// <summary>HMBL — ball horizontal motion.</summary>
	public sbyte MotionBall;

	// Collision registers (read-only on hardware)
	/// <summary>CXM0P — missile 0 / player collisions.</summary>
	public byte CollisionCxm0p;
	/// <summary>CXM1P — missile 1 / player collisions.</summary>
	public byte CollisionCxm1p;
	/// <summary>CXP0FB — player 0 / playfield-ball collisions.</summary>
	public byte CollisionCxp0fb;
	/// <summary>CXP1FB — player 1 / playfield-ball collisions.</summary>
	public byte CollisionCxp1fb;
	/// <summary>CXM0FB — missile 0 / playfield-ball collisions.</summary>
	public byte CollisionCxm0fb;
	/// <summary>CXM1FB — missile 1 / playfield-ball collisions.</summary>
	public byte CollisionCxm1fb;
	/// <summary>CXBLPF — ball / playfield collision.</summary>
	public byte CollisionCxblpf;
	/// <summary>CXPPMM — player-player / missile-missile collisions.</summary>
	public byte CollisionCxppmm;

	/// <summary>Internal render revision counter.</summary>
	public UInt32 RenderRevision;

	// Audio state
	/// <summary>AUDC0 ($15) — audio control channel 0.</summary>
	public byte AudioControl0;
	/// <summary>AUDC1 ($16) — audio control channel 1.</summary>
	public byte AudioControl1;
	/// <summary>AUDF0 ($17) — audio frequency channel 0.</summary>
	public byte AudioFrequency0;
	/// <summary>AUDF1 ($18) — audio frequency channel 1.</summary>
	public byte AudioFrequency1;
	/// <summary>AUDV0 ($19) — audio volume channel 0.</summary>
	public byte AudioVolume0;
	/// <summary>AUDV1 ($1A) — audio volume channel 1.</summary>
	public byte AudioVolume1;
	/// <summary>Audio counter channel 0.</summary>
	public UInt16 AudioCounter0;
	/// <summary>Audio counter channel 1.</summary>
	public UInt16 AudioCounter1;
	/// <summary>Audio phase channel 0.</summary>
	public byte AudioPhase0;
	/// <summary>Audio phase channel 1.</summary>
	public byte AudioPhase1;
	/// <summary>Last mixed audio sample.</summary>
	public byte LastMixedSample;
	/// <summary>Audio mix accumulator for averaging.</summary>
	public UInt64 AudioMixAccumulator;
	/// <summary>Total audio samples generated.</summary>
	public UInt64 AudioSampleCount;
	/// <summary>Audio revision counter.</summary>
	public UInt32 AudioRevision;
	/// <summary>Total color clocks elapsed.</summary>
	public UInt64 TotalColorClocks;
}

/// <summary>Complete Atari 2600 console state snapshot for debugger interop.</summary>
/// <remarks>Aggregates CPU, TIA, and RIOT state for save states and debugging.</remarks>
public struct Atari2600State : BaseState {
	/// <summary>6507 CPU state.</summary>
	public Atari2600CpuState Cpu;
	/// <summary>TIA (graphics/audio) state.</summary>
	public Atari2600TiaState Tia;
	/// <summary>RIOT (timer/I/O) state.</summary>
	public Atari2600RiotState Riot;
}
