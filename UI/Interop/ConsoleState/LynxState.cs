using System;
using System.Runtime.InteropServices;

namespace Nexen.Interop;

public enum LynxModel : byte {
	LynxI = 0,
	LynxII = 1
}

public enum LynxRotation : byte {
	None = 0,
	Left = 1,
	Right = 2
}

public enum LynxCpuStopState : byte {
	Running = 0,
	Stopped = 1,
	WaitingForIrq = 2
}

public enum LynxEepromType : byte {
	None = 0,
	Eeprom93c46 = 1,
	Eeprom93c66 = 2,
	Eeprom93c86 = 3
}

public enum LynxEepromState : byte {
	Idle = 0,
	ReceivingOpcode = 1,
	ReceivingAddress = 2,
	ReceivingData = 3,
	SendingData = 4
}

public struct LynxCpuState : BaseState {
	public UInt64 CycleCount;
	public UInt16 PC;
	public byte SP;
	public byte A;
	public byte X;
	public byte Y;
	public byte PS;
	public byte IRQFlag;
	[MarshalAs(UnmanagedType.I1)] public bool NmiFlag;
	public LynxCpuStopState StopState;
}

public struct LynxTimerState {
	public byte BackupValue;
	public byte ControlA;
	public byte Count;
	public byte ControlB;
	public UInt64 LastTick;
	[MarshalAs(UnmanagedType.I1)] public bool TimerDone;
	[MarshalAs(UnmanagedType.I1)] public bool Linked;
}

public struct LynxAudioChannelState {
	public byte Volume;
	public byte FeedbackEnable;
	public sbyte Output;
	public UInt16 ShiftRegister;
	public byte BackupValue;
	public byte Control;
	public byte Counter;
	public byte LeftAtten;
	public byte RightAtten;
	[MarshalAs(UnmanagedType.I1)] public bool Integrate;
	[MarshalAs(UnmanagedType.I1)] public bool Enabled;
}

public struct LynxApuState {
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
	public LynxAudioChannelState[] Channels;
	public byte MasterVolume;
	[MarshalAs(UnmanagedType.I1)] public bool StereoEnabled;
}

public struct LynxMikeyState : BaseState {
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

	public byte SerialControl;

	public UInt32 UartTxCountdown;
	public UInt32 UartRxCountdown;
	public UInt16 UartTxData;
	public UInt16 UartRxData;
	[MarshalAs(UnmanagedType.I1)] public bool UartRxReady;
	[MarshalAs(UnmanagedType.I1)] public bool UartTxIrqEnable;
	[MarshalAs(UnmanagedType.I1)] public bool UartRxIrqEnable;
	[MarshalAs(UnmanagedType.I1)] public bool UartParityEnable;
	[MarshalAs(UnmanagedType.I1)] public bool UartParityEven;
	[MarshalAs(UnmanagedType.I1)] public bool UartSendBreak;
	[MarshalAs(UnmanagedType.I1)] public bool UartRxOverrunError;
	[MarshalAs(UnmanagedType.I1)] public bool UartRxFramingError;

	public byte HardwareRevision;
}

public struct LynxPpuState : BaseState {
	public UInt32 FrameCount;
	public UInt16 Cycle;
	public UInt16 Scanline;
	public UInt16 DisplayAddress;
	public byte DisplayControl;
	[MarshalAs(UnmanagedType.I1)] public bool LcdEnabled;
}

public struct LynxSuzyState : BaseState {
	public UInt16 SCBAddress;
	public byte SpriteControl0;
	public byte SpriteControl1;
	public byte SpriteInit;
	[MarshalAs(UnmanagedType.I1)] public bool SpriteBusy;
	[MarshalAs(UnmanagedType.I1)] public bool SpriteEnabled;

	public UInt16 MathA;
	public UInt16 MathB;
	public Int16 MathC;
	public Int16 MathD;
	public UInt16 MathE;
	public UInt16 MathF;
	public UInt16 MathG;
	public UInt16 MathH;
	public UInt16 MathJ;
	public UInt16 MathK;
	public UInt16 MathM;
	public UInt16 MathN;
	[MarshalAs(UnmanagedType.I1)] public bool MathSign;
	[MarshalAs(UnmanagedType.I1)] public bool MathAccumulate;
	[MarshalAs(UnmanagedType.I1)] public bool MathInProgress;
	[MarshalAs(UnmanagedType.I1)] public bool MathOverflow;

	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
	public byte[] CollisionBuffer;

	public byte Joystick;
	public byte Switches;
}

public struct LynxMemoryManagerState : BaseState {
	public byte Mapctl;
	[MarshalAs(UnmanagedType.I1)] public bool VectorSpaceVisible;
	[MarshalAs(UnmanagedType.I1)] public bool MikeySpaceVisible;
	[MarshalAs(UnmanagedType.I1)] public bool SuzySpaceVisible;
	[MarshalAs(UnmanagedType.I1)] public bool RomSpaceVisible;
	[MarshalAs(UnmanagedType.I1)] public bool BootRomActive;
}

public struct LynxState : BaseState {
	public LynxModel Model;
	public LynxCpuState Cpu;
	public LynxPpuState Ppu;
	public LynxMikeyState Mikey;
	public LynxSuzyState Suzy;
	public LynxApuState Apu;
	public LynxMemoryManagerState MemoryManager;
	public LynxControlManagerState ControlManager;
	public LynxCartState Cart;
	public LynxEepromSerialState Eeprom;
}

public struct LynxControlManagerState : BaseState {
	public byte Joystick;
	public byte Switches;
}

public struct LynxCartInfo {
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 33)] public string Name;
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 17)] public string Manufacturer;
	public UInt32 RomSize;
	public UInt16 PageSizeBank0;
	public UInt16 PageSizeBank1;
	public LynxRotation Rotation;
	[MarshalAs(UnmanagedType.I1)] public bool HasEeprom;
	public LynxEepromType EepromType;
	public UInt16 Version;
}

public struct LynxCartState : BaseState {
	public LynxCartInfo Info;
	public UInt16 CurrentBank;
	public byte ShiftRegister;
	public UInt32 AddressCounter;
}

public struct LynxEepromSerialState : BaseState {
	public LynxEepromType Type;
	public LynxEepromState State;
	public UInt16 Opcode;
	public UInt16 Address;
	public UInt16 DataBuffer;
	public byte BitCount;
	[MarshalAs(UnmanagedType.I1)] public bool WriteEnabled;
	[MarshalAs(UnmanagedType.I1)] public bool CsActive;
	[MarshalAs(UnmanagedType.I1)] public bool ClockState;
	[MarshalAs(UnmanagedType.I1)] public bool DataOut;
}
