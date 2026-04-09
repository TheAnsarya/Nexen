using System;
using System.Text;
using Nexen.Interop;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Debugger.StatusViews; 
public sealed partial class SpcStatusViewModel : BaseConsoleStatusViewModel {
	[Reactive] public partial byte RegA { get; set; }
	[Reactive] public partial byte RegX { get; set; }
	[Reactive] public partial byte RegY { get; set; }
	[Reactive] public partial byte RegSP { get; set; }
	[Reactive] public partial UInt16 RegPC { get; set; }
	[Reactive] public partial byte RegPS { get; set; }

	[Reactive] public partial bool FlagN { get; set; }
	[Reactive] public partial bool FlagV { get; set; }
	[Reactive] public partial bool FlagP { get; set; }
	[Reactive] public partial bool FlagB { get; set; }
	[Reactive] public partial bool FlagH { get; set; }
	[Reactive] public partial bool FlagI { get; set; }
	[Reactive] public partial bool FlagZ { get; set; }
	[Reactive] public partial bool FlagC { get; set; }

	[Reactive] public partial string StackPreview { get; private set; } = "";

	public SpcStatusViewModel() {
		this.WhenAnyValue(x => x.FlagC, x => x.FlagP, x => x.FlagB, x => x.FlagH).Subscribe(x => UpdatePsValue());
		this.WhenAnyValue(x => x.FlagI, x => x.FlagN, x => x.FlagV, x => x.FlagZ).Subscribe(x => UpdatePsValue());

		this.WhenAnyValue(x => x.RegPS).Subscribe(x => {
			using var delayNotifs = DelayChangeNotifications(); //don't reupdate PS while updating the flags
			FlagN = (x & (byte)SpcFlags.Negative) != 0;
			FlagV = (x & (byte)SpcFlags.Overflow) != 0;
			FlagP = (x & (byte)SpcFlags.DirectPage) != 0;
			FlagB = (x & (byte)SpcFlags.Break) != 0;
			FlagH = (x & (byte)SpcFlags.HalfCarry) != 0;
			FlagI = (x & (byte)SpcFlags.IrqEnable) != 0;
			FlagZ = (x & (byte)SpcFlags.Zero) != 0;
			FlagC = (x & (byte)SpcFlags.Carry) != 0;
		});
	}

	private void UpdatePsValue() {
		RegPS = (byte)(
			(FlagN ? (byte)SpcFlags.Negative : 0) |
			(FlagV ? (byte)SpcFlags.Overflow : 0) |
			(FlagP ? (byte)SpcFlags.DirectPage : 0) |
			(FlagB ? (byte)SpcFlags.Break : 0) |
			(FlagH ? (byte)SpcFlags.HalfCarry : 0) |
			(FlagI ? (byte)SpcFlags.IrqEnable : 0) |
			(FlagZ ? (byte)SpcFlags.Zero : 0) |
			(FlagC ? (byte)SpcFlags.Carry : 0)
		);
	}

	protected override void InternalUpdateUiState() {
		SpcState cpu = DebugApi.GetCpuState<SpcState>(CpuType.Spc);

		UpdateCycleCount(cpu.Cycle);

		RegA = cpu.A;
		RegX = cpu.X;
		RegY = cpu.Y;
		RegSP = cpu.SP;
		RegPC = cpu.PC;
		RegPS = (byte)cpu.PS;

		StringBuilder sb = new StringBuilder();
		for (UInt32 i = (UInt32)0x100 + cpu.SP + 1; i < 0x200; i++) {
			sb.Append($"${DebugApi.GetMemoryValue(MemoryType.SpcMemory, i):X2} ");
		}

		StackPreview = sb.ToString();
	}

	protected override void InternalUpdateConsoleState() {
		SpcState cpu = DebugApi.GetCpuState<SpcState>(CpuType.Spc);

		cpu.A = RegA;
		cpu.X = RegX;
		cpu.Y = RegY;
		cpu.SP = RegSP;
		cpu.PC = RegPC;
		cpu.PS = (SpcFlags)RegPS;

		DebugApi.SetCpuState(cpu, CpuType.Spc);
	}
}
