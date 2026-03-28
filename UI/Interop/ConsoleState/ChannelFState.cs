using System;
using System.Runtime.InteropServices;

namespace Nexen.Interop;

/// <summary>Fairchild F8 CPU register state for debugger interop.</summary>
/// <remarks>Matches memory layout of ChannelFCpuState in Core/ChannelF/ChannelFTypes.h.</remarks>
public struct ChannelFCpuState : BaseState {
	/// <summary>Total CPU cycles executed since power-on.</summary>
	public UInt64 CycleCount;
	/// <summary>Program Counter 0 (active).</summary>
	public UInt16 PC0;
	/// <summary>Program Counter 1 (backup/stack).</summary>
	public UInt16 PC1;
	/// <summary>Data Counter 0 (active).</summary>
	public UInt16 DC0;
	/// <summary>Data Counter 1 (backup).</summary>
	public UInt16 DC1;
	/// <summary>Accumulator register.</summary>
	public byte A;
	/// <summary>Status register (W) — flags: Sign(3), Carry(2), Zero(1), Overflow(0).</summary>
	public byte W;
	/// <summary>Indirect Scratchpad Address Register (6-bit).</summary>
	public byte ISAR;
	/// <summary>Interrupts enabled flag.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool InterruptsEnabled;
	/// <summary>64-byte internal scratchpad RAM (R0-R63).</summary>
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)]
	public byte[] Scratchpad;
}

/// <summary>Channel F video state for debugger interop.</summary>
/// <remarks>Matches memory layout of ChannelFVideoState in Core/ChannelF/ChannelFTypes.h.</remarks>
public struct ChannelFVideoState : BaseState {
	/// <summary>Current drawing color (2 bits, 0-3).</summary>
	public byte Color;
	/// <summary>Current X position (0-127).</summary>
	public byte X;
	/// <summary>Current Y position (0-63).</summary>
	public byte Y;
}

/// <summary>Channel F audio state for debugger interop.</summary>
/// <remarks>Matches memory layout of ChannelFAudioState in Core/ChannelF/ChannelFTypes.h.</remarks>
public struct ChannelFAudioState : BaseState {
	/// <summary>Tone control value.</summary>
	public byte Tone;
	/// <summary>Frequency control value.</summary>
	public byte Frequency;
	/// <summary>Whether sound output is enabled.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool SoundEnabled;
}

/// <summary>Channel F I/O port state for debugger interop.</summary>
/// <remarks>Matches memory layout of ChannelFPortState in Core/ChannelF/ChannelFTypes.h.</remarks>
public struct ChannelFPortState : BaseState {
	/// <summary>Port 0 — console buttons / LED.</summary>
	public byte Port0;
	/// <summary>Port 1 — video write data.</summary>
	public byte Port1;
	/// <summary>Port 4 — right controller input (active-low).</summary>
	public byte Port4;
	/// <summary>Port 5 — left controller input (active-low).</summary>
	public byte Port5;
}
