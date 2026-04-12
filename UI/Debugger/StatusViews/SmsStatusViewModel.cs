using System;
using System.Text;
using Nexen.Interop;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Debugger.StatusViews; 
public sealed partial class SmsStatusViewModel : BaseConsoleStatusViewModel {
	[Reactive] public partial byte RegA { get; set; }
	[Reactive] public partial byte RegB { get; set; }
	[Reactive] public partial byte RegC { get; set; }
	[Reactive] public partial byte RegD { get; set; }
	[Reactive] public partial byte RegE { get; set; }
	[Reactive] public partial byte RegFlags { get; set; }

	[Reactive] public partial byte RegH { get; set; }
	[Reactive] public partial byte RegL { get; set; }

	[Reactive] public partial UInt16 RegIX { get; set; }
	[Reactive] public partial UInt16 RegIY { get; set; }

	[Reactive] public partial byte RegR { get; set; }
	[Reactive] public partial byte RegI { get; set; }

	[Reactive] public partial byte RegAltA { get; set; }
	[Reactive] public partial byte RegAltFlags { get; set; }
	[Reactive] public partial byte RegAltB { get; set; }
	[Reactive] public partial byte RegAltC { get; set; }
	[Reactive] public partial byte RegAltD { get; set; }
	[Reactive] public partial byte RegAltE { get; set; }
	[Reactive] public partial byte RegAltH { get; set; }
	[Reactive] public partial byte RegAltL { get; set; }

	[Reactive] public partial UInt16 RegSP { get; set; }
	[Reactive] public partial UInt16 RegPC { get; set; }

	[Reactive] public partial UInt16 Scanline { get; set; }
	[Reactive] public partial UInt16 Cycle { get; set; }

	[Reactive] public partial bool FlagCarry { get; set; }
	[Reactive] public partial bool FlagAddSub { get; set; }
	[Reactive] public partial bool FlagParity { get; set; }
	[Reactive] public partial bool FlagF3 { get; set; }
	[Reactive] public partial bool FlagHalf { get; set; }
	[Reactive] public partial bool FlagF5 { get; set; }
	[Reactive] public partial bool FlagZero { get; set; }
	[Reactive] public partial bool FlagSign { get; set; }

	[Reactive] public partial bool FlagIFF1 { get; set; }
	[Reactive] public partial bool FlagIFF2 { get; set; }
	[Reactive] public partial bool FlagHalted { get; set; }
	[Reactive] public partial byte IM { get; set; }

	[Reactive] public partial string StackPreview { get; private set; } = "";

	public SmsStatusViewModel() {
		AddDisposable(this.WhenAnyValue(x => x.FlagCarry, x => x.FlagHalf, x => x.FlagAddSub, x => x.FlagZero).Subscribe(x => UpdateFlagsValue()));

		AddDisposable(this.WhenAnyValue(x => x.RegFlags).Subscribe(x => {
			using var delayNotifs = DelayChangeNotifications(); //don't reupdate RegFlags while updating the flags
			FlagCarry = (x & (byte)SmsCpuFlags.Carry) != 0;
			FlagAddSub = (x & (byte)SmsCpuFlags.AddSub) != 0;
			FlagParity = (x & (byte)SmsCpuFlags.Parity) != 0;
			FlagF3 = (x & (byte)SmsCpuFlags.F3) != 0;
			FlagHalf = (x & (byte)SmsCpuFlags.HalfCarry) != 0;
			FlagF5 = (x & (byte)SmsCpuFlags.F5) != 0;
			FlagZero = (x & (byte)SmsCpuFlags.Zero) != 0;
			FlagSign = (x & (byte)SmsCpuFlags.Sign) != 0;
		}));
	}

	private void UpdateFlagsValue() {
		RegFlags = (byte)(
			(FlagCarry ? (byte)SmsCpuFlags.Carry : 0) |
			(FlagAddSub ? (byte)SmsCpuFlags.AddSub : 0) |
			(FlagParity ? (byte)SmsCpuFlags.Parity : 0) |
			(FlagF3 ? (byte)SmsCpuFlags.F3 : 0) |
			(FlagHalf ? (byte)SmsCpuFlags.HalfCarry : 0) |
			(FlagF5 ? (byte)SmsCpuFlags.F5 : 0) |
			(FlagZero ? (byte)SmsCpuFlags.Zero : 0) |
			(FlagSign ? (byte)SmsCpuFlags.Sign : 0)
		);
	}

	protected override void InternalUpdateUiState() {
		SmsState state = DebugApi.GetConsoleState<SmsState>(ConsoleType.Sms);

		SmsCpuState cpu = state.Cpu;
		SmsVdpState ppu = DebugApi.GetPpuState<SmsVdpState>(CpuType.Sms);

		UpdateCycleCount(state.Cpu.CycleCount);

		RegA = cpu.A;
		RegB = cpu.B;
		RegC = cpu.C;
		RegD = cpu.D;
		RegE = cpu.E;
		RegFlags = cpu.Flags;

		RegH = cpu.H;
		RegL = cpu.L;

		RegIX = (UInt16)(cpu.IXL | (cpu.IXH << 8));
		RegIY = (UInt16)(cpu.IYL | (cpu.IYH << 8));

		RegPC = cpu.PC;
		RegSP = cpu.SP;

		RegR = cpu.R;
		RegI = cpu.I;

		RegAltA = cpu.AltA;
		RegAltB = cpu.AltB;
		RegAltC = cpu.AltC;
		RegAltD = cpu.AltD;
		RegAltE = cpu.AltE;
		RegAltFlags = cpu.AltFlags;

		RegAltH = cpu.AltH;
		RegAltL = cpu.AltL;

		FlagIFF1 = cpu.IFF1;
		FlagIFF2 = cpu.IFF2;
		FlagHalted = cpu.Halted;
		IM = cpu.IM;

		Scanline = ppu.Scanline;
		Cycle = ppu.Cycle;

		StringBuilder sb = new StringBuilder();
		for (UInt32 i = (UInt32)cpu.SP; (i & 0xFF) != 0; i++) {
			sb.Append($"${DebugApi.GetMemoryValue(MemoryType.SmsMemory, i):X2} ");
		}

		StackPreview = sb.ToString();
	}

	protected override void InternalUpdateConsoleState() {
		SmsCpuState cpu = DebugApi.GetCpuState<SmsCpuState>(CpuType.Sms);

		cpu.A = RegA;
		cpu.B = RegB;
		cpu.C = RegC;
		cpu.D = RegD;
		cpu.E = RegE;
		cpu.Flags = RegFlags;

		cpu.H = RegH;
		cpu.L = RegL;

		cpu.IXL = (byte)(RegIX & 0xFF);
		cpu.IXH = (byte)((RegIX >> 8) & 0xFF);
		cpu.IYL = (byte)(RegIY & 0xFF);
		cpu.IYH = (byte)((RegIY >> 8) & 0xFF);

		cpu.PC = RegPC;
		cpu.SP = RegSP;

		cpu.R = RegR;
		cpu.I = RegI;

		cpu.AltA = RegAltA;
		cpu.AltB = RegAltB;
		cpu.AltC = RegAltC;
		cpu.AltD = RegAltD;
		cpu.AltE = RegAltE;
		cpu.AltFlags = RegAltFlags;

		cpu.AltH = RegAltH;
		cpu.AltL = RegAltL;

		cpu.IFF1 = FlagIFF1;
		cpu.IFF2 = FlagIFF2;
		cpu.Halted = FlagHalted;
		cpu.IM = IM;

		DebugApi.SetCpuState(cpu, CpuType.Sms);
	}
}
