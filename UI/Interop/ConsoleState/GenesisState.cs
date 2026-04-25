using System;
using System.Runtime.InteropServices;

namespace Nexen.Interop;

public struct GenesisM68kState : BaseState {
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
	public UInt32[] D;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
	public UInt32[] A;
	public UInt32 PC;
	public UInt32 USP;
	public UInt32 SSP;
	public UInt16 SR;
	public UInt64 CycleCount;
	[MarshalAs(UnmanagedType.I1)] public bool Stopped;
}

public struct GenesisVdpState : BaseState {
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 24)]
	public byte[] Registers;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)]
	public UInt16[] Cram;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 40)]
	public UInt16[] Vsram;

	public UInt16 StatusRegister;
	public UInt32 AddressRegister;
	public byte CodeRegister;
	[MarshalAs(UnmanagedType.I1)] public bool WritePending;
	public UInt16 DataPortBuffer;

	public UInt16 HCounter;
	public UInt16 VCounter;
	public UInt32 FrameCount;
	public byte HIntCounter;

	[MarshalAs(UnmanagedType.I1)] public bool DmaActive;
	public byte DmaMode;
}

public struct GenesisIoState : BaseState {
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
	public byte[] DataPort;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
	public byte[] CtrlPort;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
	public byte[] TxData;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
	public byte[] RxData;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
	public byte[] SCtrl;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
	public byte[] ThCount;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
	public byte[] ThState;
}

public struct GenesisPsgChannelState {
	public UInt16 ToneCounter;
	public byte Volume;
}

public struct GenesisPsgState : BaseState {
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
	public GenesisPsgChannelState[] Channels;
	public byte LatchedRegister;
	public byte NoiseMode;
	public UInt16 NoiseShiftRegister;
	public UInt32 WriteCount;
}

public struct GenesisState : BaseState {
	public GenesisM68kState Cpu;
	public GenesisVdpState Vdp;
	public GenesisIoState Io;
	public GenesisPsgState Psg;
}
